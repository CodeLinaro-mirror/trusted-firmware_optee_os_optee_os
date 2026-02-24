// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <kernel/panic.h>
#include <kernel/spinlock.h>
#include <string.h>
#include <trace.h>
#include <util.h>

#include "glink_xport.h"
#include "xport_qmp.h"

/*
 * Link Notification Structure
 */
struct glink_link_notify {
	const char *remote_ss;
	glink_link_state_notif_cb link_notifier;
	void *priv;
	enum glink_link_state_type link_state;
};

static unsigned int glink_cmn_spinlock = SPINLOCK_UNLOCK;
static struct glink_transport_if *glink_transports[GLINK_CFG_MAX_REMOTE_HOSTS];
static struct glink_link_notify glink_link_notifies[GLINK_CFG_MAX_NOTIFY_CBS];

/*
 * Finds transport interface by remote subsystem name.
 * Searches through registered transports and returns matching interface.
 */
static struct glink_transport_if *
glinki_find_xport_by_name(const char *remote_ss)
{
	struct glink_transport_if *if_ptr = NULL;
	uint32_t indx;
	uint32_t exceptions;

	if (!remote_ss)
		return NULL;

	exceptions = cpu_spin_lock_xsave(&glink_cmn_spinlock);

	for (indx = 0; indx < GLINK_CFG_MAX_REMOTE_HOSTS &&
	     glink_transports[indx]; indx++) {
		if (!strcmp(glink_transports[indx]->remote_ss, remote_ss)) {
			if_ptr = glink_transports[indx];
			break;
		}
	}

	cpu_spin_unlock_xrestore(&glink_cmn_spinlock, exceptions);

	return if_ptr;
}

/*
 * Notifies registered callbacks about link state changes.
 * Iterates through callback list and invokes matching callbacks.
 */
static void glink_link_state_notify(const char *remote_ss,
				    enum glink_link_state_type link_state)
{
	struct glink_link_notify *link;
	struct glink_link_info link_info;
	uint32_t indx;
	uint32_t exceptions;
	glink_link_state_notif_cb notif_cb;
	void *notif_cb_priv;

	exceptions = cpu_spin_lock_xsave(&glink_cmn_spinlock);

	link_info.remote_ss = remote_ss;
	link_info.link_state = link_state;

	for (indx = 0; indx < GLINK_CFG_MAX_NOTIFY_CBS; indx++) {
		link = &glink_link_notifies[indx];

		if (!link->link_notifier || link->link_state == link_state)
			continue;

		if (!link->remote_ss || !strcmp(link->remote_ss, remote_ss)) {
			notif_cb = link->link_notifier;
			notif_cb_priv = link->priv;

			link->link_state = link_state;

			cpu_spin_unlock_xrestore(&glink_cmn_spinlock,
						 exceptions);
			notif_cb(&link_info, notif_cb_priv);
			exceptions = cpu_spin_lock_xsave(&glink_cmn_spinlock);
		}
	}

	cpu_spin_unlock_xrestore(&glink_cmn_spinlock, exceptions);
}

/*
 * Handles remote channel close and notifies callbacks.
 * Aborts pending transmissions and updates channel state.
 */
static void glink_rx_cmd_remote_close(struct glink_core_xport_ctx *core_priv,
				      struct glink_channel_ctx *ch_ctx)
{
	glink_state_notification_cb state_cb = NULL;
	glink_notify_tx_abort_cb tx_abort_cb = NULL;
	const void *ch_priv = NULL;
	const void *pkt_priv = NULL;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	if (!ch_ctx->if_ptr) {
		cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);
		return;
	}

	ch_priv = ch_ctx->priv;
	state_cb = ch_ctx->notify_state;

	if (ch_ctx->tx_pkt.data) {
		ch_ctx->tx_pkt.data = NULL;
		tx_abort_cb = ch_ctx->notify_tx_abort;
		pkt_priv = ch_ctx->tx_pkt.pkt_priv;
	}

	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);

	if (tx_abort_cb)
		tx_abort_cb(ch_ctx, ch_priv, pkt_priv);

	if (state_cb)
		state_cb(ch_ctx, ch_priv, GLINK_REMOTE_DISCONNECTED);
}

