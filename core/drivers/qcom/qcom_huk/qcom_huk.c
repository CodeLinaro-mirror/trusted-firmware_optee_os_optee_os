// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/qcom_huk.h>
#include <inttypes.h>
#include <kernel/huk_subkey.h>
#include <kernel/mutex.h>
#include <kernel/tee_common_otp.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <io.h>
#include <platform_config.h>
#include <pta_nand_fs_enc.h>
#include <string.h>
#include <string_ext.h>
#include <tee_api_types.h>
#include <trace.h>
#include <util.h>

#include <tmekm_client.h>

#ifdef CFG_QCOM_TMEL_KM
/* Shared by base-HUK and NAND derivation users of TCSR slot 0. */
static struct mutex tcsr_key_lock = MUTEX_INITIALIZER;
#endif

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
	mutex_lock(&tcsr_key_lock);
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
	mutex_unlock(&tcsr_key_lock);
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

#if defined(CFG_QCOM_TMEL_KM)
static TEE_Result aes_huk_tme_derive(const struct aes_huk_derive_req *req,
				     uint8_t *subkey, size_t subkey_len)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	TEE_Result clear_res = TEE_SUCCESS;
	struct tme_kdf_spec kdf_spec = { };
	struct tme_key_policy key_policy = { };
	tme_key_handle_t key_handle = TME_KEY_HANDLE_INVALID;
	uint32_t tcsr_key[8] = { };
	uint32_t key_length_bits = subkey_len * 8;
	uint32_t algo_mode = 0;
	uint32_t lineage = TME_KLI_NP_CU;

	if (!subkey || (req->sw_context_len && !req->sw_context) ||
	    (req->salt_label_len && !req->salt_label)) {
		EMSG("AES HUK: missing output, context or label buffer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (subkey_len == PTA_AES_KEY_SIZE_128) {
		algo_mode = TME_KAL_AES128_ECB;
	} else if (subkey_len == PTA_AES_KEY_SIZE_256) {
		algo_mode = TME_KAL_AES256_ECB;
	} else {
		EMSG("AES HUK: unsupported key length %zu", subkey_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (req->seed_type != PTA_AES_HUK_SEED_CRBK &&
	    req->seed_type != PTA_AES_HUK_SEED_OEM_PRODSEED) {
		EMSG("AES HUK: bad seed_type %u", req->seed_type);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (req->sw_context_len > TME_KDF_SW_CONTEXT_BYTES_MAX) {
		EMSG("AES HUK: sw_context too long (%u)", req->sw_context_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}
	if (req->seed_type == PTA_AES_HUK_SEED_OEM_PRODSEED &&
	    req->sw_context_len == 0) {
		EMSG("AES HUK: OEM product seed requires a SW context");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	/* Match the seed-specific lineage used by the eMMC ICE PTA. */
	if (req->seed_type == PTA_AES_HUK_SEED_OEM_PRODSEED)
		lineage = TME_KLI_NA;

	/* Export through TCSR for use with the OP-TEE cipher API. */
	tme_km_create_key_policy(key_length_bits, algo_mode,
				 TME_KD_TCSR_ENDPOINT, lineage,
				 &key_policy);

	kdf_spec.kdf_algo = TME_KAL_KDF_NIST;
	kdf_spec.policy = key_policy;
	kdf_spec.prf_digest_algo = TME_KAL_SHA512_HMAC;

	if (req->seed_type == PTA_AES_HUK_SEED_OEM_PRODSEED) {
		kdf_spec.input_key = TME_KID_OEM_PRODUCT_SEED;
		kdf_spec.security_context = TME_KSC_SWContext;
	} else {
		kdf_spec.input_key = TME_KID_CHIP_RAND_BASE;
		kdf_spec.l2_key = TME_KID_L2_SECURESTRGSVC;
		kdf_spec.security_context = TME_KSC_SOCSecBootState |
					    TME_KSC_TMELifecycleState |
					    TME_KSC_SOCDebugState |
					    TME_KSC_ChildKeyPolicy |
					    TME_KSC_SWContext;
	}

	if (req->sw_context_len) {
		memcpy(kdf_spec.sw_context, req->sw_context,
		       req->sw_context_len);
		kdf_spec.sw_context_length = req->sw_context_len;
	}

	if (req->salt_label_len) {
		if (req->salt_label_len > TME_KDF_SALT_LABEL_BYTES_MAX) {
			EMSG("AES HUK: label too long (%u)",
			     req->salt_label_len);
			return TEE_ERROR_BAD_PARAMETERS;
		}
		memcpy(kdf_spec.salt_label, req->salt_label,
		       req->salt_label_len);
		kdf_spec.salt_label_length = req->salt_label_len;
	}

	res = tme_km_derive_key(&kdf_spec, &key_handle);
	if (res) {
		EMSG("AES HUK: tme_km_derive_key failed: 0x%x", res);
		goto out_wipe;
	}
	if (key_handle == TME_KEY_HANDLE_INVALID) {
		EMSG("AES HUK: TME returned an invalid key handle");
		res = TEE_ERROR_GENERIC;
		goto out_wipe;
	}

	mutex_lock(&tcsr_key_lock);
	res = tme_km_distribute_key(key_handle, TME_KD_TCSR_ENDPOINT, 0);
	if (res) {
		EMSG("AES HUK: tme_km_distribute_key failed: 0x%x", res);
		goto out_clear;
	}

	res = tme_km_read_tcsr_key_and_clear(tcsr_key, sizeof(tcsr_key), 0);
	if (res) {
		EMSG("AES HUK: failed to read TCSR key: 0x%x", res);
		goto out_clear;
	}

	memcpy(subkey, tcsr_key, subkey_len);

out_clear:
	mutex_unlock(&tcsr_key_lock);
	clear_res = tme_km_clear_key(key_handle);
	if (clear_res) {
		EMSG("AES HUK: tme_km_clear_key failed: %#"PRIx32, clear_res);
		if (!res)
			res = clear_res;
	}
out_wipe:
	memzero_explicit(tcsr_key, sizeof(tcsr_key));
	memzero_explicit(&kdf_spec, sizeof(kdf_spec));
	if (res)
		memzero_explicit(subkey, subkey_len);
	return res;
}
#endif /* CFG_QCOM_TMEL_KM */

TEE_Result huk_subkey_derive(enum huk_subkey_usage usage,
			     const void *const_data, size_t const_data_len,
			     uint8_t *subkey, size_t subkey_len)
{
	if (usage == HUK_SUBKEY_AES_HUK) {
#if defined(CFG_QCOM_TMEL_KM)
		const struct aes_huk_derive_req *req = const_data;

		if (!req || const_data_len != sizeof(*req)) {
			EMSG("AES HUK: bad derive request");
			return TEE_ERROR_BAD_PARAMETERS;
		}
		if (subkey_len > HUK_SUBKEY_MAX_LEN) {
			EMSG("AES HUK: subkey_len %zu too large", subkey_len);
			return TEE_ERROR_BAD_PARAMETERS;
		}

		return aes_huk_tme_derive(req, subkey, subkey_len);
#else
		EMSG("CFG_QCOM_TMEL_KM is not enabled");
		return TEE_ERROR_NOT_SUPPORTED;
#endif
	}

	return __huk_subkey_derive(usage, const_data, const_data_len,
				   subkey, subkey_len);
}
