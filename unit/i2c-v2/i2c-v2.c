/* I2Cv2 register, polling, interrupt, DMA, FIFO, and state-machine tests. */

#include <pmb887x.h>

#include "dmac.h"
#include "i2c.h"
#include "driver.h"
#include "i2c-v2.h"
#include "test.h"

#ifdef PMB8876

#include <string.h>

#if defined(BOARD_HAS_PMIC_PMB6812)
#include <pmic/PMB6812.h>
#elif defined(BOARD_HAS_PMIC_D1094XX)
#include <pmic/D1094XX.h>
#endif

#define I2C_STATUS_CLEAR 0x3F
#define I2C_PROTOCOL_CLEAR 0x7F
#define I2C_ERROR_CLEAR 0x0F
#define I2C_DMA_TX_CHANNEL 0
#define I2C_DMA_RX_CHANNEL 1
#define I2C_DMA_TIMEOUT_MS 100
#define I2C_READ_COMPLETION_STATUS (I2C_PIRQSS_RX | I2C_PIRQSS_TX_END)
#define I2C_REQUEST_STATUS (I2C_RIS_LSREQ_INT | I2C_RIS_SREQ_INT | I2C_RIS_LBREQ_INT | I2C_RIS_BREQ_INT)

enum i2c_test_write_verify {
	I2C_TEST_WRITE_READBACK,
	I2C_TEST_WRITE_CONFIRM,
};

struct i2c_test_target {
	uint8_t address;
	uint8_t write_register;
	uint8_t read_register;
	uint8_t write_value;
	uint8_t restore_value;
	bool write_uses_pec;
	enum i2c_test_write_verify write_verify;
};

#if defined(BOARD_HAS_PMIC_PMB6812)
static const struct i2c_test_target I2C_TEST_TARGET = {
	.address = PMB6812_I2C_ADDR,
	.write_register = PMB6812_LEDCTRL1,
	.read_register = PMB6812_GEF1,
	.write_value = 1 << PMB6812_LEDCTRL1_LEDPWM_SHIFT,
	.restore_value = 0,
	.write_uses_pec = true,
	.write_verify = I2C_TEST_WRITE_CONFIRM,
};
#elif defined(BOARD_HAS_PMIC_D1094XX)
static const struct i2c_test_target I2C_TEST_TARGET = {
	.address = D1094XX_I2C_ADDR,
	.write_register = D1094XX_LIGHT_PWM1,
	.read_register = D1094XX_LIGHT_PWM1,
	.write_value = 1,
	.restore_value = 0,
	.write_uses_pec = false,
	.write_verify = I2C_TEST_WRITE_READBACK,
};
#else
#error "i2c-v2 test: unsupported I2C target for this board"
#endif

static i2c_v2_result_t target_read(uint8_t reg, uint8_t *data, uint32_t size) {
	return i2c_v2_smbus_read(I2C_TEST_TARGET.address, reg, data, size);
}

static i2c_v2_result_t target_write(uint8_t value) {
	if (I2C_TEST_TARGET.write_uses_pec) {
		return i2c_v2_smbus_write_pec(I2C_TEST_TARGET.address, I2C_TEST_TARGET.write_register, value);
	}

	return i2c_v2_smbus_write(I2C_TEST_TARGET.address, I2C_TEST_TARGET.write_register, value);
}

static i2c_v2_result_t target_write_verification_read(uint8_t *actual) {
	if (I2C_TEST_TARGET.write_verify == I2C_TEST_WRITE_CONFIRM) {
		return i2c_v2_transfer_bytes(I2C_TEST_TARGET.address, NULL, actual, 1);
	}

	return target_read(I2C_TEST_TARGET.write_register, actual, 1);
}

static uint8_t target_write_verification_value(uint8_t value) {
	if (I2C_TEST_TARGET.write_verify == I2C_TEST_WRITE_CONFIRM) {
		return i2c_v2_smbus_pec(I2C_TEST_TARGET.address, I2C_TEST_TARGET.write_register, value);
	}

	return value;
}

/* Zero the IRQ record, so an IRQ assertion describes THIS transfer only. */
static void target_irq_counters_clear(void) {
	I2C_IMSC = 0;
	i2c_v2_state.request_irqs = 0;
	i2c_v2_state.protocol_irqs = 0;
	i2c_v2_state.error_irqs = 0;
	i2c_v2_state.request_status = 0;
	i2c_v2_state.tx_request_status = 0;
	i2c_v2_state.rx_request_status = 0;
	i2c_v2_state.protocol_status = 0;
	i2c_v2_state.error_status = 0;
	i2c_v2_state.total_irqs = 0;
	i2c_v2_state.last_irq = 0;
}

static void wait_for_bus_free(void) {
	stopwatch_t start = stopwatch_get();

	while ((I2C_BUSSTAT & I2C_BUSSTAT_BS) != I2C_BUSSTAT_BS_FREE) {
		if (stopwatch_elapsed_ms(start) >= I2C_DMA_TIMEOUT_MS)
			return;
		test_watchdog_serve();
	}
}

static i2c_v2_result_t target_read_repeated_start(uint8_t reg, uint8_t *data, uint32_t size, bool *bus_was_held) {
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG &= ~I2C_ADDRCFG_SOPE;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	i2c_v2_result_t result = i2c_v2_transfer_bytes_running(I2C_TEST_TARGET.address, &reg, NULL, 1);
	*bus_was_held = (
		result == I2C_V2_DONE &&
		(I2C_BUSSTAT & I2C_BUSSTAT_BS) == I2C_BUSSTAT_BS_BUSY_MASTER
	);
	if (result == I2C_V2_DONE)
		result = i2c_v2_transfer_bytes_running(I2C_TEST_TARGET.address, NULL, data, size);

	I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;
	wait_for_bus_free();

	I2C_RUNCTRL = 0;
	I2C_ADDRCFG |= I2C_ADDRCFG_SOPE;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	return result;
}

