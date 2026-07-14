#include <pmb887x.h>

#include "test.h"

#ifdef PMB8876

#include <string.h>

#define PMIC_I2C_ADDR 0x31
#define PMIC_LIGHT_PWM1_REG 0x12
#define PMIC_LED_CONTROL_REG 0x14
#define I2C_STATUS_CLEAR 0x3F
#define I2C_PROTOCOL_CLEAR 0x7F
#define I2C_ERROR_CLEAR 0x0F
#define I2C_DMA_TX_CHANNEL 0
#define I2C_DMA_RX_CHANNEL 1
#define I2C_DMA_TIMEOUT_MS 100
#define I2C_REQUEST_IRQS (I2C_IMSC_LSREQ_INT | I2C_IMSC_SREQ_INT | I2C_IMSC_LBREQ_INT | I2C_IMSC_BREQ_INT)
#define I2C_TRANSFER_IRQS (I2C_REQUEST_IRQS | I2C_IMSC_I2C_ERR_INT | I2C_IMSC_I2C_P_INT)
#define I2C_PROTOCOL_IRQS (I2C_IMSC_I2C_ERR_INT | I2C_IMSC_I2C_P_INT)
#define I2C_DMA_REQUESTS (I2C_DMAE_LSREQ_INT | I2C_DMAE_SREQ_INT | I2C_DMAE_LBREQ_INT | I2C_DMAE_BREQ_INT)

enum transfer_result {
	TRANSFER_PENDING,
	TRANSFER_DONE,
	TRANSFER_NACK,
	TRANSFER_ERROR,
};

static volatile struct transfer_state {
	const uint8_t *tx;
	uint8_t *rx;
	uint32_t remaining;
	uint8_t address;
	bool address_sent;
	bool reading;
	enum transfer_result result;
	uint32_t request_irqs;
	uint32_t protocol_irqs;
	uint32_t error_irqs;
	uint32_t request_status;
	uint32_t tx_request_status;
	uint32_t rx_request_status;
	uint32_t protocol_status;
	uint32_t error_status;
	uint32_t total_irqs;
	uint32_t last_irq;
} transfer;

static uint32_t dma_words[64] __attribute__((aligned(16)));
static volatile struct dma_state {
	bool active;
	bool started;
} dma_rx;

