// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/qcom_geni_uart.h>
#include <io.h>
#include <platform_config.h>
#include <tee_api_types.h>
#include <types_ext.h>
#include <util.h>

/* Register addresses */
#define GENI_FORCE_DEFAULT_REG			0x20
#define GENI_OUTPUT_CTRL_REG			0x24
#define GENI_STATUS_REG				0x40
#define GENI_SER_M_CLK_CFG_REG			0x48
#define GENI_CLK_SEL_REG			0x7c
#define GENI_TX_TRANS_CFG_REG			0x25c
#define GENI_TX_PACKING_CFG0_REG		0x260
#define GENI_TX_PACKING_CFG1_REG		0x264
#define GENI_TX_WORD_LEN_REG			0x268
#define GENI_TX_TRANS_LEN_REG			0x270
#define GENI_M_CMD0_REG				0x600
#define GENI_M_CMD_CTRL_REG			0x604
#define GENI_M_IRQ_STATUS_REG			0x610
#define GENI_M_IRQ_EN_REG			0x614
#define GENI_M_IRQ_CLEAR_REG			0x618
#define GENI_M_IRQ_EN_CLEAR_REG			0x620
#define GENI_TX_FIFO_REG			0x700
#define GENI_TX_FIFO_STATUS_REG			0x800

/* Command and status bit definitions */
#define GENI_M_CMD0_REG_START_TX		0x8000000
#define GENI_STATUS_REG_CMD_ACTIVE		BIT(0)
#define GENI_M_IRQ_STATUS_REG_CMD_DONE_EN	BIT(0)
#define GENI_M_IRQ_STATUS_REG_CMD_ABORT_EN	BIT(5)
#define GENI_M_CMD_CTRL_REG_CMD_ABORT		BIT(1)

/* Configuration values */
#define GENI_TX_PACKING_CFG0_VAL		0x4380e
#define GENI_TX_PACKING_CFG1_VAL		0xc3e0e
#define GENI_TX_TRANS_CFG_VAL			0x2
#define GENI_OUTPUT_CTRL_VAL			0x7f
#define GENI_CLK_SEL_VAL			0x1
#define GENI_SER_M_CLK_CFG_VAL			0x11
#define GENI_FORCE_DEFAULT_VAL			0x1

/* Timeout value in microseconds */
#define UART_TIMEOUT_US				100000

static TEE_Result qcom_geni_uart_poll_tx_done(vaddr_t base)
{
	uint32_t irq_clear = GENI_M_IRQ_STATUS_REG_CMD_DONE_EN;
	uint32_t status;
	uint64_t timeout;

	status = io_read32(base + GENI_M_IRQ_STATUS_REG);
	if (!(status & GENI_M_IRQ_STATUS_REG_CMD_DONE_EN)) {
		io_write32(base + GENI_M_CMD_CTRL_REG,
			   GENI_M_CMD_CTRL_REG_CMD_ABORT);
		irq_clear |= GENI_M_IRQ_STATUS_REG_CMD_ABORT_EN;

		timeout = timeout_init_us(UART_TIMEOUT_US);

		while (!timeout_elapsed(timeout)) {
			status = io_read32(base + GENI_M_IRQ_STATUS_REG);
			if (status & GENI_M_IRQ_STATUS_REG_CMD_ABORT_EN)
				break;
		}

		if (!(status & GENI_M_IRQ_STATUS_REG_CMD_ABORT_EN)) {
			EMSG("QUP GENI UART: TX abort command timed out");
			return TEE_ERROR_GENERIC;
		}
	}

	io_write32(base + GENI_M_IRQ_CLEAR_REG, irq_clear);
	return TEE_SUCCESS;
}

static void qcom_geni_uart_putc(struct serial_chip *chip, int ch)
{
	struct qcom_geni_uart_data *pd =
		container_of(chip, struct qcom_geni_uart_data, chip);
	vaddr_t base = io_pa_or_va(&pd->base, QUP_UART_REG_SIZE);

	io_write32(base + GENI_TX_TRANS_LEN_REG, 1);
	io_write32(base + GENI_M_CMD0_REG, GENI_M_CMD0_REG_START_TX);
	io_write32(base + GENI_TX_FIFO_REG, ch);

	if (qcom_geni_uart_poll_tx_done(base) != TEE_SUCCESS)
		EMSG("TX operation failed");
}

static const struct serial_ops qcom_geni_uart_ops = {
	.putc = qcom_geni_uart_putc,
};
DECLARE_KEEP_PAGER(qcom_geni_uart_ops);

TEE_Result qcom_geni_uart_init(struct qcom_geni_uart_data *pd)
{
	vaddr_t base;
	uint32_t status;

	if (!pd) {
		EMSG("QUP GENI UART: Invalid parameter");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	pd->base.pa = QUP_UART_BASE;
	pd->chip.ops = &qcom_geni_uart_ops;

	base = io_pa_or_va(&pd->base, QUP_UART_REG_SIZE);
	if (!base) {
		EMSG("QUP GENI UART: Failed to get base address");
		return TEE_ERROR_GENERIC;
	}

	if (io_read32(base + GENI_TX_FIFO_STATUS_REG) != 0) {
		TEE_Result res;

		EMSG("QUP GENI UART: TX FIFO not empty");
		res = qcom_geni_uart_poll_tx_done(base);
		if (res != TEE_SUCCESS) {
			EMSG("QUP GENI UART: Failed to clear TX FIFO");
			return res;
		}
	}
	io_write32(base + GENI_M_IRQ_EN_CLEAR_REG, 0xFFFFFFFF);

	io_write32(base + GENI_TX_WORD_LEN_REG, 8);
	io_write32(base + GENI_TX_PACKING_CFG0_REG, GENI_TX_PACKING_CFG0_VAL);
	io_write32(base + GENI_TX_PACKING_CFG1_REG, GENI_TX_PACKING_CFG1_VAL);
	io_write32(base + GENI_TX_TRANS_CFG_REG, GENI_TX_TRANS_CFG_VAL);
	io_write32(base + GENI_SER_M_CLK_CFG_REG, GENI_SER_M_CLK_CFG_VAL);

	io_write32(base + GENI_OUTPUT_CTRL_REG, GENI_OUTPUT_CTRL_VAL);
	io_write32(base + GENI_CLK_SEL_REG, GENI_CLK_SEL_VAL);

	status = io_read32(base + GENI_STATUS_REG);
	if (status & GENI_STATUS_REG_CMD_ACTIVE) {
		uint64_t abort_timeout;

		io_write32(base + GENI_M_CMD_CTRL_REG,
			   GENI_M_CMD_CTRL_REG_CMD_ABORT);

		abort_timeout = timeout_init_us(UART_TIMEOUT_US);
		while (!timeout_elapsed(abort_timeout)) {
			status = io_read32(base + GENI_STATUS_REG);
			if (!(status & GENI_STATUS_REG_CMD_ACTIVE))
				break;
		}

		io_write32(base + GENI_M_IRQ_CLEAR_REG, 0xFFFFFFFF);

		status = io_read32(base + GENI_STATUS_REG);
		if (status & GENI_STATUS_REG_CMD_ACTIVE) {
			EMSG("QUP GENI UART: Failed to abort active command");
			return TEE_ERROR_BUSY;
		}
	}

	io_write32(base + GENI_FORCE_DEFAULT_REG, GENI_FORCE_DEFAULT_VAL);
	io_write32(base + GENI_M_IRQ_EN_REG, GENI_M_IRQ_STATUS_REG_CMD_DONE_EN);

	return TEE_SUCCESS;
}
