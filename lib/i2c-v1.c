#include "i2c-internal.h"

#ifdef PMB8875

#define I2C_POLL_LIMIT 1000000

static const uint32_t I2C_BUS_CONFIG = I2C_BUSCON_SDAEN0 | I2C_BUSCON_SCLEN0 |
	(0x7E << I2C_BUSCON_BRP_SHIFT) | I2C_BUSCON_PREDIV_8 | I2C_BUSCON_BRPMOD_MODE1;
static const uint32_t I2C_CLEAR_STATE = I2C_WHBSYSCON_CLRAL | I2C_WHBSYSCON_CLRIRQD |
	I2C_WHBSYSCON_CLRIRQP | I2C_WHBSYSCON_CLRIRQE | I2C_WHBSYSCON_CLRRSC | I2C_WHBSYSCON_CLRBUM |
	I2C_WHBSYSCON_CLRACKDIS | I2C_WHBSYSCON_CLRTRX | I2C_WHBSYSCON_CLRSTP;

struct i2c_transfer {
	const uint8_t *prefix;
	const uint8_t *tx;
	uint8_t *rx;
	uint32_t prefix_remaining;
	uint32_t remaining;
	bool address_sent;
};

static void buffer_size_set(uint32_t size) {
	I2C_SYSCON = (I2C_SYSCON & ~I2C_SYSCON_CI) | ((size - 1) << I2C_SYSCON_CI_SHIFT);
}

static uint8_t transfer_byte_get(struct i2c_transfer *transfer) {
	if (transfer->prefix_remaining != 0) {
		transfer->prefix_remaining--;
		return *transfer->prefix++;
	}

	transfer->remaining--;
	return *transfer->tx++;
}

static void write_buffer(struct i2c_transfer *transfer, uint32_t value, uint32_t offset) {
	uint32_t count = offset;

	while (count < sizeof(value) && (transfer->prefix_remaining != 0 || transfer->remaining != 0)) {
		value |= (uint32_t) transfer_byte_get(transfer) << (count * 8);
		count++;
	}
	buffer_size_set(count);
	I2C_RTB = value;
}

static void read_buffer(struct i2c_transfer *transfer) {
	uint32_t count = (I2C_SYSCON & I2C_SYSCON_CO) >> I2C_SYSCON_CO_SHIFT;
	uint32_t requested = ((I2C_SYSCON & I2C_SYSCON_CI) >> I2C_SYSCON_CI_SHIFT) + 1;

	if (count > requested)
		count = requested;
	if (count > transfer->remaining)
		count = transfer->remaining;

	I2C_SYSCON |= I2C_SYSCON_INT;
	uint32_t value = I2C_RTB;
	I2C_SYSCON &= ~I2C_SYSCON_INT;
	for (uint32_t i = 0; i < count; i++)
		*transfer->rx++ = value >> (i * 8);
	transfer->remaining -= count;
}

static void transfer_stop(void) {
	I2C_WHBSYSCON = I2C_CLEAR_STATE;
}

static bool transfer_wait(struct i2c_transfer *transfer, bool reading) {
	for (uint32_t i = 0; i < I2C_POLL_LIMIT; i++) {
		uint32_t status = I2C_SYSCON;

		if ((status & I2C_SYSCON_IRQP) != 0)
			I2C_WHBSYSCON = I2C_WHBSYSCON_CLRIRQP;

		if ((status & I2C_SYSCON_IRQD) != 0) {
			bool nack = (status & I2C_SYSCON_LRB) != 0 && (!reading || !transfer->address_sent);

			if (nack) {
				transfer_stop();
				return false;
			}

			if (!reading) {
				if (transfer->prefix_remaining != 0 || transfer->remaining != 0) {
					write_buffer(transfer, 0, 0);
				} else {
					I2C_WHBSYSCON = I2C_WHBSYSCON_CLRBUM | I2C_WHBSYSCON_CLRIRQD;
				}
			} else {
				if (transfer->address_sent) {
					read_buffer(transfer);
				} else {
					transfer->address_sent = true;
					I2C_WHBSYSCON = I2C_WHBSYSCON_CLRTRX;
				}

				if (transfer->remaining != 0) {
					uint32_t count = MIN(transfer->remaining, 4);

					buffer_size_set(count);
					if (transfer->remaining <= 4)
						I2C_WHBSYSCON = I2C_WHBSYSCON_SETACKDIS | I2C_WHBSYSCON_SETSTP;
				}
				(void) I2C_RTB;
			}
		}

		if ((status & I2C_SYSCON_IRQE) != 0) {
			bool complete = transfer->remaining == 0 && transfer->prefix_remaining == 0;

			transfer_stop();
			return complete;
		}
		wdt_serve();
	}

	transfer_stop();
	return false;
}

void i2c_hw_init(void) {
	GPIO_PIN(GPIO_I2C_SCL) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(GPIO_I2C_SDA) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	I2C_CLC = 16 << MOD_CLC_RMC_SHIFT;
	I2C_BUSCON = I2C_BUS_CONFIG;
	I2C_SYSCON = I2C_SYSCON_MOD_MASTER;
	transfer_stop();
}

bool i2c_hw_write(
	uint8_t address,
	const uint8_t *prefix,
	uint32_t prefix_size,
	const uint8_t *data,
	uint32_t size
) {
	struct i2c_transfer transfer = {
		.prefix = prefix,
		.tx = data,
		.prefix_remaining = prefix_size,
		.remaining = size,
		.address_sent = true,
	};

	I2C_SYSCON = I2C_SYSCON_MOD_MASTER;
	write_buffer(&transfer, address << 1, 1);
	I2C_WHBSYSCON = I2C_WHBSYSCON_SETBUM;
	return transfer_wait(&transfer, false);
}

bool i2c_hw_read(uint8_t address, uint8_t *data, uint32_t size) {
	if (size == 0)
		return true;

	struct i2c_transfer transfer = {
		.rx = data,
		.remaining = size,
	};

	buffer_size_set(1);
	I2C_RTB = (address << 1) | 1;
	I2C_WHBSYSCON = I2C_WHBSYSCON_SETBUM;
	return transfer_wait(&transfer, true);
}

#endif
