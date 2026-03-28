/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __TMECOM_CLIENT_H
#define __TMECOM_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <tee_api_types.h>

/*
 * Default timeout for blocking calls (1 second in microseconds)
 */
#define TMECOM_DEFAULT_TIMEOUT 1000000

/* Generic TME-L buffer descriptor */
struct tmel_buf_desc {
	uint32_t buf;
	uint32_t buf_len;
};

/*
 * Tmecom Response Codes
 */
enum tmecom_response {
	/* Success codes (0 to 127) */
	TMECOM_RSP_SUCCESS = 0,
	TMECOM_RSP_SUCCESS_MAXVAL = 127,

	/* Failure codes (-32 to -128) */
	TMECOM_RSP_FAILURE = -32,
	TMECOM_RSP_FAILURE_BAD_ADDR = -33,
	TMECOM_RSP_FAILURE_INVALID_ARGS = -34,
	TMECOM_RSP_FAILURE_CHANNEL_ERR = -35,
	TMECOM_RSP_FAILURE_LINK_ERR = -36,
	TMECOM_RSP_FAILURE_TX_ERR = -37,
	TMECOM_RSP_FAILURE_RX_ERR = -38,
	TMECOM_RSP_FAILURE_TIMEOUT = -39,
	TMECOM_RSP_FAILURE_BUSY = -40,
	TMECOM_RSP_FAILURE_INVALID_MESSAGE = -41,
	TMECOM_SERVICE_API_RETURNED_ERR = -42,
	TMECOM_RSP_FAILURE_NOT_SUPPORTED = -43,
	TMECOM_RSP_FAILURE_MAX = -128,
};

/*
 * Tmecom Callback Data
 */
struct tmecom_callback_data {
	uint32_t tme_msg_uid;
	void *generic_payload;
	uint32_t generic_payload_len;
	void *user_data;
};

/*
 * RX Notification Callback Type
 */
typedef void (*tmecom_notify_rx_callback_t)(enum tmecom_response response,
					    struct tmecom_callback_data *data);

/*
 * Public API Functions
 */

/*
 * Initialize TME COM session
 * Returns: TEE_Result
 * tme_err: Output parameter for TME COM error code
 */
TEE_Result tmecom_client_session_start(void);

/*
 * Terminate TME COM session
 * Returns: TEE_Result
 * tme_err: Output parameter for TME COM error code
 */
TEE_Result tmecom_client_session_end(void);

/*
 * Send message to TME
 * Returns: TEE_Result
 * tme_err: Output parameter for TME COM error code (or remote response)
 */
TEE_Result tmecom_client_send_message(uint32_t tme_msg_uid,
				      uint32_t tme_msg_param_id,
				      bool is_blocking,
				      uint32_t timeout,
				      void *generic_payload,
				      uint32_t generic_payload_len,
				      tmecom_notify_rx_callback_t cb_api,
				      void *user_data,
				      enum tmecom_response *tme_err);

/*
 * Allocate cache-coherent buffer
 * Returns: Coherent virtual address, or NULL on failure
 * orig_addr: Output parameter for original virtual address (for cleanup)
 * phys_addr: Output parameter for physical/DMA address
 */
void *tmecom_client_malloc_coherent(size_t size, size_t alignment,
				    void **orig_addr, paddr_t *phys_addr);

/*
 * Free cache-coherent buffer
 */
void tmecom_client_free_coherent(void *coherent_addr, void *orig_addr,
				 size_t size);

/*
 * Convert TMECOM response code to TEE Result
 */
TEE_Result tmecom_to_tee_result(enum tmecom_response status);

#endif /* __TMECOM_CLIENT_H */
