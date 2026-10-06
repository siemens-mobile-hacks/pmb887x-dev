#pragma once

#include <pmb887x.h>

void i2c_hw_init(void);
bool i2c_hw_write(
	uint8_t address,
	const uint8_t *prefix,
	uint32_t prefix_size,
	const uint8_t *data,
	uint32_t size
);
bool i2c_hw_read(uint8_t address, uint8_t *data, uint32_t size);
