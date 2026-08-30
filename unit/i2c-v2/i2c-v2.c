#include <pmb887x.h>

#include "i2c-v2.h"
#include "test.h"

#ifdef PMB8876

#include <string.h>

#if defined(BOARD_HAS_PMIC_PMB6812)
#include <pmic/PMB6812.h>
/* Infineon PMB6812 (e.g. LG KE970, I2C address 0x08). */
#define PMIC_I2C_ADDR PMB6812_I2C_ADDR
#define PMIC_LIGHT_PWM1_REG PMB6812_LEDCTRL2
#define PMIC_LED_CONTROL_REG PMB6812_LEDCTRL1
#elif defined(BOARD_HAS_PMIC_D1094XX)
#include <pmic/D1094XX.h>
/* Dialog D1094xx (e.g. Siemens EL71, I2C address 0x31). */
#define PMIC_I2C_ADDR D1094XX_I2C_ADDR
#define PMIC_LIGHT_PWM1_REG D1094XX_LIGHT_PWM1
#define PMIC_LED_CONTROL_REG D1094XX_LIGHT_CONTROL
#else
#error "i2c-v2 test: unsupported PMIC for this board"
#endif
#define I2C_STATUS_CLEAR 0x3F
#define I2C_PROTOCOL_CLEAR 0x7F
#define I2C_ERROR_CLEAR 0x0F
#define I2C_DMA_TX_CHANNEL 0
#define I2C_DMA_RX_CHANNEL 1
#define I2C_DMA_TIMEOUT_MS 100
#define I2C_READ_COMPLETION_STATUS (I2C_PIRQSS_RX | I2C_PIRQSS_TX_END)

static i2c_v2_result_t smbus_read(uint8_t reg, uint8_t *data, uint32_t size) {
	return i2c_v2_smbus_read(PMIC_I2C_ADDR, reg, data, size);
}

static i2c_v2_result_t smbus_write(uint8_t reg, uint8_t value) {
	return i2c_v2_smbus_write(PMIC_I2C_ADDR, reg, value);
}

static void wait_for_bus_free(void) {
	stopwatch_t start = stopwatch_get();

	while ((I2C_BUSSTAT & I2C_BUSSTAT_BS) != I2C_BUSSTAT_BS_FREE) {
		if (stopwatch_elapsed_ms(start) >= I2C_DMA_TIMEOUT_MS)
			return;
		test_watchdog_serve();
	}
}

static i2c_v2_result_t smbus_read_repeated_start(uint8_t reg, uint8_t *data, uint32_t size, bool *bus_was_held) {
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG &= ~I2C_ADDRCFG_SOPE;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	i2c_v2_result_t result = i2c_v2_transfer_bytes_running(PMIC_I2C_ADDR, &reg, NULL, 1);
	*bus_was_held = (
		result == I2C_V2_DONE &&
		(I2C_BUSSTAT & I2C_BUSSTAT_BS) == I2C_BUSSTAT_BS_BUSY_MASTER
	);
	if (result == I2C_V2_DONE)
		result = i2c_v2_transfer_bytes_running(PMIC_I2C_ADDR, NULL, data, size);

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
	I2C_TXD = (PMIC_I2C_ADDR << 1) | 1;

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

static i2c_v2_result_t dma_smbus_read(uint8_t reg, uint8_t *data, uint32_t size) {
	i2c_v2_result_t result = i2c_v2_dma_write(I2C_DMA_TX_CHANNEL, PMIC_I2C_ADDR, &reg, 1);

	if (result != I2C_V2_DONE)
		return result;

	return i2c_v2_dma_read(I2C_DMA_RX_CHANNEL, PMIC_I2C_ADDR, data, size);
}

static i2c_v2_result_t dma_smbus_write(uint8_t reg, uint8_t value) {
	uint8_t data[] = {reg, value};

	return i2c_v2_dma_write(I2C_DMA_TX_CHANNEL, PMIC_I2C_ADDR, data, sizeof(data));
}

static void test_reset_values(void) {
	test_category("Reset values");
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
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 | I2C_FIFOCFG_RXFC,
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

static void test_pmic(void) {
	uint8_t before = 0;
	uint8_t after = 0;

	test_eq_u32(
		"PMIC SMBus read completes",
		I2C_V2_DONE,
		smbus_read(PMIC_LIGHT_PWM1_REG, &before, sizeof(before))
	);
	test_check("PMIC read uses request IRQ", i2c_v2_state.request_irqs != 0);
	test_check("PMIC read uses protocol IRQ", i2c_v2_state.protocol_irqs != 0);
	test_eq_u32("PMIC read has no controller error IRQ", 0, i2c_v2_state.error_irqs);
	test_check("PMIC read has no NACK", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) == 0);

	test_eq_u32(
		"PMIC LED control read completes",
		I2C_V2_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &before, sizeof(before))
	);
	test_eq_u32("PMIC SMBus write completes", I2C_V2_DONE, smbus_write(PMIC_LED_CONTROL_REG, before));
	test_eq_u32("PMIC write has no controller error IRQ", 0, i2c_v2_state.error_irqs);
	test_check("PMIC write has no NACK", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) == 0);

	test_eq_u32(
		"PMIC SMBus readback completes",
		I2C_V2_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &after, sizeof(after))
	);
	test_eq_u32("PMIC write preserves register value", before, after);
	printf("# PMIC LED_CONTROL: %02X\n", before);
}