/*
 * Initializes GLink subsystem and transport layers.
 * Sets up QMP transport for inter-processor communication.
 */
void glink_init(void)
{
	static bool glink_init_state;

	if (glink_init_state)
		return;

	glink_init_state = true;

	/* Initialize QMP transport */
	xport_qmp_init();
}

/*
 * Opens GLink channel for communication with remote subsystem.
 * Validates parameters and establishes channel connection.
 */
enum glink_err_type glink_open(const struct glink_open_config *cfg_ptr,
			       glink_handle_type *handle)
{
	struct glink_transport_if *if_ptr;
	struct glink_core_xport_ctx *core_priv;
	struct glink_channel_ctx *ch_ctx;
	enum glink_err_type status = GLINK_STATUS_SUCCESS;
	uint32_t exceptions;

	if (!cfg_ptr || !cfg_ptr->remote_ss || !cfg_ptr->name ||
	    !cfg_ptr->notify_state || !handle)
		return GLINK_STATUS_INVALID_PARAM;

	if_ptr = glinki_find_xport_by_name(cfg_ptr->remote_ss);
	if (!if_ptr)
		return GLINK_STATUS_NOT_INIT;

	core_priv = &if_ptr->core_priv;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	if (core_priv->status != GLINK_XPORT_LINK_UP) {
		status = GLINK_STATUS_NOT_INIT;
		goto unlock_return;
	}

	ch_ctx = &core_priv->ch_ctx;

	/* Support only one channel per transport */
	if (ch_ctx->if_ptr || strcmp(if_ptr->ch_name, cfg_ptr->name)) {
		status = GLINK_STATUS_OUT_OF_RESOURCES;
		goto unlock_return;
	}

	ch_ctx->if_ptr = if_ptr;
	ch_ctx->ch_name = if_ptr->ch_name;
	ch_ctx->remote_ss = if_ptr->remote_ss;
	ch_ctx->notify_rx = cfg_ptr->notify_rx;
	ch_ctx->notify_state = cfg_ptr->notify_state;
	ch_ctx->notify_tx_done = cfg_ptr->notify_tx_done;
	ch_ctx->notify_tx_abort = cfg_ptr->notify_tx_abort;
	ch_ctx->priv = cfg_ptr->priv;

	*handle = ch_ctx;

	status = if_ptr->tx_cmd_ch_open(if_ptr);
	if (status != GLINK_STATUS_SUCCESS) {
		ch_ctx->if_ptr = NULL;
		*handle = NULL;
	}

unlock_return:
	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);
	return status;
}

/*
 * Closes GLink channel and terminates communication.
 * Sends close command to remote subsystem and cleans up resources.
 */
enum glink_err_type glink_close(glink_handle_type handle)
{
	struct glink_channel_ctx *ch_ctx = (struct glink_channel_ctx *)handle;
	struct glink_core_xport_ctx *core_priv;
	struct glink_transport_if *if_ptr;
	enum glink_err_type status;
	uint32_t exceptions;

	if (!ch_ctx || !ch_ctx->if_ptr)
		return GLINK_STATUS_INVALID_PARAM;

	if_ptr = ch_ctx->if_ptr;
	core_priv = &if_ptr->core_priv;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	if (core_priv->status == GLINK_XPORT_LINK_UP) {
		status = if_ptr->tx_cmd_ch_close(if_ptr);
		cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);
	} else {
		cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);
		glink_core_rx_cmd_ch_close_ack(if_ptr);
		status = GLINK_STATUS_SUCCESS;
	}

	return status;
}

/*
 * Transmits data packet over GLink channel.
 * Queues packet for transmission and invokes transport layer.
 */
