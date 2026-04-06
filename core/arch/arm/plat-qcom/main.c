// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <config.h>
#include <drivers/gic.h>
#include <kernel/boot.h>
#include <kernel/misc.h>
#include <platform_config.h>
#include <drivers/qcom_diag_log.h>
#include <tee/tee_fs.h>
#include <trace.h>
#include <io.h>
#include <initcall.h>
#ifndef CFG_ARM_GICV3
#include <gicv2_config.h>
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

#ifdef CFG_QCOM_SEC_WDOG
register_phys_mem_pgdir(MEM_AREA_IO_SEC, APSS_WDT_TMR2_BASE, 0x1000);
#endif

#if defined(CFG_QCOM_TMEL_KM)
/*
 * Register TCSR FUSE Hardware Key region as device memory.
 * Both PRI and SEC HW key registers are within the same page,
 * so a single registration covers both address ranges.
 */
register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			(TCSR_FUSE_PRI_HW_KEY_BASE_START & ~SMALL_PAGE_MASK),
			SMALL_PAGE_SIZE);
#endif

#ifdef CFG_EMMC_ICE_FS_ENC_PTA
/*
 * Register TCSR TME KEYSLOT region for ICE key policy access.
 */
register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			(TCSR_KEYSLOT_ADDR & ~SMALL_PAGE_MASK),
			SMALL_PAGE_SIZE);

/*
 * Register ICE (Inline Crypto Engine) register regions.
 */
register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			(SDCC_ICE_LUT_KEYS & ~SMALL_PAGE_MASK),
			SDCC_ICE_LUT_KEYS_SIZE);
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
static TEE_Result qcom_sec_wdog_init(void)
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
#ifdef CFG_QCOM_DIAG_LOG
	qcom_diag_log_init();
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

#if defined(CFG_RPMB_FS)
bool plat_rpmb_key_is_ready(void)
{
	/*
	 * Check if QCOM HUK is enabled at runtime.
	 * If CFG_QCOM_HUK is not enabled, RPMB key derivation is not available.
	 * When enabled, the HUK hardware registers are accessible and configured,
	 * so RPMB keys can be derived using huk_subkey_derive().
	 */
	if (!IS_ENABLED(CFG_QCOM_HUK))
		return false;

	return true;
}
#endif
