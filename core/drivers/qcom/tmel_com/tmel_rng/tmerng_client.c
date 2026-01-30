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

#include "tmerng_client.h"
#include "tmecom_client.h"
#include "tmemessages_uids.h"

/* TME Status codes */
#define TME_STATUS_SUCCESS		0
#define TME_STATUS_INVALID_INPUT	2
#define TME_STATUS_UNKNOWN		0xFFFFFFFF

static TEE_Result tme_status_to_tee_result(uint32_t tme_status)
{
	switch (tme_status) {
	case TME_STATUS_SUCCESS:
		return TEE_SUCCESS;
	case TME_STATUS_INVALID_INPUT:
		return TEE_ERROR_BAD_PARAMETERS;
	default:
		return TEE_ERROR_GENERIC;
	}
}

TEE_Result tme_rng_get_random(void *buf, size_t len)
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
