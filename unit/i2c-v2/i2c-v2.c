/* Tests the INTERRUPT path against this board's PMIC.  Pass: protocol_status 0x20
   with last_irq 158; the defect it catches is protocol 0 with last_irq 156. */

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
/* 0x00-0x7F are write-only (read 0xFF): a written value cannot be read back. */
#define PMIC_LED_CONTROL_READABLE 0
/* Data-check source. GEF1 (0x80) is a plain read with no latch/freeze/release and no clear
   (datasheet 4.2 + 3.13.1; release-on-read is documented only for ISF/CHST, 3.13.2), unlike
   the write-only 0x00-0x7F. */
#define PMIC_ALIGNMENT_REG PMB6812_GEF1
/* This part drops every write that does not carry the SMBus PEC byte. */
#define PMIC_NEEDS_PEC 1
#elif defined(BOARD_HAS_PMIC_D1094XX)
#include <pmic/D1094XX.h>
/* Dialog D1094xx (e.g. Siemens EL71, I2C address 0x31). */
#define PMIC_I2C_ADDR D1094XX_I2C_ADDR
#define PMIC_LIGHT_PWM1_REG D1094XX_LIGHT_PWM1
#define PMIC_LED_CONTROL_REG D1094XX_LIGHT_CONTROL
#define PMIC_LED_CONTROL_READABLE 1
/* Light PWM is readable on this part. */
#define PMIC_ALIGNMENT_REG D1094XX_LIGHT_PWM1
/* No PEC on this part: a PEC byte would be a second data byte. */
#define PMIC_NEEDS_PEC 0
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
/* The polled read never populates `i2c_v2_state`: a polled section can only read what an earlier IRQ transfer left. */
#define SKIP_POLLED_STATE "polled i2c_v2_smbus_read() does not populate i2c_v2_state (state is from an earlier IRQ transfer)"

static i2c_v2_result_t smbus_read(uint8_t reg, uint8_t *data, uint32_t size) {
	return i2c_v2_smbus_read(PMIC_I2C_ADDR, reg, data, size);
}

/* `RPSSTAT` reports the final ≤4-byte chunk, not the transfer's total. */
static uint32_t smbus_read_last_packet(uint32_t size) {
	return ((size - 1) % 4) + 1;
}

/* PMB6812 needs the SMBus PEC byte, PASIC/Dialog must not have one (per board, not
   vendor: sl98 is PMB6812, cl61 vs cl61a differ).  CHST would decide it but its read clears CHV/CHMD/CCAL. */
static i2c_v2_result_t pmic_write(uint8_t reg, uint8_t value) {
#if PMIC_NEEDS_PEC
	return i2c_v2_smbus_write_pec(PMIC_I2C_ADDR, reg, value);
#else
	return i2c_v2_smbus_write(PMIC_I2C_ADDR, reg, value);
#endif
}