static uint32_t read_polled_protocol_status(void) {
	cpu_enable_irq(false);
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG &= ~I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_IMSC = 0;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_MRPSCTRL = 1;
	I2C_TPSCTRL = 1;
	I2C_TXD = (I2C_TEST_TARGET.address << 1) | 1;

	stopwatch_t start = stopwatch_get();
	uint32_t status;
	while (true) {
		status = I2C_PIRQSS;
		if ((status & I2C_READ_COMPLETION_STATUS) == I2C_READ_COMPLETION_STATUS)
			break;
		if ((status & I2C_PIRQSS_NACK) != 0)
			break;
		if (stopwatch_elapsed_ms(start) >= I2C_DMA_TIMEOUT_MS)
			break;
		test_watchdog_serve();
	}

	I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;
	wait_for_bus_free();
	while ((I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0)
		(void) I2C_RXD;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG |= I2C_ADDRCFG_SOPE;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	cpu_enable_irq(true);

	return status;
}

static i2c_v2_result_t dma_target_read(uint8_t reg, uint8_t *data, uint32_t size) {
	i2c_v2_result_t result = i2c_v2_dma_write(I2C_DMA_TX_CHANNEL, I2C_TEST_TARGET.address, &reg, 1);

	if (result != I2C_V2_DONE)
		return result;

	return i2c_v2_dma_read(I2C_DMA_RX_CHANNEL, I2C_TEST_TARGET.address, data, size);
}

static i2c_v2_result_t dma_target_write(uint8_t value) {
	uint8_t data[] = {
		I2C_TEST_TARGET.write_register,
		value,
		i2c_v2_smbus_pec(I2C_TEST_TARGET.address, I2C_TEST_TARGET.write_register, value),
	};
	uint32_t size = I2C_TEST_TARGET.write_uses_pec ? ARRAY_SIZE(data) : ARRAY_SIZE(data) - 1;

	return i2c_v2_dma_write(I2C_DMA_TX_CHANNEL, I2C_TEST_TARGET.address, data, size);
}

static void test_reset_values(void) {
	test_category("Reset values");
	SCU_RST_REQ |= SCU_RST_REQ_I2C;
	SCU_RST_REQ &= ~SCU_RST_REQ_I2C;
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, I2C_CLC);
	I2C_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("RUNCTRL reset value", 0, I2C_RUNCTRL);
	test_eq_u32("ENDDCTRL reset value", 0, I2C_ENDDCTRL);
	test_eq_u32("FDIVCFG reset value", 0, I2C_FDIVCFG);
	test_eq_u32("FDIVHIGHCFG reset value", 0, I2C_FDIVHIGHCFG);
	test_eq_u32("ADDRCFG reset value", 0, I2C_ADDRCFG);
	/* BUSSTAT reflects the live external bus and is not a reset value. */
	test_eq_u32(
		"FIFOCFG reset value",
		I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD,
		I2C_FIFOCFG
	);
	test_eq_u32("MRPSCTRL reset value", 0, I2C_MRPSCTRL);
	test_eq_u32("RPSSTAT reset value", 0, I2C_RPSSTAT);
	test_eq_u32("TPSCTRL reset value", 0, I2C_TPSCTRL);
	test_eq_u32("FFSSTAT reset value", 0, I2C_FFSSTAT);
	/* TIMCFG is not readable on the hardware. */
	test_eq_u32("ERRIRQSM reset value", I2C_ERROR_CLEAR, I2C_ERRIRQSM);
	test_eq_u32("ERRIRQSS reset value", 0, I2C_ERRIRQSS);
	test_eq_u32("PIRQSM reset value", I2C_PROTOCOL_CLEAR, I2C_PIRQSM);
	test_eq_u32("PIRQSS reset value", 0, I2C_PIRQSS);
	test_eq_u32("RIS reset value", 0, I2C_RIS);
	test_eq_u32("IMSC reset value", 0, I2C_IMSC);
	test_eq_u32("MIS reset value", 0, I2C_MIS);
	test_eq_u32("DMAE reset value", 0, I2C_DMAE);
}

static void test_registers(void) {
	test_module_id("module ID", 0xF057C000, I2C_ID);
	test_module_clock("module clock", I2C_CLC);
	test_eq_u32(
		"master mode readback",
		I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE,
		I2C_ADDRCFG & (I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE)
	);
	test_eq_u32(
		"fractional divider readback",
		(0x0A << I2C_FDIVCFG_DEC_SHIFT) | (1 << I2C_FDIVCFG_INC_SHIFT),
		I2C_FDIVCFG
	);
	test_eq_u32(
		"FIFO configuration readback",
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 | I2C_FIFOCFG_RXFC |
			I2C_FIFOCFG_TXFC,
		I2C_FIFOCFG
	);
	test_check("I2C interface is running", (I2C_RUNCTRL & I2C_RUNCTRL_RUN) != 0);
}

static void test_irq_status(void) {
	cpu_enable_irq(false);
	I2C_IMSC = 0;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_ISR = I2C_ISR_SREQ_INT;

	test_check("software IRQ sets raw status", (I2C_RIS & I2C_RIS_SREQ_INT) != 0);
	test_eq_u32("masked IRQ stays hidden", 0, I2C_MIS & I2C_MIS_SREQ_INT);
	I2C_IMSC = I2C_IMSC_SREQ_INT;
	test_check("unmasked IRQ appears in masked status", (I2C_MIS & I2C_MIS_SREQ_INT) != 0);
	I2C_ICR = I2C_ICR_SREQ_INT;
	test_eq_u32("IRQ clear resets raw status", 0, I2C_RIS & I2C_RIS_SREQ_INT);
	test_eq_u32("IRQ clear resets masked status", 0, I2C_MIS & I2C_MIS_SREQ_INT);

	I2C_IMSC = 0;
	cpu_enable_irq(true);
}

static void test_idle_rx_underflow(void) {
	cpu_enable_irq(false);
	I2C_IMSC = 0;
	I2C_RUNCTRL = 0;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;

	(void) I2C_RXD;
	uint32_t error_status = I2C_ERRIRQSS;
	uint32_t protocol_status = I2C_PIRQSS;
	uint32_t bus_status = I2C_BUSSTAT & I2C_BUSSTAT_BS;

	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	cpu_enable_irq(true);

	test_eq_u32("empty RX FIFO read raises RXF_UFL", I2C_ERRIRQSS_RXF_UFL, error_status);
	test_eq_u32("idle RX underflow does not end a transfer", 0, protocol_status);
	test_eq_u32("idle RX underflow leaves the bus free", I2C_BUSSTAT_BS_FREE, bus_status);
}