static void test_repeated_start(void) {
	uint8_t expected;
	uint8_t actual = 0;
	bool bus_was_held = false;

	test_eq_u32(
		"repeated START reference read completes",
		I2C_V2_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &expected, sizeof(expected))
	);
	test_eq_u32(
		"combined SMBus read completes",
		I2C_V2_DONE,
		smbus_read_repeated_start(PMIC_LED_CONTROL_REG, &actual, sizeof(actual), &bus_was_held)
	);
	test_check("bus is held before repeated START", bus_was_held);
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

	printf("# polled read protocol status: %02X\n", status);
	test_check("polled PMIC read is acknowledged", (status & I2C_PIRQSS_NACK) == 0);
	test_eq_u32(
		"completed polled read keeps RX asserted with TX_END",
		I2C_READ_COMPLETION_STATUS,
		status & I2C_READ_COMPLETION_STATUS
	);
}

static void test_dma(void) {
	uint8_t led_control;
	uint8_t actual = 0;
	uint8_t data[9];

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);
	test_eq_u32(
		"LED control reference read completes",
		I2C_V2_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &led_control, 1)
	);
	test_eq_u32(
		"MEM2PER_PER SMBus write completes",
		I2C_V2_DONE,
		dma_smbus_write(PMIC_LED_CONTROL_REG, led_control)
	);
	test_check("DMA write reaches terminal count", (DMAC_RAW_TC_STATUS & BIT(I2C_DMA_TX_CHANNEL)) != 0);
	test_eq_u32("DMA write has no bus error", 0, DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_TX_CHANNEL));
	test_eq_u32(
		"DMA write IRQ readback completes",
		I2C_V2_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &actual, 1)
	);
	test_eq_u32("DMA write preserves register value", led_control, actual);

	test_eq_u32(
		"PER2MEM_PER four-byte read completes",
		I2C_V2_DONE,
		dma_smbus_read(0, data, 4)
	);
	test_eq_u32(
		"PER2MEM_PER four-byte read has no DMA error",
		0,
		DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_RX_CHANNEL)
	);
	test_eq_u32(
		"PER2MEM_PER nine-byte read completes",
		I2C_V2_DONE,
		dma_smbus_read(0, data, sizeof(data))
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
		dma_smbus_read(PMIC_LED_CONTROL_REG, &actual, 1)
	);
	test_eq_u32("DMA recovery returns PMIC data", led_control, actual);
}

static void test_packet_sizes(void) {
	static const uint8_t sizes[] = {1, 2, 3, 4, 5, 8, 9};
	uint8_t data[9];

	for (uint32_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		memset(data, 0, sizeof(data));
		test_eq_u32(
			"multi-byte SMBus read completes",
			I2C_V2_DONE,
			smbus_read(PMIC_LIGHT_PWM1_REG, data, sizes[i])
		);
		test_eq_u32("received packet size", sizes[i], I2C_RPSSTAT & I2C_RPSSTAT_RPS);
		test_eq_u32("multi-byte read has no controller error IRQ", 0, i2c_v2_state.error_irqs);
	}
}

