// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <stdint.h>
#include <tee_api_types.h>
#include <trace.h>
#include <kernel/delay.h>
#include <kernel/mutex.h>
#include <io.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <qrng.h>

/* Register QRNG memory regions for secure access */
register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			(QRNG_BASE_ADDR & ~SMALL_PAGE_MASK),
			SMALL_PAGE_SIZE);
register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			(QRNG_EE5_BASE_ADDR & ~SMALL_PAGE_MASK),
			SMALL_PAGE_SIZE);

/* Global QRNG state - initialized once during bootup */
static struct {
	vaddr_t base_va;
	vaddr_t ee5_va;
	bool initialized;
	/* Protects initialization state */
	struct mutex init_lock;
	/* Protects hardware read operations */
	struct mutex read_lock;
} qrng_state = {
	.initialized = false,
	.init_lock = MUTEX_INITIALIZER,
	.read_lock = MUTEX_INITIALIZER
};

/* Check if random data is available */
static bool qrng_is_data_available(void)
{
	uint32_t status_val;

	status_val = io_read32(qrng_state.ee5_va + QRNG_EE5_STATUS_OFFSET);
	return (status_val & QRNG_EE5_STATUS_DATA_AVAIL_BMSK) != 0;
}

/* Check QRNG error status registers */
static bool qrng_check_errors(uint32_t *trng_status, uint32_t *drbg_error,
			       uint32_t *timer_status)
{
	*trng_status = io_read32(qrng_state.base_va +
				 QRNG_TRNG_STATUS_OFFSET);
	*drbg_error = io_read32(qrng_state.base_va +
				QRNG_DRBG_ERROR_STATUS_OFFSET);
	*timer_status = io_read32(qrng_state.base_va +
				  QRNG_HW_TIMER_STS_OFFSET);

	return (*trng_status & QRNG_TRNG_ERROR_MASK) != 0 ||
	       *drbg_error != 0 ||
	       *timer_status != 0;
}

/* Perform QRNG soft reset */
static void qrng_soft_reset(void)
{
	uint32_t ctrl_val;

	/* Set soft reset bit */
	ctrl_val = io_read32(qrng_state.base_va + QRNG_CONTROL_OFFSET);
	ctrl_val |= QRNG_CONTROL_QRNG_SOFT_RST;
	io_write32(qrng_state.base_va + QRNG_CONTROL_OFFSET, ctrl_val);

	/* Wait at least 3 clock cycles */
	udelay(10);

	/* Clear soft reset bit */
	ctrl_val &= ~QRNG_CONTROL_QRNG_SOFT_RST;
	io_write32(qrng_state.base_va + QRNG_CONTROL_OFFSET, ctrl_val);
}

/* Wait for random data to be available */
static TEE_Result qrng_wait_for_data(void)
{
	uint64_t timeout_expire;
	uint64_t error_timeout;
	uint32_t status_val;
	uint32_t trng_status;
	uint32_t drbg_error;
	uint32_t timer_status;
	bool data_available = false;
	unsigned int reset_count = 0;
	const unsigned int MAX_RESETS = 10;

	timeout_expire = timeout_init_us(QRNG_TIMEOUT_MS * 1000);

	/* Poll for data availability */
	while (!data_available && !timeout_elapsed(timeout_expire)) {
		if (qrng_is_data_available())
			data_available = true;
	}

	if (data_available)
		return TEE_SUCCESS;

	/* Check for errors within timeout window with recovery attempts */
	error_timeout = timeout_init_us(QRNG_TIMEOUT_MS * 1000);

	while (!timeout_elapsed(error_timeout)) {
		if (qrng_check_errors(&trng_status, &drbg_error,
				      &timer_status)) {
			EMSG("QRNG: Error detected (TRNG=0x%x DRBG=0x%x TIMER=0x%x), reset attempt %u/%u",
			     trng_status, drbg_error, timer_status,
			     reset_count + 1, MAX_RESETS);

			if (reset_count < MAX_RESETS) {
				/* Perform soft reset and retry */
				mutex_lock(&qrng_state.init_lock);
				qrng_state.initialized = false;
				mutex_unlock(&qrng_state.init_lock);
				qrng_soft_reset();
				reset_count++;

			} else {
				/* Max resets exceeded */
				EMSG("QRNG: Max reset attempts (%u) exceeded",
				     MAX_RESETS);
				return TEE_ERROR_GENERIC;
			}
		}

		if (qrng_is_data_available())
			return TEE_SUCCESS;
	}

	EMSG("QRNG: Timeout waiting for data");
	return TEE_ERROR_GENERIC;
}

/* Initialize QRNG driver on first use */
static TEE_Result qrng_init(void)
{
	uint32_t drbg_ctrl;
	TEE_Result res = TEE_SUCCESS;

	mutex_lock(&qrng_state.init_lock);

	if (qrng_state.initialized) {
		mutex_unlock(&qrng_state.init_lock);
		return TEE_SUCCESS;
	}

	qrng_state.base_va = (vaddr_t)phys_to_virt_io(QRNG_BASE_ADDR, 0x300);
	if (!qrng_state.base_va) {
		EMSG("QRNG: Failed to map base address");
		res = TEE_ERROR_GENERIC;
		goto out;
	}

	qrng_state.ee5_va = (vaddr_t)phys_to_virt_io(QRNG_EE5_BASE_ADDR, 0x10);
	if (!qrng_state.ee5_va) {
		EMSG("QRNG: Failed to map EE5 address");
		res = TEE_ERROR_GENERIC;
		goto out;
	}

	drbg_ctrl = io_read32(qrng_state.base_va +
			      QRNG_DRBG_CONTROL_OFFSET);
	drbg_ctrl &= ~QRNG_DRBG_RESEED_TIMER_EN;
	io_write32(qrng_state.base_va + QRNG_DRBG_CONTROL_OFFSET,
		   drbg_ctrl);

	io_write32(qrng_state.base_va + QRNG_CONTROL_OFFSET,
		   QRNG_CONTROL_QRNG_ENABLE);

	qrng_state.initialized = true;

out:
	mutex_unlock(&qrng_state.init_lock);
	return res;
}

TEE_Result qrng_get_random_data(void *buf, size_t len)
{
	uint8_t *output = (uint8_t *)buf;
	uint32_t random_val;
	size_t bytes_remaining;
	uint8_t *out_ptr;
	uint32_t byte_idx;
	TEE_Result res;

	if (!buf || len == 0)
		return TEE_ERROR_BAD_PARAMETERS;

	res = qrng_init();
	if (res != TEE_SUCCESS) {
		EMSG("QRNG: Init failed");
		return res;
	}

	bytes_remaining = len;
	out_ptr = output;

	while (bytes_remaining > 0) {
		/* Serialize hardware access to prevent concurrent reads */
		mutex_lock(&qrng_state.read_lock);

		res = qrng_wait_for_data();
		if (res != TEE_SUCCESS) {
			mutex_unlock(&qrng_state.read_lock);
			EMSG("QRNG: Data read failed");
			return res;
		}

		random_val = io_read32(qrng_state.ee5_va +
				       QRNG_EE5_DATA_OUT_OFFSET);

		mutex_unlock(&qrng_state.read_lock);

		for (byte_idx = 0; byte_idx < 4 && bytes_remaining > 0;
		     byte_idx++) {
			*out_ptr = (uint8_t)(random_val >> (8 * byte_idx));
			out_ptr++;
			bytes_remaining--;
		}
	}

	return TEE_SUCCESS;
}
