/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef QRNG_H
#define QRNG_H

#include <tee_api_types.h>

#ifdef CFG_QCOM_QRNG

/* QRNG Register Offsets */
#define QRNG_CONTROL_OFFSET             0x14
#define QRNG_DRBG_CONTROL_OFFSET        0x128
#define QRNG_TRNG_STATUS_OFFSET         0x100
#define QRNG_DRBG_ERROR_STATUS_OFFSET   0x120
#define QRNG_HW_TIMER_STS_OFFSET        0x208
#define QRNG_EE5_STATUS_OFFSET          0x4
#define QRNG_EE5_DATA_OUT_OFFSET        0x0

/* Register field definitions */
#define QRNG_CONTROL_QRNG_ENABLE        0x2
#define QRNG_CONTROL_QRNG_SOFT_RST      0x1
#define QRNG_DRBG_RESEED_TIMER_EN       0x10000
#define QRNG_EE5_STATUS_DATA_AVAIL_BMSK 0x1

/* Timeout in milliseconds */
#define QRNG_TIMEOUT_MS                 2

/* Error status register masks */
#define QRNG_TRNG_ERROR_MASK            0xF

/*
 * qrng_get_random_data() - Get random bytes from QRNG hardware
 * @buf: Buffer to receive random bytes
 * @len: Number of bytes to generate
 *
 * Implements QRNG driver with all 7 operational steps:
 * Step 1: Disable DRBG timer-based reseed
 * Step 2: Enable QRNG
 * Step 3: Poll for DATA_AVAIL bit
 * Step 4: Read random data from QRNG_DATA_OUT register
 * Step 5: Error checking with 2ms timeout window
 * Step 6: Soft reset on error with proper timing
 * Step 7: Timeout handling without error
 *
 * Return: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result qrng_get_random_data(void *buf, size_t len);

#endif /* CFG_QCOM_QRNG */

#endif /* QRNG_H */