static void test_pmic_registers(void) {
	uint8_t registers[256];

	memset(registers, 0, sizeof(registers));
	test_eq_u32(
		"all 256 PMIC registers are readable",
		I2C_V2_DONE,
		smbus_read(0, registers, sizeof(registers))
	);
	test_eq_u32("256-byte PMIC packet size", sizeof(registers), I2C_RPSSTAT & I2C_RPSSTAT_RPS);
	test_eq_u32("256-byte PMIC read has no controller error IRQ", 0, i2c_v2_state.error_irqs);

	for (uint32_t row = 0; row < ARRAY_SIZE(registers); row += 16) {
		printf("# %02X:", row);
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
	static const uint8_t requests[] = {
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
		test_eq_u32("burst SMBus read completes", I2C_V2_DONE, smbus_read(0, data, sizes[i]));
		test_eq_u32("burst received packet size", sizes[i], I2C_RPSSTAT & I2C_RPSSTAT_RPS);
		printf("# RXBS=%u size=%u RX requests=%02X\n", bursts[i], sizes[i], i2c_v2_state.rx_request_status & 0x0F);
		test_eq_u32("burst request sequence", requests[i], i2c_v2_state.rx_request_status & 0x0F);
		test_eq_u32("burst read has no controller error IRQ", 0, i2c_v2_state.error_irqs);
	}
}

static void test_fifo_modes(void) {
	uint8_t data[4];

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_2_WORD | I2C_FIFOCFG_TXBS_2_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);
	test_eq_u32("FIFO ON read completes", I2C_V2_DONE, smbus_read(0, data, sizeof(data)));
	uint32_t fifo_on_requests = i2c_v2_state.rx_request_status;

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_2_WORD | I2C_FIFOCFG_TXBS_2_WORD | I2C_FIFOCFG_TXFC;
	memset(data, 0, sizeof(data));
	test_eq_u32("FIFO OFF read completes", I2C_V2_DONE, smbus_read(0, data, sizeof(data)));
	test_eq_u32(
		"FIFO OFF request sequence",
		I2C_RIS_SREQ_INT,
		i2c_v2_state.rx_request_status & 0x0F
	);
	test_eq_u32(
		"FIFO OFF releases the bus",
		I2C_BUSSTAT_BS_FREE,
		I2C_BUSSTAT & I2C_BUSSTAT_BS
	);
	test_check("FIFO ON and FIFO OFF use different requests", fifo_on_requests != i2c_v2_state.rx_request_status);
	test_eq_u32("FIFO OFF raises one FIFO status IRQ", 1, i2c_v2_state.error_irqs);
	test_eq_u32(
		"FIFO OFF reports RX FIFO overflow",
		I2C_ERRIRQSS_RXF_OFL,
		i2c_v2_state.error_status
	);
	printf(
		"# FIFO requests: ON=%02X OFF=%02X\n",
		fifo_on_requests & 0x0F,
		i2c_v2_state.rx_request_status & 0x0F
	);
}

static void test_scan(void) {
	bool complete = true;
	bool pmic_found = false;
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
			pmic_found |= address == PMIC_I2C_ADDR;
		} else if (result != I2C_V2_NACK) {
			printf(
				"# I2C scan failed at 0x%02X: result=%u error=%08X\n",
				address,
				result,
				i2c_v2_state.error_status
			);
			complete = false;
		}
	}

	printf("# found %u I2C device(s)\n", devices);
	test_check("I2C scan completes", complete);
	printf("# expected PMIC at 0x%02X\n", PMIC_I2C_ADDR);
	test_check("I2C scan finds the PMIC", pmic_found);
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
		smbus_read(PMIC_LIGHT_PWM1_REG, &expected, sizeof(expected))
	);

	bool completed = true;
	bool no_errors = true;
	for (uint32_t i = 0; i < ARRAY_SIZE(configs); i++) {
		actual = 0;
		I2C_RUNCTRL = 0;
		I2C_FIFOCFG = configs[i] | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
		test_eq_u32(
			"RX alignment configuration readback",
			configs[i],
			I2C_FIFOCFG & (I2C_FIFOCFG_RXBS | I2C_FIFOCFG_TXBS | I2C_FIFOCFG_RXFA | I2C_FIFOCFG_TXFA)
		);
		completed &= smbus_read(PMIC_LIGHT_PWM1_REG, &actual, sizeof(actual)) == I2C_V2_DONE;
		no_errors &= i2c_v2_state.error_irqs == 0;
		test_eq_u32(names[i], expected, actual);
	}

	test_check("all RX FIFO alignments complete", completed);
	test_check("all RX FIFO alignments have no errors", no_errors);

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
	test_eq_u32("missing slave returns NACK", I2C_V2_NACK, i2c_v2_transfer_bytes(0x7F, &value, NULL, 1));
	test_check("NACK protocol status is set", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) != 0);
	test_eq_u32("NACK has no FIFO error", 0, i2c_v2_state.error_irqs);
	test_eq_u32(
		"PMIC read recovers after NACK",
		I2C_V2_DONE,
		smbus_read(PMIC_LIGHT_PWM1_REG, &value, sizeof(value))
	);
	test_check("recovered PMIC read has no NACK", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) == 0);
}

int i2c_v2_test(void) {
	test_start("I2Cv2 peripheral test");
	test_reset_values();
	i2c_v2_init();

	test_category("Registers");
	test_registers();
	test_category("IRQ status and masks");
	test_irq_status();
	test_category("PMIC SMBus");
	test_pmic();
	test_category("Repeated START");
	test_repeated_start();
	test_category("Polled protocol status");
	test_polled_protocol_status();
	test_category("Packet sizes");
	test_packet_sizes();
	test_category("PMIC register dump");
	test_pmic_registers();
	test_category("FIFO burst sizes");
	test_burst_sizes();
	test_category("FIFO ON / FIFO OFF");
	test_fifo_modes();
	test_category("Bus scan");
	test_scan();
	test_category("FIFO alignment");
	test_fifo_alignment();
	test_category("Recovery");
	test_nack_recovery();

	return test_finish();
}

int i2c_v2_dma_test(void) {
	test_start("I2Cv2 DMA test");
	i2c_v2_init();
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	SCU_DMARS |= BIT(8) | BIT(9);

	test_category("DMA SMBus");
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
