/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

#include <mm/generic_ram_layout.h>

#define STACK_ALIGNMENT		64

#if defined(CFG_ARM_GICV3)
#define GIC_BASE		CFG_GIC_BASE
#define GIC_SIZE		CFG_GIC_SIZE
#define GICD_OFFSET		CFG_GICD_OFFSET
#define GICR_OFFSET		CFG_GICR_OFFSET

#define GICD_BASE		(GIC_BASE + GICD_OFFSET)
#define GICR_BASE		(GIC_BASE + GICR_OFFSET)
#endif

#if defined(CFG_QCOM_GENI_UART)
#define QUP_UART_BASE          CFG_QCOM_QUP_UART_BASE
#define QUP_UART_REG_SIZE      CFG_QCOM_QUP_UART_SIZE
#endif

#if defined(CFG_QCOM_DIAG_LOG)
#define DIAG_BASE		CFG_QCOM_DIAG_BASE
#define DIAG_SIZE		CFG_QCOM_DIAG_SIZE
#define DIAG_LOG_START_INFO	CFG_QCOM_DIAG_LOG_START_INFO
#define DIAG_LOG_SIZE_INFO	CFG_QCOM_DIAG_LOG_SIZE_INFO
#define TCSR_BOOT_MISC_DETECT	CFG_QCOM_TCSR_BOOT_MISC_DETECT
#endif

/* DDR memory configuration */
#ifdef CFG_DRAM0_BASE
#define DRAM0_BASE		CFG_DRAM0_BASE
#define DRAM0_SIZE		CFG_DRAM0_SIZE
#endif

#ifdef CFG_DRAM1_BASE
#define DRAM1_BASE		CFG_DRAM1_BASE
#define DRAM1_SIZE		CFG_DRAM1_SIZE
#endif

#endif /* PLATFORM_CONFIG_H */
