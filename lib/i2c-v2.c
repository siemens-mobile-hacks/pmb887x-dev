#include "i2c-internal.h"

#ifdef PMB8876

#define I2C_POLL_LIMIT 1000000
#define I2C_STATUS_CLEAR 0x3F
#define I2C_PROTOCOL_CLEAR 0x7F
#define I2C_ERROR_CLEAR 0x0F
#define I2C_REQUESTS (I2C_RIS_LSREQ_INT | I2C_RIS_SREQ_INT | I2C_RIS_LBREQ_INT | I2C_RIS_BREQ_INT)

static void status_clear(void) {
	I2C_ICR = I2C_STATUS_CLEAR;
	I2C_PIRQSC = I2C_PROTOCOL_CLEAR;
	I2C_ERRIRQSC = I2C_ERROR_CLEAR;
}

static void transfer_abort(void) {
	I2C_ENDDCTRL = I2C_ENDDCTRL_SETEND;
	for (uint32_t i = 0; i < I2C_POLL_LIMIT; i++) {
		if ((I2C_BUSSTAT & I2C_BUSSTAT_BS) == I2C_BUSSTAT_BS_FREE)
			break;
		wdt_serve();
	}
	I2C_RUNCTRL = 0;
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	status_clear();
}

static uint32_t request_wait(void) {
	for (uint32_t i = 0; i < I2C_POLL_LIMIT; i++) {
		uint32_t request = I2C_RIS & I2C_REQUESTS;

		if (request != 0)
			return request;
		if ((I2C_PIRQSS & I2C_PIRQSS_NACK) != 0 || I2C_ERRIRQSS != 0)
			return 0;
		wdt_serve();
	}

	return 0;
}

static bool transfer_wait(void) {
	for (uint32_t i = 0; i < I2C_POLL_LIMIT; i++) {
		uint32_t status = I2C_PIRQSS;

		if ((status & I2C_PIRQSS_NACK) != 0 || I2C_ERRIRQSS != 0)
			return false;
		if ((status & I2C_PIRQSS_TX_END) != 0)
			return true;
		wdt_serve();
	}

	return false;
}

static uint8_t byte_get(
	const uint8_t **prefix,
	uint32_t *prefix_size,
	const uint8_t **data,
	uint32_t *size
) {
	if (*prefix_size != 0) {
		(*prefix_size)--;
		return *(*prefix)++;
	}

	(*size)--;
	return *(*data)++;
}

void i2c_hw_init(void) {
	GPIO_PIN(GPIO_I2C_SCL) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(GPIO_I2C_SDA) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	I2C_CLC = 10 << MOD_CLC_RMC_SHIFT;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS | I2C_ADDRCFG_SONA | I2C_ADDRCFG_SOPE;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_1_WORD | I2C_FIFOCFG_TXBS_1_WORD | I2C_FIFOCFG_RXFA_1 |
		I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_FDIVCFG = (0x0A << I2C_FDIVCFG_DEC_SHIFT) | (1 << I2C_FDIVCFG_INC_SHIFT);
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	I2C_IMSC = 0;
	status_clear();
}

bool i2c_hw_write(
	uint8_t address,
	const uint8_t *prefix,
	uint32_t prefix_size,
	const uint8_t *data,
	uint32_t size
) {
	uint32_t total = prefix_size + size + 1;
	uint32_t remaining = total;
	bool address_pending = true;

	status_clear();
	I2C_TPSCTRL = total;
	while (remaining != 0) {
		uint32_t request = request_wait();

		if (request == 0) {
			transfer_abort();
			return false;
		}

		uint32_t count = remaining < sizeof(uint32_t) ? remaining : sizeof(uint32_t);
		uint32_t value = 0;
		for (uint32_t offset = 0; offset < count; offset++) {
			uint8_t byte;

			if (address_pending) {
				byte = address << 1;
				address_pending = false;
			} else {
				byte = byte_get(&prefix, &prefix_size, &data, &size);
			}
			value |= (uint32_t) byte << (offset * 8);
		}
		I2C_TXD = value;
		I2C_ICR = request;
		remaining -= count;
	}

	bool complete = transfer_wait();
	if (complete) {
		status_clear();
	} else {
		transfer_abort();
	}
	return complete;
}

bool i2c_hw_read(uint8_t address, uint8_t *data, uint32_t size) {
	uint32_t remaining = size;
	bool complete = false;
	bool receiving = false;

	if (size == 0)
		return true;

	status_clear();
	I2C_MRPSCTRL = size;
	I2C_TPSCTRL = 1;
	uint32_t request = request_wait();
	if (request == 0) {
		transfer_abort();
		return false;
	}
	I2C_TXD = (address << 1) | 1;
	I2C_ICR = request;

	for (uint32_t i = 0; i < I2C_POLL_LIMIT; i++) {
		uint32_t protocol_status = I2C_PIRQSS;

		if ((protocol_status & I2C_PIRQSS_NACK) != 0 || I2C_ERRIRQSS != 0)
			break;
		if ((protocol_status & I2C_PIRQSS_RX) != 0)
			receiving = true;

		request = I2C_RIS & I2C_REQUESTS;
		if (receiving && request != 0 && (I2C_FFSSTAT & I2C_FFSSTAT_FFS) != 0 && remaining != 0) {
			uint32_t value = I2C_RXD;
			uint32_t count = MIN(remaining, sizeof(value));

			for (uint32_t offset = 0; offset < count; offset++)
				*data++ = value >> (offset * 8);
			remaining -= count;
			I2C_ICR = request;
		}

		if (remaining == 0 && (protocol_status & I2C_PIRQSS_TX_END) != 0) {
			complete = (I2C_RPSSTAT & I2C_RPSSTAT_RPS) == size;
			break;
		}
		wdt_serve();
	}

	if (complete) {
		status_clear();
	} else {
		transfer_abort();
	}
	return complete;
}

#endif
