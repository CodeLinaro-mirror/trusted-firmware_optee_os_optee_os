// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

/* eMMC ICE PTA - Hardware key configuration for ICE-accelerated storage encryption */

#include <kernel/pseudo_ta.h>
#include <kernel/panic.h>
#include <mm/core_memprot.h>
#include <string.h>
#include <trace.h>
#include <io.h>
#include <pta_emmc_ice.h>
#include "emmc_ice_config.h"
#include "tmekm_client.h"

/* Write ICE register */
static inline void ice_reg_write(paddr_t addr, uint32_t mask, uint32_t shift,
				 uint32_t val)
{
	vaddr_t va = (vaddr_t)phys_to_virt(addr, MEM_AREA_IO_SEC, sizeof(uint32_t));

	if (mask == 0 && shift == 0)
		io_write32(va, val);
	else
		io_mask32(va, val << shift, mask);
}

/* Configure OPS (OEM Product Seed) derived key */
static TEE_Result ice_ops_key_cfg(uint32_t key_len, uint32_t alg_mode,
				   uint8_t *sw_context, uint32_t ctx_len,
				   uint32_t dstKeyIndex)
{
	TEE_Result status = TEE_ERROR_GENERIC;
	struct tme_kdf_spec kdfspec = {};
	struct tme_key_policy keyPolicy;
	tme_key_handle_t derivedKeyHandle = TME_KEY_HANDLE_INVALID;
	uint8_t salt_salt[TME_KDF_SALT_LABEL_BYTES_MAX] = {0};
	uint32_t key_policy_lo32, key_policy_hi32;

	if (key_len != ICE_CRYPTO_KEY_SIZE_128 &&
	    key_len != ICE_CRYPTO_KEY_SIZE_256) {
		EMSG("ICE: Invalid key length %u", key_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (alg_mode != ICE_CRYPTO_ALGO_MODE_AES_ECB &&
	    alg_mode != ICE_CRYPTO_ALGO_MODE_AES_XTS &&
	    alg_mode != ICE_CRYPTO_ALGO_MODE_BITLOCKER) {
		EMSG("ICE: Invalid algorithm mode %u", alg_mode);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (!sw_context || ctx_len == 0 ||
	    ctx_len > TME_KDF_SW_CONTEXT_BYTES_MAX) {
		EMSG("ICE: Invalid sw_context or ctx_len %u", ctx_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (dstKeyIndex != TMEL_ICE_ENDPOINT_DATA_KEY_SLOT_3 &&
	    dstKeyIndex != TMEL_ICE_ENDPOINT_SALT_KEY_SLOT_2) {
		EMSG("ICE: Invalid destination key index %u", dstKeyIndex);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	key_policy_lo32 = TME_KT_Symmetric | TME_KP_Generic |
			  TME_KOP_Encryption | TME_KOP_Decryption |
			  TME_KSL_HWKey | TME_KO_TZ | TME_KLI_NA;
	key_policy_hi32 = TME_KPV_Version | TME_KAU_TZ | TME_KD_ICE;

	if (key_len == ICE_CRYPTO_KEY_SIZE_128) {
		if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_ECB)
			key_policy_lo32 |= TME_KL_128 | TME_KAL_AES128_ECB;
		else if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS)
			key_policy_lo32 |= TME_KL_256 | TME_KAL_AES128_XTS;
		else if (alg_mode == ICE_CRYPTO_ALGO_MODE_BITLOCKER)
			key_policy_lo32 |= TME_KL_128 | TME_KAL_AES128_CBC;
		else
			return TEE_ERROR_BAD_PARAMETERS;
	} else {
		if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_ECB)
			key_policy_lo32 |= TME_KL_256 | TME_KAL_AES256_ECB;
		else if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS)
			key_policy_lo32 |= TME_KL_256 | TME_KAL_AES128_XTS;
		else if (alg_mode == ICE_CRYPTO_ALGO_MODE_BITLOCKER)
			key_policy_lo32 |= TME_KL_256 | TME_KAL_AES256_CBC;
		else
			return TEE_ERROR_BAD_PARAMETERS;
	}

	memset(&kdfspec, 0, sizeof(kdfspec));
	memset(kdfspec.sw_context, 0x0, TME_KDF_SW_CONTEXT_BYTES_MAX);

	keyPolicy.low = key_policy_lo32;
	keyPolicy.high = key_policy_hi32;
	kdfspec.kdf_algo = TME_KAL_KDF_NIST;
	kdfspec.input_key = TME_KID_OEM_PRODUCT_SEED;
	kdfspec.policy = keyPolicy;
	kdfspec.security_context = TME_KSC_SWContext;
	kdfspec.prf_digest_algo = TME_KAL_SHA512_HMAC;

	memcpy(kdfspec.salt_label, salt_salt, TME_KDF_SALT_LABEL_BYTES_MAX);
	kdfspec.salt_label_length = sizeof(salt_salt);

	memcpy(kdfspec.sw_context, sw_context, ctx_len);
	kdfspec.sw_context_length = ctx_len;

	status = tme_km_derive_key(&kdfspec, &derivedKeyHandle);
	if (status != TEE_SUCCESS) {
		EMSG("ICE: TME key derivation failed for OPS (error: 0x%x)",
		     status);
		return TEE_ERROR_GENERIC;
	}

	status = tme_km_distribute_key(derivedKeyHandle, TME_KD_ICE_ENDPOINT,
				       dstKeyIndex);
	if (status != TEE_SUCCESS) {
		EMSG("ICE: TME key distribution failed (error: 0x%x)", status);
		tme_km_clear_key(derivedKeyHandle);
		return TEE_ERROR_GENERIC;
	}

	status = tme_km_clear_key(derivedKeyHandle);
	if (status != TEE_SUCCESS) {
		EMSG("ICE: TME key clear failed (error: 0x%x)", status);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

/* Configure hardware key (CRBK derived) */
static TEE_Result ice_hw_key_cfg(uint32_t key_len, uint32_t alg_mode,
				  uint32_t dstKeyIndex)
{
	TEE_Result status = TEE_ERROR_GENERIC;
	struct tme_kdf_spec kdfspec = {};
	struct tme_key_policy keyPolicy;
	tme_key_handle_t derivedKeyHandle = TME_KEY_HANDLE_INVALID;
	uint32_t key_policy_lo32, key_policy_hi32;

	/* Salt data for data key (slot 0) */
	uint8_t salt_data[TME_KDF_SALT_LABEL_BYTES_MAX] = {
		0x34, 0x61, 0x31, 0x32, 0x61, 0x37, 0x38, 0x39,
		0x64, 0x34, 0x31, 0x63, 0x63, 0x32, 0x37, 0x38,
		0x65, 0x61, 0x66, 0x34, 0x31, 0x32, 0x36, 0x36,
		0x35, 0x61, 0x38, 0x31, 0x64, 0x35, 0x30, 0x31,
		0x34, 0x61, 0x31, 0x32, 0x61, 0x37, 0x38, 0x39,
		0x64, 0x34, 0x31, 0x63, 0x63, 0x32, 0x37, 0x38,
		0x65, 0x61, 0x66, 0x34, 0x31, 0x32, 0x36, 0x36,
		0x35, 0x61, 0x38, 0x31, 0x64, 0x35, 0x30, 0x31
	};
	/* Salt data for salt key (slot 1) */
	uint8_t salt_salt[TME_KDF_SALT_LABEL_BYTES_MAX] = {
		0x49, 0x43, 0x45, 0x20, 0x53, 0x41, 0x4C, 0x54,
		0x20, 0x4B, 0x45, 0x59, 0x20, 0x44, 0x45, 0x52,
		0x49, 0x56, 0x45, 0x20, 0x46, 0x52, 0x4F, 0x4D,
		0x20, 0x54, 0x4D, 0x45, 0x4C, 0x20, 0x44, 0x45,
		0x52, 0x49, 0x56, 0x45, 0x20, 0x4B, 0x45, 0x59,
		0x20, 0x52, 0x45, 0x51, 0x55, 0x45, 0x53, 0x54,
		0x20, 0x46, 0x52, 0x4F, 0x4D, 0x20, 0x54, 0x5A,
		0x20, 0x74, 0x6F, 0x20, 0x54, 0x4D, 0x45, 0x4C
	};

	if (key_len != ICE_CRYPTO_KEY_SIZE_128 &&
	    key_len != ICE_CRYPTO_KEY_SIZE_256) {
		EMSG("ICE: Invalid key length %u", key_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (alg_mode != ICE_CRYPTO_ALGO_MODE_AES_ECB &&
	    alg_mode != ICE_CRYPTO_ALGO_MODE_AES_XTS &&
	    alg_mode != ICE_CRYPTO_ALGO_MODE_BITLOCKER) {
		EMSG("ICE: Invalid algorithm mode %u", alg_mode);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (dstKeyIndex != TMEL_ICE_ENDPOINT_DATA_KEY_SLOT_3 &&
	    dstKeyIndex != TMEL_ICE_ENDPOINT_SALT_KEY_SLOT_2) {
		EMSG("ICE: Invalid destination key index %u", dstKeyIndex);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	key_policy_lo32 = TME_KT_Symmetric | TME_KP_Generic |
			  TME_KOP_Encryption | TME_KOP_Decryption |
			  TME_KSL_HWKey | TME_KO_TZ | TME_KLI_NP_CU;
	key_policy_hi32 = TME_KPV_Version | TME_KAU_TZ | TME_KD_ICE;

	if (key_len == ICE_CRYPTO_KEY_SIZE_128) {
		if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_ECB)
			key_policy_lo32 |= TME_KL_128 | TME_KAL_AES128_ECB;
		else if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS)
			key_policy_lo32 |= TME_KL_256 | TME_KAL_AES128_XTS;
		else if (alg_mode == ICE_CRYPTO_ALGO_MODE_BITLOCKER)
			key_policy_lo32 |= TME_KL_128 | TME_KAL_AES128_CBC;
		else
			return TEE_ERROR_BAD_PARAMETERS;
	} else {
		if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_ECB)
			key_policy_lo32 |= TME_KL_256 | TME_KAL_AES256_ECB;
		else if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS)
			key_policy_lo32 |= TME_KL_256 | TME_KAL_AES128_XTS;
		else if (alg_mode == ICE_CRYPTO_ALGO_MODE_BITLOCKER)
			key_policy_lo32 |= TME_KL_256 | TME_KAL_AES256_CBC;
		else
			return TEE_ERROR_BAD_PARAMETERS;
	}

	memset(&kdfspec, 0, sizeof(kdfspec));

	keyPolicy.low = key_policy_lo32;
	keyPolicy.high = key_policy_hi32;
	kdfspec.kdf_algo = TME_KAL_KDF_NIST;
	kdfspec.input_key = TME_KID_CHIP_RAND_BASE;
	kdfspec.l2_key = TME_KID_L2_SECURESTRGSVC;
	kdfspec.policy = keyPolicy;
	kdfspec.security_context = TME_KSC_SOCSecBootState |
				   TME_KSC_TMELifecycleState |
				   TME_KSC_SOCDebugState |
				   TME_KSC_ChildKeyPolicy |
				   TME_KSC_SWContext;
	kdfspec.prf_digest_algo = TME_KAL_SHA512_HMAC;

	if (dstKeyIndex == TMEL_ICE_ENDPOINT_DATA_KEY_SLOT_3) {
		memcpy(kdfspec.salt_label, salt_data,
		       TME_KDF_SALT_LABEL_BYTES_MAX);
		kdfspec.salt_label_length = sizeof(salt_data);
	} else if (dstKeyIndex == TMEL_ICE_ENDPOINT_SALT_KEY_SLOT_2) {
		memcpy(kdfspec.salt_label, salt_salt,
		       TME_KDF_SALT_LABEL_BYTES_MAX);
		kdfspec.salt_label_length = sizeof(salt_salt);
	}

	status = tme_km_derive_key(&kdfspec, &derivedKeyHandle);
	if (status != TEE_SUCCESS) {
		EMSG("ICE: TME key derivation failed for CRBK (error: 0x%x)",
		     status);
		return TEE_ERROR_GENERIC;
	}

	status = tme_km_distribute_key(derivedKeyHandle, TME_KD_ICE_ENDPOINT,
				       dstKeyIndex);
	if (status != TEE_SUCCESS) {
		EMSG("ICE: TME key distribution failed (error: 0x%x)", status);
		tme_km_clear_key(derivedKeyHandle);
		return TEE_ERROR_GENERIC;
	}

	status = tme_km_clear_key(derivedKeyHandle);
	if (status != TEE_SUCCESS) {
		EMSG("ICE: TME key clear failed (error: 0x%x)", status);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

/* Set hardware key context configuration */
static TEE_Result cmd_ice_set_hw_key_ctx(uint32_t param_types,
					 TEE_Param params[TEE_NUM_PARAMS])
{
	uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE);
	ice_set_key_config_context_req *config_data = NULL;
	uint32_t algo_allowed;
	uint32_t capidx_val;
	vaddr_t va;

	if (param_types != exp_pt) {
		EMSG("ICE: Invalid parameter types");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	config_data = (ice_set_key_config_context_req *)params[0].memref.buffer;

	if (!config_data || params[0].memref.size < sizeof(*config_data)) {
		EMSG("ICE: Invalid config data");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (!(config_data->index < ICE_MAX_KEY_IDX)) {
		EMSG("ICE: Invalid key index %u", config_data->index);
		return TEE_ERROR_NOT_SUPPORTED;
	}

	if ((config_data->key_len != ICE_CRYPTO_KEY_SIZE_128 &&
	     config_data->key_len != ICE_CRYPTO_KEY_SIZE_256) ||
	    (config_data->alg_mode != ICE_CRYPTO_ALGO_MODE_AES_ECB &&
	     config_data->alg_mode != ICE_CRYPTO_ALGO_MODE_AES_XTS) ||
	    (config_data->key_mode != ICE_CRYPTO_USE_KEY0_HW_KEY &&
	     config_data->key_mode != ICE_CRYPTO_USE_KEY1_HW_KEY)) {
		EMSG("ICE: Invalid parameters");
		return TEE_ERROR_NOT_SUPPORTED;
	}

	va = (vaddr_t)phys_to_virt(TCSR_KEYSLOT_ADDR,
				   MEM_AREA_IO_SEC, sizeof(uint32_t));
	algo_allowed = (io_read32(va) & TCSR_KEYSLOT_ALGO_ALLOWED_BMSK) >>
		       TCSR_KEYSLOT_ALGO_ALLOWED_SHFT;

	switch (algo_allowed) {
	case 0x0:
		if (!(config_data->key_len == ICE_CRYPTO_KEY_SIZE_128 &&
		      config_data->alg_mode == ICE_CRYPTO_ALGO_MODE_AES_ECB)) {
			EMSG("ICE: Invalid key/algo combination");
			return TEE_ERROR_BAD_PARAMETERS;
		}
		capidx_val = 0x2;
		break;
	case 0x1:
		if (!(config_data->key_len == ICE_CRYPTO_KEY_SIZE_256 &&
		      config_data->alg_mode == ICE_CRYPTO_ALGO_MODE_AES_ECB)) {
			EMSG("ICE: Invalid key/algo combination");
			return TEE_ERROR_BAD_PARAMETERS;
		}
		capidx_val = 0x5;
		break;
	case 0xF:
		if (config_data->alg_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS) {
			if (config_data->key_len == ICE_CRYPTO_KEY_SIZE_128)
				capidx_val = 0x0;
			else
				capidx_val = 0x3;
		} else {
			EMSG("ICE: Algorithm mode mismatch for algo_allowed 0xF");
			return TEE_ERROR_BAD_PARAMETERS;
		}
		break;
	default:
		EMSG("ICE: Unsupported algorithm allowed value 0x%x",
		     algo_allowed);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	ice_reg_write(SDC1_SDCC_ICE_CRYPTOCFG_r_16_ADDR(config_data->index),
		      SDC1_SDCC_ICE_CRYPTOCFG_r_16_CFGE_BMSK,
		      SDC1_SDCC_ICE_CRYPTOCFG_r_16_CFGE_SHFT, 0x1);

	ice_reg_write(SDC1_SDCC_ICE_CRYPTOCFG_r_16_ADDR(config_data->index),
		      SDC1_SDCC_ICE_CRYPTOCFG_r_16_CAPIDX_BMSK,
		      SDC1_SDCC_ICE_CRYPTOCFG_r_16_CAPIDX_SHFT, capidx_val);

	ice_reg_write(SDC1_SDCC_ICE_CRYPTOCFG_r_16_ADDR(config_data->index),
		      SDC1_SDCC_ICE_CRYPTOCFG_r_16_DUSIZE_BMSK,
		      SDC1_SDCC_ICE_CRYPTOCFG_r_16_DUSIZE_SHFT, 0x1);

	ice_reg_write(SDC1_SDCC_ICE_SEC_CONTROL_ADDR,
		      SDC1_SDCC_ICE_SEC_CONTROL_CRYPTOCFG_VS_BITS_EN_BMSK,
		      SDC1_SDCC_ICE_SEC_CONTROL_CRYPTOCFG_VS_BITS_EN_SHFT, 0x1);

	ice_reg_write(SDC1_SDCC_ICE_CRYPTOCFG_r_17_ADDR(config_data->index),
		      SDC1_SDCC_ICE_CRYPTOCFG_r_17_KEY_SELECTION_BMSK,
		      SDC1_SDCC_ICE_CRYPTOCFG_r_17_KEY_SELECTION_SHFT,
		      config_data->key_mode);

	ice_reg_write(SDC1_SDCC_ICE_CRYPTOCFG_r_17_ADDR(config_data->index),
		      SDC1_SDCC_ICE_CRYPTOCFG_r_17_DECR_BYPASS_BMSK,
		      SDC1_SDCC_ICE_CRYPTOCFG_r_17_DECR_BYPASS_SHFT,
		      BYPASS_DISABLE);

	ice_reg_write(SDC1_SDCC_ICE_CRYPTOCFG_r_17_ADDR(config_data->index),
		      SDC1_SDCC_ICE_CRYPTOCFG_r_17_ENCR_BYPASS_BMSK,
		      SDC1_SDCC_ICE_CRYPTOCFG_r_17_ENCR_BYPASS_SHFT,
		      BYPASS_DISABLE);

	for (int i = 0; i <= 15; i++) {
		ice_reg_write(SDC1_SDCC_ICE_CRYPTOCFG_r_n_ADDR(
			      config_data->index, i), 0, 0, 0xFFFFFFFF);
	}

	if (config_data->key_mode == ICE_CRYPTO_USE_KEY0_HW_KEY)
		ice_reg_write(SDC1_SDCC_ICE_HWKEY0_CAPIDX_ADDR, 0, 0,
			      capidx_val << 8);
	else
		ice_reg_write(SDC1_SDCC_ICE_HWKEY1_CAPIDX_ADDR, 0, 0,
			      capidx_val << 8);

	return TEE_SUCCESS;
}

/* Generate and configure hardware key */
static TEE_Result cmd_ice_generate_hw_key(uint32_t param_types,
					  TEE_Param params[TEE_NUM_PARAMS])
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	uint32_t seed_type, key_len, alg_mode, ctx_len1, ctx_len2;
	ice_context_config *config = NULL;
	uint8_t *sw_context1 = NULL;
	uint8_t *sw_context2 = NULL;

	if (TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) {
		EMSG("ICE: Invalid parameter type");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	config = (ice_context_config *)params[0].memref.buffer;
	if (!config || params[0].memref.size < sizeof(*config)) {
		EMSG("ICE: Invalid config buffer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	seed_type = config->seed_type;
	key_len = config->key_size;
	alg_mode = config->algo_mode;

	if (TEE_PARAM_TYPE_GET(param_types, 1) == TEE_PARAM_TYPE_MEMREF_INPUT) {
		sw_context1 = params[1].memref.buffer;
		ctx_len1 = params[1].memref.size;
	} else {
		ctx_len1 = 0;
	}

	if (TEE_PARAM_TYPE_GET(param_types, 2) == TEE_PARAM_TYPE_MEMREF_INPUT) {
		sw_context2 = params[2].memref.buffer;
		ctx_len2 = params[2].memref.size;
	} else {
		ctx_len2 = 0;
	}

	if (seed_type != ICE_CRYPTO_CRBK_TYPE &&
	    seed_type != ICE_CRYPTO_OEMPRODSEED_TYPE) {
		EMSG("ICE: Invalid seed type %u", seed_type);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (seed_type == ICE_CRYPTO_OEMPRODSEED_TYPE) {
		if (key_len != ICE_CRYPTO_KEY_SIZE_128 &&
		    key_len != ICE_CRYPTO_KEY_SIZE_256) {
			EMSG("ICE: Invalid key length");
			return TEE_ERROR_BAD_PARAMETERS;
		}

		if (alg_mode != ICE_CRYPTO_ALGO_MODE_AES_ECB &&
		    alg_mode != ICE_CRYPTO_ALGO_MODE_AES_XTS) {
			EMSG("ICE: Invalid algorithm mode");
			return TEE_ERROR_BAD_PARAMETERS;
		}

		if (!sw_context1 ||
		    ctx_len1 > TME_KDF_SW_CONTEXT_BYTES_MAX) {
			EMSG("ICE: Invalid sw_context1");
			return TEE_ERROR_BAD_PARAMETERS;
		}

		if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS &&
		    (!sw_context2 ||
		     ctx_len2 > TME_KDF_SW_CONTEXT_BYTES_MAX)) {
			EMSG("ICE: XTS mode requires two contexts");
			return TEE_ERROR_BAD_PARAMETERS;
		}

		ret = ice_ops_key_cfg(key_len, alg_mode, sw_context1,
				      ctx_len1,
				      TMEL_ICE_ENDPOINT_DATA_KEY_SLOT_3);
		if (ret == TEE_SUCCESS) {
			if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS) {
				ret = ice_ops_key_cfg(
					key_len, alg_mode,
					sw_context2, ctx_len2,
					TMEL_ICE_ENDPOINT_SALT_KEY_SLOT_2);
				if (ret != TEE_SUCCESS)
					EMSG("ICE: OPS salt key config failed (error: 0x%x)",
					     ret);
			}
		} else {
			EMSG("ICE: OPS data key config failed (error: 0x%x)",
			     ret);
		}
	} else {
		ret = ice_hw_key_cfg(key_len, alg_mode,
				     TMEL_ICE_ENDPOINT_DATA_KEY_SLOT_3);
		if (ret == TEE_SUCCESS) {
			if (alg_mode == ICE_CRYPTO_ALGO_MODE_AES_XTS) {
				ret = ice_hw_key_cfg(
					key_len, alg_mode,
					TMEL_ICE_ENDPOINT_SALT_KEY_SLOT_2);
				if (ret != TEE_SUCCESS)
					EMSG("ICE: CRBK salt key config failed (error: 0x%x)",
					     ret);
			}
		} else {
			EMSG("ICE: CRBK data key config failed (error: 0x%x)",
			     ret);
		}
	}

	return ret;
}

/* PTA command dispatcher */
static TEE_Result invoke_command(void *sess_ctx __unused, uint32_t cmd_id,
				 uint32_t param_types,
				 TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd_id) {
	case PTA_CMD_ICE_GENERATE_HW_KEY:
		return cmd_ice_generate_hw_key(param_types, params);
	case PTA_CMD_ICE_SET_HW_KEY_CTX:
		return cmd_ice_set_hw_key_ctx(param_types, params);
	default:
		break;
	}

	EMSG("ICE: Command not implemented: %u", cmd_id);
	return TEE_ERROR_NOT_IMPLEMENTED;
}

pseudo_ta_register(.uuid = PTA_EMMC_ICE_UUID,
		   .name = "emmc_ice.pta",
		   .flags = PTA_DEFAULT_FLAGS | TA_FLAG_DEVICE_ENUM,
		   .invoke_command_entry_point = invoke_command);