static bool wait_for_mask(volatile uint32_t *reg, uint32_t mask) {
	stopwatch_t start = stopwatch_get();

	while ((*reg & mask) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();

	return (*reg & mask) != 0;
}

static void reset_dma_channel(uint32_t channel) {
	DMAC_CH_CONFIG(channel) = 0;
	DMAC_TC_CLEAR = BIT(channel);
	DMAC_ERR_CLEAR = BIT(channel);
}

static enum transfer_result dma_write_bytes(uint8_t address, const uint8_t *data, uint32_t size) {
	uint32_t bytes = size + 1;

	if (bytes > sizeof(dma_words))
		return TRANSFER_ERROR;

	memset(dma_words, 0, sizeof(dma_words));
	((uint8_t *) dma_words)[0] = address << 1;
	memcpy((uint8_t *) dma_words + 1, data, size);

	cpu_enable_irq(false);
	I2C_RUNCTRL = 0;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_IMSC = I2C_REQUEST_IRQS;
	I2C_PIRQSM = I2C_PIRQSM_NACK | I2C_PIRQSM_TX_END;
	I2C_ERRIRQSM = I2C_ERROR_CLEAR;
	I2C_DMAE = I2C_DMA_REQUESTS;

	reset_dma_channel(I2C_DMA_TX_CHANNEL);
	DMAC_CH_SRC_ADDR(I2C_DMA_TX_CHANNEL) = (uint32_t) dma_words;
	DMAC_CH_DST_ADDR(I2C_DMA_TX_CHANNEL) = (uint32_t) &I2C_TXD;
	DMAC_CH_CONTROL(I2C_DMA_TX_CHANNEL) = (
		DMAC_CH_CONTROL_SB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(I2C_DMA_TX_CHANNEL) = (
		(9 << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER | DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC |
		DMAC_CH_CONFIG_ENABLE
	);
	I2C_TPSCTRL = bytes;

	bool dma_done = wait_for_mask(&DMAC_RAW_TC_STATUS, BIT(I2C_DMA_TX_CHANNEL));
	bool protocol_done = wait_for_mask(&I2C_PIRQSS, I2C_PIRQSS_NACK | I2C_PIRQSS_TX_END);
	enum transfer_result result = (
		!dma_done || !protocol_done ||
		(DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_TX_CHANNEL)) != 0 ? TRANSFER_ERROR :
		(I2C_PIRQSS & I2C_PIRQSS_NACK) != 0 ? TRANSFER_NACK : TRANSFER_DONE
	);
	I2C_DMAE = 0;
	I2C_IMSC = 0;
	DMAC_CH_CONFIG(I2C_DMA_TX_CHANNEL) = 0;
	cpu_enable_irq(true);

	return result;
}

static enum transfer_result dma_read_bytes(uint8_t address, uint8_t *data, uint32_t size) {
	if (size > sizeof(dma_words))
		return TRANSFER_ERROR;

	memset(dma_words, 0, sizeof(dma_words));
	reset_dma_channel(I2C_DMA_RX_CHANNEL);
	DMAC_CH_SRC_ADDR(I2C_DMA_RX_CHANNEL) = (uint32_t) &I2C_RXD;
	DMAC_CH_DST_ADDR(I2C_DMA_RX_CHANNEL) = (uint32_t) dma_words;
	DMAC_CH_CONTROL(I2C_DMA_RX_CHANNEL) = (
		DMAC_CH_CONTROL_SB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(I2C_DMA_RX_CHANNEL) = (
		(9 << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER | DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC
	);

	transfer = (struct transfer_state) {.reading = true, .result = TRANSFER_PENDING};
	dma_rx = (struct dma_state) {.active = true};
	VIC_CON(VIC_I2C_SINGLE_REQ_IRQ) = 0;
	VIC_CON(VIC_I2C_BURST_REQ_IRQ) = 0;
	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_IMSC = I2C_PROTOCOL_IRQS;
	I2C_PIRQSM = I2C_PIRQSM_NACK | I2C_PIRQSM_TX_END | I2C_PIRQSM_RX;
	I2C_ERRIRQSM = I2C_ERROR_CLEAR;
	I2C_DMAE = 0;
	I2C_MRPSCTRL = size;
	I2C_TPSCTRL = 1;
	I2C_TXD = (address << 1) | 1;
	I2C_ICR = I2C_STATUS_CLEAR;

	stopwatch_t start = stopwatch_get();
	while (!dma_rx.started && transfer.result == TRANSFER_PENDING &&
		stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	start = stopwatch_get();
	while (dma_rx.started && transfer.result == TRANSFER_PENDING &&
		(DMAC_RAW_TC_STATUS & BIT(I2C_DMA_RX_CHANNEL)) == 0 && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	bool dma_done = (DMAC_RAW_TC_STATUS & BIT(I2C_DMA_RX_CHANNEL)) != 0;
	start = stopwatch_get();
	while (dma_done && transfer.result == TRANSFER_PENDING && stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();
	enum transfer_result result = transfer.result;
	if (result != TRANSFER_NACK && (!dma_done || (DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_RX_CHANNEL)) != 0))
		result = TRANSFER_ERROR;
	if (result == TRANSFER_DONE)
		memcpy(data, dma_words, size);
	dma_rx.active = false;
	I2C_DMAE = 0;
	I2C_IMSC = 0;
	DMAC_CH_CONFIG(I2C_DMA_RX_CHANNEL) = 0;
	VIC_CON(VIC_I2C_SINGLE_REQ_IRQ) = 1;
	VIC_CON(VIC_I2C_BURST_REQ_IRQ) = 1;

	return result;
}

static void write_fifo(void) {
	uint32_t alignment = 1 << ((I2C_FIFOCFG & I2C_FIFOCFG_TXFA) >> I2C_FIFOCFG_TXFA_SHIFT);
	uint32_t capacity = sizeof(uint32_t) / alignment;

	while (!transfer.address_sent || (!transfer.reading && transfer.remaining != 0)) {
		uint32_t value = 0;
		uint32_t offset = 0;

		if (!transfer.address_sent) {
			value = transfer.address;
			transfer.address_sent = true;
			offset = 1;
		}

		while (!transfer.reading && offset < capacity && transfer.remaining != 0) {
			value |= (uint32_t) *transfer.tx++ << (offset * alignment * 8);
			transfer.remaining--;
			offset++;
		}

		I2C_TXD = value;
	}
}

static void read_fifo(uint32_t stages) {
	uint32_t available = I2C_FFSSTAT & I2C_FFSSTAT_FFS;

	if (stages > available)
		stages = available;

	while (stages-- != 0 && transfer.remaining != 0) {
		uint32_t value = I2C_RXD;
		uint32_t alignment = 1 << ((I2C_FIFOCFG & I2C_FIFOCFG_RXFA) >> I2C_FIFOCFG_RXFA_SHIFT);
		uint32_t capacity = sizeof(value) / alignment;
		uint32_t bytes = transfer.remaining < capacity ? transfer.remaining : capacity;

		for (uint32_t offset = 0; offset < bytes; offset++)
			*transfer.rx++ = value >> (offset * alignment * 8);
		transfer.remaining -= bytes;
	}
}

static enum transfer_result transfer_bytes_running(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size) {
	transfer = (struct transfer_state) {
		.tx = tx,
		.rx = rx,
		.remaining = size,
		.address = (address << 1) | (rx != NULL),
		.reading = rx != NULL,
		.result = TRANSFER_PENDING,
	};

	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_IMSC = I2C_TRANSFER_IRQS;
	I2C_PIRQSM = I2C_PIRQSM_NACK | I2C_PIRQSM_TX_END | I2C_PIRQSM_RX;
	I2C_ERRIRQSM = I2C_ERROR_CLEAR;

	if (transfer.reading) {
		I2C_MRPSCTRL = size;
		I2C_TPSCTRL = 1;
	} else {
		I2C_TPSCTRL = size + 1;
	}
	if ((I2C_FIFOCFG & I2C_FIFOCFG_TXFC) == 0)
		write_fifo();

	stopwatch_t start = stopwatch_get();
	while (transfer.result == TRANSFER_PENDING && stopwatch_elapsed_ms(start) < 100) {
		test_watchdog_serve();
	}

	return transfer.result;
}

static enum transfer_result transfer_bytes(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size) {
	I2C_RUNCTRL = 0;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	return transfer_bytes_running(address, tx, rx, size);
}

static enum transfer_result smbus_read(uint8_t reg, uint8_t *data, uint32_t size) {
	if (transfer_bytes(PMIC_I2C_ADDR, &reg, NULL, 1) != TRANSFER_DONE)
		return transfer.result;

	return transfer_bytes(PMIC_I2C_ADDR, NULL, data, size);
}

static enum transfer_result smbus_write(uint8_t reg, uint8_t value) {
	uint8_t data[] = {reg, value};

	return transfer_bytes(PMIC_I2C_ADDR, data, NULL, sizeof(data));
}

static enum transfer_result smbus_read_repeated_start(uint8_t reg, uint8_t *data, uint32_t size,
	bool *bus_was_held) {
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG &= ~I2C_ADDRCFG_SOPE;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	enum transfer_result result = transfer_bytes_running(PMIC_I2C_ADDR, &reg, NULL, 1);
	*bus_was_held = (
		result == TRANSFER_DONE &&
		(I2C_BUSSTAT & I2C_BUSSTAT_BS) == I2C_BUSSTAT_BS_BUSY_MASTER
	);
	if (result == TRANSFER_DONE)
		result = transfer_bytes_running(PMIC_I2C_ADDR, NULL, data, size);

	I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;
	stopwatch_t start = stopwatch_get();
	while ((I2C_BUSSTAT & I2C_BUSSTAT_BS) != I2C_BUSSTAT_BS_FREE &&
		stopwatch_elapsed_ms(start) < I2C_DMA_TIMEOUT_MS)
		test_watchdog_serve();

	I2C_RUNCTRL = 0;
	I2C_ADDRCFG |= I2C_ADDRCFG_SOPE;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	return result;
}

static enum transfer_result dma_smbus_read(uint8_t reg, uint8_t *data, uint32_t size) {
	enum transfer_result result = dma_write_bytes(PMIC_I2C_ADDR, &reg, 1);

	if (result != TRANSFER_DONE)
		return result;

	return dma_read_bytes(PMIC_I2C_ADDR, data, size);
}

static enum transfer_result dma_smbus_write(uint8_t reg, uint8_t value) {
	uint8_t data[] = {reg, value};

	return dma_write_bytes(PMIC_I2C_ADDR, data, sizeof(data));
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
		(0x3D << I2C_FDIVCFG_DEC_SHIFT) | (4 << I2C_FDIVCFG_INC_SHIFT),
		I2C_FDIVCFG
	);
	test_eq_u32(
		"FIFO configuration readback",
		I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC,
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
		TRANSFER_DONE,
		smbus_read(PMIC_LIGHT_PWM1_REG, &before, sizeof(before))
	);
	test_check("PMIC read uses request IRQ", transfer.request_irqs != 0);
	test_check("PMIC read uses protocol IRQ", transfer.protocol_irqs != 0);
	test_eq_u32("PMIC read has no controller error IRQ", 0, transfer.error_irqs);
	test_check("PMIC read has no NACK", (transfer.protocol_status & I2C_PIRQSS_NACK) == 0);

	test_eq_u32(
		"PMIC LED control read completes",
		TRANSFER_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &before, sizeof(before))
	);
	test_eq_u32("PMIC SMBus write completes", TRANSFER_DONE, smbus_write(PMIC_LED_CONTROL_REG, before));
	test_eq_u32("PMIC write has no controller error IRQ", 0, transfer.error_irqs);
	test_check("PMIC write has no NACK", (transfer.protocol_status & I2C_PIRQSS_NACK) == 0);

	test_eq_u32(
		"PMIC SMBus readback completes",
		TRANSFER_DONE,
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
		TRANSFER_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &expected, sizeof(expected))
	);
	test_eq_u32(
		"combined SMBus read completes",
		TRANSFER_DONE,
		smbus_read_repeated_start(PMIC_LED_CONTROL_REG, &actual, sizeof(actual), &bus_was_held)
	);
	test_check("bus is held before repeated START", bus_was_held);
	test_eq_u32("repeated START read data", expected, actual);
	test_eq_u32("repeated START read has no controller error IRQ", 0, transfer.error_irqs);
	test_eq_u32(
		"combined SMBus read releases the bus",
		I2C_BUSSTAT_BS_FREE,
		I2C_BUSSTAT & I2C_BUSSTAT_BS
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
		TRANSFER_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &led_control, 1)
	);
	test_eq_u32(
		"MEM2PER_PER SMBus write completes",
		TRANSFER_DONE,
		dma_smbus_write(PMIC_LED_CONTROL_REG, led_control)
	);
	test_check("DMA write reaches terminal count", (DMAC_RAW_TC_STATUS & BIT(I2C_DMA_TX_CHANNEL)) != 0);
	test_eq_u32("DMA write has no bus error", 0, DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_TX_CHANNEL));
	test_eq_u32(
		"DMA write IRQ readback completes",
		TRANSFER_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &actual, 1)
	);
	test_eq_u32("DMA write preserves register value", led_control, actual);

	test_eq_u32(
		"PER2MEM_PER four-byte read completes",
		TRANSFER_DONE,
		dma_smbus_read(0, data, 4)
	);
	test_eq_u32(
		"PER2MEM_PER four-byte read has no DMA error",
		0,
		DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_RX_CHANNEL)
	);
	test_eq_u32(
		"PER2MEM_PER nine-byte read completes",
		TRANSFER_DONE,
		dma_smbus_read(0, data, sizeof(data))
	);
	test_eq_u32(
		"PER2MEM_PER nine-byte read has no DMA error",
		0,
		DMAC_RAW_ERR_STATUS & BIT(I2C_DMA_RX_CHANNEL)
	);

	test_eq_u32("DMA write reports NACK", TRANSFER_NACK, dma_write_bytes(0x7F, &actual, 1));
	test_eq_u32("DMA read reports NACK", TRANSFER_NACK, dma_read_bytes(0x7F, &actual, 1));
	test_eq_u32(
		"DMA read recovers after NACK",
		TRANSFER_DONE,
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
			TRANSFER_DONE,
			smbus_read(PMIC_LIGHT_PWM1_REG, data, sizes[i])
		);
		test_eq_u32("received packet size", sizes[i], I2C_RPSSTAT & I2C_RPSSTAT_RPS);
		test_eq_u32("multi-byte read has no controller error IRQ", 0, transfer.error_irqs);
	}
}