enum glink_err_type glink_tx(glink_handle_type handle,
			     const void *pkt_priv,
			     const void *data,
			     size_t size,
			     uint32_t options __unused)
{
	struct glink_channel_ctx *ch_ctx = (struct glink_channel_ctx *)handle;
	struct glink_core_xport_ctx *core_priv;
	struct glink_transport_if *if_ptr;
	struct glink_core_tx_pkt *tx_pkt;
	enum glink_err_type status;
	uint32_t exceptions;

	if (!ch_ctx || !ch_ctx->if_ptr || !data || !size)
		return GLINK_STATUS_INVALID_PARAM;

	if_ptr = ch_ctx->if_ptr;
	core_priv = &if_ptr->core_priv;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	tx_pkt = &ch_ctx->tx_pkt;

	/* Support one packet at a time */
	if (tx_pkt->data) {
		status = GLINK_STATUS_CH_TX_BUSY;
	} else {
		tx_pkt->data = data;
		tx_pkt->pkt_priv = pkt_priv;
		tx_pkt->size = size;
		tx_pkt->size_remaining = size;

		status = if_ptr->tx_data(if_ptr, tx_pkt);
		if (status != GLINK_STATUS_SUCCESS)
			tx_pkt->data = NULL;
	}

	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);
	return status;
}

/*
 * Acknowledges completion of received data processing.
 * Notifies transport layer that received buffer can be reused.
 */
enum glink_err_type glink_rx_done(glink_handle_type handle,
				  const void *ptr,
				  bool reuse __unused)
{
	struct glink_channel_ctx *ch_ctx = (struct glink_channel_ctx *)handle;
	struct glink_core_xport_ctx *core_priv;
	struct glink_transport_if *if_ptr;
	enum glink_err_type status;
	uint32_t exceptions;

	if (!ch_ctx || !ch_ctx->if_ptr || !ptr)
		return GLINK_STATUS_INVALID_PARAM;

	if_ptr = ch_ctx->if_ptr;
	core_priv = &if_ptr->core_priv;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	if (ch_ctx->rx_pkt.data == ptr) {
		ch_ctx->rx_pkt.data = NULL;
		status = if_ptr->tx_cmd_local_rx_done(if_ptr, ptr);
	} else {
		status = GLINK_STATUS_INVALID_PARAM;
	}

	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);
	return status;
}

/*
 * Registers callback for link state change notifications.
 * Adds callback to notification list and handles immediate notifications.
 */
enum glink_err_type
glink_register_link_state_cb(struct glink_link_id *link_id, void *priv)
{
	struct glink_transport_if *if_ptr;
	struct glink_link_notify *link;
	struct glink_link_info link_info;
	uint32_t indx;
	uint32_t cmn_lock_exceptions;
	uint32_t priv_lock_exceptions;

	if (!link_id || !link_id->link_notifier)
		return GLINK_STATUS_INVALID_PARAM;

	link_id->handle = NULL;

	cmn_lock_exceptions = cpu_spin_lock_xsave(&glink_cmn_spinlock);

	for (indx = 0; indx < GLINK_CFG_MAX_NOTIFY_CBS; indx++) {
		link = &glink_link_notifies[indx];
		if (!link->link_notifier) {
			link->remote_ss = link_id->remote_ss;
			link->link_notifier = link_id->link_notifier;
			link->priv = priv;
			link->link_state = GLINK_LINK_STATE_DOWN;
			link_id->handle = link;
			break;
		}
	}

	if (!link_id->handle) {
		cpu_spin_unlock_xrestore(&glink_cmn_spinlock,
					 cmn_lock_exceptions);
		return GLINK_STATUS_OUT_OF_RESOURCES;
	}

	/* Notify if link is already up */
	for (indx = 0; indx < GLINK_CFG_MAX_REMOTE_HOSTS; indx++) {
		if_ptr = glink_transports[indx];

		if (!if_ptr)
			continue;

		priv_lock_exceptions =
			cpu_spin_lock_xsave(&if_ptr->core_priv.cs);
		if (if_ptr->core_priv.status != GLINK_XPORT_LINK_UP) {
			cpu_spin_unlock_xrestore(&if_ptr->core_priv.cs,
						 priv_lock_exceptions);
			continue;
		}
		cpu_spin_unlock_xrestore(&if_ptr->core_priv.cs,
					 priv_lock_exceptions);

		if (!link_id->remote_ss ||
		    !strcmp(link_id->remote_ss, if_ptr->remote_ss)) {
			link_info.remote_ss = if_ptr->remote_ss;
			link_info.link_state = GLINK_LINK_STATE_UP;
			link->link_state = GLINK_LINK_STATE_UP;

			cpu_spin_unlock_xrestore(&glink_cmn_spinlock,
						 cmn_lock_exceptions);
			link_id->link_notifier(&link_info, priv);
			cmn_lock_exceptions =
				cpu_spin_lock_xsave(&glink_cmn_spinlock);
		}
	}

	cpu_spin_unlock_xrestore(&glink_cmn_spinlock, cmn_lock_exceptions);
	return GLINK_STATUS_SUCCESS;
}

