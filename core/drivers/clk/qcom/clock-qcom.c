// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <assert.h>
#include <drivers/clk.h>
#include <drivers/clk_qcom.h>
#include <io.h>
#include <kernel/delay.h>
#include <kernel/panic.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <platform_config.h>
#include <string.h>
#include <trace.h>
#include <util.h>

#ifdef CFG_QCOM_PAS_PTA
/* Include CDSP header for cdsp_hw structure */
#include "cdsp.h"
#endif

#if defined(PLATFORM_FLAVOR_ipq96xx) || defined(PLATFORM_FLAVOR_ipq96xx_lm)
/* GCC (Global Clock Controller) Register Offsets for CDSP */
/* GCC Clock Branch Control Register offsets */
#define GCC_Q6SS_TSCTR_1TO2_CLK_CBCR		0x1801c
#define GCC_TURING_EPCB_RX_CLK_CBCR		0x18020
#define GCC_TURING_Q6_AXIM_DIV_CLK_CBCR		0x18024
#define GCC_TURING_PCLK_DBG_CLK_CBCR		0x18028
#define GCC_TURING_Q6SS_TRIG_CLK_CBCR		0x1802c
#define GCC_TURING_CXO_CLK_CBCR			0x18030
#define GCC_TURING_ATBM_AT_CLK_CBCR		0x18034
#define GCC_TURING_AHBS_CLK_CBCR		0x18038
#define GCC_TURING_GEMNOC_CLK_CBCR		0x1907c
#define GCC_CNOC_TURING_AHBS_CLK_CBCR		0x310c0

/* Clock Branch Control Register bit fields */
#define CBCR_CLK_ENABLE				BIT(0)

/* Clock control bit definitions */
#define CLK_ENABLE_BIT                  BIT(0)
#define CLK_HW_CTL_BIT                  BIT(1)
#define CLK_SLEEP_SHIFT                 4
#define CLK_WAKEUP_SHIFT                8

#define CLK_SLEEP_CYCLES                0x2
#define CLK_WAKEUP_CYCLES               0x2

#define CLK_ENABLE_HW_CTL               (CLK_ENABLE_BIT | CLK_HW_CTL_BIT)
#define CLK_ENABLE_WITH_TIMING \
	(CLK_ENABLE_BIT | CLK_HW_CTL_BIT | \
	(CLK_SLEEP_CYCLES << CLK_SLEEP_SHIFT) | \
	(CLK_WAKEUP_CYCLES << CLK_WAKEUP_SHIFT))

#define CDSPAUX_BRIDGE_DELAY_CYCLES      0x9

/* TURING_CC (Turing Clock Controller) register offsets */
#define TURING_CC_Q6SS_Q6_AXIM_CBCR             0x424
#define TURING_CC_CENG_CDSP_CBCR                0x288
#define TURING_CC_CENG_PROC_CBCR                0x294
#define TURING_CC_CDSPNOC_CBCR                  0x254
#define TURING_CC_Q6SS_AHBS_AON_CBCR            0x408
#define TURING_CC_CENG_CDSP_AO_CBCR             0x28C
#define TURING_CC_CENG_AHBS_CBCR                0x240
#define TURING_CC_CDSPNOC_AHBS_CBCR             0x244
#define TURING_CC_CDSPAUX_XO_CBCR               0x250
#define TURING_CC_Q6SS_AHBS_AON_MXC_CBCR        0x40C
#define TURING_CC_XO_DIV_CBCR                   0x050
#define TURING_CC_CDSPNOC_APB_CBCR              0x268
#define TURING_CC_Q6SS_AHBM_AON_CBCR            0x404
#define TURING_CC_ALT_RESET_AON_CBCR            0x410
#define TURING_CC_DEBUG_CBCR                    0xFEC
#define TURING_CC_PLL_TEST_CBCR                 0xFFC

/* QDSP6SS (QDSP6 Subsystem) register offsets */
#define QDSP6SS_CORE_CBCR                       0x41040
#define QDSP6SS_SLPGEN_CBCR                     0x41060
#define QDSP6SS_L2MEM_SLPGEN_CBCR               0x41080
#define QDSP6SS_L2VTCM_SLPGEN_CBCR              0x410C0
#define QDSP6SS_MON_CBCR                        0x41100
#define QDSP6SS_DEBUG_CBCR                      0x411C8

/* CDSPAUX register offsets */
#define CDSPAUX_BUS_BRIDGE_HALT                 0x10010
#else
#error "Platform specific clock offsets not defined..."
#endif

/*
 * Enable GCC clocks by writing directly to registers
 */
static TEE_Result cdsp_gcc_clk_enable(void)
{
#ifdef CFG_QCOM_PAS_PTA
	/* Enable GCC clocks by writing to CBCR registers */
	io_write32(cdsp_hw.gcc.va + GCC_Q6SS_TSCTR_1TO2_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_TURING_EPCB_RX_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_TURING_Q6_AXIM_DIV_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_TURING_PCLK_DBG_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_TURING_Q6SS_TRIG_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_TURING_CXO_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_TURING_ATBM_AT_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_TURING_AHBS_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_TURING_GEMNOC_CLK_CBCR,
		   CBCR_CLK_ENABLE);
	io_write32(cdsp_hw.gcc.va + GCC_CNOC_TURING_AHBS_CLK_CBCR,
		   CBCR_CLK_ENABLE);
#endif /* CFG_QCOM_PAS_PTA */

	return TEE_SUCCESS;
}

