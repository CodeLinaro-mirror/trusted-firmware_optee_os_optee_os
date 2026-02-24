/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __GLINK_COM_H
#define __GLINK_COM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <tee_api_types.h>

#define GLINK_CFG_MAX_REMOTE_HOSTS  1
#define GLINK_CFG_MAX_NOTIFY_CBS    1

enum glink_err_type {
	GLINK_STATUS_SUCCESS = 0,
	GLINK_STATUS_INVALID_PARAM = -1,
	GLINK_STATUS_NOT_INIT = -2,
	GLINK_STATUS_OUT_OF_RESOURCES = -3,
	GLINK_STATUS_CH_TX_BUSY = -4,
	GLINK_STATUS_CH_NOT_FULLY_OPENED = -5,
	GLINK_STATUS_FAILURE = -32,
};

enum glink_link_state_type {
	GLINK_LINK_STATE_UP = 0,
	GLINK_LINK_STATE_DOWN = 1,
};

enum glink_channel_event_type {
	GLINK_CONNECTED = 0,
	GLINK_LOCAL_DISCONNECTED = 1,
	GLINK_REMOTE_DISCONNECTED = 2,
};

/* Handle types - using void* directly to avoid typedef warnings */
#define glink_handle_type void *
#define glink_link_handle_type void *

struct glink_link_info {
	const char *remote_ss;
	enum glink_link_state_type link_state;
};

/*
 * Callback function types - using function pointer types
 */
typedef void (*glink_rx_notification_cb)(glink_handle_type handle,
					 const void *priv,
					 const void *pkt_priv,
					 const void *ptr,
					 size_t size,
					 size_t intent_used);

typedef void (*glink_tx_notification_cb)(glink_handle_type handle,
					 const void *priv,
					 const void *pkt_priv,
					 const void *ptr,
					 size_t size);

typedef void (*glink_state_notification_cb)(glink_handle_type handle,
					    const void *priv,
					    enum glink_channel_event_type event);

typedef void (*glink_notify_tx_abort_cb)(glink_handle_type handle,
					 const void *priv,
					 const void *pkt_priv);

typedef void (*glink_link_state_notif_cb)(struct glink_link_info *link_info,
					  void *priv);

struct glink_open_config {
	const char *remote_ss;
	const char *name;
	const void *priv;
	glink_rx_notification_cb notify_rx;
	glink_tx_notification_cb notify_tx_done;
	glink_state_notification_cb notify_state;
	glink_notify_tx_abort_cb notify_tx_abort;
};

struct glink_link_id {
	const char *remote_ss;
	glink_link_state_notif_cb link_notifier;
	glink_link_handle_type handle;
};

#define GLINK_LINK_ID_STRUCT_INIT(link_id) \
	do { \
		(link_id).remote_ss = NULL; \
		(link_id).link_notifier = NULL; \
		(link_id).handle = NULL; \
	} while (0)

/*
 * GLink API Functions
 */

void glink_init(void);

/**
 * glink_open() - Open a GLink channel
 * @cfg_ptr: Channel configuration
 * @handle: Output handle for the channel
 *
 * Return: GLINK_STATUS_SUCCESS on success, error code otherwise
 */
enum glink_err_type glink_open(const struct glink_open_config *cfg_ptr,
			       glink_handle_type *handle);

/**
 * glink_close() - Close a GLink channel
 * @handle: Channel handle
 *
 * Return: GLINK_STATUS_SUCCESS on success, error code otherwise
 */
enum glink_err_type glink_close(glink_handle_type handle);

/**
 * glink_tx() - Transmit data over GLink
 * @handle: Channel handle
 * @pkt_priv: Per-packet private data
 * @data: Data buffer to transmit
 * @size: Size of data buffer
 * @options: Transmission options
 *
 * Return: GLINK_STATUS_SUCCESS on success, error code otherwise
 */
enum glink_err_type glink_tx(glink_handle_type handle,
			     const void *pkt_priv,
			     const void *data,
			     size_t size,
			     uint32_t options);

/**
 * glink_rx_done() - Signal receive completion
 * @handle: Channel handle
 * @ptr: Received data pointer
 * @reuse: Whether to reuse the intent
 *
 * Return: GLINK_STATUS_SUCCESS on success, error code otherwise
 */
enum glink_err_type glink_rx_done(glink_handle_type handle,
				  const void *ptr,
				  bool reuse);

/**
 * glink_register_link_state_cb() - Register link state callback
 * @link_id: Link identifier
 * @priv: Private data for callback
 *
 * Return: GLINK_STATUS_SUCCESS on success, error code otherwise
 */
enum glink_err_type
glink_register_link_state_cb(struct glink_link_id *link_id, void *priv);

/**
 * glink_deregister_link_state_cb() - Deregister link state callback
 * @handle: Link handle
 *
 * Return: GLINK_STATUS_SUCCESS on success, error code otherwise
 */
enum glink_err_type
glink_deregister_link_state_cb(glink_link_handle_type handle);

#endif /* __GLINK_COM_H */
