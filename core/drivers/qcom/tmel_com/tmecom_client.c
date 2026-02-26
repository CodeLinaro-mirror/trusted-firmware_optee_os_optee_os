// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <io.h>
#include <kernel/spinlock.h>
#include <kernel/delay.h>
#include <malloc.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <platform_config.h>
#include <stdlib_ext.h>
#include <string.h>
#include <trace.h>
#include <util.h>

#include "tmecom_client.h"
#include "glink_com.h"
#include <initcall.h>

/*
 * IPC Packet Definitions
 */
/* MBOX related macros */
#define TMECOM_HW_MBOX_SIZE			32u
#define TMECOM_MBOX_QMP_CONTROL_DATA_SIZE	8u
#define TMECOM_MBOX_IPC_PACKET_SIZE		\
	(TMECOM_HW_MBOX_SIZE - TMECOM_MBOX_QMP_CONTROL_DATA_SIZE)
#define TMECOM_MBOX_IPC_MAX_PARAMS		5u

/*
 * 32bit ParamID consists of paramCount(4b) and paramType(2b) for each params.
 * Max 14 params can be described using 32b paramID.
 */
#define TMECOM_MAX_PARAM_IN_PARAM_ID		14u

/* bufAddr, bufLen, bufOutLen */
#define TMECOM_PARAM_CNT_FOR_PARAM_TYPE_OUTBUF	3u

/*
 * In worst case when all 14 params are of type TME_MSG_PARAM_TYPE_BUF_OUT or
 * TME_MSG_PARAM_TYPE_BUF_IN_OUT, then total params will be 42(14*3)
 */
#define TMECOM_SRAM_IPC_MAX_PARAMS		\
	((TMECOM_MAX_PARAM_IN_PARAM_ID) * \
	 (TMECOM_PARAM_CNT_FOR_PARAM_TYPE_OUTBUF)) /* 14*3 */

#define TMECOM_SRAM_IPC_MAX_BUF_SIZE		\
	(TMECOM_SRAM_IPC_MAX_PARAMS * sizeof(uint32_t))

#define TMECOM_4_BYTE_ALIGNED			4u
#define TMECOM_IPC_MAX_WAIT_FOR_RESPONSE	TMECOM_DEFAULT_TIMEOUT

/*
 * IPC Types
 */
enum tmecom_ipc_t {
	TMECOM_IPC_TYPE_MBOX_ONLY = 0,
	TMECOM_IPC_TYPE_MBOX_SRAM = 1,
};

/*
 * Message Header structure to uniquely identify service API and get response
 * Bit layout: ipcType(31) | msgLen(30-24) | msgType(23-16) |
 * actionId(15-8) | response(7-0)
 */
struct tmecom_ipc_header {
	uint8_t ipc_type : 1;  /* 0:MBOX_ONLY, 1:MBOX_SRAM */
	uint8_t msg_len : 7;   /* message length in mailbox */
	uint8_t msg_type;      /* command id */
	uint8_t action_id;     /* subcommand id */
	int8_t  response;      /* TME response (Success/Failure) */
} __packed;

/*
 * Mailbox Payload - for MBOX_ONLY type
 */
struct tmecom_mbox_only_payload {
	uint32_t param[5]; /* Max 5 params (20 bytes) */
} __packed;

/*
 * SRAM Payload - for MBOX_SRAM type
 */
struct tmecom_sram_payload {
	uint32_t payload_ptr;
	size_t payload_len;
} __packed;

/*
 * IPC Payload Union
 */
union tmecom_mbox_ipc_payload {
	struct tmecom_mbox_only_payload mailbox_payload;
	struct tmecom_sram_payload sram_payload;
} __packed;

/*
 * Complete IPC Packet - Total 32 bytes (24 payload + 8 QMP control)
 */
struct tmecom_mbox_ipc_pkt {
	struct tmecom_ipc_header msg_hdr;
	union tmecom_mbox_ipc_payload payload;
} __packed;

/*
 * RX/TX States
 */
enum tmecom_rx_state {
	TMECOM_RX_NONE = 0,
	TMECOM_RX_PENDING = 1,
	TMECOM_RX_IN_PROGRESS = 2,
	TMECOM_RX_DONE = 3,
};

enum tmecom_tx_state {
	TMECOM_TX_NONE = 0,
	TMECOM_TX_ABORT = 1,
	TMECOM_TX_DONE = 2,
	TMECOM_TX_IN_PROGRESS = 3,
};

