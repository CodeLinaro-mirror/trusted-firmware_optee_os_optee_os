// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <assert.h>
#include <drivers/clk.h>
#include <drivers/clk_dt.h>
#include <drivers/clk_qcom.h>
#include "cdsp.h"
#include <initcall.h>
#include <io.h>
#include <kernel/panic.h>
#include <kernel/delay.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <platform_config.h>
#include <string.h>
#include <trace.h>
#include <util.h>

#define MPM2_MPM_BASE			0x004A0000
#define MPM2_MPM_SIZE			0x10000
#define CDSP_TURING_CC_BASE		0x26008000
#define CDSP_TURING_CC_SIZE		0x14000
#define CDSP_TURING_TCSR_BASE		0x26080000
#define CDSP_TURING_TCSR_SIZE		0x1F000
#define CDSP_TURING_QDSP6SS_BASE	0x26300000
#define CDSP_TURING_QDSP6SS_SIZE	0x100000

static_assert(TCSR_BASE != 0);
#define CDSP_TCSR_BASE			(TCSR_BASE + 0x66000)
#define CDSP_TCSR_SIZE			0x1000

register_phys_mem_pgdir(MEM_AREA_IO_SEC, MPM2_MPM_BASE,
			MPM2_MPM_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, CDSP_TURING_CC_BASE,
			CDSP_TURING_CC_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, CDSP_TURING_TCSR_BASE,
			CDSP_TURING_TCSR_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, CDSP_TURING_QDSP6SS_BASE,
			CDSP_TURING_QDSP6SS_SIZE);

/* GCC reset control registers */
#define GCC_RST_CTL_COMPUTESS_RESTART		0x47020
#define GCC_TURINGSS_BCR			0x18000

/* CDSP Q6SS boot registers */
#define CDSP_Q6SS_BOOT_CORE_START_REG    0x400
#define CDSP_Q6SS_BOOT_CMD_REG           0x404
#define CDSP_Q6SS_BOOT_STATUS_REG        0x408
#define CDSP_Q6SS_BOOT_AUTO_BREAK_EN_REG 0x410
#define CDSP_Q6SS_BOOT_RESUME_CMD_REG    0x414

/* Turing TCSR registers */
#define TURING_TCSR_RST_EVB_SEL_REG     0x1000
#define TURING_TCSR_RST_EVB_ADDR_REG    0x1004

/* TCSR control registers */
#define TCSR_TURING_HALTREQ             0x0000
#define TCSR_TURING_HALTACK             0x0004
#define TCSR_TURING_MASTER_IDLE         0x0008
#define TCSR_TURING_PWR_ON              0x000C
#define TCSR_TURING_IL1_MASTER_IDLE     0x0010
#define TCSR_SPARE_REG0                 0x1959000

/* MPM2 control register */
#define MPM2_MPM_CONTROL_CNTCR          0x1000

/* Boot status values */
#define Q6SS_BOOT_STATUS_STAGE1         0x80000000
#define Q6SS_BOOT_STATUS_STAGE2         0x1

#define BOOT_TIMEOUT_MS                 1000

/* Boot control values */
#define CDSP_BOOT_CORE_START_ENABLE      0x1
#define CDSP_BOOT_AUTO_BREAK_ENABLE      0x1
#define CDSP_BOOT_CMD_START              0x1
#define CDSP_BOOT_CMD_STOP               0x0
#define CDSP_BOOT_CORE_START_DISABLE     0x0

#define CDSP_RST_EVB_SEL_ENABLE          0x1
#define TURING_CC_ALT_RESET_CTL          0x10034

/* DTB configuration registers */
#define CDSP_DTB_CONFIG_0_REG            0x60
#define CDSP_DTB_CONFIG_1_REG            0x64
#define CDSP_DTB_CONFIG_2_REG            0x68
#define CDSP_DTB_CONFIG_3_REG            0x6C
#define CDSP_DTB_CONFIG_4_REG            0x70
#define CDSP_DTB_CONFIG_5_REG            0x74
#define CDSP_Q6SS_BOOT_CTRL_REG          0x18

/* Q6 Debug control value */
#define CDSP_Q6SS_BREAK_AT_START         0x20000001

/* DTB configuration values */
#define CDSP_DTB_CHIP_FAMILY_ID          0x00B40303
#define CDSP_DTB_VERSION                 0x00000100

struct cdsp_hw_resources cdsp_hw = {
	.initialized = false,
};

