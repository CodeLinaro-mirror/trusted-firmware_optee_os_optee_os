// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/qcom/qfprom.h>
#include <initcall.h>
#include <kernel/cache_helpers.h>
#include <qfprom_target.h>
#include <tee_api.h>
#include <tmecom_client.h>
#include <tmel_auth/tmel_auth.h>
#include <tmel_fuse/tmefuse_client.h>
#include <trace.h>

/* Read a row from QFPROM */
TEE_Result qfprom_read_row(uint32_t row_address, bool corrected,
			   uint32_t *lsb_val, uint32_t *msb_val)
{
	TEE_Result res = TEE_SUCCESS;
	struct tme_fuse_payload *fuse = NULL;
	void *fuse_orig_addr = NULL;
	paddr_t fuse_phys_addr = 0;

	if (!lsb_val || !msb_val)
		return TEE_ERROR_BAD_PARAMETERS;

	fuse = tmecom_client_malloc_coherent(sizeof(*fuse), 64,
					     &fuse_orig_addr, &fuse_phys_addr);
	if (!fuse) {
		EMSG("Failed to allocate coherent memory for fuse read");
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	fuse->fuse_addr = row_address;
	if (corrected)
		fuse->fuse_addr |= QFPROM_CORR_ADDR_OFFSET;

	res = tmel_qfprom_fuselist_read(fuse, sizeof(*fuse));
	if (res != TEE_SUCCESS) {
		EMSG("Failed to read fuse at address 0x%08x: %#"PRIx32,
		     row_address, res);
	} else {
		*lsb_val = fuse->lsb_val;
		*msb_val = fuse->msb_val;
	}

	tmecom_client_free_coherent(fuse, fuse_orig_addr, sizeof(*fuse));

	return res;
}

/* Write a row to QFPROM */
TEE_Result qfprom_write_row(uint32_t row_address, uint32_t lsb_val,
			    uint32_t msb_val)
{
	TEE_Result res = TEE_SUCCESS;
	struct tme_fuse_payload *fuse = NULL;
	void *fuse_orig_addr = NULL;
	paddr_t fuse_phys_addr = 0;

	fuse = tmecom_client_malloc_coherent(sizeof(*fuse), 64,
					     &fuse_orig_addr, &fuse_phys_addr);
	if (!fuse) {
		EMSG("Failed to allocate coherent memory for fuse write");
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	fuse->fuse_addr = row_address;
	fuse->lsb_val = lsb_val;
	fuse->msb_val = msb_val;

	res = tmel_qfprom_fuselist_write(fuse, sizeof(*fuse));

	/* Invalidate cache to ensure we read fresh response from TME */
	dcache_inv_range(fuse, sizeof(*fuse));

	if (res != TEE_SUCCESS)
		EMSG("Failed to write fuse at address 0x%08x: %#"PRIx32,
		     row_address, res);

	tmecom_client_free_coherent(fuse, fuse_orig_addr, sizeof(*fuse));

	return res;
}

/* Write TME OEM MRC state vector fuse */
TEE_Result qfprom_write_tme_oem_mrc(uint32_t *row_data)
{
	TEE_Result res = TEE_SUCCESS;
	uint32_t act_vector = 0;
	uint32_t rev_vector = 0;

	if (!row_data) {
		EMSG("Invalid row_data pointer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	/* Extract activation and revocation vectors from row data */
	act_vector = row_data[0] & TME_OEM_MRC_ACTIVATION_VECTOR_MASK;
	rev_vector = (row_data[0] >> TME_OEM_MRC_ACTIVATION_VECTOR_BITS) &
		     TME_OEM_MRC_REVOCATION_VECTOR_MASK;

	/* Send MRC state update request to TME via TME fuse client */
	res = tmel_qfprom_oem_mrc_state_update(act_vector, rev_vector);
	if (res != TEE_SUCCESS) {
		EMSG("TME MRC state update failed: %#"PRIx32, res);
		return res;
	}

	return TEE_SUCCESS;
}
