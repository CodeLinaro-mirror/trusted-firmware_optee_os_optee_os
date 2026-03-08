// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include "pas.h"
#include "cdsp.h"
#include <drivers/clk.h>
#include <drivers/clk_qcom.h>
#include <initcall.h>
#include <io.h>
#include <kernel/delay.h>
#include <kernel/pseudo_ta.h>
#include <kernel/user_ta.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <pta_qcom_pas.h>
#include <string.h>
#include <tee_api_defines.h>
#include <tee_api_types.h>
#include <trace.h>

#define PTA_NAME	"pta.qcom.pas"
#define CLOCK_DELAY_MS	10

static enum {
	PAS_STATE_OFFLINE = QCOM_PAS_STATE_OFFLINE,
	PAS_STATE_LOADED = QCOM_PAS_STATE_LOADED,
	PAS_STATE_RUNNING = QCOM_PAS_STATE_RUNNING,
} cdsp_state = PAS_STATE_OFFLINE;

struct qcom_pas_data cdsp_dtb_data = {
	.pas_id = PAS_ID_CDSP_DTB,
};

struct qcom_pas_data cdsp_data = {
	.pas_id = PAS_ID_CDSP,
	.clk_group = QCOM_CLKS_CDSP,
};

static TEE_Result
qcom_pas_is_supported(uint32_t pt,
		      TEE_Param params[TEE_NUM_PARAMS] __unused)
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	uint32_t pas_id;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	pas_id = params[0].value.a;

	if (pas_id != PAS_ID_CDSP && pas_id != PAS_ID_CDSP_DTB) {
		EMSG("Unsupported PAS ID: %u", pas_id);
		return TEE_ERROR_NOT_SUPPORTED;
	}

	return TEE_SUCCESS;
}

static TEE_Result qcom_pas_capabilities(uint32_t pt,
					TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_OUTPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	params[1].value.a = 0;

	return TEE_SUCCESS;
}

static TEE_Result qcom_pas_init_image(uint32_t pt,
				      TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	uint32_t pas_id = params[0].value.a;
	bool is_dtb;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	is_dtb = (pas_id == PAS_ID_CDSP_DTB);

	if (is_dtb)
		return TEE_SUCCESS;

	if (cdsp_state != PAS_STATE_OFFLINE)
		return TEE_ERROR_BAD_STATE;

	cdsp_state = PAS_STATE_LOADED;

	return TEE_SUCCESS;
}

static TEE_Result
qcom_pas_mem_setup(uint32_t pt, TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	uint32_t pas_id = params[0].value.a;
	struct qcom_pas_data *data = NULL;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	switch (pas_id) {
	case PAS_ID_CDSP_DTB:
		data = &cdsp_dtb_data;
		break;
	case PAS_ID_CDSP:
		data = &cdsp_data;
		break;
	default:
		EMSG("Unsupported PAS ID: %u", pas_id);
		return TEE_ERROR_NOT_SUPPORTED;
	}

	/* Configure firmware parameters using the selected data structure */
	data->fw_size = params[0].value.b;
	data->fw_base = params[1].value.a;
	data->fw_base |= ((paddr_t)params[1].value.b << 32);

	return TEE_SUCCESS;
}

static TEE_Result
qcom_pas_get_resource_table(uint32_t pt,
			    TEE_Param params[TEE_NUM_PARAMS] __unused)
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_MEMREF_INOUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	return TEE_ERROR_NOT_IMPLEMENTED;
}

