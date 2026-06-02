// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <tee_api_types.h>
#include <trace.h>
#include <mm/core_memprot.h>
#include <config.h>
#include <kernel/thread.h>
#include <rng_support.h>
#include <atomic.h>

#include "tmerng_client.h"
#include "tmecom_client.h"
#include "tmemessages_uids.h"

#define RNG_POOL_BASE		(IMEM_BASE + 0x920UL)
#define RNG_POOL_SIZE		32U

/*
 * Get random bytes from bootloader-populated RNG pool in IMEM.
 * U-Boot SPL initializes this pool during early boot.
 */
static TEE_Result get_bootloader_rng_pool(void *buf, size_t len)
{
	static uint32_t offset;
	uint8_t *output = (uint8_t *)buf;
	uint8_t *rng_pool;
	size_t i;

	if (len > RNG_POOL_SIZE)
		IMSG("Bootloader RNG pool request (%zu) exceeds size (%u)",
		     len, RNG_POOL_SIZE);

	rng_pool = (uint8_t *)phys_to_virt(RNG_POOL_BASE,
					   MEM_AREA_IO_SEC,
					   RNG_POOL_SIZE);
	if (!rng_pool) {
		EMSG("IMEM RNG POOL region not mapped (phys 0x%lx)",
		     (unsigned long)RNG_POOL_BASE);
		return TEE_ERROR_GENERIC;
	}

	for (i = 0; i < len; i++) {
		uint32_t idx = atomic_inc32(&offset);
		size_t src_idx = (idx - 1) % RNG_POOL_SIZE;

		output[i] = rng_pool[src_idx];
		rng_pool[src_idx] = (uint8_t)(0xBEEFCAFEU >>
					     ((src_idx % 4) * 8));
	}

	return TEE_SUCCESS;
}

static TEE_Result tme_rng_get_data(void *buf, size_t len)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct tme_rng_get_msg msg = { };
	void *rng_buf = NULL;
	void *rng_orig_addr = NULL;
	paddr_t rng_paddr = 0;
	size_t rng_buf_size = 0;

	if (!buf || len == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	if (len > TME_RNG_MAX_LENGTH) {
		EMSG("Requested length %zu exceeds maximum %u",
		     len, TME_RNG_MAX_LENGTH);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	/* Allocate coherent buffer for RNG output */
	rng_buf_size = len;
	rng_buf = tmecom_client_malloc_coherent(rng_buf_size, 64,
						&rng_orig_addr, &rng_paddr);
	if (!rng_buf) {
		EMSG("Failed to allocate coherent buffer for RNG output");
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	/* Clear the buffer */
	memset(rng_buf, 0, rng_buf_size);

	/* Verify physical address was obtained */
	if (!rng_paddr) {
		EMSG("Failed to get physical address for RNG buffer");
		ret = TEE_ERROR_GENERIC;
		goto cleanup;
	}

	/* Prepare RNG get message */
	msg.input.length = len;

	/* Initialize output */
	msg.output.rng_buf_pdata = (uint32_t)rng_paddr;
	msg.output.rng_buf_length = rng_buf_size;
	msg.output.rng_buf_length_used = 0;
	msg.output.status = TME_STATUS_UNKNOWN;
	memset(&msg.output.seq_status, 0, sizeof(msg.output.seq_status));

	/* Send RNG get request to TME */
	ret = tmecom_client_send_message(TME_MSG_UID_HCS_RNG_GET,
					 TME_MSG_UID_HCS_RNG_GET_PARAM_ID,
					 true,
					 TMECOM_DEFAULT_TIMEOUT,
					 &msg,
					 sizeof(msg),
					 NULL,
					 NULL,
					 NULL);

	if (ret != TEE_SUCCESS) {
		EMSG("TME RNG get message failed: 0x%x", ret);
		goto cleanup;
	}

	/* Check TME response status */
	ret = tme_status_to_tee_result(msg.output.status);
	if (ret != TEE_SUCCESS) {
		EMSG("TME RNG get failed, status: 0x%x", msg.output.status);
		EMSG("Sequencer status: tme=0x%x, seq=0x%x, kp0=0x%x, kp1=0x%x, rsp=0x%x",
		     msg.output.seq_status.tme_error_status,
		     msg.output.seq_status.seq_error_status,
		     msg.output.seq_status.seq_kp_error_status0,
		     msg.output.seq_status.seq_kp_error_status1,
		     msg.output.seq_status.seq_rsp_status);
		goto cleanup;
	}

	/* Verify the length returned */
	if (msg.output.rng_buf_length_used != len) {
		EMSG("TME returned %u bytes, expected %zu",
		     msg.output.rng_buf_length_used, len);
		ret = TEE_ERROR_GENERIC;
		goto cleanup;
	}

	/* Copy random bytes to output buffer */
	memcpy(buf, rng_buf, len);

cleanup:
	if (rng_buf)
		tmecom_client_free_coherent(rng_buf, rng_orig_addr,
					    rng_buf_size);

	return ret;
}

TEE_Result tme_hw_get_random_bytes(void *buf, size_t len)
{
	TEE_Result res;
	size_t filled = 0;
	uint8_t *output = (uint8_t *)buf;

	if (!buf || len == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	/*
	 * Use bootloader RNG pool when TMEL is bypassed or interrupts
	 * are masked. This ensures RNG always succeeds for boot-critical
	 * operations.
	 */
	if (tmecom_is_tmel_bypassed() ||
	    (thread_get_exceptions() & THREAD_EXCP_NATIVE_INTR))
		return get_bootloader_rng_pool(buf, len);

	while (filled < len) {
		size_t chunk = len - filled;

		if (chunk > TME_RNG_MAX_LENGTH)
			chunk = TME_RNG_MAX_LENGTH;

		res = tme_rng_get_data(output + filled, chunk);
		if (res != TEE_SUCCESS) {
			EMSG("TMEL IPC RNG failed: 0x%x", res);
			return res;
		}
		filled += chunk;
	}

	return TEE_SUCCESS;
}
