/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __PTA_QCOM_QFPROM_H
#define __PTA_QCOM_QFPROM_H

#include <tee_api_types.h>
#include <util.h>

/* UUID of the QFPROM PTA */
#define PTA_QFPROM_UUID { 0x7e5e8c5d, 0x5375, 0x4ab0, \
		{ 0x8b, 0x11, 0x95, 0x14, 0x96, 0x65, 0xdc, 0x88 } }

/*
 * Blow fuses from SECELF buffer.
 *
 * [in]  params[0].memref: ELF metadata buffer
 * [in]  params[1].memref: Regions array buffer
 * [out] params[2].value.a: Operation status (TEE_Result)
 */
#define PTA_CMD_QFPROM_BLOW_SECELF	0

TEE_Result pta_qfprom_invoke_command(void *sess_ctx, uint32_t cmd_id,
				     uint32_t param_types,
				     TEE_Param params[TEE_NUM_PARAMS]);

#endif /* __PTA_QCOM_QFPROM_H */
