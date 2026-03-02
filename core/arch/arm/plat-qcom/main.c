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
#include <io.h>
#include <initcall.h>
#ifndef CFG_ARM_GICV3
#include <gicv2_config.h>
#endif

#ifdef CFG_QCOM_GENI_UART
static struct qcom_geni_uart_data console_data;
#endif

register_phys_mem_pgdir(MEM_AREA_IO_SEC, GIC_BASE, GIC_SIZE);

#ifdef CFG_QCOM_DIAG_LOG
register_phys_mem_pgdir(MEM_AREA_IO_SEC, DIAG_BASE, DIAG_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			(DIAG_LOG_START_INFO & ~SMALL_PAGE_MASK),
			SMALL_PAGE_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			(TCSR_BOOT_MISC_DETECT & ~SMALL_PAGE_MASK),
			SMALL_PAGE_SIZE);
#endif

#ifdef CFG_QCOM_TMEL_COM
register_phys_mem(MEM_AREA_IO_SEC,
		  (FEATURE_CONFIG2_ADDR & ~SMALL_PAGE_MASK),
		  SMALL_PAGE_SIZE);
#endif

#ifdef CFG_QCOM_GENI_UART
register_phys_mem_pgdir(MEM_AREA_IO_NSEC, QUP_UART_BASE, QUP_UART_REG_SIZE);
#endif

#ifdef CFG_QCOM_SEC_WDOG
register_phys_mem_pgdir(MEM_AREA_IO_SEC, APSS_WDT_TMR2_BASE, 0x1000);
#endif

#ifdef DRAM0_BASE
register_ddr(DRAM0_BASE, DRAM0_SIZE);
#endif

#ifdef DRAM1_BASE
register_ddr(DRAM1_BASE, DRAM1_SIZE);
#endif

#ifndef CFG_ARM_GICV3
enum itr_return handle_el3_delegated_interrupt(struct itr_handler *h);

/*
 * Handler for interrupts that are delegated from SEL1 to EL3.
 *
 * In GICv2 systems, all secure interrupts are routed to OP-TEE (SEL1)
 * because GICv2 supports only a single group for secure interrupts (Group 0).
 * This handler provides a mechanism to forward specific interrupts to EL3 via
 * SMC, enabling platform-specific interrupt handling at EL3 when desired.
 */
enum itr_return handle_el3_delegated_interrupt(struct itr_handler *h)
{
	struct thread_smc_args args = { };
	size_t i;
	bool found = false;

	for (i = 0; i < ARRAY_SIZE(el3_delegated_interrupts); i++) {
		if (el3_delegated_interrupts[i] == h->it) {
			found = true;
			break;
		}
	}

	/* Verify interrupt is in the delegation list */
	if (!found) {
		EMSG("Interrupt ID 0x%zx not in delegation list", h->it);
		return ITRR_NONE;
	}

	args.a0 = QCOM_EL3_INTR_DELEGATION_SVC_ID;
	args.a1 = h->it;
	thread_smccc(&args);

	if (args.a0 != 0x0 || args.a1 != 0x0) {
		EMSG("EL3 delegated interrupt error: ");
		EMSG("Intr=0x%zx, a0=0x%lx, a1=0x%lx", h->it, args.a0, args.a1);
	}

	/*
	 * Log error and always return ITRR_HANDLED. Returning ITR_NONE will
	 * make OPTEE mask the interrupt and prevent further delivery.
	 * Interrupts like Wdog should not be disabled permanently if one
	 * instance fails.
	 */
	return ITRR_HANDLED;
}

/*
 * Register interrupt handlers for all interrupts that need EL3 delegation.
 * Each interrupt will use the same handler which forwards the interrupt
 * to EL3 via SMC for processing.
 */
static void register_el3_delegated_interrupts(itr_handler_t handler)
{
	size_t i;
	struct itr_chip *chip = interrupt_get_main_chip();

	for (i = 0; i < ARRAY_SIZE(el3_delegated_interrupts); i++) {
		if (interrupt_create_handler(chip, el3_delegated_interrupts[i],
					     handler, 0u, 0u, NULL)) {
			EMSG("Failed to register handler for interrupt 0x%x",
			     el3_delegated_interrupts[i]);
			panic();
		}
		DMSG("Registered EL3-delegated interrupt handler for ID 0x%x",
		     el3_delegated_interrupts[i]);
	}
}
#endif

#ifdef CFG_QCOM_SEC_WDOG

/*
 * Secure watchdog bark interrupt handler
 * This handler pets the watchdog by writing to the reset register
 */
static enum itr_return sec_wdog_bark_handler(struct itr_handler *h __unused)
{
	vaddr_t wdog_base;

	/* Map the watchdog register base */
	wdog_base = (vaddr_t)phys_to_virt(APSS_WDT_TMR2_BASE, MEM_AREA_IO_SEC, 1);
	if (!wdog_base) {
		EMSG("Failed to map watchdog registers");
		return ITRR_HANDLED;
	}

	/* Pet the watchdog by writing to WDOG_RESET register */
	io_write32(wdog_base + WDOG_RESET_REG_OFFSET, 1);

	/* Data synchronization barrier */
	dsb();

	return ITRR_HANDLED;
}

/*
 * Initialize and register the secure watchdog bark interrupt handler
 */
TEE_Result qcom_sec_wdog_init(void)
{
	TEE_Result res;
	struct itr_handler *handler = NULL;

	/* Register the interrupt handler */
	res = interrupt_create_handler(interrupt_get_main_chip(),
				      SEC_WDOG_BARK_INT_ID,
				      sec_wdog_bark_handler,
				      0u, 0u, &handler);

	if (res != TEE_SUCCESS) {
		EMSG("Failed to register secure watchdog bark interrupt handler");
		return res;
	}
	return TEE_SUCCESS;
}
#endif

void plat_trace_ext_puts(const char *str __maybe_unused)
{
#ifdef CFG_QCOM_DIAG_LOG
	qcom_diag_log_puts(str);
#endif
}

void plat_console_init(void)
{
	TEE_Result res __maybe_unused = TEE_SUCCESS;

#ifdef CFG_QCOM_DIAG_LOG
	qcom_diag_log_init();
#endif

#ifdef CFG_QCOM_GENI_UART
	res = qcom_geni_uart_init(&console_data);

	if (res == TEE_SUCCESS) {
		register_serial_console(&console_data.chip);
		IMSG("QCOM GENI UART: Console initialized");
	} else {
		EMSG("QCOM GENI UART: Console init failed (0x%x)", res);
	}
#endif
}

#ifdef CFG_QCOM_GENI_UART
static TEE_Result plat_console_deinit(void)
{
	register_serial_console(NULL);
	IMSG("QCOM GENI UART: Console deinitialized");

	return TEE_SUCCESS;
}

boot_final(plat_console_deinit);
#endif

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

void boot_primary_init_intc(void)
{
#ifdef CFG_ARM_GICV3
	gic_init_v3(0, GICD_BASE, GICR_BASE);
#ifdef CFG_QCOM_SEC_WDOG
	qcom_sec_wdog_init();
#endif
#else /* CFG_ARM_GICV2 */
	gic_init(GICC_BASE, GICD_BASE);
	register_el3_delegated_interrupts(handle_el3_delegated_interrupt);
#endif
}

void boot_secondary_init_intc(void)
{
	gic_init_per_cpu();
}