TEE_Result cdsp_start(struct qcom_pas_data *rproc)
{
	uint32_t boot_status;
	uint64_t timeout_expire;
	uint32_t boot_vector_evb;
	int debug_q6 = 0;
	vaddr_t spare_reg_va;

	if (!cdsp_hw.initialized) {
		EMSG("CDSP hardware resources not initialized");
		return TEE_ERROR_BAD_STATE;
	}

	boot_vector_evb = (uint32_t)(rproc->fw_base >> 4);
	spare_reg_va = (vaddr_t)phys_to_virt(TCSR_SPARE_REG0, MEM_AREA_IO_SEC,
					     sizeof(uint32_t));
	if (spare_reg_va)
		debug_q6 = io_read32(spare_reg_va) & 0x1;
	else
		EMSG("TCSR_SPARE_REG0 mapping failed");


	/* Configure boot vector */
	io_write32(cdsp_hw.turing_tcsr.va + TURING_TCSR_RST_EVB_SEL_REG,
		   CDSP_RST_EVB_SEL_ENABLE);
	io_write32(cdsp_hw.turing_tcsr.va + TURING_TCSR_RST_EVB_ADDR_REG,
		   boot_vector_evb);

	/* Start boot sequence */
	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_Q6SS_BOOT_CORE_START_REG,
		   CDSP_BOOT_CORE_START_ENABLE);
	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_Q6SS_BOOT_AUTO_BREAK_EN_REG,
		   CDSP_BOOT_AUTO_BREAK_ENABLE);

	/* Configure DTB */
	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_DTB_CONFIG_0_REG,
		   (uint32_t)cdsp_dtb_data.fw_base);
	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_DTB_CONFIG_1_REG,
		   (uint32_t)(cdsp_dtb_data.fw_base >> 32));
	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_DTB_CONFIG_2_REG,
		   CDSP_DTB_CHIP_FAMILY_ID);
	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_DTB_CONFIG_3_REG,
		   CDSP_DTB_VERSION);
	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_DTB_CONFIG_5_REG,
		   cdsp_dtb_data.fw_size);
	if (debug_q6)
		io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_Q6SS_BOOT_CTRL_REG,
			   CDSP_Q6SS_BREAK_AT_START);

	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_Q6SS_BOOT_CMD_REG,
		   CDSP_BOOT_CMD_START);

	dsb();
	isb();

	/* Wait for stage 1 boot */
	timeout_expire = timeout_init_us(BOOT_TIMEOUT_MS * 1000);
	do {
		boot_status = io_read32(cdsp_hw.turing_qdsp6ss.va +
					CDSP_Q6SS_BOOT_STATUS_REG);
		if (boot_status == Q6SS_BOOT_STATUS_STAGE1)
			break;
	} while (!timeout_elapsed(timeout_expire));

	if (boot_status != Q6SS_BOOT_STATUS_STAGE1) {
		EMSG("CDSP stage 1 boot timeout - status: 0x%08x", boot_status);
		return TEE_ERROR_TIMEOUT;
	}

	io_write32(cdsp_hw.mpm2_mpm.va + MPM2_MPM_CONTROL_CNTCR, BIT(0));
	io_write32(cdsp_hw.turing_qdsp6ss.va + CDSP_Q6SS_BOOT_RESUME_CMD_REG,
		   BIT(0));

	/* Wait for stage 2 boot */
	timeout_expire = timeout_init_us(BOOT_TIMEOUT_MS * 1000);
	do {
		boot_status = io_read32(cdsp_hw.turing_qdsp6ss.va +
					CDSP_Q6SS_BOOT_STATUS_REG);
		if (boot_status == Q6SS_BOOT_STATUS_STAGE2)
			break;
	} while (!timeout_elapsed(timeout_expire));

	if (boot_status != Q6SS_BOOT_STATUS_STAGE2) {
		EMSG("CDSP stage 2 boot timeout - status: 0x%08x", boot_status);
		return TEE_ERROR_TIMEOUT;
	}

	DMSG("CDSP boot completed successfully");
	dsb();
	return TEE_SUCCESS;
}

