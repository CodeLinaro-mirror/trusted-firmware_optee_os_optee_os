// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/qcom/qfprom.h>
#include <kernel/cache_helpers.h>
#include <kernel/pseudo_ta.h>
#include <malloc.h>
#include <mm/core_memprot.h>
#include <pta_qcom_qfprom.h>
#include <string.h>
#include <tee/tee_svc.h>
#include <tee/uuid.h>
#include <trace.h>

#define PTA_NAME "qfprom.qcom.pta"

static TEE_Result
pta_qfprom_blow_secelf(uint32_t param_types,
		       TEE_Param params[TEE_NUM_PARAMS])
{
	TEE_Result res = TEE_SUCCESS;
	void *elf_metadata_buf = NULL;
	uint32_t elf_metadata_size = 0;
	void *internal_elf_buf = NULL;
	void *regions_buf = NULL;
	uint32_t regions_size = 0;
	struct mem_region_64 *internal_regions = NULL;
	uint32_t region_count = 0;
	uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_VALUE_OUTPUT,
				TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) {
		EMSG("Bad parameter types - expected 0x%08x, got 0x%08x",
		     exp_param_types, param_types);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	elf_metadata_buf = params[0].memref.buffer;
	elf_metadata_size = params[0].memref.size;
	if (!elf_metadata_buf || elf_metadata_size == 0) {
		EMSG("Invalid ELF metadata buffer");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	dcache_inv_range(elf_metadata_buf, elf_metadata_size);

	internal_elf_buf = malloc(elf_metadata_size);
	if (!internal_elf_buf) {
		EMSG("Failed to allocate internal ELF buffer");
		return TEE_ERROR_OUT_OF_MEMORY;
	}

	memcpy(internal_elf_buf, elf_metadata_buf, elf_metadata_size);
	dcache_clean_range(internal_elf_buf, elf_metadata_size);

	regions_buf = params[1].memref.buffer;
	regions_size = params[1].memref.size;
	if (!regions_buf || regions_size == 0) {
		EMSG("No region list provided");
		res = TEE_ERROR_BAD_PARAMETERS;
		goto cleanup_elf;
	}

	if (regions_size < sizeof(struct mem_region_64) ||
	    (regions_size % sizeof(struct mem_region_64)) != 0) {
		EMSG("Invalid regions size: %u", regions_size);
		res = TEE_ERROR_BAD_PARAMETERS;
		goto cleanup_elf;
	}

	dcache_inv_range(regions_buf, regions_size);

	internal_regions = malloc(regions_size);
	if (!internal_regions) {
		EMSG("Failed to allocate internal regions buffer");
		res = TEE_ERROR_OUT_OF_MEMORY;
		goto cleanup_elf;
	}

	memcpy(internal_regions, regions_buf, regions_size);
	dcache_clean_range(internal_regions, regions_size);

	region_count = regions_size / sizeof(struct mem_region_64);

	res = prov_qfprom_fuses_with_auth((vaddr_t)internal_elf_buf,
					  elf_metadata_size,
					  internal_regions, region_count);

	if (res != TEE_SUCCESS)
		EMSG("Fuse provisioning FAILED: 0x%08x", res);

	params[2].value.a = res;

	free(internal_regions);
cleanup_elf:
	free(internal_elf_buf);

	return res;
}

TEE_Result pta_qfprom_invoke_command(void *sess_ctx __unused,
				     uint32_t cmd_id,
				     uint32_t param_types,
				     TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd_id) {
	case PTA_CMD_QFPROM_BLOW_SECELF:
		return pta_qfprom_blow_secelf(param_types, params);
	default:
		return TEE_ERROR_NOT_IMPLEMENTED;
	}
}

pseudo_ta_register(.uuid = PTA_QFPROM_UUID, .name = PTA_NAME,
		   .flags = PTA_DEFAULT_FLAGS,
		   .invoke_command_entry_point = pta_qfprom_invoke_command);