static TEE_Result
qcom_pas_auth_and_reset(uint32_t pt, TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_NONE);
	uint32_t pas_id = params[0].value.a;
	TEE_Result res;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (pas_id == PAS_ID_CDSP) {
		if (cdsp_state == PAS_STATE_RUNNING)
			return TEE_SUCCESS;

		if (cdsp_state != PAS_STATE_LOADED)
			return TEE_ERROR_BAD_STATE;
	}

	switch (pas_id) {
	case PAS_ID_CDSP_DTB:
		return TEE_SUCCESS;

	case PAS_ID_CDSP:
		if (!cdsp_data.fw_base || !cdsp_dtb_data.fw_base)
			return TEE_ERROR_BAD_STATE;

		res = qcom_clock_enable(cdsp_data.clk_group, 1);
		if (res != TEE_SUCCESS)
			return res;

		mdelay(CLOCK_DELAY_MS);

		res = cdsp_start(&cdsp_data);
		if (res != TEE_SUCCESS)
			return res;

		mdelay(CLOCK_DELAY_MS);

		cdsp_state = PAS_STATE_RUNNING;
		return TEE_SUCCESS;

	default:
		return TEE_ERROR_NOT_SUPPORTED;
	}
}

static TEE_Result
qcom_pas_set_remote_state(uint32_t pt __unused,
			  TEE_Param params[TEE_NUM_PARAMS] __unused)
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	return TEE_ERROR_NOT_IMPLEMENTED;
}

static TEE_Result qcom_pas_shutdown(uint32_t pt,
				    TEE_Param params[TEE_NUM_PARAMS] __unused)
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	uint32_t pas_id = params[0].value.a;
	TEE_Result res;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	switch (pas_id) {
	case PAS_ID_CDSP_DTB:
		return TEE_SUCCESS;

	case PAS_ID_CDSP:
		if (cdsp_state == PAS_STATE_OFFLINE)
			return TEE_SUCCESS;

		res = cdsp_stop(&cdsp_data);
		if (res)
			return res;

		cdsp_state = PAS_STATE_OFFLINE;
		cdsp_dtb_data.fw_base = 0;
		cdsp_dtb_data.fw_size = 0;

		return TEE_SUCCESS;

	default:
		return TEE_ERROR_NOT_SUPPORTED;
	}
}

static TEE_Result
pta_qcom_pas_invoke_command(void *session __unused, uint32_t cmd_id,
			    uint32_t param_types,
			    TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd_id) {
	case PTA_QCOM_PAS_IS_SUPPORTED:
		return qcom_pas_is_supported(param_types, params);
	case PTA_QCOM_PAS_CAPABILITIES:
		return qcom_pas_capabilities(param_types, params);
	case PTA_QCOM_PAS_INIT_IMAGE:
		return qcom_pas_init_image(param_types, params);
	case PTA_QCOM_PAS_MEM_SETUP:
		return qcom_pas_mem_setup(param_types, params);
	case PTA_QCOM_PAS_GET_RESOURCE_TABLE:
		return qcom_pas_get_resource_table(param_types, params);
	case PTA_QCOM_PAS_AUTH_AND_RESET:
		return qcom_pas_auth_and_reset(param_types, params);
	case PTA_QCOM_PAS_SET_REMOTE_STATE:
		return qcom_pas_set_remote_state(param_types, params);
	case PTA_QCOM_PAS_SHUTDOWN:
		return qcom_pas_shutdown(param_types, params);
	default:
		EMSG("Unknown command ID: %u", cmd_id);
		return TEE_ERROR_NOT_IMPLEMENTED;
	}
}

/*
 * Pseudo Trusted Application entry points
 */
static TEE_Result
pta_qcom_pas_open_session(uint32_t pt __unused,
			  TEE_Param params[TEE_NUM_PARAMS] __unused,
			  void **sess_ctx __unused)
{
	uint32_t login = to_ta_session(ts_get_current_session())->clnt_id.login;

	if (login == TEE_LOGIN_REE_KERNEL)
		return TEE_SUCCESS;

	return TEE_ERROR_ACCESS_DENIED;
}

pseudo_ta_register(.uuid = PTA_QCOM_PAS_UUID, .name = PTA_NAME,
		   .flags = PTA_DEFAULT_FLAGS | TA_FLAG_DEVICE_ENUM,
		   .invoke_command_entry_point = pta_qcom_pas_invoke_command,
		   .open_session_entry_point = pta_qcom_pas_open_session);