/* Zero the IRQ record, so an IRQ assertion describes THIS transfer only. */
static void pmic_irq_counters_clear(void) {
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
	/* Same frame as the ISR path; the PEC byte is a compile-time choice. */
#if PMIC_NEEDS_PEC
	uint8_t data[] = {reg, value, i2c_v2_smbus_pec(PMIC_I2C_ADDR, reg, value)};
#else
	uint8_t data[] = {reg, value};
#endif

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
	/* The polled path enters no ISR, so the error/NACK counters below are not its own. */
	test_skip("PMIC read has no controller error IRQ", SKIP_POLLED_STATE);
	test_skip("PMIC read has no NACK", SKIP_POLLED_STATE);

	test_eq_u32(
		"PMIC LED control read completes",
		I2C_V2_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &before, sizeof(before))
	);

	/* The interrupt path: completion is delivered only by handle_protocol_irq(). */
	pmic_irq_counters_clear();
	test_eq_u32("PMIC SMBus write completes", I2C_V2_DONE, pmic_write(PMIC_LED_CONTROL_REG, before));
	/* RIS seen by the request handler: the pre-fix storm reads 0x38 (error+protocol set, but
	   protocol_irqs=0, i.e. lost); request bits alone mean the protocol condition never arose. */
	printf("# PMIC write interrupts: request=%u protocol=%u error=%u last=%u protocol_status=%08X request_status=%08X\n",
		(unsigned int) i2c_v2_state.request_irqs, (unsigned int) i2c_v2_state.protocol_irqs,
		(unsigned int) i2c_v2_state.error_irqs, (unsigned int) i2c_v2_state.last_irq,
		(unsigned int) i2c_v2_state.protocol_status, (unsigned int) i2c_v2_state.request_status);
	test_check("PMIC write uses the request IRQ", i2c_v2_state.request_irqs != 0);
	test_check("PMIC write uses the protocol IRQ", i2c_v2_state.protocol_irqs != 0);
	/* Pass: TX_END through the protocol IRQ.  Fail: protocol 0, last 156. */
	test_check("PMIC write raises TX_END through the protocol IRQ",
		(i2c_v2_state.protocol_status & I2C_PIRQSS_TX_END) != 0);
	test_eq_u32("PMIC write's last interrupt is the protocol line", VIC_I2C_PROTOCOL_IRQ,
		i2c_v2_state.last_irq);
	test_eq_u32("PMIC write has no controller error IRQ", 0, i2c_v2_state.error_irqs);
	test_check("PMIC write has no NACK", (i2c_v2_state.protocol_status & I2C_PIRQSS_NACK) == 0);
	/* Write-only register: the write's result code, not a readback, is the evidence. */
	if (PMIC_LED_CONTROL_READABLE) {
		test_eq_u32(
			"PMIC SMBus readback completes",
			I2C_V2_DONE,
			smbus_read(PMIC_LED_CONTROL_REG, &after, sizeof(after))
		);
		test_eq_u32("PMIC write preserves register value", before, after);
	} else {
		printf("# PMIC write value not compared: register 0x%02X is write-only on this part "
			"(reads 0xFF), so the accepted write above is the evidence\n",
			(unsigned int) PMIC_LED_CONTROL_REG);
	}
	printf("# PMIC LED_CONTROL: %02X\n", before);
}

static void test_repeated_start(void) {
	uint8_t expected = 0xAA;
	uint8_t actual = 0;
	bool bus_was_held = false;

	test_eq_u32(
		"repeated START reference read completes",
		I2C_V2_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &expected, sizeof(expected))
	);
	/* RPSSTAT says how many bytes the reference delivered, not just that it ran. */
	uint32_t reference_bytes = I2C_RPSSTAT & I2C_RPSSTAT_RPS;

	test_eq_u32(
		"combined SMBus read completes",
		I2C_V2_DONE,
		smbus_read_repeated_start(PMIC_LED_CONTROL_REG, &actual, sizeof(actual), &bus_was_held)
	);
	test_check("bus is held before repeated START", bus_was_held);
	printf("# repeated START: polled reference %02X (RPSSTAT=%u); combined read %02X\n",
		(unsigned int) expected, (unsigned int) reference_bytes, (unsigned int) actual);
	test_eq_u32("repeated START reference delivered a byte", 1, reference_bytes);
	/* Compare across paths only where the register has a defined readback. */
	if (PMIC_LED_CONTROL_READABLE) {
		test_eq_u32("repeated START read data", expected, actual);
	} else {
		printf("# repeated START data not compared: register 0x%02X has no defined readback on this part\n",
			(unsigned int) PMIC_LED_CONTROL_REG);
	}
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
	if (PMIC_LED_CONTROL_READABLE) {
		test_eq_u32("DMA write preserves register value", led_control, actual);
	} else {
		test_skip("DMA write preserves register value", "LEDCTRL1 is write-only on this part (reads 0xFF)");
	}

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
	if (PMIC_LED_CONTROL_READABLE) {
		test_eq_u32("DMA recovery returns PMIC data", led_control, actual);
	} else {
		test_skip("DMA recovery returns PMIC data", "LEDCTRL1 is write-only on this part (reads 0xFF)");
	}
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
		test_eq_u32("last received packet size", smbus_read_last_packet(sizes[i]), I2C_RPSSTAT & I2C_RPSSTAT_RPS);
		test_skip("multi-byte read has no controller error IRQ", SKIP_POLLED_STATE);
	}
}