static void test_tx_fifo_overflow(void) {
	uint32_t saved_fifocfg = I2C_FIFOCFG;

	cpu_enable_irq(false);
	I2C_IMSC = 0;
	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = I2C_FIFOCFG_TXFA_4 | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;

	uint32_t writes = 0;
	while (writes < 16 && (I2C_ERRIRQSS & I2C_ERRIRQSS_TXF_OFL) == 0) {
		I2C_TXD = 0xFE;
		writes++;
	}
	uint32_t fifo_stages = I2C_FFSSTAT & I2C_FFSSTAT_FFS;
	uint32_t error_status = I2C_ERRIRQSS;
	uint32_t protocol_status = I2C_PIRQSS;

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = saved_fifocfg;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	cpu_enable_irq(true);

	test_eq_u32("TX overflow occurs on the tenth word-aligned write", 10, writes);
	test_eq_u32("TX overflow preserves the full FIFO", 8, fifo_stages);
	test_eq_u32("full TX FIFO raises TXF_OFL", I2C_ERRIRQSS_TXF_OFL, error_status);
	test_eq_u32("TX FIFO overflow does not end a transfer", 0, protocol_status);
}

static bool public_target_write(uint8_t value) {
	uint8_t data[] = {
		I2C_TEST_TARGET.write_register,
		value,
	};

	if (I2C_TEST_TARGET.write_uses_pec)
		return i2c_smbus_write_pec_confirmed(I2C_TEST_TARGET.address, data, ARRAY_SIZE(data));

	return i2c_write(I2C_TEST_TARGET.address, data, ARRAY_SIZE(data));
}

static bool public_target_read(uint8_t reg, uint8_t *value) {
	return i2c_write(I2C_TEST_TARGET.address, &reg, 1) && i2c_read(I2C_TEST_TARGET.address, value, 1);
}

static void test_public_polling_driver(void) {
	i2c_init();
	uint8_t reference = 0;
	test_check("public polling read completes", public_target_read(I2C_TEST_TARGET.read_register, &reference));
	test_check("public polling write is accepted", public_target_write(I2C_TEST_TARGET.write_value));
	if (I2C_TEST_TARGET.write_verify == I2C_TEST_WRITE_READBACK) {
		uint8_t actual = 0;
		test_check("public polling write verification completes",
			public_target_read(I2C_TEST_TARGET.write_register, &actual));
		test_eq_u32("public polling write data", I2C_TEST_TARGET.write_value, actual);
	}
	test_check("public polling restore is accepted", public_target_write(I2C_TEST_TARGET.restore_value));
	if (I2C_TEST_TARGET.write_verify == I2C_TEST_WRITE_READBACK) {
		uint8_t actual = 0;
		test_check("public polling restore verification completes",
			public_target_read(I2C_TEST_TARGET.write_register, &actual));
		test_eq_u32("public polling restore data", I2C_TEST_TARGET.restore_value, actual);
	}

	uint8_t value = 0;
	test_check("public polling write reports a missing slave", !i2c_write(0x7F, &value, sizeof(value)));
	test_check("public polling read completes after NACK", public_target_read(I2C_TEST_TARGET.read_register, &value));
	test_eq_u32("public polling read recovers after NACK", reference, value);
}

static void test_target(void) {
	uint8_t actual = 0;

	test_eq_u32(
		"target register read completes",
		I2C_V2_DONE,
		target_read(I2C_TEST_TARGET.read_register, &actual, sizeof(actual))
	);
	test_eq_u32("target read has no controller error", 0, i2c_v2_state.error_status);
	test_check("target read has no NACK", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) == 0);

	/* The interrupt path: completion is delivered only by handle_protocol_irq(). */
	target_irq_counters_clear();
	test_eq_u32("target register write completes", I2C_V2_DONE, target_write(I2C_TEST_TARGET.write_value));
	/* RIS seen by the request handler: the pre-fix storm reads 0x38 (error+protocol set, but
	   protocol_irqs=0, i.e. lost); request bits alone mean the protocol condition never arose. */
	printf("# target write interrupts: request=%u protocol=%u error=%u last=%u protocol_status=%08X "
		"request_status=%08X error_status=%08X\n",
		(unsigned int) i2c_v2_state.request_irqs, (unsigned int) i2c_v2_state.protocol_irqs,
		(unsigned int) i2c_v2_state.error_irqs, (unsigned int) i2c_v2_state.last_irq,
		(unsigned int) i2c_v2_state.protocol_status, (unsigned int) i2c_v2_state.request_status,
		(unsigned int) i2c_v2_state.error_status);
	test_check("target write uses the request IRQ", i2c_v2_state.request_irqs != 0);
	test_check("target write uses the protocol IRQ", i2c_v2_state.protocol_irqs != 0);
	/* Pass: TX_END through the protocol IRQ.  Fail: protocol 0, last 156. */
	test_check("target write raises TX_END through the protocol IRQ",
		(i2c_v2_state.protocol_status & I2C_PIRQSS_TX_END) != 0);
	test_eq_u32("target write's last interrupt is the protocol line", VIC_I2C_PROTOCOL_IRQ,
		i2c_v2_state.last_irq);
	test_eq_u32("target write has no controller error IRQ", 0, i2c_v2_state.error_irqs);
	test_check("target write has no NACK", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) == 0);
	test_eq_u32(
		"target write verification completes",
		I2C_V2_DONE,
		target_write_verification_read(&actual)
	);
	test_eq_u32(
		"target write is accepted",
		target_write_verification_value(I2C_TEST_TARGET.write_value),
		actual
	);
	test_eq_u32("target register restore completes", I2C_V2_DONE, target_write(I2C_TEST_TARGET.restore_value));
	test_eq_u32(
		"target restore verification completes",
		I2C_V2_DONE,
		target_write_verification_read(&actual)
	);
	test_eq_u32(
		"target restore is accepted",
		target_write_verification_value(I2C_TEST_TARGET.restore_value),
		actual
	);
}

