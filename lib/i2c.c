#include "i2c.h"

#include "i2c-internal.h"

static uint8_t smbus_pec_update(uint8_t pec, uint8_t value) {
	pec ^= value;
	for (uint32_t bit = 0; bit < 8; bit++) {
		if ((pec & 0x80) != 0) {
			pec = (pec << 1) ^ 0x07;
		} else {
			pec <<= 1;
		}
	}

	return pec;
}

uint8_t i2c_smbus_pec_calculate(uint8_t address, const uint8_t *data, uint32_t size) {
	uint8_t pec = smbus_pec_update(0, address << 1);

	for (uint32_t i = 0; i < size; i++)
		pec = smbus_pec_update(pec, data[i]);

	return pec;
}

void i2c_init(void) {
	i2c_hw_init();
}

bool i2c_write(uint8_t address, const uint8_t *data, uint32_t size) {
	return i2c_hw_write(address, NULL, 0, data, size);
}

bool i2c_read(uint8_t address, uint8_t *data, uint32_t size) {
	return i2c_hw_read(address, data, size);
}

bool i2c_smbus_write_pec(uint8_t address, const uint8_t *data, uint32_t size) {
	uint8_t pec = i2c_smbus_pec_calculate(address, data, size);

	return i2c_hw_write(address, data, size, &pec, 1);
}

bool i2c_smbus_write_byte(uint8_t address, uint8_t reg, uint8_t value) {
	return i2c_smbus_write(address, reg, 1, &value);
}

bool i2c_smbus_read_byte(uint8_t address, uint8_t reg, uint8_t *value) {
	return i2c_smbus_read(address, reg, 1, value);
}

bool i2c_smbus_write(uint8_t address, uint8_t reg, uint32_t size, const uint8_t *data) {
	uint8_t prefix = reg;

	return i2c_hw_write(address, &prefix, 1, data, size);
}

bool i2c_smbus_read(uint8_t address, uint8_t reg, uint32_t size, uint8_t *data) {
	if (!i2c_hw_write(address, &reg, 1, NULL, 0))
		return false;

	return i2c_hw_read(address, data, size);
}
