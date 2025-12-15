// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <tee_api_types.h>
#include <trace.h>
#include <mm/core_memprot.h>

#include "tmefuse_client.h"
#include "tmecom_client.h"
#include "tmemessages_uids.h"

/*
 * Reads multiple fuse values from TME secure storage.
 * Sends fuse read request to TME and retrieves fuse data.
 */
TEE_Result tme_ipc_fuselist_read(struct tme_fuse_payload *fuse, size_t size)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct tme_fuse_read_multiple_msg msg = { };
	paddr_t fuse_paddr = virt_to_phys(fuse);

	if (!fuse || !size || !fuse_paddr)
		return TEE_ERROR_BAD_PARAMETERS;

	msg.fuse_read_data.p_buffer = (uint32_t)fuse_paddr;
	msg.fuse_read_data.buf_len = size;
	msg.fuse_read_data.buf_out_len = 0;

	/* Send fuse read request to TME */
	ret = tmecom_client_send_message
			(TME_MSG_UID_FUSE_READ_MULTIPLE_ROW,
			 TME_MSG_UID_FUSE_READ_MULTIPLE_ROW_PARAM_ID,
			 true,
			 TMECOM_DEFAULT_TIMEOUT,
			 &msg,
			 sizeof(msg),
			 NULL,
			 NULL,
			 NULL);

	if (ret != TEE_SUCCESS) {
		EMSG("Tmecom send message failed: 0x%x", ret);
		return ret;
	}

	return TEE_SUCCESS;
}