static void test_repeated_start(void) {
	uint8_t expected = 0xAA;
	uint8_t actual = 0;
	bool bus_was_held = false;

	test_eq_u32(
		"repeated START reference read completes",
		I2C_V2_DONE,
		target_read(I2C_TEST_TARGET.read_register, &expected, sizeof(expected))
	);
	/* RPSSTAT says how many bytes the reference delivered, not just that it ran. */
	uint32_t reference_bytes = I2C_RPSSTAT & I2C_RPSSTAT_RPS;

	test_eq_u32(
		"combined SMBus read completes",
		I2C_V2_DONE,
		target_read_repeated_start(I2C_TEST_TARGET.read_register, &actual, sizeof(actual), &bus_was_held)
	);
	test_check("bus is held before repeated START", bus_was_held);
	printf("# repeated START: polled reference %02X (RPSSTAT=%u); combined read %02X\n",
		(unsigned int) expected, (unsigned int) reference_bytes, (unsigned int) actual);
	test_eq_u32("repeated START reference delivered a byte", 1, reference_bytes);
	test_eq_u32("repeated START read data", expected, actual);
	test_eq_u32("repeated START read has no controller error IRQ", 0, i2c_v2_state.error_irqs);
	test_eq_u32(
		"combined SMBus read releases the bus",
		I2C_BUSSTAT_BS_FREE,
		I2C_BUSSTAT & I2C_BUSSTAT_BS
	);
}

static void test_polled_protocol_status(void) {
	uint32_t status = read_polled_protocol_status();

	printf("# polled read protocol status: %02lX\n", status);
	test_check("polled target read is acknowledged", (status & I2C_PIRQSS_NACK) == 0);
	test_eq_u32(
		"completed polled read keeps RX asserted with TX_END",
		I2C_READ_COMPLETION_STATUS,
		status & I2C_READ_COMPLETION_STATUS
	);
}

static void test_dma(void) {
	uint8_t actual = 0;
	uint8_t data[9];

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);
	test_eq_u32(
		"MEM2PER_PER target write completes",
		I2C_V2_DONE,
		dma_target_write(I2C_TEST_TARGET.write_value)
	);
	test_check("DMA write reaches terminal count", (DMAC_RAW_TC_STATUS & BIT(I2C_DMA_TX_CHANNEL)) != 0);
	test_eq_u32("DMA write has no bus error", 0, DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_TX_CHANNEL));
	test_eq_u32(
		"DMA write verification completes",
		I2C_V2_DONE,
		target_write_verification_read(&actual)
	);
	test_eq_u32(
		"DMA write is accepted",
		target_write_verification_value(I2C_TEST_TARGET.write_value),
		actual
	);
	test_eq_u32("DMA restore completes", I2C_V2_DONE, dma_target_write(I2C_TEST_TARGET.restore_value));
	test_eq_u32("DMA restore verification completes", I2C_V2_DONE, target_write_verification_read(&actual));
	test_eq_u32(
		"DMA restore is accepted",
		target_write_verification_value(I2C_TEST_TARGET.restore_value),
		actual
	);

	test_eq_u32(
		"PER2MEM_PER four-byte read completes",
		I2C_V2_DONE,
		dma_target_read(I2C_TEST_TARGET.read_register, data, 4)
	);
	test_eq_u32(
		"PER2MEM_PER four-byte read has no DMA error",
		0,
		DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_RX_CHANNEL)
	);
	test_eq_u32(
		"PER2MEM_PER nine-byte read completes",
		I2C_V2_DONE,
		dma_target_read(I2C_TEST_TARGET.read_register, data, sizeof(data))
	);
	test_eq_u32(
		"PER2MEM_PER nine-byte read has no DMA error",
		0,
		DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_RX_CHANNEL)
	);

	test_eq_u32(
		"DMA write reports NACK",
		I2C_V2_NACK,
		i2c_v2_dma_write(I2C_DMA_TX_CHANNEL, 0x7F, &actual, 1)
	);
	test_eq_u32(
		"DMA read reports NACK",
		I2C_V2_NACK,
		i2c_v2_dma_read(I2C_DMA_RX_CHANNEL, 0x7F, &actual, 1)
	);
	test_eq_u32(
		"DMA read recovers after NACK",
		I2C_V2_DONE,
		dma_target_read(I2C_TEST_TARGET.read_register, &actual, 1)
	);
	test_eq_u32("DMA recovery returns target data", data[0], actual);
}

static void test_packet_sizes(void) {
	static const uint8_t sizes[] = {1, 2, 3, 4, 5, 8, 9};
	uint8_t data[9];

	for (uint32_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		memset(data, 0, sizeof(data));
		test_eq_u32(
			"multi-byte SMBus read completes",
			I2C_V2_DONE,
			target_read(I2C_TEST_TARGET.read_register, data, sizes[i])
		);
		test_eq_u32("received packet size", sizes[i], I2C_RPSSTAT & I2C_RPSSTAT_RPS);
		test_eq_u32("multi-byte read has no controller error", 0, i2c_v2_state.error_status);
	}
}

static void test_target_registers(void) {
	uint8_t registers[256];

	memset(registers, 0, sizeof(registers));
	test_eq_u32(
		"256-byte transfer completes",
		I2C_V2_DONE,
		target_read(I2C_TEST_TARGET.read_register, registers, sizeof(registers))
	);
	test_eq_u32("256-byte target packet size", sizeof(registers), I2C_RPSSTAT & I2C_RPSSTAT_RPS);
	test_eq_u32("256-byte target read has no controller error", 0, i2c_v2_state.error_status);

	for (uint32_t row = 0; row < ARRAY_SIZE(registers); row += 16) {
		printf("# %02lX:", row);
		for (uint32_t column = 0; column < 16; column++)
			printf(" %02X", registers[row + column]);
		printf("\n");
	}
}

