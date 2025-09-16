// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/qcom_diag_log.h>
#include <io.h>
#include <kernel/misc.h>
#include <kernel/thread.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <platform_config.h>
#include <string.h>
#include <trace.h>

/* Magic value that appears as "DIAG" in memory dumps (ASCII "DIAG" in
 * little-endian)
 */
#define DIAG_MAGIC		0x47414944

/* Magic value indicating initialization failed */
#define DIAG_MAGIC_FAILED	0xDEADBEEF

/* Magic cookie for download mode detection (Qualcomm specification) */
#define DLOAD_MAGIC_COOKIE	0x10

/**
 * struct diag - Main diagnostic region structure
 * @magic:       Magic identifier to validate the region
 * @in_use:      Flag indicating if the region is currently in use
 * @ring_off:    Offset to the ring buffer from the start of the region
 * @ring_len:    Length of the ring buffer
 * @wrap:        Number of times the buffer has wrapped around
 * @offset:      Current offset in the buffer
 * @log_buf:     Flexible array for the actual log data
 */
struct diag {
	uint32_t magic;
	uint32_t in_use;
	uint32_t ring_off;
	uint32_t ring_len;
	uint32_t wrap;
	uint32_t offset;
	uint8_t log_buf[];
};

/**
 * get_diag_region() - Get the diagnostic region
 *
 * Return: Pointer to the diagnostic region, or NULL if mapping fails
 */
static struct diag *get_diag_region(void)
{
	struct io_pa_va diag_pa_va = {
		.pa = DIAG_BASE,
		.va = 0,
	};

	return (struct diag *)io_pa_or_va(&diag_pa_va, DIAG_SIZE);
}

/**
 * is_dload_mode_set() - Check if device is in crashdump download mode
 *
 * The download mode is a special boot state after a crash where the system
 * preserves memory for crashdump collection. We avoid initializing or writing
 * to the diagnostic buffer in this mode to preserve the previous state's logs,
 * allowing external tools to collect crashdump data for debugging.
 *
 * Return: true if in download mode, false otherwise
 */
static bool is_dload_mode_set(void)
{
	struct io_pa_va tcsr_pa_va = {
		.pa = TCSR_BOOT_MISC_DETECT,
		.va = 0,
	};
	uint32_t *tcsr_reg;
	uint32_t reg_value;

	tcsr_reg = (uint32_t *)io_pa_or_va(&tcsr_pa_va, sizeof(uint32_t));
	if (!tcsr_reg)
		return false;

	reg_value = io_read32((vaddr_t)tcsr_reg);

	return (reg_value == DLOAD_MAGIC_COOKIE);
}

/**
 * qcom_diag_log_init() - Initialize the diagnostic log buffer
 *
 * Sets up a shared ring buffer for external diagnostic tools.
 * Skips initialization if device is in download mode (to preserve previous
 * logs for crashdump collection) or if the buffer is already initialized.
 * Updates hardware registers with buffer location for external tool access.
 */
void qcom_diag_log_init(void)
{
	struct diag *diag;
	size_t diag_size = DIAG_SIZE;
	size_t available_size;
	struct io_pa_va diag_info_pa_va = {
		.pa = DIAG_LOG_START_INFO,
		.va = 0,
	};
	uint32_t *diag_info_addr;

	diag = get_diag_region();

	/* Early exit conditions: no region, already initialized, or in use */
	if (!diag || diag->magic == DIAG_MAGIC || diag->in_use == 1)
		return;

	memset(diag, 0, diag_size);

	diag->in_use = 1;

	diag->ring_off = offsetof(struct diag, log_buf);
	available_size = diag_size - diag->ring_off;
	diag->ring_len = available_size & (~0xF);

	/* Ensure we have a usable buffer */
	if (diag_size <= diag->ring_off || diag->ring_len == 0) {
		diag->magic = DIAG_MAGIC_FAILED;
		diag->in_use = 0;
		return;
	}

	/* Set magic last - indicates buffer is ready */
	diag->magic = DIAG_MAGIC;
	diag->in_use = 0;

	IMSG("DIAG LOG: Initialized successfully (ring buffer: %u bytes)",
	     diag->ring_len);

	/* Update external pointers - hardware register writes */
	/* Map both consecutive 32-bit registers (start address and size) */
	diag_info_addr = (uint32_t *)io_pa_or_va(&diag_info_pa_va,
						  2 * sizeof(uint32_t));
	if (diag_info_addr) {
		/* Write diagnostic buffer start address */
		io_write32((vaddr_t)&diag_info_addr[0], DIAG_BASE);
		/* Write diagnostic buffer size */
		io_write32((vaddr_t)&diag_info_addr[1], DIAG_SIZE);
	}

	/* Memory barrier to ensure all initialization is visible to other cores
	 * and hardware
	 */
	dsb();
}

/**
 * qcom_diag_log_puts() - Write a string to the diagnostic log buffer
 * @str: Null-terminated string to write
 *
 * Writes string to shared ring buffer for external diagnostic tools.
 * Follows standard OP-TEE pattern with no artificial length restrictions.
 */
void qcom_diag_log_puts(const char *str)
{
	struct diag *diag;
	const char *p;
	uint32_t offset;

	/* Skip logging in download mode to preserve previous logs for crashdump
	 */
	if (is_dload_mode_set() || !str)
		return;

	/* Get diag region and initialize if needed */
	diag = get_diag_region();
	if (!diag)
		return;

	/* Initialize if not ready, skip if failed */
	if (diag->magic != DIAG_MAGIC) {
		/* Skip if initialization previously failed */
		if (diag->magic == DIAG_MAGIC_FAILED)
			return;

		qcom_diag_log_init();
		/* Skip if initialization just failed */
		if (diag->magic != DIAG_MAGIC)
			return;
	}

	/* Write each character to the ring buffer */
	for (p = str; *p; p++) {
		offset = diag->offset;

		/* Validate offset to prevent buffer overflow */
		if (offset >= diag->ring_len) {
			/* Corrupted offset - reset to safe value */
			offset = 0;
			diag->offset = 0;
			if (diag->wrap < UINT32_MAX)
				diag->wrap++;
		}

		diag->log_buf[offset] = *p;
		diag->offset = (offset + 1) % diag->ring_len;

		/* Update wrap counter when buffer wraps around */
		if (diag->offset == 0 && diag->wrap < UINT32_MAX)
			diag->wrap++;
	}
}