/*
 * Deregisters link state change notification callback.
 * Removes callback from notification list to stop receiving updates.
 */
enum glink_err_type
glink_deregister_link_state_cb(glink_link_handle_type handle)
{
	struct glink_link_notify *link = (struct glink_link_notify *)handle;
	uint32_t exceptions;

	if (!link || !link->link_notifier)
		return GLINK_STATUS_INVALID_PARAM;

	exceptions = cpu_spin_lock_xsave(&glink_cmn_spinlock);
	link->link_notifier = NULL;
	cpu_spin_unlock_xrestore(&glink_cmn_spinlock, exceptions);

	return GLINK_STATUS_SUCCESS;
}

/*
 * Core Functions - called by transport layer
 */

/*
 * Registers transport interface with GLink core.
 * Adds transport to global list and initializes core context.
 */
void glink_core_register_transport(struct glink_transport_if *if_ptr)
{
	uint32_t indx;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&glink_cmn_spinlock);

	for (indx = 0; indx < GLINK_CFG_MAX_REMOTE_HOSTS; indx++) {
		if (!glink_transports[indx]) {
			glink_transports[indx] = if_ptr;
			if_ptr->core_priv.cs = SPINLOCK_UNLOCK;
			break;
		}
	}

	if (indx == GLINK_CFG_MAX_REMOTE_HOSTS)
		panic("GLink: Transport registration failed");

	cpu_spin_unlock_xrestore(&glink_cmn_spinlock, exceptions);
}

/*
 * Processes link up command from transport layer.
 * Updates transport status and notifies registered callbacks.
 */
void glink_core_rx_cmd_link_up(struct glink_transport_if *if_ptr)
{
	struct glink_core_xport_ctx *core_priv = &if_ptr->core_priv;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);
	core_priv->status = GLINK_XPORT_LINK_UP;
	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);

	glink_link_state_notify(if_ptr->remote_ss, GLINK_LINK_STATE_UP);
}

/*
 * Processes link down command from transport layer.
 * Updates transport status and handles channel cleanup.
 */
void glink_core_rx_cmd_link_down(struct glink_transport_if *if_ptr)
{
	struct glink_core_xport_ctx *core_priv = &if_ptr->core_priv;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);
	core_priv->status = GLINK_XPORT_LINK_DOWN;
	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);

	glink_link_state_notify(if_ptr->remote_ss, GLINK_LINK_STATE_DOWN);
	glink_rx_cmd_remote_close(core_priv, &core_priv->ch_ctx);
}

/*
 * Processes remote channel open command from transport.
 * Notifies channel state callback about connection establishment.
 */
