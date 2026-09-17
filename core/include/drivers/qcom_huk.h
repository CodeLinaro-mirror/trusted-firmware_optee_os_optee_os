/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __DRIVERS_QCOM_HUK_H
#define __DRIVERS_QCOM_HUK_H

#include <stdint.h>

/*
 * Parameters for huk_subkey_derive(HUK_SUBKEY_AES_HUK).
 * Context and label buffers must remain valid until the call returns.
 */
struct aes_huk_derive_req {
	uint32_t seed_type;
	uint32_t sw_context_len;
	const uint8_t *sw_context;
	uint32_t salt_label_len;
	const uint8_t *salt_label;
};

#endif /* __DRIVERS_QCOM_HUK_H */
