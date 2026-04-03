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
#include <mm/core_memprot.h>
#include <rng_support.h>
#include <trace.h>
#include <config.h>
#include <kernel/thread.h>

#ifdef CFG_QCOM_TMEL_RNG
#include <tmerng_client.h>
#endif

TEE_Result hw_get_random_bytes(void *buf, size_t len)
{
#ifdef CFG_QCOM_TMEL_RNG
	return tme_hw_get_random_bytes(buf, len);
#else
	return TEE_ERROR_NOT_SUPPORTED;
#endif
}