static void test_burst_sizes(void) {
	static const uint32_t configs[] = {
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD,
		I2C_FIFOCFG_RXBS_2_WORD | I2C_FIFOCFG_TXBS_2_WORD,
		I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD,
		I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD,
	};
	static const uint8_t bursts[] = {1, 2, 4, 4};
	static const uint8_t sizes[] = {5, 13, 21, 32};
	static const uint32_t requests[] = {
		I2C_RIS_BREQ_INT | I2C_RIS_LBREQ_INT,
		I2C_RIS_BREQ_INT | I2C_RIS_LBREQ_INT,
		I2C_RIS_BREQ_INT | I2C_RIS_SREQ_INT | I2C_RIS_LSREQ_INT,
		I2C_RIS_BREQ_INT | I2C_RIS_LBREQ_INT,
	};
	uint8_t data[32];

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = configs[0] | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;

	for (uint32_t i = 0; i < ARRAY_SIZE(configs); i++) {
		I2C_RUNCTRL = 0;
		I2C_FIFOCFG = configs[i] | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
		test_eq_u32(
			"burst target read completes",
			I2C_V2_DONE,
			target_read(I2C_TEST_TARGET.read_register, data, sizes[i])
		);
		test_eq_u32("burst packet size", sizes[i], I2C_RPSSTAT & I2C_RPSSTAT_RPS);
		printf("# RXBS=%u size=%u rx_request_status=%02lX\n", bursts[i], sizes[i], i2c_v2_state.rx_request_status & 0x0F);
		test_eq_u32("burst request sequence", requests[i], i2c_v2_state.rx_request_status);
		test_eq_u32("burst read has no controller error", 0, i2c_v2_state.error_status);
	}
}