/*
 * User Callback Data
 */
struct tmecom_user_cb_data {
	tmecom_notify_rx_callback_t cb_after_rx;
	struct tmecom_callback_data cb_data;
	uint32_t param_id;
};

/*
 * Glink Configuration
 */
struct tmecom_glink_cfg {
	const char *remote;
	const char *channel;
};

/*
 * Client Context
 */
struct tmecom_glink_ctx {
	const struct tmecom_glink_cfg *cfg;
	glink_handle_type ch_handle;
	bool link_up;
	struct glink_link_id link_id;
	enum glink_channel_event_type glink_state;
	unsigned int tmecom_lock;
	bool tmecom_blocking;
	enum tmecom_tx_state tx_state;
	enum tmecom_rx_state rx_state;
	enum tmecom_response remote_rsp;
	struct tmecom_user_cb_data user_data;
};

static struct tmecom_glink_ctx glink_ctx;
static struct tmecom_mbox_ipc_pkt g_ipc_mailbox;
static uint8_t *g_sram_payload_data_coh;
static void *g_sram_payload_orig_addr;
static paddr_t g_sram_payload_data_paddr;

/*
 * Configuration
 */
static const struct tmecom_glink_cfg tmecom_cfg = {
	.remote = "tme",
	.channel = "tmeRequest",
};

/*
 * Helper Functions
 */

/*
 * Converts TME COM response codes to TEE result codes.
 * Maps TME-specific error codes to standard TEE API error codes.
 */
TEE_Result tmecom_to_tee_result(enum tmecom_response status)
{
	switch (status) {
	case TMECOM_RSP_SUCCESS:
		return TEE_SUCCESS;
	case TMECOM_RSP_FAILURE_BAD_ADDR:
	case TMECOM_RSP_FAILURE_INVALID_ARGS:
		return TEE_ERROR_BAD_PARAMETERS;
	case TMECOM_RSP_FAILURE_CHANNEL_ERR:
	case TMECOM_RSP_FAILURE_LINK_ERR:
	case TMECOM_RSP_FAILURE_TX_ERR:
	case TMECOM_RSP_FAILURE_RX_ERR:
	case TMECOM_RSP_FAILURE_INVALID_MESSAGE:
		return TEE_ERROR_COMMUNICATION;
	case TMECOM_RSP_FAILURE_TIMEOUT:
		return TEE_ERROR_TIMEOUT;
	case TMECOM_RSP_FAILURE_BUSY:
		return TEE_ERROR_BUSY;
	case TMECOM_RSP_FAILURE_NOT_SUPPORTED:
		return TEE_ERROR_NOT_SUPPORTED;
	case TMECOM_SERVICE_API_RETURNED_ERR:
		return TEE_ERROR_GENERIC;
	case TMECOM_RSP_FAILURE:
	default:
		return TEE_ERROR_GENERIC;
	}
}

/*
 * Allocate cache-coherent buffer for TME COM communication.
 * Returns coherent address, original address, and physical address.
 */
void *tmecom_client_malloc_coherent(size_t size, size_t alignment,
				    void **orig_addr, paddr_t *phys_addr)
{
	void *buf_tmp = NULL;
	void *buf_coherent = NULL;
	paddr_t buf_paddr = 0;

	if (!size || !alignment || !orig_addr || !phys_addr)
		return NULL;

	/* Allocate aligned buffer */
	buf_tmp = memalign(alignment, size);
	if (!buf_tmp) {
		EMSG("Failed to allocate buffer: size=%zu", size);
		return NULL;
	}

	/* Get physical address */
	buf_paddr = virt_to_phys(buf_tmp);
	if (!buf_paddr) {
		EMSG("Failed to get physical address");
		free(buf_tmp);
		return NULL;
	}

	/*
	 * Remap as MEM_AREA_TEE_COHERENT for hardware cache coherency.
	 * This ensures the shared buffer remains cache coherent between
	 * OP-TEE and TME without manual cache maintenance.
	 */
	buf_coherent = core_mmu_add_mapping(MEM_AREA_TEE_COHERENT, buf_paddr,
					    size);
	if (!buf_coherent) {
		EMSG("Failed to remap buffer as coherent");
		free(buf_tmp);
		return NULL;
	}

	/* Clear the buffer to ensure no stale data */
	memset(buf_coherent, 0, size);

	/* Return original address and physical address via parameters */
	*orig_addr = buf_tmp;
	*phys_addr = buf_paddr;

	return buf_coherent;
}

