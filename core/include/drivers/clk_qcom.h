/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef DRIVERS_QCOM_CLK_H
#define DRIVERS_QCOM_CLK_H

#include <tee_api_types.h>

/*
 * Clock group enum for simplified API
 */
enum qcom_clk_group {
	QCOM_CLKS_CDSP,              /* CDSP clocks (GCC + subsystem) */
	QCOM_CLKS_GCC_TURING_AHBS,   /* GCC_TURING_AHBS_CLK_CBCR clock */
	QCOM_CLKS_MAX,               /* MAX count - for future use */
};

/*
 * Enable clocks for specified clock group
 * @group: Clock group enum (QCOM_CLKS_CDSP, etc.)
 * @clk_enable: For QCOM_CLKS_GCC_TURING_AHBS:
 *              0 = disable clock
 *              1 = enable clock
 *              Ignored for other groups.
 * @return: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result qcom_clock_enable(enum qcom_clk_group group, int clk_enable);

#endif /* DRIVERS_QCOM_CLK_H */
