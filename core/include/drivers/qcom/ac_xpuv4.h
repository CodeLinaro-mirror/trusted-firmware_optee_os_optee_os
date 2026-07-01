/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef AC_XPUV4_H
#define AC_XPUV4_H

#include <mm/core_mmu.h>
#include <tee_api_types.h>
#include <types_ext.h>
#include <util.h>

/*
 * QAD vector for the secure APPS domain: bit 0 = QAD_APPS identifier,
 * bit 31 = secure qualifier.
 */
#define AC_QAD_APPS_SEC		(BIT32(0) | BIT32(31))

/*
 * AP execution-environment QAD vector (secure + non-secure APPS).
 */
#define AC_QAD_ENV_APPS		(BIT32(0) | BIT32(31) | BIT32(30))

/*
 * IMEM_MPU_BASE is well below CORE_MMU_PGDIR_SIZE, so a pgdir-granularity
 * mapping would round it down to address 0. ac_xpuv4.c maps a small,
 * explicit range instead; callers targeting the IMEM MPU must pass this
 * as ac_xpu_protect_region()'s map_size.
 */
#define AC_XPU_IMEM_MPU_MAP_SIZE	(2 * SMALL_PAGE_SIZE)

#if defined(CFG_QCOM_XPUV4)
/**
 * ac_xpu_protect_region() - Program and lock an XPU4 resource group
 * @xpu_base:   XPU4 register base (SoC physical address)
 * @map_size:   Size of the register window to map at @xpu_base
 * @rg:         Resource group index to program
 * @addr_off:   Memory-region base the XPU's address registers are
 *              relative to; subtracted from @start before programming
 *              the region's start/end address registers
 * @start:      Region start address (must be 4KB aligned)
 * @size:       Region size in bytes (must be 4KB aligned, non-zero)
 * @read_perm:  Read QAD permission vector
 * @write_perm: Write QAD permission vector
 *
 * Restricts [@start, @start + @size) to @read_perm/@write_perm and locks
 * the resource group's configuration. The lock is held by whichever AP
 * domains (secure/non-secure APPS) are present in @read_perm/@write_perm.
 *
 * Safe to call before the MMU is enabled: falls back to a physical
 * address MMIO access in that case.
 */
TEE_Result ac_xpu_protect_region(paddr_t xpu_base, size_t map_size,
				 unsigned int rg, paddr_t addr_off,
				 paddr_t start, size_t size,
				 uint32_t read_perm, uint32_t write_perm);
#else
static inline TEE_Result ac_xpu_protect_region(paddr_t xpu_base __unused,
					       size_t map_size __unused,
					       unsigned int rg __unused,
					       paddr_t addr_off __unused,
					       paddr_t start __unused,
					       size_t size __unused,
					       uint32_t read_perm __unused,
					       uint32_t write_perm __unused)
{
	return TEE_SUCCESS;
}
#endif /* CFG_QCOM_XPUV4 */

#endif /* AC_XPUV4_H */