void glink_core_rx_cmd_remote_open(struct glink_transport_if *if_ptr)
{
	struct glink_core_xport_ctx *core_priv = &if_ptr->core_priv;
	struct glink_channel_ctx *ch_ctx = &core_priv->ch_ctx;
	glink_state_notification_cb state_cb = NULL;
	const void *ch_priv = NULL;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	if (ch_ctx->if_ptr) {
		state_cb = ch_ctx->notify_state;
		ch_priv = ch_ctx->priv;
	}

	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);

	if (state_cb)
		state_cb(ch_ctx, ch_priv, GLINK_CONNECTED);
}

/*
 * Processes remote channel close command from transport.
 * Handles channel disconnection and resource cleanup.
 */
void glink_core_rx_cmd_remote_close(struct glink_transport_if *if_ptr)
{
	glink_rx_cmd_remote_close(&if_ptr->core_priv,
				  &if_ptr->core_priv.ch_ctx);
}

/*
 * Processes channel close acknowledgment from transport.
 * Completes local channel closure and notifies callbacks.
 */
void glink_core_rx_cmd_ch_close_ack(struct glink_transport_if *if_ptr)
{
	struct glink_core_xport_ctx *core_priv = &if_ptr->core_priv;
	struct glink_channel_ctx *ch_ctx = &core_priv->ch_ctx;
	glink_state_notification_cb state_cb = NULL;
	const void *ch_priv = NULL;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	if (ch_ctx->if_ptr) {
		state_cb = ch_ctx->notify_state;
		ch_priv = ch_ctx->priv;
		ch_ctx->if_ptr = NULL;
	}

	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);

	if (state_cb)
		state_cb(ch_ctx, ch_priv, GLINK_LOCAL_DISCONNECTED);
}

/*
 * Processes transmission completion notification from transport.
 * Invokes TX done callback to notify upper layer of completion.
 */
void glink_core_rx_cmd_tx_done(struct glink_transport_if *if_ptr,
			       struct glink_core_tx_pkt *tx_pkt)
{
	struct glink_core_xport_ctx *core_priv = &if_ptr->core_priv;
	struct glink_channel_ctx *ch_ctx = &core_priv->ch_ctx;
	glink_tx_notification_cb tx_done_cb = NULL;
	const void *ch_priv = NULL;
	const void *pkt_priv;
	const void *data;
	uint32_t size;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	if (!ch_ctx->if_ptr || !tx_pkt->data || tx_pkt->size_remaining) {
		cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);
		return;
	}

	ch_priv = ch_ctx->priv;
	tx_done_cb = ch_ctx->notify_tx_done;
	pkt_priv = tx_pkt->pkt_priv;
	data = tx_pkt->data;
	size = tx_pkt->size;

	tx_pkt->data = NULL;

	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);

	if (tx_done_cb)
		tx_done_cb(ch_ctx, ch_priv, pkt_priv, data, size);
}

/*
 * Processes received data from transport layer.
 * Invokes RX callback to deliver data to upper layer.
 */
void glink_core_rx_cmd_data(struct glink_transport_if *if_ptr,
			    void *ptr,
			    uint32_t size)
{
	struct glink_core_xport_ctx *core_priv = &if_ptr->core_priv;
	struct glink_channel_ctx *ch_ctx = &core_priv->ch_ctx;
	glink_rx_notification_cb rx_cb = NULL;
	uint32_t exceptions;

	exceptions = cpu_spin_lock_xsave(&core_priv->cs);

	if (!ch_ctx->if_ptr || !ptr || !size) {
		cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);
		return;
	}

	rx_cb = ch_ctx->notify_rx;
	ch_ctx->rx_pkt.data = ptr;
	ch_ctx->rx_pkt.size = size;

	cpu_spin_unlock_xrestore(&core_priv->cs, exceptions);

	if (rx_cb)
		rx_cb(ch_ctx, ch_ctx->priv, &ch_ctx->rx_pkt, ptr, size, size);
}
