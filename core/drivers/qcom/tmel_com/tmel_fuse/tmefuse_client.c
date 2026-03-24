// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <mm/core_memprot.h>
#include <tmefuse_client.h>
#include <tmecom_client.h>
#include <tmemessages_uids.h>
#include <trace.h>

/*
 * Reads multiple fuse values from TME secure storage.
 * Sends fuse read request to TME and retrieves fuse data.
 */
TEE_Result tmel_qfprom_fuselist_read(struct tme_fuse_payload *fuse, size_t size)
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

/*
 * Write single fuse row to TME
 */
TEE_Result tmel_qfprom_fuselist_write(struct tme_fuse_payload *fuse,
				      size_t size)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct {
		uint32_t status;
		uint32_t fuse_addr;
		uint32_t lsb_val;
		uint32_t msb_val;
	} msg = { 0 };

	if (!fuse || !size)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Set up the message for single fuse write */
	msg.fuse_addr = fuse->fuse_addr;
	msg.lsb_val = fuse->lsb_val;
	msg.msb_val = fuse->msb_val;

	/* Send fuse write request to TME */
	ret = tmecom_client_send_message
			(TME_MSG_UID_FUSE_WRITE_SINGLE_ROW,
			 TME_MSG_UID_FUSE_WRITE_SINGLE_ROW_PARAM_ID,
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

/*
 * Update TME OEM MRC state vector
 */
TEE_Result
tmel_qfprom_oem_mrc_state_update(uint32_t activate_vector,
				 uint32_t revocate_vector)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct {
		uint32_t activate_vector;
		uint32_t revocate_vector;
		uint32_t status;
	} msg = { 0 };

	/* Prepare MRC state update message */
	msg.activate_vector = activate_vector;
	msg.revocate_vector = revocate_vector;
	msg.status = TEE_SUCCESS;

	/* Send MRC state update request to TME */
	ret = tmecom_client_send_message
			(TME_MSG_UID_SECBOOT_OEM_MRC_STATE_UPDATE,
			 TME_MSG_UID_SECBOOT_OEM_MRC_STATE_UPDATE_PARAM_ID,
			 true,
			 TMECOM_DEFAULT_TIMEOUT,
			 &msg,
			 sizeof(msg),
			 NULL,
			 NULL,
			 NULL);

	if (ret != TEE_SUCCESS) {
		EMSG("TME MRC state update failed: 0x%x", ret);
		return ret;
	}

	return TEE_SUCCESS;
}