static void test_pmic_registers(void) {
	uint8_t registers[256];

	memset(registers, 0, sizeof(registers));
	test_eq_u32(
		"all 256 PMIC registers are readable",
		TRANSFER_DONE,
		smbus_read(0, registers, sizeof(registers))
	);
	test_eq_u32("256-byte PMIC packet size", sizeof(registers), I2C_RPSSTAT & I2C_RPSSTAT_RPS);
	test_eq_u32("256-byte PMIC read has no controller error IRQ", 0, transfer.error_irqs);

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
		test_eq_u32("burst SMBus read completes", TRANSFER_DONE, smbus_read(0, data, sizes[i]));
		test_eq_u32("burst received packet size", sizes[i], I2C_RPSSTAT & I2C_RPSSTAT_RPS);
		printf("# RXBS=%u size=%u RX requests=%02X\n", bursts[i], sizes[i], transfer.rx_request_status & 0x0F);
		test_eq_u32("burst request sequence", requests[i], transfer.rx_request_status & 0x0F);
		test_eq_u32("burst read has no controller error IRQ", 0, transfer.error_irqs);
	}
}

static void test_fifo_modes(void) {
	uint8_t data[4];

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_2_WORD | I2C_FIFOCFG_TXBS_2_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);
	test_eq_u32("FIFO ON read completes", TRANSFER_DONE, smbus_read(0, data, sizeof(data)));
	uint32_t fifo_on_requests = transfer.rx_request_status;

	I2C_RUNCTRL = 0;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_2_WORD | I2C_FIFOCFG_TXBS_2_WORD | I2C_FIFOCFG_TXFC;
	memset(data, 0, sizeof(data));
	test_eq_u32("FIFO OFF read completes", TRANSFER_DONE, smbus_read(0, data, sizeof(data)));
	test_eq_u32(
		"FIFO OFF request sequence",
		I2C_RIS_SREQ_INT,
		transfer.rx_request_status & 0x0F
	);
	test_eq_u32(
		"FIFO OFF releases the bus",
		I2C_BUSSTAT_BS_FREE,
		I2C_BUSSTAT & I2C_BUSSTAT_BS
	);
	test_check("FIFO ON and FIFO OFF use different requests", fifo_on_requests != transfer.rx_request_status);
	test_eq_u32("FIFO OFF raises one FIFO status IRQ", 1, transfer.error_irqs);
	test_eq_u32(
		"FIFO OFF reports RX FIFO overflow",
		I2C_ERRIRQSS_RXF_OFL,
		transfer.error_status
	);
	printf(
		"# FIFO requests: ON=%02X OFF=%02X\n",
		fifo_on_requests & 0x0F,
		transfer.rx_request_status & 0x0F
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
		enum transfer_result result = transfer_bytes(address, NULL, NULL, 0);

		if (result == TRANSFER_DONE) {
			printf("# found I2C device at 0x%02X\n", address);
			devices++;
			pmic_found |= address == PMIC_I2C_ADDR;
		} else if (result != TRANSFER_NACK) {
			printf(
				"# I2C scan failed at 0x%02X: result=%u error=%08X\n",
				address,
				result,
				transfer.error_status
			);
			complete = false;
		}
	}

	printf("# found %u I2C device(s)\n", devices);
	test_check("I2C scan completes", complete);
	test_check("I2C scan finds PMIC at 0x31", pmic_found);
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
		TRANSFER_DONE,
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
		completed &= smbus_read(PMIC_LIGHT_PWM1_REG, &actual, sizeof(actual)) == TRANSFER_DONE;
		no_errors &= transfer.error_irqs == 0;
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
	test_eq_u32("missing slave returns NACK", TRANSFER_NACK, transfer_bytes(0x7F, &value, NULL, 1));
	test_check("NACK protocol status is set", (transfer.protocol_status & I2C_PIRQSS_NACK) != 0);
	test_eq_u32("NACK has no FIFO error", 0, transfer.error_irqs);
	test_eq_u32(
		"PMIC read recovers after NACK",
		TRANSFER_DONE,
		smbus_read(PMIC_LIGHT_PWM1_REG, &value, sizeof(value))
	);
	test_check("recovered PMIC read has no NACK", (transfer.protocol_status & I2C_PIRQSS_NACK) == 0);
}

