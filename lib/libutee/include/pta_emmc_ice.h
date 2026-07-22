// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __PTA_EMMC_ICE_H
#define __PTA_EMMC_ICE_H

/*
 * eMMC ICE PTA - eMMC Filesystem Encryption using Inline Crypto Engine
 * Provides hardware key configuration for ICE-accelerated storage encryption
 * UUID: {29e87b9e-012a-4878-a1e1-a1b90a215b16}
 */
#define PTA_EMMC_ICE_UUID \
	{ 0x29e87b9e, 0x012a, 0x4878, \
		{ 0xa1, 0xe1, 0xa1, 0xb9, 0x0a, 0x21, 0x5b, 0x16 } }

/*
 * Invalidate ICE key slot - overwrite key registers with random data
 * [in]  params[0].value.a           Key slot index (0..ICE_MAX_KEY_IDX-1)
 */
#define PTA_CMD_ICE_INVALIDATE_KEY    0

/*
 * Set ICE key slot with raw key material and full configuration
 * [in]  params[0].value.a           Key slot index (0..ICE_MAX_KEY_IDX-1)
 * [in]  params[0].value.b           Capability index (ice_capability_index_type)
 * [in]  params[1].value.a           Data unit size (ice_data_unit_type)
 * [in]  params[2].memref.buffer     Key data: key bytes followed by salt bytes
 *                                   XTS-128: 16-byte key + 16-byte salt = 32 bytes
 *                                   XTS-256: 32-byte key + 32-byte salt = 64 bytes
 *                                   CBC-128: 16-byte key
 *                                   CBC-256: 32-byte key
 * [in]  params[2].memref.size       Total key data size
 */
#define PTA_CMD_ICE_SET_CONFIG_KEY    1

/*
 * Command IDs 2-6 are reserved for the upstream HWKM v1 ICE commands
 * (PTA_CMD_ICE_GENERATE_KEY, PTA_CMD_ICE_IMPORT_KEY, PTA_CMD_ICE_EXPORT_KEY,
 * PTA_CMD_ICE_GET_RAW_SECRET, PTA_CMD_ICE_HAS_WRAPPED_KEY_SUPPORT; see
 * upstream lib/libutee/include/pta_qcom_ice.h). They are intentionally left
 * unassigned here so the downstream HW-key commands below keep non-
 * conflicting IDs if upstreamed later.
 */

/*
 * Generate and configure hardware key using CRBK or OEM Product Seed
 * [in]  params[0].memref.buffer     ice_context_config structure
 * [in]  params[0].memref.size       Size of structure
 * [in]  params[1].memref.buffer     SW context 1 (optional)
 * [in]  params[1].memref.size       SW context 1 size
 * [in]  params[2].memref.buffer     SW context 2 (optional, for XTS)
 * [in]  params[2].memref.size       SW context 2 size
 */
#define PTA_CMD_ICE_GENERATE_HW_KEY   7

/*
 * Configure ICE hardware key context
 * [in]  params[0].memref.buffer     ice_set_key_config_context_req structure
 * [in]  params[0].memref.size       Size of structure
 */
#define PTA_CMD_ICE_SET_HW_KEY_CTX    8

#endif /* __PTA_EMMC_ICE_H */