/*
 * Free cache-coherent buffer and remove MMU mapping.
 * Takes both coherent and original addresses for proper cleanup.
 *
 * NOTE: To prevent MMU mapping collisions, we only remove MMU mappings
 * for large buffers (>= SMALL_PAGE_SIZE). Small buffers may share MMU
 * mappings with other allocations due to page granularity. Removing
 * a shared mapping would invalidate all buffers in that page, causing
 * crashes when other buffers are accessed.
 */
void tmecom_client_free_coherent(void *coherent_addr, void *orig_addr,
				 size_t size)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	if (!coherent_addr || !orig_addr || !size)
		return;

	/*
	 * Only remove MMU mapping for large buffers (>= SMALL_PAGE_SIZE).
	 * Small buffers may share MMU mappings with other allocations.
	 * For small buffers, just free the memory and leave mapping intact.
	 * The mapping will be cleaned up when the session ends or when
	 * the large buffer that owns the mapping is freed.
	 */
	if (size >= SMALL_PAGE_SIZE) {
		res = core_mmu_remove_mapping(MEM_AREA_TEE_COHERENT,
					      coherent_addr, size);
		if (res != TEE_SUCCESS)
			EMSG("Failed to remove coherent mapping: 0x%x", res);
	} else {
		DMSG("Skipping MMU mapping removal for small buffer (size=%zu < %u)",
		     size, SMALL_PAGE_SIZE);
	}

	/*
	 * Free and wipe the original buffer.
	 * This wipe ensures any secure data from TME is cleared
	 * before freeing it to prevent potential security issues.
	 */
	free_wipe(orig_addr);
}

/*
 * Check if TMEL is bypassed by reading the FEATURE_CONFIG2 fuse register.
 *
 * The TMEL_BYPASS_DISABLE bit semantics:
 *   0 = bypass is NOT disabled => TMEL is bypassed (not up)
 *   1 = bypass is disabled     => TMEL is active (up)
 *
 * Returns true if TMEL is not up (bit is 0), false if TMEL is active (bit is 1).
 */
static bool is_tmel_bypassed(void)
{
	struct io_pa_va feature_config2_pa_va = {
		.pa = FEATURE_CONFIG2_ADDR,
		.va = 0,
	};
	uint32_t *feature_config2_reg;
	uint32_t reg_value;

	feature_config2_reg = (uint32_t *)io_pa_or_va(&feature_config2_pa_va,
						       sizeof(uint32_t));
	if (!feature_config2_reg)
		return false;

	reg_value = io_read32((vaddr_t)feature_config2_reg);

	/* TMEL is bypassed (not up) when the BYPASS_DISABLE bit is 0 */
	return !(reg_value & FEATURE_CONFIG2_TMEL_BYPASS_DISABLE_BMSK);
}

/*
 * Check if TME server is connected and ready for communication.
 * Validates link state, channel handle, and connection status.
 */
static bool is_server_connected(struct tmecom_glink_ctx *ctx)
{
	if (!ctx || !ctx->link_up || !ctx->ch_handle ||
	    ctx->glink_state != GLINK_CONNECTED)
		return false;

	return true;
}

/*
 * Glink Callbacks
 */

/*
 * Handles incoming message from TME and copies response data.
 * Processes both mailbox-only and SRAM-based message types.
 */