static void configure_i2c(void) {
	GPIO_PIN(GPIO_I2C_SCL) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(GPIO_I2C_SDA) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	I2C_CLC = 1 << MOD_CLC_RMC_SHIFT;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD | I2C_FIFOCFG_RXFC |
		I2C_FIFOCFG_TXFC
	);
	I2C_FDIVCFG = (0x3D << I2C_FDIVCFG_DEC_SHIFT) | (4 << I2C_FDIVCFG_INC_SHIFT);
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	VIC_CON(VIC_I2C_SINGLE_REQ_IRQ) = 1;
	VIC_CON(VIC_I2C_BURST_REQ_IRQ) = 1;
	VIC_CON(VIC_I2C_ERROR_IRQ) = 1;
	VIC_CON(VIC_I2C_PROTOCOL_IRQ) = 1;
	cpu_enable_irq(true);
}

int i2c_v2_test(void) {
	test_start("I2Cv2 peripheral test");
	test_reset_values();
	configure_i2c();

	test_category("Registers");
	test_registers();
	test_category("IRQ status and masks");
	test_irq_status();
	test_category("PMIC SMBus");
	test_pmic();
	test_category("Repeated START");
	test_repeated_start();
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
	configure_i2c();
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	SCU_DMARS |= BIT(8) | BIT(9);

	test_category("DMA SMBus");
	test_dma();

	return test_finish();
}