/* Initialize CDSP hardware using statically registered register regions */
static TEE_Result qcom_cdsp_init(void)
{
	if (cdsp_hw.initialized)
		return TEE_SUCCESS;

	cdsp_hw.mpm2_mpm.va = (vaddr_t)phys_to_virt(MPM2_MPM_BASE,
						     MEM_AREA_IO_SEC,
						     MPM2_MPM_SIZE);
	if (!cdsp_hw.mpm2_mpm.va) {
		EMSG("Failed to map mpm2_mpm");
		return TEE_ERROR_GENERIC;
	}

	cdsp_hw.turing_cc.va = (vaddr_t)phys_to_virt(CDSP_TURING_CC_BASE,
						      MEM_AREA_IO_SEC,
						      CDSP_TURING_CC_SIZE);
	if (!cdsp_hw.turing_cc.va) {
		EMSG("Failed to map turing_cc");
		return TEE_ERROR_GENERIC;
	}

	cdsp_hw.turing_tcsr.va = (vaddr_t)phys_to_virt(CDSP_TURING_TCSR_BASE,
							MEM_AREA_IO_SEC,
							CDSP_TURING_TCSR_SIZE);
	if (!cdsp_hw.turing_tcsr.va) {
		EMSG("Failed to map turing_tcsr");
		return TEE_ERROR_GENERIC;
	}

	cdsp_hw.turing_qdsp6ss.va = (vaddr_t)phys_to_virt(CDSP_TURING_QDSP6SS_BASE,
							   MEM_AREA_IO_SEC,
							   CDSP_TURING_QDSP6SS_SIZE);
	if (!cdsp_hw.turing_qdsp6ss.va) {
		EMSG("Failed to map turing_qdsp6ss");
		return TEE_ERROR_GENERIC;
	}

	cdsp_hw.tcsr.va = (vaddr_t)phys_to_virt(CDSP_TCSR_BASE,
						 MEM_AREA_IO_SEC,
						 CDSP_TCSR_SIZE);
	if (!cdsp_hw.tcsr.va) {
		EMSG("Failed to map tcsr");
		return TEE_ERROR_GENERIC;
	}

	cdsp_hw.gcc.va = (vaddr_t)phys_to_virt(GCC_BASE,
						MEM_AREA_IO_SEC,
						GCC_SIZE);
	if (!cdsp_hw.gcc.va) {
		EMSG("Failed to map gcc");
		return TEE_ERROR_GENERIC;
	}

	cdsp_hw.initialized = true;
	return TEE_SUCCESS;
}

TEE_Result cdsp_stop(struct qcom_pas_data *rproc __unused)
{
	TEE_Result res;
	uint32_t val, val2;
	uint64_t timeout_expire;

	if (!cdsp_hw.initialized) {
		EMSG("CDSP hardware resources not initialized");
		return TEE_ERROR_BAD_STATE;
	}

	/* Wait for power-on status */
	timeout_expire = timeout_init_us(1000 * 1000);
	do {
		val = io_read32(cdsp_hw.tcsr.va + TCSR_TURING_PWR_ON);
		if (val & 0x1)
			break;
	} while (!timeout_elapsed(timeout_expire));

	/* Request halt */
	io_write32(cdsp_hw.tcsr.va + TCSR_TURING_HALTREQ, 0x1);

	/* Wait for halt acknowledgment */
	timeout_expire = timeout_init_us(5000 * 1000);
	do {
		val = io_read32(cdsp_hw.tcsr.va + TCSR_TURING_HALTACK);
		if (val & 0x1)
			break;
	} while (!timeout_elapsed(timeout_expire));

	if (!(val & 0x1)) {
		EMSG("Failed to receive halt acknowledgment");
		return TEE_ERROR_TIMEOUT;
	}

	io_write32(cdsp_hw.turing_cc.va + TURING_CC_ALT_RESET_CTL, 0x1);

	res = qcom_clock_enable(QCOM_CLKS_GCC_TURING_AHBS, 0);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to disable GCC_TURING_AHBS clock: %#"PRIx32, res);
		return res;
	}

	/* Trigger subsystem restart and reset */
	val = io_read32(cdsp_hw.gcc.va + GCC_RST_CTL_COMPUTESS_RESTART);
	io_write32(cdsp_hw.gcc.va + GCC_RST_CTL_COMPUTESS_RESTART, val | 0x1);
	io_write32(cdsp_hw.gcc.va + GCC_TURINGSS_BCR, 0x1);
	mdelay(1000);

	/* Clear halt and deassert reset */
	io_write32(cdsp_hw.tcsr.va + TCSR_TURING_HALTREQ, 0x0);
	io_write32(cdsp_hw.gcc.va + GCC_TURINGSS_BCR, 0x0);
	val = io_read32(cdsp_hw.gcc.va + GCC_RST_CTL_COMPUTESS_RESTART);
	io_write32(cdsp_hw.gcc.va + GCC_RST_CTL_COMPUTESS_RESTART, val & ~0x1);

	/* Wait for idle */
	mdelay(1000);
	timeout_expire = timeout_init_us(5000 * 1000);
	do {
		val = io_read32(cdsp_hw.tcsr.va + TCSR_TURING_MASTER_IDLE);
		val2 = io_read32(cdsp_hw.tcsr.va + TCSR_TURING_IL1_MASTER_IDLE);
		if ((val & 0x1) && (val2 & 0x1))
			break;
	} while (!timeout_elapsed(timeout_expire));

	res = qcom_clock_enable(QCOM_CLKS_GCC_TURING_AHBS, 1);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to re-enable GCC_TURING_AHBS clock: %#"PRIx32,
		     res);
		return res;
	}

	io_write32(cdsp_hw.turing_cc.va + TURING_CC_ALT_RESET_CTL, 0x0);

	return TEE_SUCCESS;
}

/* Late init ensures MMU and clock framework are ready */
driver_init_late(qcom_cdsp_init);
