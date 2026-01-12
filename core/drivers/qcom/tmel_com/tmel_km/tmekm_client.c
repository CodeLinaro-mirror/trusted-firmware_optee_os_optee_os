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
#include <io.h>
#include <platform_config.h>

#include "tmekm_client.h"
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

TEE_Result tme_km_derive_key(const struct tme_kdf_spec *kdf_spec,
			     tme_key_handle_t *key_handle)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct tme_km_derive_key_msg msg = { };
	struct tme_kdf_spec *kdf_buf = NULL;
	void *kdf_orig_addr = NULL;
	paddr_t kdf_paddr = 0;
	size_t kdf_size = sizeof(struct tme_kdf_spec);
	size_t msg_size = sizeof(struct tme_km_derive_key_msg);

	if (!kdf_spec || !key_handle)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Allocate coherent buffer for KDF spec */
	kdf_buf = tmecom_client_malloc_coherent(kdf_size, 64, &kdf_orig_addr,
						&kdf_paddr);
	if (!kdf_buf) {
		EMSG("Failed to allocate coherent buffer for KDF spec");
		ret = TEE_ERROR_OUT_OF_MEMORY;
		goto cleanup;
	}

	/* Copy KDF spec to coherent buffer */
	memset(kdf_buf, 0, kdf_size);
	memcpy(kdf_buf, kdf_spec, kdf_size);

	/* Prepare derive key message */
	msg.input.key_id = TME_KEY_HANDLE_ALLOC;
	/* Check if physical address fits in 32-bit space */
	if (kdf_paddr > UINT32_MAX) {
		EMSG("KDF buffer paddr 0x%lx exceeds 32-bit range", kdf_paddr);
		ret = TEE_ERROR_GENERIC;
		goto cleanup;
	}
	msg.input.kdf_info_pdata = (uint32_t)kdf_paddr;
	msg.input.kdf_info_length = kdf_size;
	msg.input.cred_slot = TME_CRED_SLOT_ID_NONE;

	/* Initialize output */
	msg.output.status = TME_STATUS_UNKNOWN;
	msg.output.key_id = TME_KEY_HANDLE_INVALID;
	memset(&msg.output.seq_status, 0, sizeof(msg.output.seq_status));

	/* Send derive key request to TME */
	ret = tmecom_client_send_message(TME_MSG_UID_KM_DERIVE,
					 TME_MSG_UID_KM_DERIVE_PARAM_ID,
					 true,
					 TMECOM_DEFAULT_TIMEOUT,
					 &msg,
					 msg_size,
					 NULL,
					 NULL,
					 NULL);

	if (ret != TEE_SUCCESS) {
		EMSG("TME derive key message failed: 0x%x", ret);
		goto cleanup;
	}

	/* Check TME response status */
	ret = tme_status_to_tee_result(msg.output.status);
	if (ret != TEE_SUCCESS) {
		EMSG("TME derive key failed, status: 0x%x", msg.output.status);
		goto cleanup;
	}

	/* Return the derived key handle */
	*key_handle = msg.output.key_id;

cleanup:
	if (kdf_buf) {
		/* Zero sensitive key material before freeing */
		memset(kdf_buf, 0, kdf_size);
		tmecom_client_free_coherent(kdf_buf, kdf_orig_addr, kdf_size);
	}

	return ret;
}

/*
 * Distribute a key to a crypto engine
 */
TEE_Result tme_km_distribute_key(tme_key_handle_t key_handle,
				 uint32_t dst_id,
				 uint32_t dst_key_index)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct tme_km_distribute_key_msg msg = { };
	size_t msg_size = sizeof(struct tme_km_distribute_key_msg);

	if (key_handle == TME_KEY_HANDLE_INVALID)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Prepare distribute key message */
	msg.input.key_id = key_handle;
	msg.input.dst_id = dst_id;
	msg.input.dst_key_index = dst_key_index;

	/* Initialize output */
	msg.output.status = TME_STATUS_UNKNOWN;
	memset(&msg.output.seq_status, 0, sizeof(msg.output.seq_status));

	/* Send distribute key request to TME */
	ret = tmecom_client_send_message(TME_MSG_UID_KM_DISTRIBUTE,
					 TME_MSG_UID_KM_DISTRIBUTE_PARAM_ID,
					 true,
					 TMECOM_DEFAULT_TIMEOUT,
					 &msg,
					 msg_size,
					 NULL,
					 NULL,
					 NULL);

	if (ret != TEE_SUCCESS) {
		EMSG("TME distribute key message failed: 0x%x", ret);
		goto cleanup;
	}

	/* Check TME response status */
	ret = tme_status_to_tee_result(msg.output.status);
	if (ret != TEE_SUCCESS) {
		EMSG("TME distribute key failed, status: 0x%x",
		     msg.output.status);
		goto cleanup;
	}

cleanup:
	return ret;
}

/*
 * Clear a key from TME key storage
 */
