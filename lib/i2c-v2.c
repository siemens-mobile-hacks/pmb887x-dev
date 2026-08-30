#include <pmb887x.h>

#include "i2c-v2.h"

#ifdef PMB8876

#include <string.h>

#define I2C_STATUS_CLEAR 0x3F
#define I2C_PROTOCOL_CLEAR 0x7F
#define I2C_ERROR_CLEAR 0x0F
#define I2C_TIMEOUT_MS 100
#define I2C_DMA_PERIPH_ID 9
#define I2C_REQUEST_IRQS (I2C_IMSC_LSREQ_INT | I2C_IMSC_SREQ_INT | I2C_IMSC_LBREQ_INT | I2C_IMSC_BREQ_INT)
#define I2C_TRANSFER_IRQS (I2C_REQUEST_IRQS | I2C_IMSC_I2C_ERR_INT | I2C_IMSC_I2C_P_INT)
#define I2C_PROTOCOL_IRQS (I2C_IMSC_I2C_ERR_INT | I2C_IMSC_I2C_P_INT)
#define I2C_DMA_REQUESTS (I2C_DMAE_LSREQ_INT | I2C_DMAE_SREQ_INT | I2C_DMAE_LBREQ_INT | I2C_DMAE_BREQ_INT)

volatile struct i2c_v2_state i2c_v2_state;

static uint32_t dma_words[64] __attribute__((aligned(16)));
static volatile struct dma_state {
	bool active;
	bool started;
	uint32_t rx_channel;
} dma_rx;

