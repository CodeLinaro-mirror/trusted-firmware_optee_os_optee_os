/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include <arm.h>
#include <drivers/gic.h>
#include <kernel/boot.h>
#include <kernel/misc.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <stdint.h>

/* Register GIC memory regions */
#if defined(CFG_ARM_GICV3)
register_phys_mem_pgdir(MEM_AREA_IO_SEC, GICD_BASE, GIC_DIST_REG_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC, GICR_BASE, GIC_CPU_REG_SIZE);
#endif

/*
 * Generic QCOM platform implementation of get_core_pos_mpidr
 *
 * This implementation supports both symmetric and asymmetric cluster configurations
 * by using the CFG_CORE_CLUSTER_SHIFT configuration parameter.
 *
 * For symmetric clusters:
 * - CorePos = (ClusterId << CFG_CORE_CLUSTER_SHIFT) + CoreId
 * - Example with CFG_CORE_CLUSTER_SHIFT=2 (4 cores per cluster):
 *   - Cluster 0, Core 0 -> Position 0
 *   - Cluster 0, Core 1 -> Position 1
 *   - Cluster 0, Core 2 -> Position 2
 *   - Cluster 0, Core 3 -> Position 3
 *   - Cluster 1, Core 0 -> Position 4
 *   - Cluster 1, Core 1 -> Position 5
 *   - etc.
 *
 * For asymmetric clusters:
 * - The same formula works as long as CFG_CORE_CLUSTER_SHIFT is set to
 *   accommodate the largest cluster size
 * - Example with CFG_CORE_CLUSTER_SHIFT=3 for ipq96xx (5 cores in single cluster):
 *   - Cluster 0, Cores 0-4: Positions 0-4
 *
 * MPIDR layout:
 * - AFF2[23:16]: Cluster ID
 * - AFF1[15:8]:  Core ID within cluster
 * - AFF0[7:0]:   Thread ID (may be 0 or contain thread ID depending on MT bit)
 */
size_t get_core_pos_mpidr(uint32_t mpidr)
{
	uint32_t cluster_id, core_id;

	/*
	 * Handle MT bit properly - check if multithreading is enabled
	 * If MT=0: Shift MPIDR left by 8 bits to align affinity levels
	 * If MT=1: MPIDR is already properly aligned
	 */
	if (!(mpidr & MPIDR_MT_MASK)) {
		/* MT=0: Shift to align AFF2->AFF1, AFF1->AFF0 */
		mpidr <<= MPIDR_AFFINITY_BITS;
	}
	/* If MT=1: Use MPIDR as-is, AFF0 contains thread ID */

	/* Extract cluster ID from AFF2 */
	cluster_id = (mpidr >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK;

	/* Extract core ID from AFF1 */
	core_id = (mpidr >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK;

	/* Calculate core position using CFG_CORE_CLUSTER_SHIFT */
	return (cluster_id << CFG_CORE_CLUSTER_SHIFT) + core_id;
}

#if defined(CFG_ARM_GICV3)
void boot_primary_init_intc(void)
{
	/*
	 * Initialize GICv3 with:
	 * - No GICC base (not used in GICv3)
	 * - GICD base
	 * - GICR base
	 */
	gic_init_v3(0, GICD_BASE, GICR_BASE);
}

/*
 * Initialize the GIC for secondary CPUs
 */
void boot_secondary_init_intc(void)
{
	gic_init_per_cpu();
}
#endif /* CFG_ARM_GICV3 */
