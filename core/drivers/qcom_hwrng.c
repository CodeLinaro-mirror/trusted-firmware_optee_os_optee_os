// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <tee_api_types.h>
#include <crypto/crypto.h>
#include <kernel/panic.h>
#include <rng_support.h>
#include <trace.h>

/*
 * Platform-specific hardware RNG implementation for Qualcomm platforms
 * (IPQ5200, IPQ96xx, and other Qualcomm SoCs)
 *
 */

TEE_Result hw_get_random_bytes(void *buf, size_t len)
{
	uint8_t *output = (uint8_t *)buf;
	size_t i;
	static uint32_t counter;

	if (!buf || len == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	/*
	 * Temporary stub: Fill buffer with incrementing pattern
	 * This is NOT cryptographically secure and is only for testing
	 * purposes.
	 *
	 * TODO: Replace with TMEL IPC call:
	 * return tmel_ipc_get_random_bytes(buf, len);
	 */

	DMSG("%s() stub: requested %zu bytes", __func__, len);

	/* Fill buffer with incrementing pattern */
	for (i = 0; i < len; i++)
		output[i] = (uint8_t)((counter + i) & 0xFF);

	counter += len;

	DMSG("%s() stub: returning %zu bytes (NOT secure)", __func__, len);

	return TEE_SUCCESS;
}
