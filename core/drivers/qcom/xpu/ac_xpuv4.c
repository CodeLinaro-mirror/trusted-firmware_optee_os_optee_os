// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * XPU access-control protection for OP-TEE secure memory regions.
 *
 */

#include <drivers/qcom/ac_xpuv4.h>
#include <initcall.h>
#include <io.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <trace.h>
#include <util.h>

#define DDR_MPU_BASE			CFG_DDR_MPU_BASE
#define DDR_XPU_TZDRAM_RG		CFG_DDR_XPU_TZDRAM_RG
#define DDR_MPU_ADDR_OFFSET		CFG_DDR_MPU_ADDR_OFFSET

/*
 * XPU4 per-resource-group register offsets.
 * Each RG occupies 0x40 bytes starting at base + 0x1000.
 * Offsets (from hal_xpu4_hwio_generic.h):
 *   RGCR1n   +0x1004  bit 0 = RGE (enable)
 *   RGCSAR1n +0x1008  start address [63:32]
 *   RGCSAR0n +0x100C  start address [31:0]
 *   RGCEAR1n +0x1010  end   address [63:32]
 *   RGCEAR0n +0x1014  end   address [31:0]
 *   RGRDRn   +0x1018  read  QAD vector
 *   RGWRRn   +0x101C  write QAD vector
 *   QADRGLn  +0x1030  QAD lock vector (bit 31 = secure lock RGL_S)
 */
#define XPU4_RGCR1n(b, n)   ((b) + 0x1004u + 0x40u * (n))
#define XPU4_RGCSAR1n(b, n) ((b) + 0x1008u + 0x40u * (n))
#define XPU4_RGCSAR0n(b, n) ((b) + 0x100Cu + 0x40u * (n))
#define XPU4_RGCEAR1n(b, n) ((b) + 0x1010u + 0x40u * (n))
#define XPU4_RGCEAR0n(b, n) ((b) + 0x1014u + 0x40u * (n))
#define XPU4_RGRDRn(b, n)   ((b) + 0x1018u + 0x40u * (n))
#define XPU4_RGWRRn(b, n)   ((b) + 0x101Cu + 0x40u * (n))
#define XPU4_QADRGLn(b, n)  ((b) + 0x1030u + 0x40u * (n))

register_phys_mem_pgdir(MEM_AREA_IO_SEC, DDR_MPU_BASE, CORE_MMU_PGDIR_SIZE);
register_phys_mem(MEM_AREA_IO_SEC, IMEM_MPU_BASE, AC_XPU_IMEM_MPU_MAP_SIZE);

static void xpu4_program_rg(vaddr_t base, unsigned int rg, paddr_t start,
			    paddr_t end, uint32_t read_qad, uint32_t write_qad,
			    uint32_t lock_qad)
{
	io_write32(XPU4_RGCSAR1n(base, rg), (uint32_t)(start >> 32));
	io_write32(XPU4_RGCSAR0n(base, rg), (uint32_t)start);
	io_write32(XPU4_RGCEAR1n(base, rg), (uint32_t)(end >> 32));
	io_write32(XPU4_RGCEAR0n(base, rg), (uint32_t)end);
	io_write32(XPU4_RGRDRn(base, rg), read_qad);
	io_write32(XPU4_RGWRRn(base, rg), write_qad);
	io_write32(XPU4_RGCR1n(base, rg), 1);  /* RGE=1: enable the RG */
	io_write32(XPU4_QADRGLn(base, rg), lock_qad);  /* lock config */
}

TEE_Result ac_xpu_protect_region(paddr_t xpu_base, size_t map_size,
				 unsigned int rg, paddr_t addr_off,
				 paddr_t start, size_t size,
				 uint32_t read_perm, uint32_t write_perm)
{
	uint32_t lock_perm = 0;
	paddr_t xpu_start = 0;
	vaddr_t base = 0;

	/* XPU4 requires 4KB-aligned, non-zero start and end addresses */
	if (!size || !IS_ALIGNED(start, SIZE_4K) ||
	    !IS_ALIGNED(size, SIZE_4K)) {
		EMSG("AC XPU: region %#" PRIxPA "+%#zx not 4KB aligned",
		     start, size);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	/* The XPU stores addresses relative to its memory-region base */
	if (start < addr_off) {
		EMSG("AC XPU: region %#" PRIxPA " below XPU base %#" PRIxPA,
		     start, addr_off);
		return TEE_ERROR_BAD_PARAMETERS;
	}
	xpu_start = start - addr_off;

	base = io_pa_or_va_secure(&(struct io_pa_va){ .pa = xpu_base },
				  map_size);

	/* Lock held by the AP domains that have access (matches TF-A) */
	lock_perm = (read_perm | write_perm) & AC_QAD_ENV_APPS;
	if (!lock_perm) {
		EMSG("AC XPU: no AP domain in region perms, RG left unlocked");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	xpu4_program_rg(base, rg, xpu_start, xpu_start + size,
			read_perm, write_perm, lock_perm);

	return TEE_SUCCESS;
}

static TEE_Result ac_xpu_init(void)
{
	/* Compile-time guarantee for our known TZDRAM constants */
	static_assert(IS_ALIGNED(CFG_TZDRAM_START, SIZE_4K));
	static_assert(IS_ALIGNED(CFG_TZDRAM_SIZE, SIZE_4K));

	/* Restrict TZDRAM access to the secure APPS domain only */
	return ac_xpu_protect_region(DDR_MPU_BASE, CORE_MMU_PGDIR_SIZE,
				     DDR_XPU_TZDRAM_RG, DDR_MPU_ADDR_OFFSET,
				     CFG_TZDRAM_START, CFG_TZDRAM_SIZE,
				     AC_QAD_APPS_SEC, AC_QAD_APPS_SEC);
}
driver_init(ac_xpu_init);
