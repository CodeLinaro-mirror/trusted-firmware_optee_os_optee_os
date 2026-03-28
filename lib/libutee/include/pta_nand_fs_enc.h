/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __PTA_NAND_FS_ENC_H
#define __PTA_NAND_FS_ENC_H

#include <util.h>

/*
 * NAND Filesystem Encryption PTA
 * Provides AES encryption/decryption using HUK-derived keys
 * UUID: 1e64fceb-66b5-41d5-ba0a-1f487a59a8a0
 */

#define PTA_NAND_FS_ENC_UUID \
	{ 0x1e64fceb, 0x66b5, 0x41d5, \
		{ 0xba, 0x0a, 0x1f, 0x48, 0x7a, 0x59, 0xa8, 0xa0 } }

#define PTA_AES_DECRYPT		0
#define PTA_AES_ENCRYPT		1

#define PTA_AES_MODE_ECB	0
#define PTA_AES_MODE_CBC	1
#define PTA_AES_MODE_CTR	2

#define PTA_AES_KEY_SIZE_128	16
#define PTA_AES_KEY_SIZE_256	32

#define PTA_AES_BLOCK_SIZE	16
#define PTA_AES_IV_SIZE		16

#define PTA_AES_MAX_DATA_SIZE	4096

/*
 * Derive and set AES key from HUK
 * [in]  params[0].memref.buffer     Key size (size_t)
 * [in]  params[0].memref.size       sizeof(size_t)
 */
#define PTA_NAND_FS_ENC_SET_KEY		0x0

/*
 * Encrypt or decrypt data using derived key
 * [in]  params[0].value.a           Direction (PTA_AES_ENCRYPT/DECRYPT)
 * [in]  params[0].value.b           Mode (ECB/CBC/CTR)
 * [in]  params[1].memref.buffer     IV buffer (NULL for ECB)
 * [in]  params[1].memref.size       IV size (16 for CBC/CTR, 0 for ECB)
 * [in]  params[2].memref.buffer     Input data
 * [in]  params[2].memref.size       Input size (multiple of 16)
 * [out] params[3].memref.buffer     Output data
 * [out] params[3].memref.size       Output size
 */
#define PTA_NAND_FS_ENC_CRYPT		0x1

/* Clear derived key from memory */
#define PTA_NAND_FS_ENC_CLEAR_KEY	0x2

#endif /* __PTA_NAND_FS_ENC_H */
