// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <kernel/tee_common_otp.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <io.h>
#include <platform_config.h>
#include <string.h>
#include <string_ext.h>
#include <tee_api_types.h>
#include <trace.h>
#include <util.h>

#include <tmekm_client.h>

/*
 * tee_otp_get_hw_unique_key() - Get Hardware Unique Key
 * @hwkey: Pointer to structure to receive the HUK
 *
 * Retrieves the base Hardware Unique Key from Qualcomm TME (Trusted
 * Management Engine). This function derives a key using TME's key
 * derivation facilities and distributes it to TCSR ENDPOINT slot 0 for
 * extraction via TCSR registers. The key size is determined by
 * HW_UNIQUE_KEY_LENGTH.
 *
 * IMPORTANT: This function requires CFG_QCOM_TMEL_KM to be enabled.
 * If not enabled, TEE_ERROR_NOT_SUPPORTED will be returned.
 *
 * Return: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result tee_otp_get_hw_unique_key(struct tee_hw_unique_key *hwkey)
{
#if !defined(CFG_QCOM_TMEL_KM)
	EMSG("CFG_QCOM_TMEL_KM is not enabled");
	return TEE_ERROR_NOT_SUPPORTED;
#else
	TEE_Result res = TEE_ERROR_GENERIC;
	struct tme_kdf_spec kdf_spec = { };
	struct tme_key_policy key_policy = { };
	tme_key_handle_t key_handle = TME_KEY_HANDLE_INVALID;
	uint32_t tcsr_key[8] = { };  /* 256 bits = 8 x 32-bit words */
	uint32_t key_length_bits = HW_UNIQUE_KEY_LENGTH * 8;
	uint32_t algo_mode = 0;

	if (!hwkey)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Determine algorithm based on key length */
	if (key_length_bits == 128) {
		algo_mode = TME_KAL_AES128_ECB;
	} else if (key_length_bits == 256) {
		algo_mode = TME_KAL_AES256_ECB;
	} else {
		EMSG("Unsupported key length: %u bits", key_length_bits);
		return TEE_ERROR_NOT_SUPPORTED;
	}

	/* Create key policy for TCSR ENDPOINT */
	tme_km_create_key_policy(key_length_bits, algo_mode, TME_KD_TCSR_ENDPOINT,
				 TME_KLI_NP_CU, &key_policy);

	/* Setup KDF specification for base HUK derivation */
	kdf_spec.kdf_algo = TME_KAL_KDF_NIST;
	kdf_spec.policy = key_policy;
	kdf_spec.security_context = TME_KSC_SOCSecBootState |
				   TME_KSC_TMELifecycleState |
				   TME_KSC_SOCDebugState |
				   TME_KSC_ChildKeyPolicy |
				   TME_KSC_SWContext;
	kdf_spec.prf_digest_algo = TME_KAL_SHA512_HMAC;
	kdf_spec.input_key = TME_KID_CHIP_RAND_BASE;
	kdf_spec.l2_key = TME_KID_L2_KEYWRAPSVC;
	/* No context or salt for base HUK - all zeros from memset */

	/* Derive key using TME KM */
	res = tme_km_derive_key(&kdf_spec, &key_handle);
	if (res) {
		EMSG("tme_km_derive_key failed: 0x%x", res);
		return res;
	}

	if (key_handle == TME_KEY_HANDLE_INVALID) {
		EMSG("Key derivation failed - invalid handle");
		return TEE_ERROR_GENERIC;
	}

	/* Distribute key to TCSR ENDPOINT slot 0 */
	res = tme_km_distribute_key(key_handle, TME_KD_TCSR_ENDPOINT, 0);
	if (res) {
		EMSG("tme_km_distribute_key failed: 0x%x", res);
		goto out_clear;
	}

	/* Extract key bytes from TCSR registers */
	res = tme_km_read_tcsr_key_and_clear(tcsr_key, sizeof(tcsr_key), 0);
	if (res) {
		EMSG("Failed to read TCSR key: 0x%x", res);
		goto out_clear;
	}

	/* Copy the key to output buffer */
	memcpy(hwkey->data, tcsr_key, HW_UNIQUE_KEY_LENGTH);
	memzero_explicit(tcsr_key, sizeof(tcsr_key));

out_clear:
	/* Clear key from TME */
	if (tme_km_clear_key(key_handle))
		EMSG("tme_km_clear_key failed");

	return res;
#endif /* CFG_QCOM_TMEL_KM */
}

/*
 * tee_otp_get_die_id() - Get die identifier from fuse register
 * @buffer: Buffer to receive the die ID
 * @len: Length of buffer
 *
 * IMPORTANT: This function requires QCOM_SERIAL_NUM_FUSE_ADDR to be defined.
 * If not defined, -1 will be returned.
 *
 * Return: 0 on success, -1 on error
 */
int tee_otp_get_die_id(uint8_t *buffer, size_t len)
{
#if !defined(QCOM_SERIAL_NUM_FUSE_ADDR)
	EMSG("QCOM_SERIAL_NUM_FUSE_ADDR is not defined");
	return -1;
#else
	uint32_t die_id = 0;
	size_t copy_len = 0;
	vaddr_t fuse_base = 0;

	if (!buffer || !len)
		return -1;

	fuse_base = core_mmu_get_va(QCOM_SERIAL_NUM_FUSE_ADDR,
				    MEM_AREA_IO_SEC, sizeof(uint32_t));
	if (!fuse_base) {
		EMSG("Failed to map die ID fuse register");
		return -1;
	}

	die_id = io_read32(fuse_base);

	copy_len = MIN(len, sizeof(die_id));
	memcpy(buffer, &die_id, copy_len);

	if (len > sizeof(die_id))
		memset(buffer + sizeof(die_id), 0, len - sizeof(die_id));

	return 0;
#endif /* QCOM_SERIAL_NUM_FUSE_ADDR */
}
