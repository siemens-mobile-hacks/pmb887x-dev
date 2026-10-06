#pragma once

#include <pmb887x.h>

void i2c_init(void);
bool i2c_write(uint8_t address, const uint8_t *data, uint32_t size);
bool i2c_read(uint8_t address, uint8_t *data, uint32_t size);

uint8_t i2c_smbus_pec_calculate(uint8_t address, const uint8_t *data, uint32_t size);
bool i2c_smbus_write_pec(uint8_t address, const uint8_t *data, uint32_t size);
bool i2c_smbus_write_byte(uint8_t address, uint8_t reg, uint8_t value);
bool i2c_smbus_read_byte(uint8_t address, uint8_t reg, uint8_t *value);
bool i2c_smbus_write(uint8_t address, uint8_t reg, uint32_t size, const uint8_t *data);
bool i2c_smbus_read(uint8_t address, uint8_t reg, uint32_t size, uint8_t *data);
