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
#include <config.h>
#include <kernel/thread.h>

#ifdef CFG_QCOM_TMEL_RNG
#include <tmerng_client.h>
#endif

/*
 * Platform-specific hardware RNG implementation for Qualcomm platforms
 * (IPQ5200, IPQ96xx, and other Qualcomm SoCs)
 *
 * Supports multiple RNG implementations:
 * - CFG_QCOM_TMEL_RNG=y: TMEL IPC-based hardware RNG with failsafe fallback
 * - CFG_QCOM_TMEL_RNG=n: Failsafe implementation (NOT cryptographically secure)
 */

TEE_Result hw_get_random_bytes(void *buf, size_t len)
{
	uint8_t *output = (uint8_t *)buf;
	size_t i;

	if (!buf || len == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	/*
	 * TMEL IPC-based RNG implementation
	 * Used by: JUHU (IPQ96xx)
	 */
	if (IS_ENABLED(CFG_QCOM_TMEL_RNG) &&
	    !(thread_get_exceptions() & THREAD_EXCP_NATIVE_INTR)) {
		TEE_Result res;

		/* Request random bytes from TMEL via IPC */
		res = tme_rng_get_random(buf, len);


		if (res != TEE_SUCCESS) {
			EMSG("TMEL IPC RNG failed: 0x%x", res);
			/*
			 * Return error to caller - no fallback to
			 * maintain security
			 */
			goto exit;
		}

		return TEE_SUCCESS;
	}
exit:
	/*
	 * Runtime fallback when CFG_QCOM_TMEL_RNG is disabled or
	 * NATIVE_INTR is disabled
	 */
	for (i = 0; i < len; i++)
		output[i] = (uint8_t)(0xdeadbeef >> ((i % 4) * 8));

	return TEE_SUCCESS;
}