static void test_fifo_modes(void) {
	uint8_t data[4];

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_2_WORD | I2C_FIFOCFG_TXBS_2_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);
	test_eq_u32(
		"FIFO ON read completes",
		I2C_V2_DONE,
		target_read(I2C_TEST_TARGET.read_register, data, sizeof(data))
	);
	uint32_t fifo_on_requests = i2c_v2_state.rx_request_status;

	cpu_enable_irq(false);
	I2C_IMSC = 0;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_2_WORD | I2C_FIFOCFG_TXBS_2_WORD | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_MRPSCTRL = 1;
	I2C_TPSCTRL = 1;
	stopwatch_t start = stopwatch_get();
	while ((I2C_RIS & I2C_RIS_LSREQ_INT) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	I2C_TXD = (I2C_TEST_TARGET.address << 1) | 1;
	I2C_ICR = I2C_ICR_LSREQ_INT;

	start = stopwatch_get();
	while ((I2C_RIS & I2C_RIS_SREQ_INT) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	uint32_t fifo_off_requests = I2C_RIS & I2C_RIS_SREQ_INT;
	I2C_ICR = I2C_ICR_SREQ_INT;
	start = stopwatch_get();
	while ((I2C_RIS & I2C_RIS_BREQ_INT) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	fifo_off_requests |= I2C_RIS & I2C_RIS_BREQ_INT;
	uint32_t fifo_off_stages = I2C_FFSSTAT & I2C_FFSSTAT_FFS;
	uint32_t fifo_off_protocol = I2C_PIRQSS;
	I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;
	wait_for_bus_free();
	uint32_t fifo_off_packet_size = I2C_RPSSTAT & I2C_RPSSTAT_RPS;
	while ((I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0)
		(void) I2C_RXD;
	uint32_t fifo_off_error = I2C_ERRIRQSS;

	printf("# FIFO requests: ON=%02lX OFF=%02lX, OFF stages=%lu packet=%lu error=%02lX\n",
		fifo_on_requests, fifo_off_requests, fifo_off_stages, fifo_off_packet_size, fifo_off_error);
	test_eq_u32("FIFO ON uses a last-single request", I2C_RIS_LSREQ_INT, fifo_on_requests);
	test_eq_u32(
		"FIFO OFF uses level-driven single and burst requests",
		I2C_RIS_SREQ_INT | I2C_RIS_BREQ_INT,
		fifo_off_requests
	);
	test_eq_u32("FIFO OFF fills the configured burst", 2, fifo_off_stages);
	test_check("FIFO OFF ignores MRPS", fifo_off_packet_size > 1);
	test_check("FIFO OFF enters receive mode", (fifo_off_protocol & I2C_PIRQSS_RX) != 0);
	test_eq_u32("FIFO OFF SETEND completes the current byte", 9, fifo_off_packet_size);
	test_eq_u32("FIFO OFF has no FIFO error", 0, fifo_off_error);
	test_eq_u32("FIFO OFF SETEND releases the bus", I2C_BUSSTAT_BS_FREE, I2C_BUSSTAT & I2C_BUSSTAT_BS);

	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 |
		I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	cpu_enable_irq(true);
}

static void test_write_transition_with_rx_fifo_disabled(void) {
	uint32_t saved_fifocfg = I2C_FIFOCFG;

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (saved_fifocfg | I2C_FIFOCFG_TXFC) & ~I2C_FIFOCFG_RXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	i2c_v2_result_t result = i2c_v2_transfer_bytes_running(I2C_TEST_TARGET.address, NULL, NULL, 0);
	uint32_t protocol_status = i2c_v2_state.protocol_status;
	uint32_t error_status = i2c_v2_state.error_status;
	uint32_t bus_status = I2C_BUSSTAT & I2C_BUSSTAT_BS;

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = saved_fifocfg;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	test_eq_u32("write completes with RX flow control disabled", I2C_V2_DONE, result);
	test_eq_u32("write completion does not enter receive state", 0, protocol_status & I2C_PIRQSS_RX);
	test_check("write completion raises TX_END", (protocol_status & I2C_PIRQSS_TX_END) != 0);
	test_eq_u32("write completion has no FIFO error", 0, error_status);
	test_eq_u32("write completion releases the bus", I2C_BUSSTAT_BS_FREE, bus_status);
}

static void test_unlimited_flow_controlled_receive(void) {
	cpu_enable_irq(false);
	I2C_IMSC = 0;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_4 |
		I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_MRPSCTRL = 0;
	I2C_TPSCTRL = 1;

	stopwatch_t start = stopwatch_get();
	while ((I2C_RIS & I2C_REQUEST_STATUS) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	uint32_t request = I2C_RIS & I2C_REQUEST_STATUS;
	I2C_TXD = (I2C_TEST_TARGET.address << 1) | 1;
	I2C_ICR = request;

	start = stopwatch_get();
	while ((I2C_PIRQSS & I2C_PIRQSS_RX) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	uint32_t receive_status = I2C_PIRQSS;
	start = stopwatch_get();
	while ((I2C_RIS & I2C_REQUEST_STATUS) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	uint32_t receive_request = I2C_RIS & I2C_REQUEST_STATUS;
	I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;

	start = stopwatch_get();
	while ((I2C_BUSSTAT & I2C_BUSSTAT_BS) != I2C_BUSSTAT_BS_FREE &&
		stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS) {
		while ((I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0)
			(void) I2C_RXD;
		request = I2C_RIS & I2C_REQUEST_STATUS;
		if (request != 0)
			I2C_ICR = request;
		test_watchdog_serve();
	}
	uint32_t protocol_status = I2C_PIRQSS;
	uint32_t error_status = I2C_ERRIRQSS;
	uint32_t packet_size = I2C_RPSSTAT & I2C_RPSSTAT_RPS;
	uint32_t bus_status = I2C_BUSSTAT & I2C_BUSSTAT_BS;

	while ((I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0)
		(void) I2C_RXD;
	I2C_ICR = I2C_RIS & I2C_REQUEST_STATUS;
	I2C_RUNCTRL = 0;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	cpu_enable_irq(true);

	test_check("MRPS zero enters receive state", (receive_status & I2C_PIRQSS_RX) != 0);
	test_eq_u32("unlimited receive uses an ordinary burst request", I2C_RIS_BREQ_INT, receive_request);
	test_check("SETEND completes unlimited receive", (protocol_status & I2C_PIRQSS_TX_END) != 0);
	test_check("unlimited receive transfers data", packet_size != 0);
	test_eq_u32("unlimited receive has no FIFO error", 0, error_status);
	test_eq_u32("unlimited receive releases the bus", I2C_BUSSTAT_BS_FREE, bus_status);
}

static void test_receive_restart_transition(void) {
	cpu_enable_irq(false);
	I2C_IMSC = 0;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_MRPSCTRL = 0;
	I2C_TPSCTRL = 1;

	stopwatch_t start = stopwatch_get();
	while ((I2C_RIS & I2C_RIS_LSREQ_INT) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	I2C_TXD = (I2C_TEST_TARGET.address << 1) | 1;
	I2C_ICR = I2C_ICR_LSREQ_INT;

	start = stopwatch_get();
	while ((I2C_PIRQSS & I2C_PIRQSS_RX) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	I2C_ENDDCTRL = I2C_ENDDCTRL_SETRSC;

	start = stopwatch_get();
	while ((I2C_PIRQSS & I2C_PIRQSS_TX_END) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS) {
		while ((I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0)
			(void) I2C_RXD;
		uint32_t request = I2C_RIS & I2C_REQUEST_STATUS;
		if (request != 0)
			I2C_ICR = request;
		test_watchdog_serve();
	}
	uint32_t packet_size = I2C_RPSSTAT & I2C_RPSSTAT_RPS;
	uint32_t protocol_status = I2C_PIRQSS;
	uint32_t bus_status = I2C_BUSSTAT & I2C_BUSSTAT_BS;
	uint32_t error_status = I2C_ERRIRQSS;

	I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;
	wait_for_bus_free();
	while ((I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0)
		(void) I2C_RXD;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 |
		I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	cpu_enable_irq(true);

	test_eq_u32("SETRSC completes the current receive byte", 1, packet_size);
	test_eq_u32("SETRSC raises RX and TX_END", I2C_PIRQSS_RX | I2C_PIRQSS_TX_END,
		protocol_status & (I2C_PIRQSS_RX | I2C_PIRQSS_TX_END));
	test_eq_u32("SETRSC keeps master control of the bus", I2C_BUSSTAT_BS_BUSY_MASTER, bus_status);
	test_eq_u32("SETRSC receive has no FIFO error", 0, error_status);
}

static void test_tx_non_flow_requests(void) {
	static const struct {
		const char *name;
		uint32_t burst_size;
	} cases[] = {
		{ "one-stage non-flow TX burst request", I2C_FIFOCFG_TXBS_1_WORD },
		{ "two-stage non-flow TX burst request", I2C_FIFOCFG_TXBS_2_WORD },
		{ "four-stage non-flow TX burst request", I2C_FIFOCFG_TXBS_4_WORD },
	};

	cpu_enable_irq(false);
	I2C_IMSC = 0;
	for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++) {
		I2C_RUNCTRL = 0;
		I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
		I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | cases[i].burst_size | I2C_FIFOCFG_RXFC;
		I2C_RUNCTRL = I2C_RUNCTRL_RUN;
		I2C_ICR = I2C_STATUS_CLEAR;
		I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
		I2C_ERRIRQSC = I2C_ERROR_CLEAR;
		I2C_TPSCTRL = 1;

		stopwatch_t start = stopwatch_get();
		while ((I2C_RIS & I2C_RIS_BREQ_INT) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
			test_watchdog_serve();
		uint32_t request = I2C_RIS & I2C_REQUEST_STATUS;
		test_check(cases[i].name, (request & I2C_RIS_BREQ_INT) != 0);
		test_eq_u32("non-flow TX emits no single request", 0,
			request & (I2C_RIS_SREQ_INT | I2C_RIS_LSREQ_INT));

		I2C_RUNCTRL = 0;
	}
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 |
		I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	cpu_enable_irq(true);
}

static void test_rx_overflow(void) {
	uint32_t previous_packet_size = I2C_RPSSTAT & I2C_RPSSTAT_RPS;

	cpu_enable_irq(false);
	I2C_IMSC = 0;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_MRPSCTRL = 1;
	I2C_TPSCTRL = 1;

	stopwatch_t start = stopwatch_get();
	while ((I2C_RIS & I2C_RIS_LSREQ_INT) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	I2C_TXD = (I2C_TEST_TARGET.address << 1) | 1;
	I2C_ICR = I2C_ICR_LSREQ_INT;

	start = stopwatch_get();
	while ((I2C_ERRIRQSS & I2C_ERRIRQSS_RXF_OFL) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	uint32_t error_status = I2C_ERRIRQSS;
	uint32_t protocol_status = I2C_PIRQSS;
	uint32_t fifo_stages = I2C_FFSSTAT & I2C_FFSSTAT_FFS;
	uint32_t packet_size = I2C_RPSSTAT & I2C_RPSSTAT_RPS;
	uint32_t bus_status = I2C_BUSSTAT & I2C_BUSSTAT_BS;
	uint32_t request_status = I2C_RIS & I2C_REQUEST_STATUS;

	printf("# RX overflow before REQCLR: request=%08lX error=%08lX protocol=%08lX stages=%lu "
		"packet=%lu bus=%08lX\n",
		request_status, error_status, protocol_status, fifo_stages, packet_size, bus_status);
	test_check("RX overflow raises RXF_OFL", (error_status & I2C_ERRIRQSS_RXF_OFL) != 0);
	test_eq_u32("RX overflow converts the pending request to last-burst", I2C_RIS_LBREQ_INT, request_status);
	test_eq_u32("RX overflow flushes the FIFO", 0, fifo_stages);
	test_eq_u32("RX overflow leaves the previous RPSSTAT pending REQCLR", previous_packet_size, packet_size);
	test_eq_u32("RX overflow keeps only RX asserted pending REQCLR", I2C_PIRQSS_RX,
		protocol_status & (I2C_PIRQSS_RX | I2C_PIRQSS_TX_END));
	test_eq_u32("RX overflow holds the bus pending REQCLR", I2C_BUSSTAT_BS_BUSY_MASTER, bus_status);

	I2C_ICR = request_status;
	start = stopwatch_get();
	while ((I2C_PIRQSS & I2C_PIRQSS_TX_END) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	request_status = I2C_RIS & I2C_REQUEST_STATUS;
	protocol_status = I2C_PIRQSS;
	fifo_stages = I2C_FFSSTAT & I2C_FFSSTAT_FFS;
	packet_size = I2C_RPSSTAT & I2C_RPSSTAT_RPS;
	bus_status = I2C_BUSSTAT & I2C_BUSSTAT_BS;
	printf("# RX overflow after REQCLR: request=%08lX error=%08lX protocol=%08lX stages=%lu "
		"packet=%lu bus=%08lX\n",
		request_status, I2C_ERRIRQSS, protocol_status, fifo_stages, packet_size, bus_status);
	test_eq_u32("REQCLR clears the overflow request", 0, request_status);
	test_eq_u32("REQCLR completes the overflow transfer", I2C_PIRQSS_RX | I2C_PIRQSS_TX_END,
		protocol_status & (I2C_PIRQSS_RX | I2C_PIRQSS_TX_END));
	test_eq_u32("overflow completion preserves RPSSTAT", previous_packet_size, packet_size);
	test_eq_u32("overflow completion releases the bus", I2C_BUSSTAT_BS_FREE, bus_status);

	while ((I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0)
		(void) I2C_RXD;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 |
		I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	cpu_enable_irq(true);
}

static void test_scan(void) {
	bool complete = true;
	bool target_found = false;
	uint32_t devices = 0;

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);

	for (uint8_t address = 0x03; address <= 0x77; address++) {
		i2c_v2_result_t result = i2c_v2_transfer_bytes(address, NULL, NULL, 0);

		if (result == I2C_V2_DONE) {
			printf("# found I2C device at 0x%02X\n", address);
			devices++;
			target_found |= address == I2C_TEST_TARGET.address;
		} else if (result != I2C_V2_NACK) {
			printf(
				"# I2C scan failed at 0x%02X: result=%u error=%08lX\n",
				address,
				result,
				i2c_v2_state.error_status
			);
			complete = false;
		}
	}

	printf("# found %lu I2C device(s)\n", devices);
	test_check("I2C scan completes", complete);
	printf("# expected target at 0x%02X\n", I2C_TEST_TARGET.address);
	test_check("I2C scan finds the target", target_found);
}

static void test_fifo_alignment(void) {
	static const char *const names[] = {
		"byte RX alignment data",
		"half-word RX alignment data",
		"word RX alignment data",
	};
	static const uint32_t configs[] = {
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 | I2C_FIFOCFG_TXFA_1,
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_2 | I2C_FIFOCFG_TXFA_1,
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_4 | I2C_FIFOCFG_TXFA_1,
	};
	uint8_t expected;
	uint8_t actual;

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = configs[0] | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	test_eq_u32(
		"alignment reference read completes",
		I2C_V2_DONE,
		target_read(I2C_TEST_TARGET.read_register, &expected, sizeof(expected))
	);

	bool completed = true;
	for (uint32_t i = 0; i < ARRAY_SIZE(configs); i++) {
		actual = 0;
		I2C_RUNCTRL = 0;
		I2C_FIFOCFG = configs[i] | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
		test_eq_u32(
			"RX alignment configuration readback",
			configs[i],
			I2C_FIFOCFG & (I2C_FIFOCFG_RXBS | I2C_FIFOCFG_TXBS | I2C_FIFOCFG_RXFA | I2C_FIFOCFG_TXFA)
		);
		completed &= target_read(I2C_TEST_TARGET.read_register, &actual, sizeof(actual)) == I2C_V2_DONE;
		test_eq_u32(names[i], expected, actual);
	}

	test_check("all RX FIFO alignments complete", completed);
	test_eq_u32("last RX FIFO alignment has no errors", 0, i2c_v2_state.error_status);

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = I2C_FIFOCFG_RXFA_4 | I2C_FIFOCFG_TXFA_4;
	test_eq_u32(
		"word RX/TX alignment configuration readback",
		I2C_FIFOCFG_RXFA_4 | I2C_FIFOCFG_TXFA_4,
		I2C_FIFOCFG & (I2C_FIFOCFG_RXFA | I2C_FIFOCFG_TXFA)
	);
}

static void test_nack_recovery(void) {
	uint8_t value = 0;

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);
	i2c_v2_result_t result = i2c_v2_transfer_bytes(0x7F, &value, NULL, 1);
	printf("# NACK transfer: result=%u request=%lu protocol=%lu error=%lu P=%08lX E=%08lX bus=%08lX\n",
		(unsigned int) result, i2c_v2_state.request_irqs, i2c_v2_state.protocol_irqs, i2c_v2_state.error_irqs,
		i2c_v2_state.protocol_status, i2c_v2_state.error_status, I2C_BUSSTAT);
	test_eq_u32("missing slave returns NACK", I2C_V2_NACK, result);
	test_check("NACK protocol status is set", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) != 0);
	test_eq_u32("NACK has no FIFO error", 0, i2c_v2_state.error_irqs);
	test_eq_u32(
		"target read recovers after NACK",
		I2C_V2_DONE,
		target_read(I2C_TEST_TARGET.read_register, &value, sizeof(value))
	);
	test_check("recovered target read has no NACK", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) == 0);
}

static uint32_t nack_transition_bus_status(
	uint32_t behavior,
	uint32_t *protocol_status,
	uint32_t *fifo_stages
) {
	cpu_enable_irq(false);
	I2C_IMSC = 0;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | behavior;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_TXFA_4 |
		I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;

	I2C_TPSCTRL = 1;
	stopwatch_t start = stopwatch_get();
	while ((I2C_RIS & I2C_REQUEST_STATUS) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	uint32_t request = I2C_RIS & I2C_REQUEST_STATUS;
	I2C_TXD = 0x7F << 1;
	I2C_TXD = 0xA5;
	I2C_ICR = request;

	start = stopwatch_get();
	while ((I2C_PIRQSS & (I2C_PIRQSS_NACK | I2C_PIRQSS_TX_END)) !=
		(I2C_PIRQSS_NACK | I2C_PIRQSS_TX_END) && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS) {
		test_watchdog_serve();
	}
	*protocol_status = I2C_PIRQSS;
	*fifo_stages = I2C_FFSSTAT & I2C_FFSSTAT_FFS;
	uint32_t bus_status = I2C_BUSSTAT & I2C_BUSSTAT_BS;

	I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;
	wait_for_bus_free();
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	cpu_enable_irq(true);

	return bus_status;
}

static void test_nack_transitions(void) {
	static const struct {
		const char *name;
		uint32_t behavior;
		uint32_t bus_status;
	} cases[] = {
		{ "NACK without stop options holds the bus", 0, I2C_BUSSTAT_BS_BUSY_MASTER },
		{ "SONA releases the bus without SOPE", I2C_ADDRCFG_SONA, I2C_BUSSTAT_BS_FREE },
		{ "SOPE releases the bus without SONA", I2C_ADDRCFG_SOPE, I2C_BUSSTAT_BS_FREE },
		{ "SONA and SOPE release the bus", I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE, I2C_BUSSTAT_BS_FREE },
	};

	for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++) {
		uint32_t protocol_status;
		uint32_t fifo_stages;
		uint32_t bus_status = nack_transition_bus_status(
			cases[i].behavior,
			&protocol_status,
			&fifo_stages
		);
		printf("# NACK transition: options=%08lX bus=%08lX FIFO stages=%lu\n",
			cases[i].behavior, bus_status, fifo_stages);

		test_eq_u32(
			"NACK raises NACK and TX_END",
			I2C_PIRQSS_NACK | I2C_PIRQSS_TX_END,
			protocol_status & (I2C_PIRQSS_NACK | I2C_PIRQSS_TX_END)
		);
		test_eq_u32("NACK discards remaining transmit FIFO data", 0, fifo_stages);
		test_eq_u32(cases[i].name, cases[i].bus_status, bus_status);
	}
}

int i2c_v2_test(void) {
	test_start("I2Cv2 peripheral test");
	test_reset_values();
	i2c_v2_init();

	test_category("Registers");
	test_registers();
	test_category("IRQ status and masks");
	test_irq_status();
	test_category("Idle RX FIFO underflow");
	test_idle_rx_underflow();
	printf("# target write form: %s\n", I2C_TEST_TARGET.write_uses_pec ? "SMBus PEC" : "plain I2C");
	test_category("I2C target");
	test_target();
	test_category("Repeated START");
	test_repeated_start();
	test_category("Polled protocol status");
	test_polled_protocol_status();
	test_category("Packet sizes");
	test_packet_sizes();
	test_category("I2C target register dump");
	test_target_registers();
	test_category("FIFO burst sizes");
	test_burst_sizes();
	test_category("FIFO ON / FIFO OFF");
	test_fifo_modes();
	test_category("Write state-machine transition");
	test_write_transition_with_rx_fifo_disabled();
	test_category("Unlimited receive");
	test_unlimited_flow_controlled_receive();
	test_category("Receive repeated-START transition");
	test_receive_restart_transition();
	test_category("Non-flow TX requests");
	test_tx_non_flow_requests();
	test_category("RX FIFO overflow");
	test_rx_overflow();
	test_category("Bus scan");
	test_scan();
	test_category("FIFO alignment");
	test_fifo_alignment();
	test_category("Recovery");
	test_nack_recovery();
	test_category("NACK state-machine transitions");
	test_nack_transitions();
	test_category("Public polling driver");
	test_public_polling_driver();
	test_category("TX FIFO overflow");
	test_tx_fifo_overflow();

	return test_finish();
}

int i2c_v2_dma_test(void) {
	test_start("I2Cv2 DMA test");
	i2c_v2_init();
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	dmac_request_select(I2C_DMA_TX_REQUEST, I2C_DMA_TX_SELECT);
	dmac_request_select(I2C_DMA_RX_REQUEST, I2C_DMA_RX_SELECT);

	printf("# target write form: %s\n", I2C_TEST_TARGET.write_uses_pec ? "SMBus PEC" : "plain I2C");
	test_category("DMA target transfers");
	test_dma();

	return test_finish();
}

__IRQ void irq_handler(void) {
	i2c_v2_handle_irq(VIC_IRQ_CURRENT);
	VIC_IRQ_ACK = 1;
}

#else

int i2c_v2_test(void) {
	test_start("I2Cv2 peripheral test");
	test_skip("I2Cv2 tests", "unsupported");

	return test_finish();
}

int i2c_v2_dma_test(void) {
	test_start("I2Cv2 DMA test");
	test_skip("I2Cv2 DMA tests", "unsupported");

	return test_finish();
}

#endif
