/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __TMEFUSE_CLIENT_H
#define __TMEFUSE_CLIENT_H

#include <stddef.h>
#include <stdint.h>
#include <tee_api_types.h>

/*
 * Buffer parameter with input/output capability
 */
struct tme_msg_param_buf_inout {
	uint32_t p_buffer;
	uint32_t buf_len;
	uint32_t buf_out_len;
} __packed;

/*
 * Fuse address and value structure
 */
struct tme_fuse_payload {
	uint32_t fuse_addr;
	uint32_t lsb_val;
	uint32_t msb_val;
} __packed;

/*
 * IPC message structure for multiple fuse read
 */
struct tme_fuse_read_multiple_msg {
	uint32_t status;
	struct tme_msg_param_buf_inout fuse_read_data;
} __packed;

/*
 * Read multiple fuse rows from TME
 *
 * The fuse buffer should contain an array of struct tme_fuse_payload
 * structures with fuse_addr fields populated. TME will fill in the lsb_val and
 * msb_val fields with the fuse values.
 *
 * @fuse: Pointer to fuse payload array
 * @size: Size of fuse payload buffer in bytes
 * Return: TEE_SUCCESS on success, TEE_ERROR_* on failure
 */
TEE_Result tme_ipc_fuselist_read(struct tme_fuse_payload *fuse, size_t size);

#endif /* __TMEFUSE_CLIENT_H */
