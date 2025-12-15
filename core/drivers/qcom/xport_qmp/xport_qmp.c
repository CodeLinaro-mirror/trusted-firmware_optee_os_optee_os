// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <io.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <kernel/spinlock.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <string.h>
#include <trace.h>
#include <util.h>

#include "xport_qmp.h"
#include "xport_qmp_config.h"
#include "glink_xport.h"

/*
 * QMP Descriptor Bit Positions
 */
#define QMP_DESC_LOCAL_LINK_STATE_BIT		0
#define QMP_DESC_REMOTE_LINK_STATE_ACK_BIT	1
#define QMP_DESC_LOCAL_CH_STATE_BIT		2
#define QMP_DESC_REMOTE_CH_STATE_ACK_BIT	3
#define QMP_DESC_LOCAL_TX_BIT			4
#define QMP_DESC_REMOTE_TX_ACK_BIT		5
#define QMP_DESC_LOCAL_RX_DONE_BIT		6
#define QMP_DESC_REMOTE_RX_DONE_ACK_BIT		7
#define QMP_DESC_LOCAL_READ_INT_BIT		8
#define QMP_DESC_REMOTE_READ_INT_ACK_BIT	9

/*
 * QMP Descriptor Flag Manipulation Macros
 * Operate on in-RAM descriptor snapshots using READ_ONCE/WRITE_ONCE.
 */