static void tmecom_notify_rx(glink_handle_type handle,
			     const void *priv,
			     const void *pkt_priv,
			     const void *ptr,
			     size_t size,
			     size_t intents_used)
{
	struct tmecom_user_cb_data out_user_data = {0};
	tmecom_notify_rx_callback_t send_rsp_cb = NULL;
	enum tmecom_response tme_rsp = TMECOM_RSP_FAILURE;
	struct tmecom_ipc_header *msg_hdr = &g_ipc_mailbox.msg_hdr;
	union tmecom_mbox_ipc_payload *ipc_payload = &g_ipc_mailbox.payload;
	enum tmecom_ipc_t ipc_type;
	void *user_payload = NULL;
	size_t user_payload_len = 0;
	void *payload_data = NULL;
	enum glink_err_type rx_done_ret;
	uint32_t exceptions;

	/* Suppress unused parameter warnings */
	(void)handle;
	(void)priv;
	(void)pkt_priv;
	(void)intents_used;

	if (!ptr)
		return;

	/*
	 * Mask interrupts at entry to ensure critical section executes
	 * with interrupts disabled as required by OP-TEE spinlock model:
	 * mask → acquire lock → critical section → unlock → unmask
	 */
	exceptions = thread_mask_exceptions(THREAD_EXCP_FOREIGN_INTR);

	memcpy(&out_user_data, &glink_ctx.user_data, sizeof(out_user_data));
	user_payload = out_user_data.cb_data.generic_payload;
	user_payload_len = out_user_data.cb_data.generic_payload_len;

	if (!user_payload) {
		thread_unmask_exceptions(exceptions);
		return;
	}

	/* Copy mailbox data locally */
	memcpy(&g_ipc_mailbox, ptr,
	       MIN(size, sizeof(struct tmecom_mbox_ipc_pkt)));

	/* Extract response */
	glink_ctx.remote_rsp = (enum tmecom_response)(msg_hdr->response);
	tme_rsp = glink_ctx.remote_rsp;

	ipc_type = msg_hdr->ipc_type;

	/* Copy back generic payload from mailbox */
	if (ipc_type == TMECOM_IPC_TYPE_MBOX_ONLY) {
		payload_data = &ipc_payload->mailbox_payload.param;
		memcpy(user_payload, payload_data,
		       MIN(user_payload_len,
			   sizeof(struct tmecom_mbox_only_payload)));
	} else if (ipc_type == TMECOM_IPC_TYPE_MBOX_SRAM) {
		paddr_t payload_paddr =
			(paddr_t)ipc_payload->sram_payload.payload_ptr;
		/* Convert physical address to coherent virtual address */
		payload_data = phys_to_virt(payload_paddr,
					    MEM_AREA_TEE_COHERENT,
					    TMECOM_SRAM_IPC_MAX_BUF_SIZE);
		if (payload_data)
			memcpy(user_payload, payload_data,
			       MIN(user_payload_len,
				   TMECOM_SRAM_IPC_MAX_BUF_SIZE));
	}

	/* Set global rx state */
	glink_ctx.rx_state = TMECOM_RX_IN_PROGRESS;

	/* Call rx_done */
	rx_done_ret = glink_rx_done(glink_ctx.ch_handle, ptr, false);
	if (rx_done_ret != GLINK_STATUS_SUCCESS) {
		EMSG("glink_rx_done failed: %d", rx_done_ret);
		/* Continue processing despite rx_done failure */
	}
	glink_ctx.rx_state = TMECOM_RX_DONE;

	if (!glink_ctx.tmecom_blocking) {
		send_rsp_cb = out_user_data.cb_after_rx;
		if (send_rsp_cb) {
			/*
			 * Unlock and unmask interrupts before callback execution
			 */
			cpu_spin_unlock(&glink_ctx.tmecom_lock);
			thread_unmask_exceptions(exceptions);
			send_rsp_cb(tme_rsp, &out_user_data.cb_data);
			return;
		}
	}

	/* Unmask interrupts before returning (for blocking case) */
	thread_unmask_exceptions(exceptions);
}

/*
 * Callback for successful message transmission completion.
 * Updates global TX state to indicate transmission is complete.
 */
static void tmecom_notify_tx_done(glink_handle_type handle,
				  const void *priv,
				  const void *pkt_priv,
				  const void *ptr,
				  size_t size)
{
	/* Suppress unused parameter warnings */
	(void)handle;
	(void)priv;
	(void)pkt_priv;
	(void)ptr;
	(void)size;

	glink_ctx.tx_state = TMECOM_TX_DONE;
}

/*
 * Callback for aborted message transmission.
 * Updates global TX state to indicate transmission was aborted.
 */
static void tmecom_notify_tx_abort(glink_handle_type handle,
				   const void *priv,
				   const void *pkt_priv)
{
	/* Suppress unused parameter warnings */
	(void)handle;
	(void)priv;
	(void)pkt_priv;

	glink_ctx.tx_state = TMECOM_TX_ABORT;
}

/*
 * Handles GLink channel state changes and updates context.
 * Manages connection, disconnection, and cleanup operations.
 */
