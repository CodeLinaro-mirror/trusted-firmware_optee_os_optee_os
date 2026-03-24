// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/qcom/qfprom.h>
#include <kernel/pseudo_ta.h>
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
	vaddr_t elf_vaddr = 0;
	uint32_t elf_metadata_size = 0;
	struct TmeRegion_t *regions = NULL;
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

	elf_vaddr = (vaddr_t)params[0].memref.buffer;
	elf_metadata_size = params[0].memref.size;
	if (!elf_vaddr || elf_metadata_size == 0) {
		EMSG("Invalid ELF metadata buffer - vaddr=0x%lx, size=%u",
		     elf_vaddr, elf_metadata_size);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (params[1].memref.buffer && params[1].memref.size > 0) {
		if (params[1].memref.size >= sizeof(struct TmeRegion_t) &&
		    (params[1].memref.size % sizeof(struct TmeRegion_t)) == 0) {
			regions = (struct TmeRegion_t *)params[1].memref.buffer;
			region_count = params[1].memref.size /
				       sizeof(struct TmeRegion_t);
		}
	} else {
		EMSG("No region list provided");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	res = prov_qfprom_fuses_with_auth(elf_vaddr, elf_metadata_size,
					  regions, region_count);

	if (res != TEE_SUCCESS)
		EMSG("Fuse provisioning FAILED: 0x%08x", res);

	params[2].value.a = res;

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