TEE_Result tme_km_clear_key(tme_key_handle_t key_handle)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct tme_km_clear_key_msg msg = { };
	size_t msg_size = sizeof(struct tme_km_clear_key_msg);

	if (key_handle == TME_KEY_HANDLE_INVALID)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Prepare clear key message */
	msg.input.key_id = key_handle;

	/* Initialize output */
	msg.output.status = TME_STATUS_UNKNOWN;
	memset(&msg.output.seq_status, 0, sizeof(msg.output.seq_status));

	/* Send clear key request to TME */
	ret = tmecom_client_send_message(TME_MSG_UID_KM_CLEAR,
					 TME_MSG_UID_KM_CLEAR_PARAM_ID,
					 true,
					 TMECOM_DEFAULT_TIMEOUT,
					 &msg,
					 msg_size,
					 NULL,
					 NULL,
					 NULL);

	if (ret != TEE_SUCCESS) {
		EMSG("TME clear key message failed: 0x%x", ret);
		goto cleanup;
	}

	/* Check TME response status */
	ret = tme_status_to_tee_result(msg.output.status);
	if (ret != TEE_SUCCESS) {
		EMSG("TME clear key failed, status: 0x%x", msg.output.status);
		goto cleanup;
	}

cleanup:
	return ret;
}

void tme_km_create_key_policy(uint32_t key_length,
			      uint32_t algo_mode,
			      uint32_t key_destination,
			      uint32_t lineage,
			      struct tme_key_policy *policy)
{
	uint32_t key_policy_lo32 = 0;
	uint32_t key_policy_hi32 = 0;

	if (!policy)
		return;

	/* Validate key length */
	if (key_length != 128 && key_length != 256) {
		EMSG("Invalid key length: %u (must be 128 or 256)", key_length);
		return;
	}

	/* Build low 32 bits of key policy */
	key_policy_lo32 = TME_KT_Symmetric | TME_KP_Generic |
			  TME_KOP_Encryption | TME_KOP_Decryption |
			  TME_KSL_HWKey | TME_KO_TZ | lineage;

	/* Add key length */
	if (key_length == 128)
		key_policy_lo32 |= TME_KL_128;
	else
		key_policy_lo32 |= TME_KL_256;

	/* Add algorithm mode */
	key_policy_lo32 |= algo_mode;

	/* Build high 32 bits of key policy */
	key_policy_hi32 = TME_KPV_Version | TME_KAU_TZ;

	/* Add key destination */
	if (key_destination == TME_KD_ICE_ENDPOINT)
		key_policy_hi32 |= TME_KD_ICE_ENDPOINT;
	else if (key_destination == TME_KD_TCSR_ENDPOINT)
		key_policy_hi32 |= TME_KD_TCSR_ENDPOINT;

	policy->low = key_policy_lo32;
	policy->high = key_policy_hi32;
}

TEE_Result tme_km_read_tcsr_key_and_clear(uint32_t *key, uint32_t key_size,
					  uint32_t slot_id)
{
	uint32_t i;
	struct io_pa_va tcsr_pa_va = {
		.pa = 0,
		.va = 0,
	};
	uint32_t *tcsr_addr = NULL;
	uint32_t reg_count;
	size_t total_size;

	if (!key) {
		EMSG("Output buffer for TCSR key is NULL");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (slot_id > 1) {
		EMSG("Invalid slot_id: %u (must be 0 or 1)", slot_id);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	/* Select base address and register count based on slot_id */
	if (slot_id == 0) {
		tcsr_pa_va.pa = TCSR_FUSE_PRI_HW_KEY_BASE_START;
		reg_count = TCSR_FUSE_PRI_HW_KEY_REG_COUNT;
	} else {
		tcsr_pa_va.pa = TCSR_FUSE_SEC_HW_KEY_BASE_START;
		reg_count = TCSR_FUSE_SEC_HW_KEY_REG_COUNT;
	}

	/* Calculate total size needed */
	total_size = reg_count * sizeof(uint32_t);

	/* Validate buffer size */
	if (key_size < total_size) {
		EMSG("Buffer too small: provided %u bytes, need %zu bytes",
		     key_size, total_size);
		return TEE_ERROR_SHORT_BUFFER;
	}

	/* Map physical address to virtual address */
	tcsr_addr = (uint32_t *)io_pa_or_va(&tcsr_pa_va, total_size);
	if (!tcsr_addr) {
		EMSG("Failed to map TCSR registers at PA 0x%lx", tcsr_pa_va.pa);
		return TEE_ERROR_GENERIC;
	}

	/*
	 * Read reg_count x 32-bit words from TCSR registers
	 */
	for (i = 0; i < reg_count; i++) {
		key[i] = io_read32((vaddr_t)&tcsr_addr[i]);
	}

	/* Clear all TCSR registers by writing zeros */
	for (i = 0; i < reg_count; i++) {
		io_write32((vaddr_t)&tcsr_addr[i], 0);
	}

	/* Ensure all writes complete before returning */
	dsb();
	isb();

	/* Verify registers were cleared */
	for (i = 0; i < reg_count; i++) {
		if (io_read32((vaddr_t)&tcsr_addr[i]) != 0) {
			EMSG("Failed to clear TCSR register %u", i);
			/* Zero the key buffer on failure */
			memset(key, 0, total_size);
			return TEE_ERROR_SECURITY;
		}
	}

	return TEE_SUCCESS;
}
