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
TEE_Result tmel_fuse_read_multiple_rows(struct tme_fuse_payload *fuse,
					size_t size)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct tme_fuse_read_multiple_msg msg = { };
	paddr_t fuse_paddr = 0;

	if (!fuse || !size)
		return TEE_ERROR_BAD_PARAMETERS;

	fuse_paddr = virt_to_phys(fuse);
	if (!fuse_paddr)
		return TEE_ERROR_BAD_PARAMETERS;

	msg.status = TME_STATUS_UNKNOWN;
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
		EMSG("TME fuse read multiple rows message failed: 0x%x", ret);
		return ret;
	}

	/* Check TME response status */
	ret = tme_status_to_tee_result(msg.status);
	if (ret != TEE_SUCCESS)
		EMSG("TME fuse read multiple rows failed, status: 0x%x",
		     msg.status);

	return ret;
}

/*
 * Write single fuse row to TME
 */
TEE_Result tmel_fuse_write_row(struct tme_fuse_payload *fuse,
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

	msg.status = TME_STATUS_UNKNOWN;
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
		EMSG("TME fuse write row message failed: 0x%x", ret);
		return ret;
	}

	/* Check TME response status */
	ret = tme_status_to_tee_result(msg.status);
	if (ret != TEE_SUCCESS)
		EMSG("TME fuse write row failed, status: 0x%x", msg.status);

	return ret;
}

/*
 * Update TME OEM MRC state vector
 */
TEE_Result tmel_oem_mrc_state_update(uint32_t activate_vector,
				     uint32_t revocate_vector)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct {
		uint32_t activate_vector;
		uint32_t revocate_vector;
		uint32_t status;
	} msg = { 0 };

	msg.activate_vector = activate_vector;
	msg.revocate_vector = revocate_vector;
	msg.status = TME_STATUS_UNKNOWN;

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
		EMSG("TME OEM MRC state update message failed: 0x%x", ret);
		return ret;
	}

	/* Check TME response status */
	ret = tme_status_to_tee_result(msg.status);
	if (ret != TEE_SUCCESS)
		EMSG("TME OEM MRC state update failed, status: 0x%x",
		     msg.status);

	return ret;
}

/*
 * Write multiple fuse rows to TME
 */
TEE_Result tmel_fuse_write_multiple_rows(struct tme_fuse_payload *fuse,
					 uint32_t num_rows)
{
	TEE_Result ret = TEE_ERROR_GENERIC;
	struct {
		uint32_t status;
		uint32_t p_buffer;
		uint32_t buf_len;
	} msg = { 0 };
	paddr_t fuse_paddr = 0;
	size_t buffer_size = 0;

	if (!fuse || !num_rows)
		return TEE_ERROR_BAD_PARAMETERS;

	buffer_size = num_rows * sizeof(struct tme_fuse_payload);
	fuse_paddr = virt_to_phys(fuse);

	if (!fuse_paddr)
		return TEE_ERROR_BAD_PARAMETERS;

	msg.status = TME_STATUS_UNKNOWN;
	msg.p_buffer = (uint32_t)fuse_paddr;
	msg.buf_len = buffer_size;

	/* Send fuse write multiple rows request to TME */
	ret = tmecom_client_send_message
			(TME_MSG_UID_FUSE_WRITE_MULTIPLE_ROW,
			 TME_MSG_UID_FUSE_WRITE_MULTIPLE_ROW_PARAM_ID,
			 true,
			 TMECOM_DEFAULT_TIMEOUT,
			 &msg,
			 sizeof(msg),
			 NULL,
			 NULL,
			 NULL);

	if (ret != TEE_SUCCESS) {
		EMSG("TME fuse write multiple rows message failed: 0x%x", ret);
		return ret;
	}

	/* Check TME response status */
	ret = tme_status_to_tee_result(msg.status);
	if (ret != TEE_SUCCESS)
		EMSG("TME fuse write multiple rows failed, status: 0x%x",
		     msg.status);

	return ret;
}
