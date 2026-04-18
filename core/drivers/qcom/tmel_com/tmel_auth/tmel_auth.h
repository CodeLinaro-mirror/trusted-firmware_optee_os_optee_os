/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __TMEL_AUTH_H
#define __TMEL_AUTH_H

#include <tee_api_types.h>
#include <types_ext.h>
#include <tmecom_client.h>

/* Authentication flags */
#define TMEL_AUTH_FLAG_NS_INTEGRITY		1

/* Software ID for sec.elf */
#define SECELF_SW_ID				0x2B

/* TME-L status codes */
#define TMEL_STATUS_SUCCESS			0

/* TME-L V1 Authentication Request Structure */
struct tmel_sec_auth_v1_req {
	/* Input parameters */
	uint32_t sw_id;
	struct tmel_buf_desc elf_buf;
	struct tmel_buf_desc region_list;
	uint32_t relocate;

	/* Output parameters */
	uint32_t first_seg_addr;
	uint32_t first_seg_len;
	uint32_t entry_addr;
	uint32_t extended_error;
	uint32_t status;
} __packed;

/* TME-L secure authentication request structure (64 bytes) */
struct tmel_sec_auth_v2_req {
	/* Input parameters (36 bytes) */
	uint32_t sw_id;
	struct tmel_buf_desc elf_buf;
	struct tmel_buf_desc region_list;
	uint32_t relocate;
	uint32_t nsIntegrityCheck:1;
	uint32_t reservedBits:31;
	struct tmel_buf_desc reservedBuf;

	/* Output parameters (24 bytes) */
	uint32_t first_seg_addr;
	uint32_t first_seg_len;
	uint32_t entry_addr;
	uint32_t extended_error;
	uint32_t status;
	uint32_t keyHandle;
} __packed;

/* Unified authentication parameters structure */
struct tmel_sec_auth_params {
	/* Common input parameters */
	uint32_t sw_id;
	struct tmel_buf_desc elf_buf;
	struct tmel_buf_desc region_list;
	uint32_t relocate;

	/* V2-specific parameters (ignored for V1) */
	uint32_t nsIntegrityCheck;
	struct tmel_buf_desc reservedBuf;

	/* Output parameters (common) */
	uint32_t first_seg_addr;
	uint32_t first_seg_len;
	uint32_t entry_addr;
	uint32_t extended_error;
	uint32_t status;

	/* V2-specific output */
	uint32_t keyHandle;
} __packed;

/* Authenticate firmware using TME-L V1 */
TEE_Result tmel_secure_auth_v1(struct tmel_sec_auth_v1_req *params);

/* Authenticate firmware using TME-L V2 */
TEE_Result tmel_secure_auth_v2(struct tmel_sec_auth_v2_req *params);

/* Unified authentication function - handles both V1 and V2 automatically */
TEE_Result tmel_secure_auth(struct tmel_sec_auth_params *params);

/* Initialize TME auth version from device tree (call at boot time) */
TEE_Result tmel_auth_init(void);

#endif /* __TMEL_AUTH_H */
