// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/gic.h>
#include <kernel/boot.h>
#include <kernel/misc.h>
#include <platform_config.h>
#include <drivers/qcom_diag_log.h>
#include <drivers/qcom_geni_uart.h>
#include <console.h>
#include <tee_api_types.h>
#include <trace.h>

#ifdef CFG_QCOM_GENI_UART
static struct qcom_geni_uart_data console_data;
#endif

#ifdef CFG_ARM_GICV3
register_phys_mem(MEM_AREA_IO_SEC, GIC_BASE, GIC_SIZE);
#endif

#ifdef CFG_QCOM_DIAG_LOG
register_phys_mem(MEM_AREA_IO_SEC, DIAG_BASE, DIAG_SIZE);
register_phys_mem(MEM_AREA_IO_SEC, DIAG_LOG_START_INFO & ~SMALL_PAGE_MASK,
		  SMALL_PAGE_SIZE);
register_phys_mem(MEM_AREA_IO_SEC, TCSR_BOOT_MISC_DETECT & ~SMALL_PAGE_MASK,
		  SMALL_PAGE_SIZE);
#endif

#ifdef CFG_QCOM_GENI_UART
register_phys_mem_pgdir(MEM_AREA_IO_NSEC, QUP_UART_BASE, QUP_UART_REG_SIZE);
#endif

void plat_trace_ext_puts(const char *str __maybe_unused)
{
#ifdef CFG_QCOM_DIAG_LOG
	qcom_diag_log_puts(str);
#endif
}

void plat_console_init(void)
{
#ifdef CFG_QCOM_GENI_UART
	TEE_Result res = qcom_geni_uart_init(&console_data);

	if (res == TEE_SUCCESS) {
		register_serial_console(&console_data.chip);
		IMSG("QCOM GENI UART: Console initialization successful");
	} else {
		EMSG("QCOM GENI UART: Console init failed (0x%x)", res);
	}
#endif
}

/**
 * get_core_pos_mpidr() - Get core position from MPIDR
 * @mpidr: Multiprocessor Affinity Register value
 *
 * Generic QCOM platform implementation supporting both symmetric and
 * asymmetric cluster configurations using CFG_CORE_CLUSTER_SHIFT.
 *
 * Formula: CorePos = (ClusterId << CFG_CORE_CLUSTER_SHIFT) + CoreId
 *
 * Return: Core position within the SoC
 */
size_t get_core_pos_mpidr(uint32_t mpidr)
{
	uint32_t cluster_id, core_id;

	/* Handle MT bit: shift MPIDR if multithreading is disabled */
	if (!(mpidr & MPIDR_MT_MASK))
		mpidr <<= MPIDR_AFFINITY_BITS;

	cluster_id = (mpidr >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK;
	core_id = (mpidr >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK;

	return (cluster_id << CFG_CORE_CLUSTER_SHIFT) + core_id;
}

#if defined(CFG_ARM_GICV3)
void boot_primary_init_intc(void)
{
	gic_init_v3(0, GICD_BASE, GICR_BASE);
}

void boot_secondary_init_intc(void)
{
	gic_init_per_cpu();
}
#endif /* CFG_ARM_GICV3 */
