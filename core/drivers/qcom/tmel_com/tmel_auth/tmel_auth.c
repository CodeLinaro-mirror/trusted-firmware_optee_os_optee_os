// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <initcall.h>
#include <kernel/cache_helpers.h>
#include <kernel/dt.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <malloc.h>
#include <string.h>
#include <tee_api_types.h>
#include <trace.h>
#include <util.h>
#include "tmel_auth.h"
#include <tmemessages_uids.h>

static uint32_t tme_auth_version;
static bool version_initialized;

TEE_Result tmel_secure_auth_v1(struct tmel_sec_auth_v1_req *params)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t params_len = 0;
	uint32_t msg_uid = TME_MSG_UID_SECBOOT_SEC_AUTH;
	uint32_t param_id = TME_MSG_UID_SECBOOT_SEC_AUTH_PARAM_ID;

	if (!params) {
		EMSG("Invalid params pointer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (!params->elf_buf.buf || !params->elf_buf.buf_len) {
		EMSG("Invalid ELF buffer parameters");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	params_len = sizeof(struct tmel_sec_auth_v1_req);

	dcache_clean_range(params, params_len);

	res = tmecom_client_send_message(msg_uid, param_id, true,
					 TMECOM_DEFAULT_TIMEOUT, params,
					 params_len, NULL, NULL, NULL);

	dcache_inv_range(params, params_len);

	if (res != TEE_SUCCESS) {
		EMSG("TME-L auth v1 IPC failed: %#"PRIx32, res);
		return res;
	}

	/* Check TME service response status */
	res = tme_status_to_tee_result(params->status);
	if (res != TEE_SUCCESS) {
		EMSG("TME-L auth v1 failed: status=%#"PRIx32" ext_err=%#"PRIx32,
		     params->status, params->extended_error);
		return res;
	}

	if (params->extended_error) {
		EMSG("TME-L auth v1 failed: ext_err=%#"PRIx32,
		     params->extended_error);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

TEE_Result tmel_secure_auth_v2(struct tmel_sec_auth_v2_req *params)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t params_len = 0;
	uint32_t msg_uid = TME_MSG_UID_SECBOOT_SEC_AUTH_V2;
	uint32_t param_id = TME_MSG_UID_SECBOOT_SEC_AUTH_V2_PARAM_ID;

	if (!params)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!params->elf_buf.buf || !params->elf_buf.buf_len) {
		EMSG("Invalid ELF buffer parameters");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	dcache_inv_range(params, sizeof(*params));

	params->first_seg_addr = 0;
	params->first_seg_len = 0;
	params->entry_addr = 0;
	params->extended_error = 0;
	params->status = 0;
	params->keyHandle = 0;

	params_len = sizeof(struct tmel_sec_auth_v2_req);

	dcache_clean_range(params, params_len);

	res = tmecom_client_send_message(msg_uid, param_id, true,
					 TMECOM_DEFAULT_TIMEOUT, params,
					 params_len, NULL, NULL, NULL);

	dcache_inv_range(params, params_len);

	if (res != TEE_SUCCESS) {
		EMSG("TME-L auth v2 IPC failed: %#"PRIx32, res);
		return res;
	}

	/* Check TME service response status */
	res = tme_status_to_tee_result(params->status);
	if (res != TEE_SUCCESS) {
		EMSG("TME-L auth v2 failed: status=%#"PRIx32" ext_err=%#"PRIx32,
		     params->status, params->extended_error);
		return res;
	}

	if (params->extended_error) {
		EMSG("TME-L auth v2 failed: ext_err=%#"PRIx32,
		     params->extended_error);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

TEE_Result tmel_secure_auth(struct tmel_sec_auth_params *params)
{
	TEE_Result res;
	union {
		struct tmel_sec_auth_v1_req v1;
		struct tmel_sec_auth_v2_req v2;
	} auth_req;

	if (!params) {
		EMSG("Invalid params pointer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (!version_initialized) {
		EMSG("TME auth version not initialized");
		return TEE_ERROR_BAD_STATE;
	}

	memset(&auth_req, 0, sizeof(auth_req));

	/* Assign common fields to v1 (works for both v1 and v2 due to union) */
	auth_req.v1.sw_id = params->sw_id;
	auth_req.v1.elf_buf = params->elf_buf;
	auth_req.v1.region_list = params->region_list;
	auth_req.v1.relocate = params->relocate;

	if (tme_auth_version == 1) {
		dcache_clean_range(&auth_req.v1, sizeof(auth_req.v1));
		res = tmel_secure_auth_v1(&auth_req.v1);
	} else if (tme_auth_version == 2) {
		/* Assign v2-specific input fields */
		auth_req.v2.nsIntegrityCheck = params->nsIntegrityCheck;
		auth_req.v2.reservedBits = 0;
		auth_req.v2.reservedBuf = params->reservedBuf;

		dcache_clean_range(&auth_req.v2, sizeof(auth_req.v2));
		res = tmel_secure_auth_v2(&auth_req.v2);

		/* Copy v2-specific output field */
		params->keyHandle = auth_req.v2.keyHandle;
	} else {
		EMSG("Unsupported TME auth version: %u", tme_auth_version);
		return TEE_ERROR_GENERIC;
	}

	/* Copy common output fields (same position in both v1 and v2) */
	params->first_seg_addr = auth_req.v1.first_seg_addr;
	params->first_seg_len = auth_req.v1.first_seg_len;
	params->entry_addr = auth_req.v1.entry_addr;
	params->extended_error = auth_req.v1.extended_error;
	params->status = auth_req.v1.status;

	return res;
}

TEE_Result tmel_auth_init(void)
{
	const void *fdt = NULL;
	int node = -1;
	const fdt32_t *prop = NULL;
	int len = 0;

	if (version_initialized)
		return TEE_SUCCESS;

	fdt = get_dt();
	if (!fdt) {
		EMSG("No device tree found");
		return TEE_ERROR_GENERIC;
	}

	node = fdt_node_offset_by_compatible(fdt, -1, "qcom,tme-auth");
	if (node < 0) {
		EMSG("TME-AUTH node not found in device tree");
		return TEE_ERROR_GENERIC;
	}

	prop = fdt_getprop(fdt, node, "qcom,tme-auth-version", &len);
	if (!prop || len != sizeof(uint32_t)) {
		EMSG("qcom,tme-auth-version property invalid in DTS");
		return TEE_ERROR_GENERIC;
	}

	/* Convert from big-endian and cache */
	tme_auth_version = fdt32_to_cpu(*prop);

	if (tme_auth_version != 1 && tme_auth_version != 2) {
		EMSG("Invalid TME auth version %u from DTS", tme_auth_version);
		return TEE_ERROR_GENERIC;
	}

	version_initialized = true;

	return TEE_SUCCESS;
}

driver_init(tmel_auth_init);