static bool wait_for_mask(volatile uint32_t *reg, uint32_t mask) {
	stopwatch_t start = stopwatch_get();

	while ((*reg & mask) == 0 && stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
		wdt_serve();

	return (*reg & mask) != 0;
}

static void reset_dma_channel(uint32_t channel) {
	DMAC_CH_CONFIG(channel) = 0;
	DMAC_TC_CLEAR = BIT(channel);
	DMAC_ERR_CLEAR = BIT(channel);
}

i2c_v2_result_t i2c_v2_dma_write(uint32_t channel, uint8_t address, const uint8_t *data, uint32_t size) {
	uint32_t bytes = size + 1;

	if (bytes > sizeof(dma_words))
		return I2C_V2_ERROR;

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

	reset_dma_channel(channel);
	DMAC_CH_SRC_ADDR(channel) = (uint32_t) dma_words;
	DMAC_CH_DST_ADDR(channel) = (uint32_t) &I2C_TXD;
	DMAC_CH_CONTROL(channel) = (
		DMAC_CH_CONTROL_SB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(channel) = (
		(I2C_DMA_PERIPH_ID << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER | DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC |
		DMAC_CH_CONFIG_ENABLE
	);
	I2C_TPSCTRL = bytes;

	bool dma_done = wait_for_mask(&DMAC_RAW_TC_STATUS, BIT(channel));
	bool protocol_done = wait_for_mask(&I2C_PIRQSS, I2C_PIRQSS_NACK | I2C_PIRQSS_TX_END);
	i2c_v2_result_t result = (
		!dma_done || !protocol_done ||
		(DMAC_RAW_ERR_STATUS & BIT(channel)) != 0 ? I2C_V2_ERROR :
		(I2C_PIRQSS & I2C_PIRQSS_NACK) != 0 ? I2C_V2_NACK : I2C_V2_DONE
	);
	I2C_DMAE = 0;
	I2C_IMSC = 0;
	DMAC_CH_CONFIG(channel) = 0;
	cpu_enable_irq(true);

	return result;
}

i2c_v2_result_t i2c_v2_dma_read(uint32_t channel, uint8_t address, uint8_t *data, uint32_t size) {
	if (size > sizeof(dma_words))
		return I2C_V2_ERROR;

	memset(dma_words, 0, sizeof(dma_words));
	reset_dma_channel(channel);
	DMAC_CH_SRC_ADDR(channel) = (uint32_t) &I2C_RXD;
	DMAC_CH_DST_ADDR(channel) = (uint32_t) dma_words;
	DMAC_CH_CONTROL(channel) = (
		DMAC_CH_CONTROL_SB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(channel) = (
		(I2C_DMA_PERIPH_ID << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER | DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC
	);

	i2c_v2_state = (struct i2c_v2_state) {.reading = true, .result = I2C_V2_PENDING};
	dma_rx = (struct dma_state) {.active = true, .rx_channel = channel};
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
	while (!dma_rx.started && i2c_v2_state.result == I2C_V2_PENDING &&
		stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
		wdt_serve();
	start = stopwatch_get();
	while (dma_rx.started && i2c_v2_state.result == I2C_V2_PENDING &&
		(DMAC_RAW_TC_STATUS & BIT(channel)) == 0 && stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
		wdt_serve();
	bool dma_done = (DMAC_RAW_TC_STATUS & BIT(channel)) != 0;
	start = stopwatch_get();
	while (dma_done && i2c_v2_state.result == I2C_V2_PENDING && stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
		wdt_serve();
	i2c_v2_result_t result = i2c_v2_state.result;
	if (result != I2C_V2_NACK && (!dma_done || (DMAC_RAW_ERR_STATUS & BIT(channel)) != 0))
		result = I2C_V2_ERROR;
	if (result == I2C_V2_DONE)
		memcpy(data, dma_words, size);
	dma_rx.active = false;
	I2C_DMAE = 0;
	I2C_IMSC = 0;
	DMAC_CH_CONFIG(channel) = 0;
	VIC_CON(VIC_I2C_SINGLE_REQ_IRQ) = 1;
	VIC_CON(VIC_I2C_BURST_REQ_IRQ) = 1;

	return result;
}

static void write_fifo(void) {
	uint32_t alignment = 1 << ((I2C_FIFOCFG & I2C_FIFOCFG_TXFA) >> I2C_FIFOCFG_TXFA_SHIFT);
	uint32_t capacity = sizeof(uint32_t) / alignment;

	while (!i2c_v2_state.address_sent || (!i2c_v2_state.reading && i2c_v2_state.remaining != 0)) {
		uint32_t value = 0;
		uint32_t offset = 0;

		if (!i2c_v2_state.address_sent) {
			value = i2c_v2_state.address;
			i2c_v2_state.address_sent = true;
			offset = 1;
		}

		while (!i2c_v2_state.reading && offset < capacity && i2c_v2_state.remaining != 0) {
			value |= (uint32_t) *i2c_v2_state.tx++ << (offset * alignment * 8);
			i2c_v2_state.remaining--;
			offset++;
		}

		I2C_TXD = value;
	}
}

static void read_fifo(uint32_t stages) {
	uint32_t available = I2C_FFSSTAT & I2C_FFSSTAT_FFS;

	if (stages > available)
		stages = available;

	while (stages-- != 0 && i2c_v2_state.remaining != 0) {
		uint32_t value = I2C_RXD;
		uint32_t alignment = 1 << ((I2C_FIFOCFG & I2C_FIFOCFG_RXFA) >> I2C_FIFOCFG_RXFA_SHIFT);
		uint32_t capacity = sizeof(value) / alignment;
		uint32_t bytes = i2c_v2_state.remaining < capacity ? i2c_v2_state.remaining : capacity;

		for (uint32_t offset = 0; offset < bytes; offset++)
			*i2c_v2_state.rx++ = value >> (offset * alignment * 8);
		i2c_v2_state.remaining -= bytes;
	}
}

i2c_v2_result_t i2c_v2_transfer_bytes_running(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size) {
	i2c_v2_state = (struct i2c_v2_state) {
		.tx = tx,
		.rx = rx,
		.remaining = size,
		.address = (address << 1) | (rx != NULL),
		.reading = rx != NULL,
		.result = I2C_V2_PENDING,
	};

	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_IMSC = I2C_TRANSFER_IRQS;
	I2C_PIRQSM = I2C_PIRQSM_NACK | I2C_PIRQSM_TX_END | I2C_PIRQSM_RX;
	I2C_ERRIRQSM = I2C_ERROR_CLEAR;

	if (i2c_v2_state.reading) {
		I2C_MRPSCTRL = size;
		I2C_TPSCTRL = 1;
		if ((I2C_FIFOCFG & I2C_FIFOCFG_TXFC) == 0)
			write_fifo();
	} else {
		I2C_TPSCTRL = size + 1;
		write_fifo();
	}

	stopwatch_t start = stopwatch_get();
	while (i2c_v2_state.result == I2C_V2_PENDING && stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
		wdt_serve();

	return i2c_v2_state.result;
}

i2c_v2_result_t i2c_v2_transfer_bytes(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size) {
	I2C_RUNCTRL = 0;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	return i2c_v2_transfer_bytes_running(address, tx, rx, size);
}

bool i2c_v2_transfer(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size) {
	return i2c_v2_transfer_bytes(address, tx, rx, size) == I2C_V2_DONE;
}

i2c_v2_result_t i2c_v2_smbus_read(uint8_t address, uint8_t reg, uint8_t *data, uint32_t size) {
	uint32_t saved_addrcfg = I2C_ADDRCFG;

	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;

	I2C_TPSCTRL = 2;
	I2C_TXD = ((uint32_t) reg << 8) | (uint32_t) (address << 1);
	stopwatch_t start = stopwatch_get();
	while (!(I2C_PIRQSS & (I2C_PIRQSS_TX_END | I2C_PIRQSS_NACK)) &&
		stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
		wdt_serve();
	i2c_v2_result_t result = (I2C_PIRQSS & I2C_PIRQSS_NACK) != 0 ? I2C_V2_NACK : I2C_V2_DONE;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;

	uint8_t *dst = data;
	uint32_t remaining = size;
	while (result == I2C_V2_DONE && remaining > 0) {
		uint32_t chunk = remaining < 4 ? remaining : 4;

		I2C_TPSCTRL = 1;
		I2C_MRPSCTRL = chunk;
		I2C_TXD = (uint32_t) (address << 1) | 1;   // repeated START into the read

		start = stopwatch_get();
		while ((I2C_PIRQSS & (I2C_PIRQSS_RX | I2C_PIRQSS_TX_END)) != (I2C_PIRQSS_RX | I2C_PIRQSS_TX_END) &&
			(I2C_PIRQSS & I2C_PIRQSS_NACK) == 0 &&
			stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
			wdt_serve();

		start = stopwatch_get();
		while ((I2C_FFSSTAT & I2C_FFSSTAT_FFS) == 0 && (I2C_PIRQSS & I2C_PIRQSS_NACK) == 0 &&
			stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
			wdt_serve();

		I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;
		start = stopwatch_get();
		while (!(I2C_PIRQSS & (I2C_PIRQSS_TX_END | I2C_PIRQSS_NACK)) &&
			stopwatch_elapsed_ms(start) < I2C_TIMEOUT_MS)
			wdt_serve();

		if ((I2C_PIRQSS & I2C_PIRQSS_NACK) != 0) {
			result = I2C_V2_NACK;
			break;
		}

		uint32_t received = I2C_RPSSTAT & 0xFF;
		uint32_t word = I2C_RXD;
		if (received == 0) {
			result = I2C_V2_ERROR;
			break;
		}
		if (received > chunk)
			received = chunk;
		for (uint32_t i = 0; i < received && remaining > 0; i++) {
			*dst++ = (uint8_t) (word >> (8 * i));
			remaining--;
		}
		I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	}

	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = saved_addrcfg;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	return result;
}

i2c_v2_result_t i2c_v2_smbus_write(uint8_t address, uint8_t reg, uint8_t value) {
	uint8_t data[] = {reg, value};

	return i2c_v2_transfer_bytes(address, data, NULL, sizeof(data));
}

static uint8_t i2c_v2_pec(const uint8_t *data, uint32_t size) {
	uint8_t crc = 0;

	for (uint32_t i = 0; i < size; i++) {
		crc ^= data[i];
		for (int j = 0; j < 8; j++)
			crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
	}

	return crc;
}

i2c_v2_result_t i2c_v2_smbus_write_pec(uint8_t address, uint8_t reg, uint8_t value) {
	uint8_t crc_data[] = {(uint8_t) (address << 1), reg, value};
	uint8_t data[] = {reg, value, i2c_v2_pec(crc_data, sizeof(crc_data))};

	return i2c_v2_transfer_bytes(address, data, NULL, sizeof(data));
}

void i2c_v2_init(void) {
	I2C_CLC = 0x10A << MOD_CLC_RMC_SHIFT;

	GPIO_PIN(GPIO_I2C_SCL) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(GPIO_I2C_SDA) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = (
		I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 | I2C_FIFOCFG_RXFC
	);
	I2C_FDIVCFG = (0x0A << I2C_FDIVCFG_DEC_SHIFT) | (1 << I2C_FDIVCFG_INC_SHIFT);
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;

	VIC_CON(VIC_I2C_SINGLE_REQ_IRQ) = 1;
	VIC_CON(VIC_I2C_BURST_REQ_IRQ) = 1;
	VIC_CON(VIC_I2C_ERROR_IRQ) = 1;
	VIC_CON(VIC_I2C_PROTOCOL_IRQ) = 1;
	cpu_enable_irq(true);
}

static void handle_request_irq(void) {
	uint32_t status = I2C_RIS;

	i2c_v2_state.request_irqs++;
	i2c_v2_state.request_status |= status;
	if (i2c_v2_state.reading && i2c_v2_state.address_sent && (I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0) {
		i2c_v2_state.rx_request_status |= status;
		read_fifo(I2C_FFSSTAT & I2C_FFSSTAT_FFS);
		if ((I2C_FIFOCFG & I2C_FIFOCFG_RXFC) == 0 && i2c_v2_state.remaining == 0) {
			I2C_IMSC = I2C_IMSC_I2C_ERR_INT | I2C_IMSC_I2C_P_INT;
			if ((I2C_BUSSTAT & I2C_BUSSTAT_BS) == I2C_BUSSTAT_BS_FREE)
				i2c_v2_state.result = I2C_V2_DONE;
		}
	} else if (!i2c_v2_state.reading || !i2c_v2_state.address_sent) {
		i2c_v2_state.tx_request_status |= status;
		write_fifo();
	}
	I2C_ICR = status & I2C_STATUS_CLEAR;
}

static void handle_protocol_irq(void) {
	uint32_t status = I2C_PIRQSS;

	i2c_v2_state.protocol_irqs++;
	i2c_v2_state.protocol_status |= status;
	if (dma_rx.active && (status & I2C_PIRQSS_RX) != 0) {
		DMAC_CH_CONFIG(dma_rx.rx_channel) |= DMAC_CH_CONFIG_ENABLE;
		I2C_IMSC = I2C_TRANSFER_IRQS;
		I2C_DMAE = I2C_DMA_REQUESTS;
		dma_rx.started = true;
	}
	I2C_PIRQSC = status;
	if ((status & I2C_PIRQSS_NACK) != 0)
		i2c_v2_state.result = I2C_V2_NACK;
	else if ((status & I2C_PIRQSS_TX_END) != 0)
		i2c_v2_state.result = I2C_V2_DONE;
}

static void handle_error_irq(void) {
	uint32_t status = I2C_ERRIRQSS;

	i2c_v2_state.error_irqs++;
	i2c_v2_state.error_status |= status;
	I2C_ERRIRQSC = status;
	i2c_v2_state.result = I2C_V2_ERROR;
}

bool i2c_v2_handle_irq(uint32_t irq) {
	bool handled = true;

	i2c_v2_state.total_irqs++;
	i2c_v2_state.last_irq = irq;
	if (irq == VIC_I2C_SINGLE_REQ_IRQ || irq == VIC_I2C_BURST_REQ_IRQ)
		handle_request_irq();
	else if (irq == VIC_I2C_PROTOCOL_IRQ)
		handle_protocol_irq();
	else if (irq == VIC_I2C_ERROR_IRQ)
		handle_error_irq();
	else
		handled = false;

	if (i2c_v2_state.total_irqs > 100) {
		I2C_RUNCTRL = 0;
		I2C_IMSC = 0;
		i2c_v2_state.result = I2C_V2_ERROR;
	}

	return handled;
}

#endif