static void tmecom_notify_channel_state_isr(glink_handle_type handle,
					    const void *priv,
					    enum glink_channel_event_type event)
{
	struct tmecom_glink_ctx *ctx = (struct tmecom_glink_ctx *)priv;
	enum glink_err_type glink_ret;

	/* Suppress unused parameter warning */
	(void)handle;

	if (!ctx)
		return;

	switch (event) {
	case GLINK_LOCAL_DISCONNECTED:
		ctx->ch_handle = NULL;
		ctx->glink_state = GLINK_LOCAL_DISCONNECTED;
		break;

	case GLINK_CONNECTED:
		ctx->glink_state = GLINK_CONNECTED;
		break;

	case GLINK_REMOTE_DISCONNECTED:
		ctx->glink_state = GLINK_REMOTE_DISCONNECTED;
		glink_ret = glink_close(ctx->ch_handle);
		if (glink_ret != GLINK_STATUS_SUCCESS)
			EMSG("glink_close failed: %d", glink_ret);
		break;

	default:
		break;
	}
}

/*
 * Handles GLink transport link state changes.
 * Opens channel when link comes up, manages link down events.
 */
static void tmecom_notify_link_state_isr(struct glink_link_info *link_info,
					 void *priv)
{
	struct tmecom_glink_ctx *ctx = (struct tmecom_glink_ctx *)priv;
	struct glink_open_config ch_cfg;
	enum glink_err_type glink_ret;

	if (!ctx || !link_info)
		return;

	if (link_info->link_state == GLINK_LINK_STATE_UP) {
		DMSG("Link up: %s", ctx->cfg->remote);

		if (ctx->ch_handle) {
			EMSG("Channel not completely closed!");
			return;
		}

		ctx->link_up = true;

		/* Open channel */
		ch_cfg.remote_ss = ctx->cfg->remote;
		ch_cfg.name = ctx->cfg->channel;
		ch_cfg.priv = ctx;
		ch_cfg.notify_state = tmecom_notify_channel_state_isr;
		ch_cfg.notify_rx = tmecom_notify_rx;
		ch_cfg.notify_tx_done = tmecom_notify_tx_done;
		ch_cfg.notify_tx_abort = tmecom_notify_tx_abort;

		glink_ret = glink_open(&ch_cfg, &ctx->ch_handle);
		if (glink_ret != GLINK_STATUS_SUCCESS || !ctx->ch_handle) {
			EMSG("Channel open failed: %d", glink_ret);
			ctx->ch_handle = NULL;
		}
	} else if (link_info->link_state == GLINK_LINK_STATE_DOWN) {
		EMSG("Link down");
		ctx->link_up = false;
	}
}

/*
 * Public API Functions
 */

/*
 * Initialize TME COM session and establish communication channel.
 * Sets up GLink transport, registers callbacks, and allocates buffers.
 */
