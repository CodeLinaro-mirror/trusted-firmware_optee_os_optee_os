// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/qcom/ac_xpuv4.h>
#include <drivers/qcom_diag_log.h>
#include <io.h>
#include <kernel/misc.h>
#include <kernel/thread.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <string.h>
#include <trace.h>

#define DIAG_MAGIC		0x47414944 /* Buffer initialized and ready */
#define DIAG_MAGIC_FAILED	0xDEADBEEF /* Initialization failed */
#define DIAG_MAGIC_DLOAD	0xD15AB1ED /* DLOAD mode - logging disabled */
#define DLOAD_MAGIC_COOKIE	0x10       /* Download mode detection value */

/**
 * struct diag - Main diagnostic region structure
 * @magic:       Magic identifier to validate the region
 * @ring_off:    Offset to the ring buffer from the start of the region
 * @ring_len:    Length of the ring buffer
 * @wrap:        Number of times the buffer has wrapped around
 * @offset:      Current offset in the buffer
 * @log_buf:     Flexible array for the actual log data
 */
struct diag {
	uint32_t magic;
	uint32_t ring_off;
	uint32_t ring_len;
	uint32_t wrap;
	uint32_t offset;
	uint8_t log_buf[];
};

static struct diag *get_diag_region(void)
{
	struct io_pa_va diag_pa_va = {
		.pa = DIAG_BASE,
		.va = 0,
	};

	return (struct diag *)io_pa_or_va(&diag_pa_va, DIAG_SIZE);
}

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

void qcom_diag_log_init(void)
{
	struct diag *diag;
	size_t diag_size = DIAG_SIZE;
	struct io_pa_va diag_info_pa_va = {
		.pa = DIAG_LOG_START_INFO,
		.va = 0,
	};
	uint32_t *diag_info_addr;
	TEE_Result res = TEE_SUCCESS;

	res = ac_xpu_protect_region(IMEM_MPU_BASE, AC_XPU_IMEM_MPU_MAP_SIZE,
				    IMEM_XPU_LOG_RG, IMEM_BASE,
				    DIAG_BASE, DIAG_SIZE,
				    IMEM_XPU_LOG_READ_QAD, AC_QAD_APPS_SEC);
	if (res)
		EMSG("DIAG LOG: XPU protection failed: %#" PRIx32, res);

	diag = get_diag_region();
	if (!diag)
		return;

	if (is_dload_mode_set()) {
		if (diag->magic == DIAG_MAGIC) {
			diag->magic = DIAG_MAGIC_DLOAD;
			dsb();
		}
		return;
	}

	memset(diag, 0, diag_size);

	diag->ring_off = offsetof(struct diag, log_buf);
	diag->ring_len = (diag_size - diag->ring_off) & (~0xF);

	if (diag_size <= diag->ring_off || diag->ring_len == 0) {
		diag->magic = DIAG_MAGIC_FAILED;
		return;
	}

	diag->magic = DIAG_MAGIC;

	IMSG("DIAG LOG: Initialized (ring buffer: %u bytes)", diag->ring_len);

	diag_info_addr = (uint32_t *)io_pa_or_va(&diag_info_pa_va,
						  2 * sizeof(uint32_t));
	if (diag_info_addr) {
		io_write32((vaddr_t)&diag_info_addr[0], DIAG_BASE);
		io_write32((vaddr_t)&diag_info_addr[1], DIAG_SIZE);
	}

	dsb();
}

/**
 * qcom_diag_log_puts() - Write a string to the diagnostic log buffer
 * @str: Null-terminated string to write
 */
void qcom_diag_log_puts(const char *str)
{
	struct diag *diag;
	const char *p;
	uint32_t offset;

	if (!str)
		return;

	diag = get_diag_region();
	if (!diag)
		return;

	if (diag->magic != DIAG_MAGIC)
		return;

	for (p = str; *p; p++) {
		offset = diag->offset;

		diag->log_buf[offset] = *p;

		offset++;
		if (offset >= diag->ring_len) {
			offset = 0;
			if (diag->wrap < UINT32_MAX)
				diag->wrap++;
		}
		diag->offset = offset;
	}

	dsb();
}
