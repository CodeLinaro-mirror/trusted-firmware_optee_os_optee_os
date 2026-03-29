// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __PTA_EMMC_ICE_H
#define __PTA_EMMC_ICE_H

#include <util.h>

/*
 * eMMC ICE PTA - eMMC Filesystem Encryption using Inline Crypto Engine
 * Provides hardware key configuration for ICE-accelerated storage encryption
 * UUID: {29e87b9e-012a-4878-a1e1-a1b90a215b16}
 */
#define PTA_EMMC_ICE_UUID \
	{ 0x29e87b9e, 0x012a, 0x4878, \
		{ 0xa1, 0xe1, 0xa1, 0xb9, 0x0a, 0x21, 0x5b, 0x16 } }

/*
 * Generate and configure hardware key using CRBK or OEM Product Seed
 * [in]  params[0].memref.buffer     ice_context_config structure
 * [in]  params[0].memref.size       Size of structure
 * [in]  params[1].memref.buffer     SW context 1 (optional)
 * [in]  params[1].memref.size       SW context 1 size
 * [in]  params[2].memref.buffer     SW context 2 (optional, for XTS)
 * [in]  params[2].memref.size       SW context 2 size
 */
#define PTA_CMD_ICE_GENERATE_HW_KEY   0

/*
 * Configure ICE hardware key context
 * [in]  params[0].memref.buffer     ice_set_key_config_context_req structure
 * [in]  params[0].memref.size       Size of structure
 */
#define PTA_CMD_ICE_SET_HW_KEY_CTX    1

#endif /* __PTA_EMMC_ICE_H */