TEE_Result tmecom_client_session_start(void)
{
	enum tmecom_response result = TMECOM_RSP_SUCCESS;
	enum glink_err_type glink_ret;
	uint64_t start_time;
	uint32_t exceptions;

	/* Check if TMEL is bypassed */
	if (is_tmel_bypassed()) {
		DMSG("TMEL not up - Skipping session start for TMECOM");
		return TEE_SUCCESS;
	}

	/* Initialize lock */
	glink_ctx.tmecom_lock = SPINLOCK_UNLOCK;

	/* Disable foreign interrupts before acquiring spinlock */
	exceptions = thread_mask_exceptions(THREAD_EXCP_FOREIGN_INTR);

	/* Try to acquire lock - return BUSY if already held */
	if (!cpu_spin_trylock(&glink_ctx.tmecom_lock)) {
		EMSG("Session start failed: lock already held");
		thread_unmask_exceptions(exceptions);
		return tmecom_to_tee_result(TMECOM_RSP_FAILURE_BUSY);
	}

	WRITE_ONCE(glink_ctx.glink_state, GLINK_LOCAL_DISCONNECTED);

	if (!glink_ctx.link_up) {
		glink_init();

		/* Get client glink configuration */
		glink_ctx.cfg = &tmecom_cfg;
		if (!glink_ctx.cfg) {
			EMSG("Failed to get glink configuration");
			result = TMECOM_RSP_FAILURE;
			goto exit;
		}

		GLINK_LINK_ID_STRUCT_INIT(glink_ctx.link_id);
		glink_ctx.link_id.remote_ss = glink_ctx.cfg->remote;
		glink_ctx.link_id.link_notifier = tmecom_notify_link_state_isr;

		/* Register link state callback */
		glink_ret = glink_register_link_state_cb(&glink_ctx.link_id,
							 &glink_ctx);
		if (glink_ret != GLINK_STATUS_SUCCESS) {
			EMSG("Link state cb register failed: %d", glink_ret);
			result = TMECOM_RSP_FAILURE_LINK_ERR;
			goto exit;
		}
	}

	/* Wait for Connect or remote disconnect */
	start_time = timeout_init_us(TMECOM_IPC_MAX_WAIT_FOR_RESPONSE);
	while (READ_ONCE(glink_ctx.glink_state) == GLINK_LOCAL_DISCONNECTED) {
		if (timeout_elapsed(start_time)) {
			EMSG("Timeout waiting for glink connection");
			break;
		}
	}

	/* Make sure remote is connected */
	if (READ_ONCE(glink_ctx.glink_state) == GLINK_REMOTE_DISCONNECTED) {
		EMSG("Remote disconnected during session start");
		result = TMECOM_RSP_FAILURE_CHANNEL_ERR;
		goto exit;
	}

	/* Allocate g_sram_payload_data for this session */
	g_sram_payload_data_coh =
		tmecom_client_malloc_coherent(TMECOM_SRAM_IPC_MAX_BUF_SIZE,
					      TMECOM_4_BYTE_ALIGNED,
					      &g_sram_payload_orig_addr,
					      &g_sram_payload_data_paddr);
	if (!g_sram_payload_data_coh) {
		EMSG("Failed to allocate SRAM payload buffer");
		result = TMECOM_RSP_FAILURE;
		goto exit;
	}

exit:
	if (result) {
		if (g_sram_payload_data_coh) {
			/* Error occurred, free SRAM buffer */
			tmecom_client_free_coherent(g_sram_payload_data_coh,
						    g_sram_payload_orig_addr,
						    TMECOM_SRAM_IPC_MAX_BUF_SIZE);
			g_sram_payload_data_coh = NULL;
			g_sram_payload_orig_addr = NULL;
			g_sram_payload_data_paddr = 0;
		}

		/* De-register client callbacks */
		if (glink_ctx.link_id.handle)
			glink_deregister_link_state_cb(glink_ctx.link_id.handle);

		/* Clear global Glink context */
		memset(&glink_ctx, 0, sizeof(glink_ctx));

	}

	cpu_spin_unlock(&glink_ctx.tmecom_lock);
	thread_unmask_exceptions(exceptions);

	return tmecom_to_tee_result(result);
}

/*
 * Terminate TME COM session and cleanup resources.
 * Closes channel, deregisters callbacks, and frees all buffers.
 */
TEE_Result tmecom_client_session_end(void)
{
	enum tmecom_response result = TMECOM_RSP_SUCCESS;
	enum glink_err_type glink_ret;
	uint64_t start_time;
	uint32_t exceptions;

	/* Disable foreign interrupts before acquiring spinlock */
	exceptions = thread_mask_exceptions(THREAD_EXCP_FOREIGN_INTR);

	/* Try to acquire lock - return BUSY if already held */
	if (!cpu_spin_trylock(&glink_ctx.tmecom_lock)) {
		EMSG("Session end failed: lock already held");
		result = TMECOM_RSP_FAILURE_BUSY;
		thread_unmask_exceptions(exceptions);
		goto exit;
	}

	glink_ret = glink_close(glink_ctx.ch_handle);
	if (glink_ret != GLINK_STATUS_SUCCESS) {
		EMSG("Channel close failed: %d", glink_ret);
		result = TMECOM_RSP_FAILURE_LINK_ERR;
		goto unlock_exit;
	}

	/* De-register client callbacks */
	glink_ret = glink_deregister_link_state_cb(glink_ctx.link_id.handle);
	if (glink_ret != GLINK_STATUS_SUCCESS) {
		EMSG("Link state cb de-register failed: %d", glink_ret);
		result = TMECOM_RSP_FAILURE_LINK_ERR;
		goto unlock_exit;
	}

	/* Wait for GLINK_LOCAL_DISCONNECTED */
	start_time = timeout_init_us(TMECOM_IPC_MAX_WAIT_FOR_RESPONSE);
	while (READ_ONCE(glink_ctx.glink_state) != GLINK_LOCAL_DISCONNECTED) {
		if (timeout_elapsed(start_time))
			break;
	}

	/* Free g_sram_payload_data buffer */
	if (g_sram_payload_data_coh && g_sram_payload_orig_addr) {
		tmecom_client_free_coherent(g_sram_payload_data_coh,
					    g_sram_payload_orig_addr,
					    TMECOM_SRAM_IPC_MAX_BUF_SIZE);
		g_sram_payload_data_coh = NULL;
		g_sram_payload_orig_addr = NULL;
		g_sram_payload_data_paddr = 0;
	}

	/* Clear global Glink context */
	memset(&glink_ctx, 0, sizeof(glink_ctx));

unlock_exit:
	cpu_spin_unlock(&glink_ctx.tmecom_lock);
	thread_unmask_exceptions(exceptions);

exit:
	return tmecom_to_tee_result(result);
}