/*
 * Control GCC_TURING_AHBS_CLK_CBCR clock (enable/disable)
 * @is_enable: false = disable clock, true = enable clock
 * @return: TEE_SUCCESS on success, error code otherwise
 */
static TEE_Result cdsp_gcc_ahbs_clk_control(bool is_enable)
{
#ifdef CFG_QCOM_PAS_PTA
	if (is_enable) {
		/* Enable GCC clocks */
		io_write32(cdsp_hw.gcc.va + GCC_TURING_AHBS_CLK_CBCR,
			   CBCR_CLK_ENABLE);
	} else {
		/* Disable GCC clocks */
		io_write32(cdsp_hw.gcc.va + GCC_TURING_AHBS_CLK_CBCR, 0);
	}
#endif /* CFG_QCOM_PAS_PTA */

	return TEE_SUCCESS;
}

/*
 * Enable CDSP subsystem clocks (TURING_CC and QDSP6SS)
 * Uses boot-time mappings from cdsp_hw global structure
 */
static TEE_Result cdsp_cc_enable(void)
{
#ifdef CFG_QCOM_PAS_PTA
	/* Configure TURING CC clocks with hardware control */
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_Q6SS_Q6_AXIM_CBCR,
		   CLK_ENABLE_HW_CTL);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_CENG_CDSP_CBCR,
		   CLK_ENABLE_HW_CTL);

	/* Configure clocks with sleep/wakeup timing */
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_CENG_PROC_CBCR,
		   CLK_ENABLE_WITH_TIMING);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_CDSPNOC_CBCR,
		   CLK_ENABLE_WITH_TIMING);

	/* Enable TURING CC clocks */
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_Q6SS_AHBS_AON_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_CENG_CDSP_AO_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_CENG_AHBS_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_CDSPNOC_AHBS_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_CDSPAUX_XO_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_Q6SS_AHBS_AON_MXC_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_XO_DIV_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_CDSPNOC_APB_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_Q6SS_AHBM_AON_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_ALT_RESET_AON_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_DEBUG_CBCR,
		   CLK_ENABLE_BIT);
	io_write32(cdsp_hw.turing_cc.va + TURING_CC_PLL_TEST_CBCR,
		   CLK_ENABLE_BIT);

	/* Configure QDSP6SS clocks with hardware control */
	io_write32(cdsp_hw.turing_qdsp6ss.va + QDSP6SS_CORE_CBCR,
		   CLK_ENABLE_HW_CTL);
	io_write32(cdsp_hw.turing_qdsp6ss.va + QDSP6SS_SLPGEN_CBCR,
		   CLK_ENABLE_HW_CTL);
	io_write32(cdsp_hw.turing_qdsp6ss.va + QDSP6SS_L2MEM_SLPGEN_CBCR,
		   CLK_ENABLE_HW_CTL);
	io_write32(cdsp_hw.turing_qdsp6ss.va + QDSP6SS_L2VTCM_SLPGEN_CBCR,
		   CLK_ENABLE_HW_CTL);
	io_write32(cdsp_hw.turing_qdsp6ss.va + QDSP6SS_MON_CBCR,
		   CLK_ENABLE_HW_CTL);

	/* Enable QDSP6SS debug clock */
	io_write32(cdsp_hw.turing_qdsp6ss.va + QDSP6SS_DEBUG_CBCR,
		   CLK_ENABLE_BIT);

	/* Configure CDSPAUX bridge delay */
	io_write32(cdsp_hw.turing_cc.va + CDSPAUX_BUS_BRIDGE_HALT,
		   CDSPAUX_BRIDGE_DELAY_CYCLES);
#endif /* CFG_QCOM_PAS_PTA */

	return TEE_SUCCESS;
}

/*
 * Enable clocks for specified clock group
 * @group: Clock group enum (QCOM_CLKS_CDSP, etc.)
 * @clk_enable: For QCOM_CLKS_GCC_TURING_AHBS: 1 to enable clock,
 *              0 to disable clock. Ignored for other groups.
 * @return: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result qcom_clock_enable(enum qcom_clk_group group, int clk_enable)
{
	TEE_Result res;

	switch (group) {
	case QCOM_CLKS_CDSP:
		/* Enable GCC clocks first */
		res = cdsp_gcc_clk_enable();
		if (res)
			goto timeout;

		mdelay(10);  /* Wait for GCC clocks to stabilize */

		/* Enable CDSP subsystem clocks */
		res = cdsp_cc_enable();
		if (res)
			goto timeout;
		break;

	case QCOM_CLKS_GCC_TURING_AHBS:
		/*
		 * Control GCC_TURING_AHBS_CLK_CBCR clock based on
		 * clk_enable parameter
		 */
		res = cdsp_gcc_ahbs_clk_control(clk_enable);
		if (res)
			goto timeout;
		break;

	default:
		EMSG("Unsupported clock group %d\n", group);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	return TEE_SUCCESS;

timeout:
	EMSG("Clock operation failed for group %d\n", group);
	return res;
}
