/* SPDX-License-Identifier: ISC
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef DRIVERS_QCOM_CDSP_RPROC_H
#define DRIVERS_QCOM_CDSP_RPROC_H

#include <mm/core_memprot.h>
#include <tee_api_types.h>
#include <types_ext.h>
#include <drivers/clk_qcom.h>
#include <platform_config.h>

/**
 * struct qcom_pas_data - Simplified remote processor instance context
 *
 * @pas_id: PAS (Peripheral Authentication Service) ID
 * @base: Base memory region (reserved for future use)
 * @size: Size of base memory region (reserved for future use)
 * @fw_base: Physical base address of firmware
 * @fw_size: Size of firmware image
 * @clk_group: Clock group identifier for this processor
 *
 * Note: All register regions (mpm2_mpm, turing_cc, gcc, etc.) are mapped
 * at boot time via qcom_cdsp_init() and stored in the global cdsp_hw
 * structure.
 */
struct qcom_pas_data {
	uint32_t pas_id;
	struct io_pa_va base;
	size_t size;
	paddr_t fw_base;
	size_t fw_size;
	enum qcom_clk_group clk_group;
};

/* Global CDSP data structure - defined in pta_qcom_pas.c */
extern struct qcom_pas_data cdsp_data;

#endif /* DRIVERS_QCOM_CDSP_RPROC_H */