/*
 * Send message to TME and optionally wait for response.
 * Supports both blocking and non-blocking operation modes.
 */
TEE_Result
tmecom_client_send_message(uint32_t tme_msg_uid, uint32_t tme_msg_param_id,
			   bool is_blocking, uint32_t timeout,
			   void *generic_payload, uint32_t generic_payload_len,
			   tmecom_notify_rx_callback_t cb_api, void *user_data,
			   enum tmecom_response *tme_err)
{
	enum tmecom_response result = TMECOM_RSP_SUCCESS;
	void *pkt_priv = NULL;
	uint32_t tx_flags = 0;
	uint32_t msg_header_len = sizeof(struct tmecom_ipc_header);
	uint64_t start_time;
	struct tmecom_user_cb_data *in_user_data = &glink_ctx.user_data;
	struct tmecom_ipc_header *msg_hdr = &g_ipc_mailbox.msg_hdr;
	union tmecom_mbox_ipc_payload *ipc_payload = &g_ipc_mailbox.payload;
	void *payload_data = NULL;
	bool lock_held = false;
	uint32_t exceptions = 0;

	if (is_tmel_bypassed()) {
		/* TMEL is bypassed, return success to prevent IPC calls from failing */
		if (tme_err)
			*tme_err = TMECOM_RSP_SUCCESS;
		DMSG("TMEL not up - Skipping TMECOM IPC");
		return TEE_SUCCESS;
	}

	if (!generic_payload || !generic_payload_len) {
		EMSG("Invalid parameters: uid=0x%x payload=%p len=%u",
		     tme_msg_uid, generic_payload, generic_payload_len);
		result = TMECOM_RSP_FAILURE_INVALID_ARGS;
		goto exit;
	}

	/* Check if channel is connected */
	if (!is_server_connected(&glink_ctx)) {
		EMSG("Server not connected: msg uid=0x%x", tme_msg_uid);
		result = TMECOM_RSP_FAILURE;
		goto exit;
	}

	/* For non-blocking calls callback API must be passed */
	if (!is_blocking && !cb_api) {
		EMSG("Non-blocking call requires callback: msg uid=0x%x",
		     tme_msg_uid);
		result = TMECOM_RSP_FAILURE_INVALID_ARGS;
		goto exit;
	}

	/* Disable foreign interrupts before acquiring spinlock */
	exceptions = thread_mask_exceptions(THREAD_EXCP_FOREIGN_INTR);

	/* Try to acquire lock - return BUSY if already held */
	if (!cpu_spin_trylock(&glink_ctx.tmecom_lock)) {
		EMSG("Send message failed: lock already held, uid=0x%x",
		     tme_msg_uid);
		result = TMECOM_RSP_FAILURE_BUSY;
		thread_unmask_exceptions(exceptions);
		goto exit;
	}
	lock_held = true;

	/* Initialize global state variables */
	WRITE_ONCE(glink_ctx.tx_state, TMECOM_TX_NONE);
	WRITE_ONCE(glink_ctx.rx_state, TMECOM_RX_NONE);

	/* Set cb_after_rx & cb_user_data */
	in_user_data->cb_after_rx = cb_api;
	in_user_data->cb_data.user_data = user_data;
	in_user_data->cb_data.generic_payload = generic_payload;
	in_user_data->cb_data.generic_payload_len = generic_payload_len;
	in_user_data->cb_data.tme_msg_uid = tme_msg_uid;
	in_user_data->param_id = tme_msg_param_id;
	glink_ctx.tmecom_blocking = is_blocking;

