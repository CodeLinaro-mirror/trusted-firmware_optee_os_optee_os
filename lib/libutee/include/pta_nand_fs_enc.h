/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __PTA_NAND_FS_ENC_H
#define __PTA_NAND_FS_ENC_H

#include <stdint.h>
#include <util.h>

#define PTA_NAND_FS_ENC_UUID \
	{ 0x1e64fceb, 0x66b5, 0x41d5, \
		{ 0xba, 0x0a, 0x1f, 0x48, 0x7a, 0x59, 0xa8, 0xa0 } }

#define PTA_AES_DECRYPT		0
#define PTA_AES_ENCRYPT		1

#define PTA_AES_MODE_ECB	0
#define PTA_AES_MODE_CBC	1
#define PTA_AES_MODE_CTR	2
#define PTA_AES_MODE_XTS	3

#define PTA_AES_KEY_SIZE_128	16
#define PTA_AES_KEY_SIZE_256	32

#define PTA_AES_BLOCK_SIZE	16
#define PTA_AES_IV_SIZE		16

#define PTA_AES_MAX_DATA_SIZE	4096

#define PTA_AES_HUK_SEED_CRBK		0x0	/* Chip Random Base Key */
#define PTA_AES_HUK_SEED_OEM_PRODSEED	0x1	/* OEM Product Seed */

#define PTA_AES_HUK_SW_CONTEXT_MAX	128

/* SET_KEY configuration: 12 bytes in the platform's native byte order. */
struct aes_huk_set_key_req {
	uint32_t seed_type;	/* PTA_AES_HUK_SEED_* */
	uint32_t key_size;	/* Bytes per key: PTA_AES_KEY_SIZE_* */
	uint32_t algo_mode;	/* PTA_AES_MODE_* */
} __packed;

/*
 * Derive and cache AES keys for subsequent CRYPT calls.
 *
 * params[0]: MEMREF_INPUT, struct aes_huk_set_key_req, exact structure size.
 * params[1]: MEMREF_INPUT, data-key context for OPS; NONE for CRBK.
 * params[2]: MEMREF_INPUT, tweak-key context for OPS XTS; otherwise NONE.
 * params[3]: NONE.
 *
 * Each required context contains 1..PTA_AES_HUK_SW_CONTEXT_MAX raw bytes.
 * OPS uses a fixed 64-byte zero KDF label for both keys; XTS contexts must
 * differ. CRBK uses fixed labels "AES-HUK DATA KEY" (16 bytes) and
 * "AES-HUK TWEAK KEY" (17 bytes), excluding terminating NULs.
 * XTS derives two keys, each of key_size bytes. Its contexts are KDF inputs,
 * not the per-operation tweak supplied to CRYPT. Identical derived XTS keys
 * are rejected.
 *
 * Parameter validation failures preserve the cached keys. Once derivation
 * starts, the old keys are discarded; failure invalidates the cache.
 * Success replaces the cached keys and selects the mode accepted by CRYPT.
 *
 * The cache is global across sessions. Callers must serialize SET_KEY,
 * CRYPT and CLEAR_KEY as one transaction across all users of this PTA.
 */
#define PTA_NAND_FS_ENC_SET_KEY		0x0

/*
 * Encrypt or decrypt data using the cached keys.
 *
 * params[0]: VALUE_INPUT: a = PTA_AES_ENCRYPT or PTA_AES_DECRYPT,
 *                         b = PTA_AES_MODE_* (must match SET_KEY).
 * params[1]: MEMREF_INPUT, 16-byte IV for CBC/CTR or tweak input for XTS.
 *            ECB accepts NONE or MEMREF_INPUT and ignores the IV.
 * params[2]: MEMREF_INPUT, data of 16..PTA_AES_MAX_DATA_SIZE bytes,
 *            with a length divisible by PTA_AES_BLOCK_SIZE.
 * params[3]: MEMREF_OUTPUT, capacity at least the input length.
 *
 * No padding is applied. Each call starts with the supplied IV/tweak;
 * cipher state is not carried across invocations.
 *
 * On success, params[3].memref.size is the output length (the input length).
 * On TEE_ERROR_SHORT_BUFFER, it is the required capacity. Discard output on
 * any error. No cached key or a mode mismatch returns TEE_ERROR_BAD_STATE.
 */
#define PTA_NAND_FS_ENC_CRYPT		0x1

/*
 * Clear cached keys and invalidate the cache. All four parameters are NONE.
 * Clearing an already empty cache succeeds. Session close does not clear it.
 */
#define PTA_NAND_FS_ENC_CLEAR_KEY	0x2

#endif /* __PTA_NAND_FS_ENC_H */
