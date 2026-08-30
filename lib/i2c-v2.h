#pragma once

#ifdef PMB8876
#include <pmb887x.h>

typedef enum {
	I2C_V2_PENDING,
	I2C_V2_DONE,
	I2C_V2_NACK,
	I2C_V2_ERROR,
} i2c_v2_result_t;

struct i2c_v2_state {
	const uint8_t *tx;
	uint8_t *rx;
	uint32_t remaining;
	uint8_t address;
	bool address_sent;
	bool reading;
	i2c_v2_result_t result;
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
};

extern volatile struct i2c_v2_state i2c_v2_state;

void i2c_v2_init(void);

bool i2c_v2_handle_irq(uint32_t irq);

i2c_v2_result_t i2c_v2_transfer_bytes(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size);
i2c_v2_result_t i2c_v2_transfer_bytes_running(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size);
bool i2c_v2_transfer(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size);

i2c_v2_result_t i2c_v2_smbus_read(uint8_t address, uint8_t reg, uint8_t *data, uint32_t size);
i2c_v2_result_t i2c_v2_smbus_write(uint8_t address, uint8_t reg, uint8_t value);
i2c_v2_result_t i2c_v2_smbus_write_pec(uint8_t address, uint8_t reg, uint8_t value);

i2c_v2_result_t i2c_v2_dma_write(uint32_t channel, uint8_t address, const uint8_t *data, uint32_t size);
i2c_v2_result_t i2c_v2_dma_read(uint32_t channel, uint8_t address, uint8_t *data, uint32_t size);

#endif