	/* Clear stale data */
	memset(&g_ipc_mailbox, 0, sizeof(struct tmecom_mbox_ipc_pkt));
	memset(g_sram_payload_data_coh, 0, TMECOM_SRAM_IPC_MAX_BUF_SIZE);

	/* Prepare the Mailbox */
	if (msg_header_len + generic_payload_len <=
	    TMECOM_MBOX_IPC_PACKET_SIZE) {
		msg_hdr->ipc_type = TMECOM_IPC_TYPE_MBOX_ONLY;
		msg_hdr->msg_len = generic_payload_len;
		payload_data = &ipc_payload->mailbox_payload.param;
		memcpy(payload_data, generic_payload,
		       MIN(generic_payload_len,
			   sizeof(struct tmecom_mbox_only_payload)));
	} else if (generic_payload_len <= TMECOM_SRAM_IPC_MAX_BUF_SIZE) {
		msg_hdr->ipc_type = TMECOM_IPC_TYPE_MBOX_SRAM;
		msg_hdr->msg_len = 8;
		ipc_payload->sram_payload.payload_len = generic_payload_len;
		ipc_payload->sram_payload.payload_ptr =
			(uint32_t)g_sram_payload_data_paddr;

		/* Copy generic_payload data into SRAM buffer */
		memcpy(g_sram_payload_data_coh, generic_payload,
		       MIN(generic_payload_len,
			   TMECOM_SRAM_IPC_MAX_BUF_SIZE));
	} else {
		EMSG("Invalid Payload length: %u", generic_payload_len);
		result = TMECOM_RSP_FAILURE_INVALID_ARGS;
		goto exit;
	}

	msg_hdr->msg_type = (tme_msg_uid >> 8) & 0xFF;
	msg_hdr->action_id = tme_msg_uid & 0xFF;

	/* Perform the IPC transaction */
	WRITE_ONCE(glink_ctx.tx_state, TMECOM_TX_IN_PROGRESS);
	result = (enum tmecom_response)glink_tx(glink_ctx.ch_handle, pkt_priv,
					     &g_ipc_mailbox,
					     sizeof(struct tmecom_mbox_ipc_pkt),
					     tx_flags);
	if (result != (enum tmecom_response)GLINK_STATUS_SUCCESS) {
		EMSG("glink_tx failed: %d", result);
		result = TMECOM_RSP_FAILURE_TX_ERR;
		goto exit;
	}

	/* Wait for tx_done event */
	start_time = timeout_init_us(TMECOM_IPC_MAX_WAIT_FOR_RESPONSE);
	while (READ_ONCE(glink_ctx.tx_state) == TMECOM_TX_IN_PROGRESS) {
		if (timeout_elapsed(start_time)) {
			EMSG("Timeout waiting for tx_done");
			break;
		}
	}

	if (READ_ONCE(glink_ctx.tx_state) != TMECOM_TX_DONE) {
		EMSG("Invalid tx_state: %d", glink_ctx.tx_state);
		result = TMECOM_RSP_FAILURE_TX_ERR;
		goto exit;
	}

	if (is_blocking) {
		/* Wait till receiver is processing msg */
		timeout = timeout ? timeout : TMECOM_DEFAULT_TIMEOUT;
		start_time = timeout_init_us(timeout);
		while (READ_ONCE(glink_ctx.rx_state) != TMECOM_RX_DONE) {
			if (timeout_elapsed(start_time)) {
				EMSG("Timeout waiting for rx_done");
				result = TMECOM_RSP_FAILURE_RX_ERR;
				goto exit;
			}
		}

		/* Return response received from remote */
		if (glink_ctx.remote_rsp) {
			if (tme_err)
				*tme_err = glink_ctx.remote_rsp;
			EMSG("Response from TME: %d", glink_ctx.remote_rsp);
			result = TMECOM_SERVICE_API_RETURNED_ERR;
		}

		cpu_spin_unlock(&glink_ctx.tmecom_lock);
		thread_unmask_exceptions(exceptions);
		return tmecom_to_tee_result(result);
	}

exit:
	if (lock_held && result) {
		cpu_spin_unlock(&glink_ctx.tmecom_lock);
		thread_unmask_exceptions(exceptions);
	}

	if (tme_err)
		*tme_err = result;

	return tmecom_to_tee_result(result);
}

early_init(tmecom_client_session_start);
