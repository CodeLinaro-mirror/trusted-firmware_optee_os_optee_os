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

/* Authenticate firmware using TME-L */
TEE_Result tmel_secure_auth_v2(struct tmel_sec_auth_v2_req *params);

#endif /* __TMEL_AUTH_H */
