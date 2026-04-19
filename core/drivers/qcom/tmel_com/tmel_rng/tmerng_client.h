/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __TMERNG_H
#define __TMERNG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <tee_api_types.h>

/*
 * TME RNG Client Interface
 * Provides RNG (Pseudo-Random Number Generation) API for OP-TEE
 */

/* Maximum random bytes that can be requested in a single call */
#define TME_RNG_MAX_LENGTH	512

/* Sequencer Status Response */
struct tme_sequencer_status {
	uint32_t tme_error_status;
	uint32_t seq_error_status;
	uint32_t seq_kp_error_status0;
	uint32_t seq_kp_error_status1;
	uint32_t seq_rsp_status;
};

/* TME RNG Get Message Structure */
struct tme_rng_get_msg {
	struct {
		uint32_t length;
	} input;
	struct {
		uint32_t rng_buf_pdata;
		uint32_t rng_buf_length;
		uint32_t rng_buf_length_used;
		uint32_t status;
		struct tme_sequencer_status seq_status;
	} output;
};

/*
 * TMEL-backed implementation of hw_get_random_bytes.
 * Handles TMEL bypass detection and IMEM fallback when needed.
 * Delegates to TMEL IPC for normal operation.
 *
 * @buf: Buffer to store random bytes
 * @len: Number of random bytes to generate
 *
 * Returns: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result tme_hw_get_random_bytes(void *buf, size_t len);

#endif /* __TMERNG_H */
