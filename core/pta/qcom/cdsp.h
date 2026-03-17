/* SPDX-License-Identifier: ISC
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef DRIVERS_QCOM_CDSP_H
#define DRIVERS_QCOM_CDSP_H

#include <mm/core_memprot.h>
#include <tee_api_types.h>
#include "pas.h"

/**
 * struct cdsp_hw_resources - CDSP hardware register mappings
 * @mpm2_mpm: MPM2 registers
 * @turing_cc: Turing Clock Controller
 * @turing_tcsr: Turing TCSR
 * @turing_qdsp6ss: Turing QDSP6 Subsystem
 * @tcsr: Top-level TCSR
 * @gcc: Global Clock Controller
 * @initialized: Initialization status
 */
struct cdsp_hw_resources {
	struct io_pa_va mpm2_mpm;
	struct io_pa_va turing_cc;
	struct io_pa_va turing_tcsr;
	struct io_pa_va turing_qdsp6ss;
	struct io_pa_va tcsr;
	struct io_pa_va gcc;
	bool initialized;
};

extern struct cdsp_hw_resources cdsp_hw;
extern struct qcom_pas_data cdsp_dtb_data;

TEE_Result cdsp_start(struct qcom_pas_data *rproc);
TEE_Result cdsp_stop(struct qcom_pas_data *rproc);

#endif /* DRIVERS_QCOM_CDSP_H */
