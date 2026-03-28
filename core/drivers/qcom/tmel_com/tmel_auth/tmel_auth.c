// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <kernel/cache_helpers.h>
#include <kernel/panic.h>
#include <mm/core_memprot.h>
#include <malloc.h>
#include <string.h>
#include <tee_api_types.h>
#include <trace.h>
#include <util.h>
#include "tmel_auth.h"
#include <tmemessages_uids.h>

TEE_Result tmel_secure_auth_v2(struct tmel_sec_auth_v2_req *params)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	enum tmecom_response tme_err;
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
					 params_len, NULL, NULL, &tme_err);

	dcache_inv_range(params, params_len);

	if (res != TEE_SUCCESS ||
	    tme_err != TMECOM_RSP_SUCCESS ||
	    params->status != TMECOM_RSP_SUCCESS ||
	    params->extended_error) {
		EMSG("TME-L auth failed: %#"PRIx32"/%d/%#"PRIx32"/%#"PRIx32,
		     res, tme_err, params->status, params->extended_error);
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}