static void test_pmic_registers(void) {
	uint8_t registers[256];

	memset(registers, 0, sizeof(registers));
	test_eq_u32(
		"256-byte transfer completes",
		I2C_V2_DONE,
		smbus_read(0, registers, sizeof(registers))
	);
	test_eq_u32("256-byte PMIC last packet size", smbus_read_last_packet(sizeof(registers)), I2C_RPSSTAT & I2C_RPSSTAT_RPS);
	test_skip("256-byte PMIC read has no controller error IRQ", SKIP_POLLED_STATE);
#if defined(BOARD_HAS_PMIC_PMB6812)
	printf("# transfer spans 0x00-0xFF; only 0x80-0x83 (GEF1/ISF/CHST/GEF2) and 0x88/0x89 can return a value — every other address reads 0xFF from the bus. ISF/CHST are readable but read-releasing (CHST clears CHV/CHMD/CCAL); GEF1 is the non-releasing alignment source.\n");
#endif

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
	uint8_t data[32];

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = configs[0] | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;

	for (uint32_t i = 0; i < ARRAY_SIZE(configs); i++) {
		I2C_RUNCTRL = 0;
		I2C_FIFOCFG = configs[i] | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
		test_eq_u32("burst SMBus read completes", I2C_V2_DONE, smbus_read(0, data, sizes[i]));
		test_eq_u32("burst last packet size", smbus_read_last_packet(sizes[i]), I2C_RPSSTAT & I2C_RPSSTAT_RPS);
		/* Printed as raw state, not as this transfer's requests - see the skip. */
		printf("# RXBS=%u size=%u rx_request_status=%02lX\n", bursts[i], sizes[i], i2c_v2_state.rx_request_status & 0x0F);
		test_skip("burst request sequence", SKIP_POLLED_STATE);
		test_skip("burst read has no controller error IRQ", SKIP_POLLED_STATE);
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
	test_skip("FIFO OFF request sequence", SKIP_POLLED_STATE);
	test_eq_u32(
		"FIFO OFF releases the bus",
		I2C_BUSSTAT_BS_FREE,
		I2C_BUSSTAT & I2C_BUSSTAT_BS
	);
	test_skip("FIFO ON and FIFO OFF use different requests", SKIP_POLLED_STATE);
	test_skip("FIFO OFF raises one FIFO status IRQ", SKIP_POLLED_STATE);
	test_skip("FIFO OFF reports RX FIFO overflow", SKIP_POLLED_STATE);
	/* Raw state, not this transfer's requests - see the skips above. */
	printf(
		"# FIFO requests: ON=%02lX OFF=%02lX\n",
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
		smbus_read(PMIC_ALIGNMENT_REG, &expected, sizeof(expected))
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
		completed &= smbus_read(PMIC_ALIGNMENT_REG, &actual, sizeof(actual)) == I2C_V2_DONE;
		test_eq_u32(names[i], expected, actual);
	}

	test_check("all RX FIFO alignments complete", completed);
	test_skip(
		"all RX FIFO alignments have no errors",
		SKIP_POLLED_STATE
	);

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
	test_skip("recovered PMIC read has no NACK", SKIP_POLLED_STATE);
}

int i2c_v2_test(void) {
	test_start("I2Cv2 peripheral test");
	test_reset_values();
	i2c_v2_init();

	test_category("Registers");
	test_registers();
	test_category("IRQ status and masks");
	test_irq_status();
	printf("# PMIC write form for this board: %s\n", PMIC_NEEDS_PEC ? "SMBus PEC (PMB6812)" : "plain, no PEC (PASIC/Dialog)");
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

	printf("# PMIC write form for this board: %s\n", PMIC_NEEDS_PEC ? "SMBus PEC (PMB6812)" : "plain, no PEC (PASIC/Dialog)");
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
