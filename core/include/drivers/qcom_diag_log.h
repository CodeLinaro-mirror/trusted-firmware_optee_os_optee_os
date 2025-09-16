/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef QCOM_DIAG_LOG_H
#define QCOM_DIAG_LOG_H

#include <tee_api_types.h>
#include <types_ext.h>

/**
 * qcom_diag_log_init() - Initialize the diagnostic logging system
 */
void qcom_diag_log_init(void);

/**
 * qcom_diag_log_puts() - Write a string to the diagnostic log buffer
 * @str: String to write
 */
void qcom_diag_log_puts(const char *str);

#endif /* QCOM_DIAG_LOG_H */
