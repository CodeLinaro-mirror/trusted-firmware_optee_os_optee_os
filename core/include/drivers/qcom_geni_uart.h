/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef QCOM_GENI_UART_H
#define QCOM_GENI_UART_H

#include <drivers/serial.h>
#include <mm/core_memprot.h>
#include <types_ext.h>

struct qcom_geni_uart_data {
	struct io_pa_va base;
	struct serial_chip chip;
};

TEE_Result qcom_geni_uart_init(struct qcom_geni_uart_data *pd);

#endif /* QCOM_GENI_UART_H */
