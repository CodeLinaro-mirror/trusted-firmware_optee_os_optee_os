// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <crypto/crypto.h>
#include <drivers/qcom_huk.h>
#include <kernel/pseudo_ta.h>
#include <kernel/huk_subkey.h>
#include <kernel/mutex.h>
#include <pta_nand_fs_enc.h>
#include <string.h>
#include <string_ext.h>
#include <tee_api_defines.h>
#include <util.h>

struct derived_key_ctx {
	uint8_t key[PTA_AES_KEY_SIZE_256];	/* data key */
	uint8_t key2[PTA_AES_KEY_SIZE_256];	/* XTS tweak key */
	size_t key_size;
	uint32_t algo_mode;
	bool valid;
	struct mutex lock;
};

static struct derived_key_ctx g_key_ctx = {
	.key = { 0 },
	.key2 = { 0 },
	.key_size = 0,
	.algo_mode = 0,
	.valid = false,
	.lock = MUTEX_INITIALIZER,
};

static TEE_Result aes_crypt_operation(const uint8_t *key, size_t key_size,
				      const uint8_t *key2, size_t key2_size,
				      uint32_t mode, bool encrypt,
				      const void *iv, size_t iv_len,
				      const void *input, size_t input_len,
				      void *output, size_t *output_len)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	void *ctx = NULL;
	uint32_t algo = 0;

	if (!key || !input || !output || !output_len) {
		EMSG("AES: missing key, data buffer or output length");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (input_len == 0 || input_len > PTA_AES_MAX_DATA_SIZE) {
		EMSG("AES: invalid input length %zu", input_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (input_len % PTA_AES_BLOCK_SIZE != 0) {
		EMSG("AES: input length %zu is not block aligned", input_len);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (*output_len < input_len) {
		EMSG("AES: output capacity %zu, need %zu",
		     *output_len, input_len);
		return TEE_ERROR_SHORT_BUFFER;
	}

	if (mode == PTA_AES_MODE_CBC || mode == PTA_AES_MODE_CTR ||
	    mode == PTA_AES_MODE_XTS) {
		if (!iv || iv_len != PTA_AES_IV_SIZE) {
			EMSG("AES: invalid IV/tweak, mode %u size %zu",
			     mode, iv_len);
			return TEE_ERROR_BAD_PARAMETERS;
		}
	}

	if (mode == PTA_AES_MODE_XTS && (!key2 || key2_size != key_size)) {
		EMSG("AES: missing XTS key or key sizes differ (%zu/%zu)",
		     key_size, key2_size);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	switch (mode) {
	case PTA_AES_MODE_ECB:
		algo = TEE_ALG_AES_ECB_NOPAD;
		break;
	case PTA_AES_MODE_CBC:
		algo = TEE_ALG_AES_CBC_NOPAD;
		break;
	case PTA_AES_MODE_CTR:
		algo = TEE_ALG_AES_CTR;
		break;
	case PTA_AES_MODE_XTS:
		algo = TEE_ALG_AES_XTS;
		break;
	default:
		EMSG("AES: unsupported mode %u", mode);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	res = crypto_cipher_alloc_ctx(&ctx, algo);
	if (res) {
		EMSG("AES: context allocation failed, mode %u: %#x", mode, res);
		return res;
	}

	res = crypto_cipher_init(ctx,
				 encrypt ? TEE_MODE_ENCRYPT : TEE_MODE_DECRYPT,
				 key, key_size,
				 (mode == PTA_AES_MODE_XTS) ? key2 : NULL,
				 (mode == PTA_AES_MODE_XTS) ? key2_size : 0,
				 iv, iv_len);
	if (res) {
		EMSG("AES: cipher initialization failed, mode %u: %#x",
		     mode, res);
		goto out;
	}

	res = crypto_cipher_update(ctx,
				    encrypt ? TEE_MODE_ENCRYPT :
					      TEE_MODE_DECRYPT,
				    true, input, input_len, output);
	if (res) {
		EMSG("AES: cipher update failed, mode %u: %#x", mode, res);
		goto out;
	}

	crypto_cipher_final(ctx);
	*output_len = input_len;
	res = TEE_SUCCESS;

out:
	if (ctx)
		crypto_cipher_free_ctx(ctx);

	return res;
}

static TEE_Result cmd_set_key(uint32_t param_types,
			       TEE_Param params[TEE_NUM_PARAMS])
{
	/* OPS uses the same 64-byte zero label as eMMC and TZ SCM. */
	static const uint8_t ops_key_label[64] = { };
	/* CRBK has no caller context, so labels separate its two XTS keys. */
	static const uint8_t crbk_data_label[] = "AES-HUK DATA KEY";
	static const uint8_t crbk_tweak_label[] = "AES-HUK TWEAK KEY";
	TEE_Result res = TEE_ERROR_GENERIC;
	struct aes_huk_set_key_req cfg = { };
	uint8_t data_context[PTA_AES_HUK_SW_CONTEXT_MAX] = { };
	uint8_t tweak_context[PTA_AES_HUK_SW_CONTEXT_MAX] = { };
	uint32_t exp_pt = 0;
	struct aes_huk_derive_req dreq = { };
	size_t key_size = 0;
	uint32_t seed_type = 0;
	uint32_t algo_mode = 0;
	bool crbk = false;
	bool xts = false;

	if (TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) {
		EMSG("SET_KEY: config must be an input memref, types %#x",
		     param_types);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (!params[0].memref.buffer || params[0].memref.size != sizeof(cfg)) {
		EMSG("SET_KEY: invalid config, size %zu expected %zu",
		     params[0].memref.size, sizeof(cfg));
		return TEE_ERROR_BAD_PARAMETERS;
	}

	memcpy(&cfg, params[0].memref.buffer, sizeof(cfg));
	seed_type = cfg.seed_type;
	key_size = cfg.key_size;
	algo_mode = cfg.algo_mode;

	if (key_size != PTA_AES_KEY_SIZE_128 &&
	    key_size != PTA_AES_KEY_SIZE_256) {
		EMSG("SET_KEY: unsupported key size %zu", key_size);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (seed_type != PTA_AES_HUK_SEED_CRBK &&
	    seed_type != PTA_AES_HUK_SEED_OEM_PRODSEED) {
		EMSG("SET_KEY: unsupported seed %u", seed_type);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (algo_mode != PTA_AES_MODE_ECB && algo_mode != PTA_AES_MODE_CBC &&
	    algo_mode != PTA_AES_MODE_CTR && algo_mode != PTA_AES_MODE_XTS) {
		EMSG("SET_KEY: unsupported mode %u", algo_mode);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	crbk = (seed_type == PTA_AES_HUK_SEED_CRBK);
	xts = (algo_mode == PTA_AES_MODE_XTS);

	exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				 crbk ? TEE_PARAM_TYPE_NONE :
					TEE_PARAM_TYPE_MEMREF_INPUT,
				 !crbk && xts ? TEE_PARAM_TYPE_MEMREF_INPUT :
						TEE_PARAM_TYPE_NONE,
				 TEE_PARAM_TYPE_NONE);
	if (param_types != exp_pt) {
		EMSG("SET_KEY: types %#x, expected %#x (seed %u mode %u)",
		     param_types, exp_pt, seed_type, algo_mode);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (!crbk) {
		if (!params[1].memref.buffer ||
		    params[1].memref.size == 0 ||
		    params[1].memref.size > PTA_AES_HUK_SW_CONTEXT_MAX) {
			EMSG("SET_KEY: invalid OPS data context, size %zu",
			     params[1].memref.size);
			return TEE_ERROR_BAD_PARAMETERS;
		}

		if (xts &&
		    (!params[2].memref.buffer ||
		     params[2].memref.size == 0 ||
		     params[2].memref.size > PTA_AES_HUK_SW_CONTEXT_MAX)) {
			EMSG("SET_KEY: invalid OPS tweak context, size %zu",
			     params[2].memref.size);
			return TEE_ERROR_BAD_PARAMETERS;
		}
	}

	/* Shared contexts may change while TMEL requests are in flight. */
	if (!crbk) {
		memcpy(data_context, params[1].memref.buffer,
		       params[1].memref.size);
		if (xts)
			memcpy(tweak_context, params[2].memref.buffer,
			       params[2].memref.size);
	}

	mutex_lock(&g_key_ctx.lock);
	memzero_explicit(g_key_ctx.key, sizeof(g_key_ctx.key));
	memzero_explicit(g_key_ctx.key2, sizeof(g_key_ctx.key2));
	g_key_ctx.valid = false;

	dreq.seed_type = seed_type;
	dreq.salt_label = crbk ? crbk_data_label : ops_key_label;
	dreq.salt_label_len = crbk ? sizeof(crbk_data_label) - 1 :
				    sizeof(ops_key_label);
	if (!crbk) {
		dreq.sw_context = data_context;
		dreq.sw_context_len = params[1].memref.size;
	}
	res = huk_subkey_derive(HUK_SUBKEY_AES_HUK, &dreq, sizeof(dreq),
				g_key_ctx.key, key_size);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to derive AES HUK data key: 0x%x", res);
		goto out;
	}

	if (xts) {
		dreq.seed_type = seed_type;
		dreq.sw_context = NULL;
		dreq.sw_context_len = 0;
		dreq.salt_label = crbk ? crbk_tweak_label : ops_key_label;
		dreq.salt_label_len = crbk ? sizeof(crbk_tweak_label) - 1 :
					    sizeof(ops_key_label);
		if (!crbk) {
			dreq.sw_context = tweak_context;
			dreq.sw_context_len = params[2].memref.size;
		}
		res = huk_subkey_derive(HUK_SUBKEY_AES_HUK, &dreq, sizeof(dreq),
					g_key_ctx.key2, key_size);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to derive AES HUK tweak key: 0x%x", res);
			goto out;
		}

		/* XTS requires distinct data and tweak keys. */
		if (!consttime_memcmp(g_key_ctx.key, g_key_ctx.key2,
				      key_size)) {
			EMSG("AES HUK: XTS data and tweak keys are identical");
			res = TEE_ERROR_SECURITY;
			goto out;
		}
	}

	g_key_ctx.key_size = key_size;
	g_key_ctx.algo_mode = algo_mode;
	g_key_ctx.valid = true;

out:
	if (res) {
		memzero_explicit(g_key_ctx.key, sizeof(g_key_ctx.key));
		memzero_explicit(g_key_ctx.key2, sizeof(g_key_ctx.key2));
		g_key_ctx.key_size = 0;
		g_key_ctx.algo_mode = 0;
		g_key_ctx.valid = false;
	}
	mutex_unlock(&g_key_ctx.lock);
	memzero_explicit(data_context, sizeof(data_context));
	memzero_explicit(tweak_context, sizeof(tweak_context));
	return res;
}

static TEE_Result cmd_aes_crypt(uint32_t param_types,
				 TEE_Param params[TEE_NUM_PARAMS])
{
	uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
					  TEE_PARAM_TYPE_MEMREF_INPUT,
					  TEE_PARAM_TYPE_MEMREF_INPUT,
					  TEE_PARAM_TYPE_MEMREF_OUTPUT);
	uint32_t ecb_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_MEMREF_INPUT,
					  TEE_PARAM_TYPE_MEMREF_OUTPUT);
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t direction = 0;
	uint32_t mode = 0;
	bool encrypt = false;
	void *iv_buf = NULL;
	size_t iv_len = 0;
	void *input_buf = NULL;
	size_t input_len = 0;
	void *output_buf = NULL;
	size_t output_len = 0;
	uint8_t local_key[PTA_AES_KEY_SIZE_256];
	uint8_t local_key2[PTA_AES_KEY_SIZE_256];
	size_t local_key_size = 0;
	uint32_t cached_mode = 0;

	if (param_types != exp_pt && param_types != ecb_pt) {
		EMSG("CRYPT: invalid types %#x, expected %#x or %#x",
		     param_types, exp_pt, ecb_pt);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	direction = params[0].value.a;
	mode = params[0].value.b;

	if (direction == PTA_AES_ENCRYPT) {
		encrypt = true;
	} else if (direction == PTA_AES_DECRYPT) {
		encrypt = false;
	} else {
		EMSG("Invalid direction: %u (must be PTA_AES_ENCRYPT or PTA_AES_DECRYPT)", direction);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (mode != PTA_AES_MODE_ECB && mode != PTA_AES_MODE_CBC &&
	    mode != PTA_AES_MODE_CTR && mode != PTA_AES_MODE_XTS) {
		EMSG("Invalid mode: %u", mode);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (mode != PTA_AES_MODE_ECB) {
		if (param_types != exp_pt) {
			EMSG("CRYPT: mode %u needs a 16-byte IV/tweak memref",
			     mode);
			return TEE_ERROR_BAD_PARAMETERS;
		}
		iv_buf = params[1].memref.buffer;
		iv_len = params[1].memref.size;
	}

	if (mode == PTA_AES_MODE_CBC || mode == PTA_AES_MODE_CTR ||
	    mode == PTA_AES_MODE_XTS) {
		if (!iv_buf || iv_len != PTA_AES_IV_SIZE) {
			EMSG("Invalid IV/tweak: mode %u, expected %u, got %zu",
			     mode, PTA_AES_IV_SIZE, iv_len);
			return TEE_ERROR_BAD_PARAMETERS;
		}
	}

	input_buf = params[2].memref.buffer;
	input_len = params[2].memref.size;

	output_buf = params[3].memref.buffer;
	output_len = params[3].memref.size;

	if (!input_buf || input_len == 0) {
		EMSG("Invalid input buffer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (input_len % PTA_AES_BLOCK_SIZE != 0) {
		EMSG("Input size (%zu) must be multiple of %u",
		     input_len, PTA_AES_BLOCK_SIZE);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (input_len > PTA_AES_MAX_DATA_SIZE) {
		EMSG("Input size (%zu) exceeds maximum (%u)",
		     input_len, PTA_AES_MAX_DATA_SIZE);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (!output_buf) {
		EMSG("Invalid output buffer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (output_len < input_len) {
		EMSG("Output buffer too small (%zu < %zu)",
		     output_len, input_len);
		params[3].memref.size = input_len;
		return TEE_ERROR_SHORT_BUFFER;
	}

	mutex_lock(&g_key_ctx.lock);

	if (!g_key_ctx.valid) {
		mutex_unlock(&g_key_ctx.lock);
		EMSG("No valid key. Call set_key first.");
		return TEE_ERROR_BAD_STATE;
	}

	cached_mode = g_key_ctx.algo_mode;

	if (mode != cached_mode) {
		EMSG("Mode %u does not match derived key mode %u",
		     mode, cached_mode);
		mutex_unlock(&g_key_ctx.lock);
		return TEE_ERROR_BAD_STATE;
	}

	memcpy(local_key, g_key_ctx.key, g_key_ctx.key_size);
	memcpy(local_key2, g_key_ctx.key2, g_key_ctx.key_size);
	local_key_size = g_key_ctx.key_size;

	mutex_unlock(&g_key_ctx.lock);

	res = aes_crypt_operation(local_key, local_key_size,
				  local_key2, local_key_size,
				  mode, encrypt,
				  iv_buf, iv_len,
				  input_buf, input_len,
				  output_buf, &output_len);

	memzero_explicit(local_key, sizeof(local_key));
	memzero_explicit(local_key2, sizeof(local_key2));

	if (res == TEE_SUCCESS) {
		params[3].memref.size = output_len;
	} else {
		EMSG("AES operation failed: 0x%x", res);
	}

	return res;
}

static TEE_Result cmd_clear_key(uint32_t param_types,
				 TEE_Param params[TEE_NUM_PARAMS] __unused)
{
	uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE);

	if (param_types != exp_pt) {
		EMSG("CLEAR_KEY: types %#x, expected %#x", param_types, exp_pt);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	mutex_lock(&g_key_ctx.lock);

	memzero_explicit(g_key_ctx.key, sizeof(g_key_ctx.key));
	memzero_explicit(g_key_ctx.key2, sizeof(g_key_ctx.key2));
	g_key_ctx.key_size = 0;
	g_key_ctx.algo_mode = 0;
	g_key_ctx.valid = false;

	mutex_unlock(&g_key_ctx.lock);

	return TEE_SUCCESS;
}

static TEE_Result invoke_command(void *sess_ctx __unused, uint32_t cmd_id,
				  uint32_t param_types,
				  TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd_id) {
	case PTA_NAND_FS_ENC_SET_KEY:
		return cmd_set_key(param_types, params);
	case PTA_NAND_FS_ENC_CRYPT:
		return cmd_aes_crypt(param_types, params);
	case PTA_NAND_FS_ENC_CLEAR_KEY:
		return cmd_clear_key(param_types, params);
	default:
		break;
	}

	EMSG("NAND PTA: unsupported command %u", cmd_id);
	return TEE_ERROR_NOT_IMPLEMENTED;
}

pseudo_ta_register(.uuid = PTA_NAND_FS_ENC_UUID,
		   .name = "nandfs_enc.pta",
		   .flags = PTA_DEFAULT_FLAGS | TA_FLAG_DEVICE_ENUM,
		   .invoke_command_entry_point = invoke_command);