#define QMP_DESC_WORD_READ(ctx, place) \
	READ_ONCE((ctx)->place##_desc.word)

#define QMP_DESC_WORD_WRITE(ctx, place, val) \
	WRITE_ONCE((ctx)->place##_desc.word, (val))

#define QMP_FLAG_GET(ctx, place, flag_bit) \
	((QMP_DESC_WORD_READ(ctx, place) >> (flag_bit)) & 1U)

#define QMP_FLAG_SET(ctx, place, flag_bit) \
	QMP_DESC_WORD_WRITE(ctx, place, \
		QMP_DESC_WORD_READ(ctx, place) | (1U << (flag_bit)))

#define QMP_FLAG_CLR(ctx, place, flag_bit) \
	QMP_DESC_WORD_WRITE(ctx, place, \
		QMP_DESC_WORD_READ(ctx, place) & ~(1U << (flag_bit)))

#define QMP_LOCAL_FLAG_TOGGLE(ctx, flag) \
	QMP_DESC_WORD_WRITE(ctx, local, \
		QMP_DESC_WORD_READ(ctx, local) ^ \
			(1U << QMP_DESC_LOCAL_##flag##_BIT))

#define QMP_LOCAL_FLAG_GET(ctx, flag) \
	QMP_FLAG_GET(ctx, local, QMP_DESC_LOCAL_##flag##_BIT)

#define QMP_LOCAL_FLAG_SET(ctx, flag) \
	QMP_FLAG_SET(ctx, local, QMP_DESC_LOCAL_##flag##_BIT)

#define QMP_LOCAL_FLAG_CLR(ctx, flag) \
	QMP_FLAG_CLR(ctx, local, QMP_DESC_LOCAL_##flag##_BIT)

#define QMP_LOCAL_FLAG_ACK_CLR(ctx, flag) \
	QMP_FLAG_CLR(ctx, local, QMP_DESC_REMOTE_##flag##_ACK_BIT)

#define QMP_REMOTE_FLAG_GET(ctx, flag) \
	QMP_FLAG_GET(ctx, remote, QMP_DESC_LOCAL_##flag##_BIT)

#define QMP_REMOTE_ACKED_CHECK(ctx, flag) \
	(QMP_LOCAL_FLAG_GET(ctx, flag) == \
	 QMP_FLAG_GET(ctx, remote, QMP_DESC_REMOTE_##flag##_ACK_BIT))

#define QMP_LOCAL_ACKED_CHECK(ctx, flag) \
	(QMP_REMOTE_FLAG_GET(ctx, flag) == \
	 QMP_FLAG_GET(ctx, local, QMP_DESC_REMOTE_##flag##_ACK_BIT))

#define QMP_LOCAL_ACK_UPDATE(ctx, flag) \
	do { \
		uint32_t __ack_bit = QMP_DESC_REMOTE_##flag##_ACK_BIT; \
		if (QMP_REMOTE_FLAG_GET(ctx, flag)) \
			QMP_FLAG_SET(ctx, local, __ack_bit); \
		else \
			QMP_FLAG_CLR(ctx, local, __ack_bit); \
	} while (0)

#define QMP_REMOTE_FLAG_TOGGLED_CHECK(ctx, flag) \
	(QMP_REMOTE_FLAG_GET(ctx, flag) != \
	 QMP_FLAG_GET(ctx, local, QMP_DESC_REMOTE_##flag##_ACK_BIT))

/*
 * QMP States
 */
enum xport_qmp_state {
	LINK_DOWN = 0,
	LINK_NEGOTIATION = 1,
	LINK_UP = 2,
	LOCAL_CONNECTING = 3,
	LOCAL_CONNECTED = 4,
	E2ECONNECTED = 5,
	LOCAL_DISCONNECTING = 6,
};

/*
 * QMP Descriptor
 */
struct xport_qmp_desc {
	uint32_t local_link_state:1;
	uint32_t remote_link_state_ack:1;
	uint32_t local_ch_state:1;
	uint32_t remote_ch_state_ack:1;
	uint32_t local_tx:1;
	uint32_t remote_tx_ack:1;
	uint32_t local_rx_done:1;
	uint32_t remote_rx_done_ack:1;
	uint32_t local_read_int:1;
	uint32_t remote_read_int_ack:1;
	uint32_t reserved:6;
	uint32_t cur_frag_size:8;
	uint32_t rem_frags_cnt:8;
};

/*
 * Union to access descriptor as a bitfield struct or atomic 32-bit word.
 * Enables READ/WRITE_ONCE operations while avoiding strict-aliasing warnings.
 */
union qmp_desc_word {
	struct xport_qmp_desc desc;
	uint32_t word;
};

/*
 * QMP Context
 */
struct xport_qmp_ctx {
	struct glink_transport_if xport_if;
	const struct xport_qmp_config *cfg;
	unsigned int cs;
	enum xport_qmp_state state;
	union qmp_desc_word local_desc;
	union qmp_desc_word remote_desc;
	struct glink_core_tx_pkt *tx_pkt_ctx;
	uint8_t *rx_pkt_buf;
	uint32_t rx_pkt_size;
	uint32_t rx_pkt_read_size;
	vaddr_t shared_local_desc;
	vaddr_t shared_remote_desc;
	vaddr_t shared_local_mailbox;
	vaddr_t shared_remote_mailbox;
	uint32_t cfg_local_mailbox_size;
	uint32_t cfg_tx_max_pkt_size;
	uint32_t cfg_remote_mailbox_size;
	uint32_t cfg_rx_max_pkt_size;
};

static struct xport_qmp_ctx xport_qmp_ctxs[GLINK_CFG_MAX_REMOTE_HOSTS];
static struct xport_qmp_ctx *g_tmel_ctx;

/*
 * Sends interrupt to remote processor and updates shared descriptor.
 * Copies local descriptor to shared memory and triggers hardware interrupt.
 */
static void xport_qmp_intr_send(struct xport_qmp_ctx *ctx)
{
	vaddr_t reg_addr;
	uint32_t reg_val;
	uint32_t local_desc;

	/* Copy local_desc to shared local_desc */
	local_desc = READ_ONCE(ctx->local_desc.word);
	io_write32(ctx->shared_local_desc, local_desc);

	/* Trigger interrupt to TME */
	reg_addr = ctx->cfg->irq_out.reg_addr;
	reg_val = ctx->cfg->irq_out.reg_val;

	/* Set interrupt bit to fire interrupt*/
	io_setbits32(reg_addr, reg_val);

	/* Clear interrupt bit */
	io_clrbits32(reg_addr, reg_val);
}

/*
 * Writes data buffer to shared mailbox memory.
 * Handles 32-bit aligned writes with remainder byte handling.
 */
static void xport_qmp_mailbox_write(struct xport_qmp_ctx *ctx,
				    void *buf,
				    uint32_t size)
{
	uint32_t size_remainder = size % 4;
	uint32_t size_rounddown = size - size_remainder;
	uint32_t *src_ptr = (uint32_t *)buf;
	uint32_t *src_ptr_end = (uint32_t *)((uintptr_t)buf + size_rounddown);
	vaddr_t dst_addr = ctx->shared_local_mailbox;
	uint32_t data;

	/* Write in 32-bit increments */
	while (src_ptr < src_ptr_end) {
		memcpy(&data, src_ptr, sizeof(uint32_t));
		io_write32(dst_addr, data);
		dst_addr += sizeof(uint32_t);
		src_ptr++;
	}

	if (size_remainder) {
		data = 0;
		memcpy(&data, src_ptr, size_remainder);
		io_write32(dst_addr, data);
	}
}

/*
 * Reads data from shared mailbox memory to local buffer.
 * Handles 32-bit aligned reads with remainder byte handling.
 */
static void xport_qmp_mailbox_read(struct xport_qmp_ctx *ctx,
				   void *buf,
				   uint32_t size)
{
	uint32_t size_remainder = size % 4;
	uint32_t size_rounddown = size - size_remainder;
	vaddr_t src_addr = ctx->shared_remote_mailbox;
	uint32_t *dst_ptr = (uint32_t *)buf;
	uint32_t *dst_ptr_end = (uint32_t *)((uintptr_t)buf + size_rounddown);

	/* Read in 32-bit increments */
	while (dst_ptr < dst_ptr_end) {
		*dst_ptr++ = io_read32(src_addr);
		src_addr += sizeof(uint32_t);
	}

	if (size_remainder) {
		uint32_t data = io_read32(src_addr);

		memcpy(dst_ptr, &data, size_remainder);
	}
}

/*
 * Sends packet fragment over QMP transport.
 * Handles packet fragmentation and updates transmission state.
 */
static void xport_qmp_pkt_send(struct xport_qmp_ctx *ctx)
{
	struct glink_core_tx_pkt *pkt_ctx = ctx->tx_pkt_ctx;
	uint32_t mbox_size = ctx->cfg_local_mailbox_size;
	uint32_t cur_frag_size;
	uint32_t rem_frags_cnt;
	size_t size_remaining;

	size_remaining = pkt_ctx->size_remaining;

	if (size_remaining <= 0)
		return;

	if (size_remaining <= mbox_size) {
		pkt_ctx->size_remaining = 0;
		cur_frag_size = size_remaining;
		rem_frags_cnt = 0;
	} else {
		cur_frag_size = mbox_size;
		pkt_ctx->size_remaining -= cur_frag_size;
		rem_frags_cnt = (size_remaining / mbox_size) - 1;
		rem_frags_cnt += ((size_remaining % mbox_size) != 0) ? 1 : 0;
	}

	/* Write to shared payload memory */
	xport_qmp_mailbox_write(ctx,
				(void *)((uintptr_t)pkt_ctx->data +
					 (pkt_ctx->size - size_remaining)),
				cur_frag_size);

	/* Update TX sizes */
	ctx->local_desc.desc.cur_frag_size = cur_frag_size;
	ctx->local_desc.desc.rem_frags_cnt = rem_frags_cnt;

	/* Toggle TX to indicate packet fragment */
	QMP_LOCAL_FLAG_TOGGLE(ctx, TX);
}

/*
 * Clears channel state flags and resets context variables.
 * Resets TX/RX state and packet context for clean state.
 */
static void xport_qmp_state_ch_flags_clr(struct xport_qmp_ctx *ctx)
{
	/* Clear channel state flags */
	QMP_LOCAL_FLAG_CLR(ctx, CH_STATE);
	QMP_LOCAL_FLAG_ACK_CLR(ctx, CH_STATE);

	/* TX and RX flags */
	QMP_LOCAL_FLAG_CLR(ctx, TX);
	QMP_LOCAL_FLAG_ACK_CLR(ctx, TX);

	QMP_LOCAL_FLAG_CLR(ctx, RX_DONE);
	QMP_LOCAL_FLAG_ACK_CLR(ctx, RX_DONE);

	QMP_LOCAL_FLAG_CLR(ctx, READ_INT);
	QMP_LOCAL_FLAG_ACK_CLR(ctx, READ_INT);

	ctx->local_desc.desc.cur_frag_size = 0;
	ctx->local_desc.desc.rem_frags_cnt = 0;

	ctx->tx_pkt_ctx = NULL;
	ctx->rx_pkt_buf = NULL;
	ctx->rx_pkt_size = 0;
	ctx->rx_pkt_read_size = 0;
}

/*
 * Handles QMP state machine transitions and notifications.
 * Processes link and channel state changes with core notifications.
 */
static bool xport_qmp_state_handler(struct xport_qmp_ctx *ctx,
				    uint32_t *exceptions)
{
	struct glink_transport_if *xport_if = &ctx->xport_if;
	bool ret = true;
	bool core_notify_linkup = false;
	bool core_notify_remote_close = false;
	bool core_notify_remote_open = false;
	bool core_notify_close_ack = false;

	/* Check for Remote Link down first */
	if (ctx->state >= LINK_UP && !QMP_REMOTE_FLAG_GET(ctx, LINK_STATE)) {
		ctx->state = LINK_NEGOTIATION;

		cpu_spin_unlock_xrestore(&ctx->cs, *exceptions);
		glink_core_rx_cmd_link_down(xport_if);
		*exceptions = cpu_spin_lock_xsave(&ctx->cs);

		QMP_LOCAL_ACK_UPDATE(ctx, LINK_STATE);
		return ret;
	}

	switch (ctx->state) {
	case LINK_DOWN:
		/* Check for remote Acked for earlier change */
		if (!QMP_REMOTE_ACKED_CHECK(ctx, LINK_STATE)) {
			QMP_LOCAL_ACK_UPDATE(ctx, LINK_STATE);
			break;
		}

		/* Check if it is LOCAL SSR case */
		if (QMP_LOCAL_FLAG_GET(ctx, LINK_STATE)) {
			/* Local SSR, inform down event first to remote */
			QMP_LOCAL_FLAG_CLR(ctx, LINK_STATE);
			break;
		}

		/* Set the local link up state */
		QMP_LOCAL_FLAG_SET(ctx, LINK_STATE);
		ctx->state = LINK_NEGOTIATION;
		break;

	case LINK_NEGOTIATION:
		/* Check for remote Acked for earlier change */
		if (!QMP_REMOTE_ACKED_CHECK(ctx, LINK_STATE)) {
			QMP_LOCAL_ACK_UPDATE(ctx, LINK_STATE);
			break;
		}

		/* Check if remote is link up */
		if (!QMP_REMOTE_FLAG_GET(ctx, LINK_STATE)) {
			ret = false;
			break;
		}

		/* Clear all channel and tx/rx flags */
		xport_qmp_state_ch_flags_clr(ctx);

		/* Ack to remote link up */
		QMP_LOCAL_ACK_UPDATE(ctx, LINK_STATE);
		ctx->state = LINK_UP;

		/* Notify Link up to Core */
		core_notify_linkup = true;
		break;

	case LINK_UP:
		/* No need to Ack remote channel open until local opens */
		ret = false;
		break;

	case LOCAL_CONNECTING:
		/* Ack remote ch_state change */
		QMP_LOCAL_ACK_UPDATE(ctx, CH_STATE);

		/* Check if remote is Acked for channel up */
		if (!QMP_REMOTE_ACKED_CHECK(ctx, CH_STATE))
			break;

		if (!QMP_REMOTE_FLAG_GET(ctx, CH_STATE))
			break;

		ctx->state = E2ECONNECTED;
		core_notify_remote_open = true;
		break;

	case E2ECONNECTED:
		/* Check for remote channel down */
		if (!QMP_REMOTE_FLAG_GET(ctx, CH_STATE)) {
			ctx->state = LOCAL_CONNECTING;
			QMP_LOCAL_ACK_UPDATE(ctx, CH_STATE);
			core_notify_remote_close = true;
			break;
		}
		ret = false;
		break;

	case LOCAL_DISCONNECTING:
		/* Check for remote channel down */
		if (!QMP_REMOTE_FLAG_GET(ctx, CH_STATE) &&
		    !QMP_LOCAL_ACKED_CHECK(ctx, CH_STATE)) {
			QMP_LOCAL_ACK_UPDATE(ctx, CH_STATE);
		}

		/* Check if remote is Acked for channel up */
		if (!QMP_REMOTE_ACKED_CHECK(ctx, CH_STATE)) {
			ret = false;
			break;
		}

		/* Clear all channel and tx/rx flags */
		xport_qmp_state_ch_flags_clr(ctx);

		ctx->state = LINK_UP;
		core_notify_close_ack = true;
		break;

	default:
		ret = false;
		break;
	}

	/* Release lock before calling Core functions */
	cpu_spin_unlock_xrestore(&ctx->cs, *exceptions);

	if (core_notify_close_ack)
		glink_core_rx_cmd_ch_close_ack(xport_if);

	if (core_notify_remote_open)
		glink_core_rx_cmd_remote_open(xport_if);

	if (core_notify_remote_close)
		glink_core_rx_cmd_remote_close(xport_if);

	if (core_notify_linkup)
		glink_core_rx_cmd_link_up(xport_if);

	/* Acquire lock again */
	*exceptions = cpu_spin_lock_xsave(&ctx->cs);

	return ret;
}

/*
 * Handles transmission acknowledgments and completion notifications.
 * Manages packet fragmentation and TX done callbacks.
 */
static bool xport_qmp_tx_handler(struct xport_qmp_ctx *ctx,
				 uint32_t *exceptions)
{
	struct glink_core_tx_pkt *pkt_ctx = ctx->tx_pkt_ctx;
	bool ret = false;

	if (!pkt_ctx || !pkt_ctx->data)
		return ret;

	/* Check if TX packet in progress */
	if (pkt_ctx->size_remaining != 0) {
		/* Send remaining fragment of TX data */
		if (QMP_REMOTE_ACKED_CHECK(ctx, TX)) {
			xport_qmp_pkt_send(ctx);
			ret = true;
		}
	}
	/* Check if remote did RX Done */
	else if (QMP_REMOTE_FLAG_TOGGLED_CHECK(ctx, RX_DONE)) {
		/* Ack to remote */
		QMP_LOCAL_ACK_UPDATE(ctx, RX_DONE);

		ctx->tx_pkt_ctx = NULL;

		cpu_spin_unlock_xrestore(&ctx->cs, *exceptions);
		glink_core_rx_cmd_tx_done(&ctx->xport_if, pkt_ctx);
		*exceptions = cpu_spin_lock_xsave(&ctx->cs);

		ret = true;
	}

	return ret;
}

/*
 * Handles incoming packet reception and fragmentation.
 * Processes RX fragments and delivers complete packets to core.
 */
static bool xport_qmp_rx_handler(struct xport_qmp_ctx *ctx,
				 uint32_t *exceptions)
{
	uint32_t mbox_size = ctx->cfg_remote_mailbox_size;
	uint32_t cur_frag_size;
	uint32_t rem_frags_cnt;
	uint32_t *rx_pkt_size = &ctx->rx_pkt_size;
	uint32_t *rx_pkt_read_size = &ctx->rx_pkt_read_size;
	bool ret = false;

	/* Check if there is any change in remote */
	if (!QMP_REMOTE_FLAG_TOGGLED_CHECK(ctx, TX))
		return ret;

	/* Read RX packet details */
	cur_frag_size = ctx->remote_desc.desc.cur_frag_size;
	rem_frags_cnt = ctx->remote_desc.desc.rem_frags_cnt;

	if (!cur_frag_size || cur_frag_size > mbox_size) {
		EMSG("RX fragment size(%u) invalid (max %u)",
		     cur_frag_size, mbox_size);
		return ret;
	}

	/* Check if it is new rx packet */
	if (!*rx_pkt_size && !*rx_pkt_read_size) {
		/* Max RX packet size */
		*rx_pkt_size = cur_frag_size + rem_frags_cnt * mbox_size;

		if (*rx_pkt_size > ctx->cfg_rx_max_pkt_size) {
			EMSG("Max RX size(%u) > max pkt size(%u)",
			     *rx_pkt_size, ctx->cfg_rx_max_pkt_size);
		} else {
			ctx->rx_pkt_buf =
				(uint8_t *)(uintptr_t)ctx->cfg->rx_pkt_static_buf;
			*rx_pkt_read_size = cur_frag_size;

			/* Read shared payload memory */
			if ((vaddr_t)ctx->cfg->rx_pkt_static_buf !=
			    ctx->shared_remote_mailbox) {
				xport_qmp_mailbox_read(ctx, ctx->rx_pkt_buf,
						       cur_frag_size);
			}

			/* Do TX ack to remote */
			QMP_LOCAL_ACK_UPDATE(ctx, TX);

			ret = true;

			if (*rx_pkt_size == *rx_pkt_read_size) {
				cpu_spin_unlock_xrestore(&ctx->cs, *exceptions);
				glink_core_rx_cmd_data(&ctx->xport_if,
						       ctx->rx_pkt_buf,
						       *rx_pkt_size);
				*exceptions = cpu_spin_lock_xsave(&ctx->cs);
			}
		}
	} else if (*rx_pkt_size && *rx_pkt_read_size != *rx_pkt_size) {
		if ((*rx_pkt_read_size + cur_frag_size +
		     (rem_frags_cnt * mbox_size)) > *rx_pkt_size) {
			EMSG("RX size overflow");
		} else {
			void *rx_buf_ptr = &ctx->rx_pkt_buf[*rx_pkt_read_size];

			/* Read shared payload memory */
			xport_qmp_mailbox_read(ctx, rx_buf_ptr, cur_frag_size);
			*rx_pkt_read_size += cur_frag_size;

			/* Do TX ack to remote */
			QMP_LOCAL_ACK_UPDATE(ctx, TX);

			ret = true;

			if (!rem_frags_cnt) {
				if (*rx_pkt_size - *rx_pkt_read_size <=
				    mbox_size) {
					/* Update actual packet size */
					*rx_pkt_size = *rx_pkt_read_size;

					cpu_spin_unlock_xrestore(&ctx->cs,
								 *exceptions);
					glink_core_rx_cmd_data(&ctx->xport_if,
							       ctx->rx_pkt_buf,
							       *rx_pkt_size);
					*exceptions =
						cpu_spin_lock_xsave(&ctx->cs);
				}
			}
		}
	}

	return ret;
}

/*
 * Interrupt service routine for QMP transport events.
 * Processes state changes, TX/RX events, and triggers responses.
 */
static enum itr_return xport_qmp_isr(struct itr_handler *h __unused)
{
	struct xport_qmp_ctx *ctx = g_tmel_ctx;
	bool intr_send = false;
	enum xport_qmp_state prv_state;
	uint32_t exceptions;
	uint32_t shared_remote_desc;

	if (!ctx)
		return ITRR_HANDLED;

	exceptions = cpu_spin_lock_xsave(&ctx->cs);

	/* Read remote descriptor and process it */
	shared_remote_desc = io_read32(ctx->shared_remote_desc);
	WRITE_ONCE(ctx->remote_desc.word, shared_remote_desc);

	do {
		prv_state = ctx->state;
		intr_send |= xport_qmp_state_handler(ctx, &exceptions);
	} while (prv_state != ctx->state);

	if (ctx->state == E2ECONNECTED) {
		intr_send |= xport_qmp_tx_handler(ctx, &exceptions);
		intr_send |= xport_qmp_rx_handler(ctx, &exceptions);
	}

	if (intr_send)
		xport_qmp_intr_send(ctx);

	cpu_spin_unlock_xrestore(&ctx->cs, exceptions);

	return ITRR_HANDLED;
}

/*
 * Transport Interface Functions - called by the Glink core layer
 */
/*
 * Opens QMP channel for communication with remote subsystem.
 * Updates channel state and sends open command to remote.
 */
static enum glink_err_type
xport_qmp_tx_cmd_ch_open(struct glink_transport_if *if_ptr)
{
	struct xport_qmp_ctx *ctx = (struct xport_qmp_ctx *)if_ptr;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&ctx->cs);

	if (ctx->state != LINK_UP) {
		cpu_spin_unlock_xrestore(&ctx->cs, exceptions);
		return GLINK_STATUS_NOT_INIT;
	}

	/* Update local channel state */
	QMP_LOCAL_FLAG_SET(ctx, CH_STATE);
	ctx->state = LOCAL_CONNECTING;

	xport_qmp_intr_send(ctx);

	cpu_spin_unlock_xrestore(&ctx->cs, exceptions);

	return GLINK_STATUS_SUCCESS;
}

/*
 * Closes QMP channel and terminates communication.
 * Updates channel state and sends close command to remote.
 */
static enum glink_err_type
xport_qmp_tx_cmd_ch_close(struct glink_transport_if *if_ptr)
{
	struct xport_qmp_ctx *ctx = (struct xport_qmp_ctx *)if_ptr;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&ctx->cs);

	if (ctx->state == LOCAL_CONNECTED ||
	    ctx->state == LOCAL_CONNECTING ||
	    ctx->state == E2ECONNECTED) {
		/* Update local channel state */
		QMP_LOCAL_FLAG_CLR(ctx, CH_STATE);
		ctx->state = LOCAL_DISCONNECTING;

		xport_qmp_intr_send(ctx);
	}

	cpu_spin_unlock_xrestore(&ctx->cs, exceptions);

	return GLINK_STATUS_SUCCESS;
}

/*
 * Acknowledges completion of received packet processing.
 * Validates RX state and sends acknowledgment to remote.
 */
static enum glink_err_type
xport_qmp_tx_cmd_local_rx_done(struct glink_transport_if *if_ptr,
			       const void *ptr)
{
	struct xport_qmp_ctx *ctx = (struct xport_qmp_ctx *)if_ptr;
	enum glink_err_type status = GLINK_STATUS_SUCCESS;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&ctx->cs);

	if (ctx->state != E2ECONNECTED) {
		status = GLINK_STATUS_CH_NOT_FULLY_OPENED;
		goto unlock_return;
	}

	if (ctx->rx_pkt_buf != ptr ||
	    !ctx->rx_pkt_size ||
	    ctx->rx_pkt_size != ctx->rx_pkt_read_size) {
		status = GLINK_STATUS_INVALID_PARAM;
		goto unlock_return;
	}

	/* Reset RX variable states */
	ctx->rx_pkt_size = 0;
	ctx->rx_pkt_buf = NULL;
	ctx->rx_pkt_read_size = 0;

	/* Update local rx_done */
	QMP_LOCAL_FLAG_TOGGLE(ctx, RX_DONE);

	xport_qmp_intr_send(ctx);

unlock_return:
	cpu_spin_unlock_xrestore(&ctx->cs, exceptions);
	return status;
}

/*
 * Transmits data packet over QMP transport.
 * Validates channel state and initiates packet transmission.
 */
static enum glink_err_type xport_qmp_tx_data(struct glink_transport_if *if_ptr,
					     struct glink_core_tx_pkt *pkt_ctx)
{
	struct xport_qmp_ctx *ctx = (struct xport_qmp_ctx *)if_ptr;
	enum glink_err_type ret = GLINK_STATUS_SUCCESS;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&ctx->cs);

	if (ctx->state != E2ECONNECTED) {
		ret = GLINK_STATUS_CH_NOT_FULLY_OPENED;
		goto exit;
	}

	if (pkt_ctx->size > ctx->cfg_tx_max_pkt_size) {
		ret = GLINK_STATUS_OUT_OF_RESOURCES;
		goto exit;
	}

	ctx->tx_pkt_ctx = pkt_ctx;

	xport_qmp_pkt_send(ctx);
	xport_qmp_intr_send(ctx);

exit:
	cpu_spin_unlock_xrestore(&ctx->cs, exceptions);
	return ret;
}

/*
 * Initializes QMP transport layer and registers with GLink core.
 * Sets up shared memory, interrupts, and transport interfaces.
 */
void xport_qmp_init(void)
{
	uint32_t ind;

	/* Initialize context for each configured transport */
	for (ind = 0; ind < GLINK_CFG_MAX_REMOTE_HOSTS; ind++) {
		struct xport_qmp_ctx *ctx = &xport_qmp_ctxs[ind];
		struct glink_transport_if *xport_if = &ctx->xport_if;
		const struct xport_qmp_config *cfg = xport_qmp_get_config(ind);
		const size_t desc_size = sizeof(struct xport_qmp_desc);
		paddr_t local_mem;
		paddr_t remote_mem;
		size_t local_mem_size;
		size_t remote_mem_size;
		vaddr_t mapped_addr;
		vaddr_t desc_addr;
		struct xport_qmp_config *cfg_rw;
		TEE_Result itr;
		uint32_t shared_loc_desc;

		if (!cfg)
			continue;

		local_mem = cfg->local_shared_mem;
		remote_mem = cfg->remote_shared_mem;
		local_mem_size = cfg->local_shared_mem_size;
		remote_mem_size = cfg->remote_shared_mem_size;
		cfg_rw = (struct xport_qmp_config *)cfg;

		/*
		 * Map the mailbox and interrupt registers.
		 * Update config with virtual addresses where required.
		 */
		mapped_addr = (vaddr_t)core_mmu_add_mapping(MEM_AREA_IO_SEC,
							    local_mem,
							    local_mem_size);
		if (!mapped_addr)
			panic("Failed to map local shared memory");

		mapped_addr = (vaddr_t)core_mmu_add_mapping(MEM_AREA_IO_SEC,
							    remote_mem,
							    remote_mem_size);
		if (!mapped_addr)
			panic("Failed to map remote shared memory");
		cfg_rw->rx_pkt_static_buf = mapped_addr + 4;

		mapped_addr = (vaddr_t)core_mmu_add_mapping(MEM_AREA_IO_SEC,
							    cfg->irq_out.reg_addr,
							    sizeof(uint32_t));
		if (!mapped_addr)
			panic("Failed to map IPC interrupt register");
		cfg_rw->irq_out.reg_addr = mapped_addr;

		/*
		 * Initialize context
		 */
		ctx->cfg = cfg;
		ctx->cs = SPINLOCK_UNLOCK;
		ctx->state = LINK_DOWN;

		/* Update local descriptor and mailbox info*/
		desc_addr = (vaddr_t)phys_to_virt(local_mem, MEM_AREA_IO_SEC,
						  desc_size);
		ctx->shared_local_desc = desc_addr;
		ctx->shared_local_mailbox = desc_addr + desc_size;
		ctx->cfg_local_mailbox_size = local_mem_size - desc_size;

		/* Update remote descriptor and mailbox info*/
		desc_addr = (vaddr_t)phys_to_virt(remote_mem, MEM_AREA_IO_SEC,
						  desc_size);
		ctx->shared_remote_desc = desc_addr;
		ctx->shared_remote_mailbox = desc_addr + desc_size;
		ctx->cfg_remote_mailbox_size = remote_mem_size - desc_size;

		ctx->cfg_tx_max_pkt_size = cfg->tx_max_pkt_size;
		ctx->cfg_rx_max_pkt_size = cfg->rx_max_pkt_size;

		/* Initialize local descriptor from shared memory */
		shared_loc_desc = io_read32(ctx->shared_local_desc);
		WRITE_ONCE(ctx->local_desc.word, shared_loc_desc);

		g_tmel_ctx = ctx;

		/*
		 * Register transport interface
		 */
		xport_if->remote_ss = cfg->remote_ss;
		xport_if->ch_name = cfg->ch_name;
		xport_if->tx_cmd_ch_open = xport_qmp_tx_cmd_ch_open;
		xport_if->tx_cmd_ch_close = xport_qmp_tx_cmd_ch_close;
		xport_if->tx_cmd_local_rx_done =
			xport_qmp_tx_cmd_local_rx_done;
		xport_if->tx_data = xport_qmp_tx_data;

		glink_core_register_transport(xport_if);

		/*
		 * Register interrupt handler
		 * Configuration of the interrupt is done by bl31
		 */
		itr = interrupt_create_handler(interrupt_get_main_chip(),
					       cfg->irq_in,
					       xport_qmp_isr, 0u, 0u,
					       NULL);
		if (itr)
			panic("Failed to register QMP interrupt");

		interrupt_enable(interrupt_get_main_chip(), cfg->irq_in);

		/* Trigger initial ISR to start state machine */
		xport_qmp_isr(NULL);
	}
}