static void handle_request_irq(void) {
	uint32_t status = I2C_RIS;

	transfer.request_irqs++;
	transfer.request_status |= status;
	if (transfer.reading && transfer.address_sent && (I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0) {
		uint32_t stages = 1;

		transfer.rx_request_status |= status;
		if ((I2C_FIFOCFG & I2C_FIFOCFG_RXFC) != 0 &&
			(status & (I2C_RIS_LBREQ_INT | I2C_RIS_BREQ_INT)) != 0)
			stages = 1 << ((I2C_FIFOCFG & I2C_FIFOCFG_RXBS) >> I2C_FIFOCFG_RXBS_SHIFT);
		read_fifo(stages);
		if ((I2C_FIFOCFG & I2C_FIFOCFG_RXFC) == 0 && transfer.remaining == 0) {
			I2C_IMSC = I2C_IMSC_I2C_ERR_INT | I2C_IMSC_I2C_P_INT;
			if ((I2C_BUSSTAT & I2C_BUSSTAT_BS) == I2C_BUSSTAT_BS_FREE)
				transfer.result = TRANSFER_DONE;
		}
	} else if (!transfer.reading || !transfer.address_sent) {
		transfer.tx_request_status |= status;
		write_fifo();
	}
	I2C_ICR = status & I2C_STATUS_CLEAR;
}

static void handle_protocol_irq(void) {
	uint32_t status = I2C_PIRQSS;

	transfer.protocol_irqs++;
	transfer.protocol_status |= status;
	if (dma_rx.active && (status & I2C_PIRQSS_RX) != 0) {
		DMAC_CH_CONFIG(I2C_DMA_RX_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;
		I2C_IMSC = I2C_TRANSFER_IRQS;
		I2C_DMAE = I2C_DMA_REQUESTS;
		dma_rx.started = true;
	}
	I2C_PIRQSC = status;
	if ((status & I2C_PIRQSS_NACK) != 0)
		transfer.result = TRANSFER_NACK;
	else if ((status & I2C_PIRQSS_TX_END) != 0)
		transfer.result = TRANSFER_DONE;
}

static void handle_error_irq(void) {
	uint32_t status = I2C_ERRIRQSS;

	transfer.error_irqs++;
	transfer.error_status |= status;
	I2C_ERRIRQSC = status;
	transfer.result = TRANSFER_ERROR;
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	transfer.total_irqs++;
	transfer.last_irq = irq;
	if (irq == VIC_I2C_SINGLE_REQ_IRQ || irq == VIC_I2C_BURST_REQ_IRQ)
		handle_request_irq();
	else if (irq == VIC_I2C_PROTOCOL_IRQ)
		handle_protocol_irq();
	else if (irq == VIC_I2C_ERROR_IRQ)
		handle_error_irq();

	if (transfer.total_irqs > 100) {
		I2C_RUNCTRL = 0;
		I2C_IMSC = 0;
		transfer.result = TRANSFER_ERROR;
	}

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
