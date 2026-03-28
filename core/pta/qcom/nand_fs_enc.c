// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

/*
 * NAND Filesystem Encryption PTA Implementation
 * Provides AES encryption/decryption using HUK-derived keys
 */

#include <crypto/crypto.h>
#include <kernel/pseudo_ta.h>
#include <kernel/huk_subkey.h>
#include <kernel/mutex.h>
#include <pta_nand_fs_enc.h>
#include <string.h>
#include <string_ext.h>
#include <tee_api_defines.h>
#include <util.h>

/* Key context for thread-safe access to derived HUK key */
struct derived_key_ctx {
	uint8_t key[PTA_AES_KEY_SIZE_256];
	size_t key_size;
	bool valid;
	struct mutex lock;
};

static struct derived_key_ctx g_key_ctx = {
	.key = { 0 },
	.key_size = 0,
	.valid = false,
	.lock = MUTEX_INITIALIZER,
};

/* Perform AES encryption or decryption */
static TEE_Result aes_crypt_operation(const uint8_t *key, size_t key_size,
				      uint32_t mode, bool encrypt,
				      const void *iv, size_t iv_len,
				      const void *input, size_t input_len,
				      void *output, size_t *output_len)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	void *ctx = NULL;
	uint32_t algo = 0;

	if (!key || !input || !output || !output_len)
		return TEE_ERROR_BAD_PARAMETERS;

	if (input_len == 0 || input_len > PTA_AES_MAX_DATA_SIZE)
		return TEE_ERROR_BAD_PARAMETERS;

	if (input_len % PTA_AES_BLOCK_SIZE != 0)
		return TEE_ERROR_BAD_PARAMETERS;

	if (*output_len < input_len)
		return TEE_ERROR_SHORT_BUFFER;

	if ((mode == PTA_AES_MODE_CBC || mode == PTA_AES_MODE_CTR)) {
		if (!iv || iv_len != PTA_AES_IV_SIZE)
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
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}

	res = crypto_cipher_alloc_ctx(&ctx, algo);
	if (res)
		return res;

	if (encrypt)
		res = crypto_cipher_init(ctx, TEE_MODE_ENCRYPT, key,
					 key_size, NULL, 0, iv, iv_len);
	else
		res = crypto_cipher_init(ctx, TEE_MODE_DECRYPT, key,
					 key_size, NULL, 0, iv, iv_len);

	if (res)
		goto out;

	res = crypto_cipher_update(ctx,
				    encrypt ? TEE_MODE_ENCRYPT :
					      TEE_MODE_DECRYPT,
				    true, input, input_len, output);
	if (res)
		goto out;

	crypto_cipher_final(ctx);
	*output_len = input_len;
	res = TEE_SUCCESS;

out:
	if (ctx)
		crypto_cipher_free_ctx(ctx);

	return res;
}

/* Derive AES key from HUK and store in global context */
static TEE_Result cmd_set_key(uint32_t param_types,
			       TEE_Param params[TEE_NUM_PARAMS])
{
	uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE);
	TEE_Result res = TEE_ERROR_GENERIC;
	size_t key_size = 0;
	static const TEE_UUID pta_uuid = PTA_NAND_FS_ENC_UUID;

	if (param_types != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	key_size = *(size_t *)params[0].memref.buffer;

	if (key_size != PTA_AES_KEY_SIZE_128 &&
	    key_size != PTA_AES_KEY_SIZE_256)
		return TEE_ERROR_BAD_PARAMETERS;

	mutex_lock(&g_key_ctx.lock);

	res = huk_subkey_derive(HUK_SUBKEY_UNIQUE_TA,
				&pta_uuid, sizeof(TEE_UUID),
				g_key_ctx.key, key_size);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to derive HUK subkey");
		mutex_unlock(&g_key_ctx.lock);
		return res;
	}

	g_key_ctx.key_size = key_size;
	g_key_ctx.valid = true;

	mutex_unlock(&g_key_ctx.lock);

	return TEE_SUCCESS;
}

/* Encrypt or decrypt data using derived key */
static TEE_Result cmd_aes_crypt(uint32_t param_types,
				 TEE_Param params[TEE_NUM_PARAMS])
{
	uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
					  TEE_PARAM_TYPE_MEMREF_INPUT,
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
	size_t local_key_size = 0;

	if (param_types != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	mutex_lock(&g_key_ctx.lock);

	if (!g_key_ctx.valid) {
		mutex_unlock(&g_key_ctx.lock);
		EMSG("No valid key. Call derive_key first.");
		return TEE_ERROR_BAD_STATE;
	}

	memcpy(local_key, g_key_ctx.key, g_key_ctx.key_size);
	local_key_size = g_key_ctx.key_size;

	mutex_unlock(&g_key_ctx.lock);

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

	if (mode != PTA_AES_MODE_ECB &&
	    mode != PTA_AES_MODE_CBC &&
	    mode != PTA_AES_MODE_CTR) {
		EMSG("Invalid mode: %u", mode);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	iv_buf = params[1].memref.buffer;
	iv_len = params[1].memref.size;

	if (mode == PTA_AES_MODE_CBC || mode == PTA_AES_MODE_CTR) {
		if (!iv_buf || iv_len != PTA_AES_IV_SIZE) {
			EMSG("Invalid IV for mode %u (exp %u bytes, got %zu)",
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

	/* Validate output buffer */
	if (!output_buf) {
		EMSG("Invalid output buffer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (output_len < input_len) {
		EMSG("Output buffer too small (%zu < %zu)",
		     output_len, input_len);
		return TEE_ERROR_SHORT_BUFFER;
	}

	res = aes_crypt_operation(local_key, local_key_size,
				  mode, encrypt,
				  iv_buf, iv_len,
				  input_buf, input_len,
				  output_buf, &output_len);

	memzero_explicit(local_key, sizeof(local_key));

	if (res == TEE_SUCCESS) {
		params[3].memref.size = output_len;
	} else {
		EMSG("AES operation failed: 0x%x", res);
	}

	return res;
}

/* Clear derived key from memory */
static TEE_Result cmd_clear_key(uint32_t param_types,
				 TEE_Param params[TEE_NUM_PARAMS] __unused)
{
	uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE,
					  TEE_PARAM_TYPE_NONE);

	if (param_types != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	mutex_lock(&g_key_ctx.lock);

	memzero_explicit(g_key_ctx.key, sizeof(g_key_ctx.key));
	g_key_ctx.key_size = 0;
	g_key_ctx.valid = false;

	mutex_unlock(&g_key_ctx.lock);

	return TEE_SUCCESS;
}

/* PTA command dispatcher */
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

	return TEE_ERROR_NOT_IMPLEMENTED;
}

pseudo_ta_register(.uuid = PTA_NAND_FS_ENC_UUID,
		   .name = "nandfs_enc.pta",
		   .flags = PTA_DEFAULT_FLAGS | TA_FLAG_DEVICE_ENUM,
		   .invoke_command_entry_point = invoke_command);
