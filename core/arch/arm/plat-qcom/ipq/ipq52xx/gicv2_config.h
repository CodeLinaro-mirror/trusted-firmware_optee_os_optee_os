/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef GICV2_CONFIG_H
#define GICV2_CONFIG_H

#include <stdint.h>

/* Interrupt IDs for EL3 delegation */
#define SEC_WDOG_BARK_INT_ID		0x23u
#define NON_SEC_WDOG_BITE_INT_ID	0x136u
#define XPU_VIOLATION_INT_ID		0xB1u

/*
 * List of interrupt IDs to be delegated to EL3 from SEL1.
 * Add new interrupt IDs to this list as needed.
 */
static const uint32_t el3_delegated_interrupts[] = {
	SEC_WDOG_BARK_INT_ID,
	NON_SEC_WDOG_BITE_INT_ID,
	XPU_VIOLATION_INT_ID,
};

#endif /* GICV2_CONFIG_H */
