/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#include <mm/generic_ram_layout.h>

/* Make stacks aligned to data cache line length */
#define STACK_ALIGNMENT		64

/* GIC configuration from build system */
#if defined(CFG_ARM_GICV3)
#define GIC_BASE		CFG_GIC_BASE
#define GICD_OFFSET		CFG_GICD_OFFSET
#define GICR_OFFSET		CFG_GICR_OFFSET

/* GIC register addresses derived from base addresses */
#define GICD_BASE		(GIC_BASE + GICD_OFFSET)
#define GICR_BASE		(GIC_BASE + GICR_OFFSET)
#endif

#endif /* PLATFORM_CONFIG_H */
