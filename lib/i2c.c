#include "i2c.h"

// from: https://github.com/todbot/SoftI2CMaster

/* I2C */
void i2c_init() {
	i2c_sda_hi();
	i2c_scl_hi();
    
    i2c_delay();
}

void i2c_start_read(uint8_t addr) {
	i2c_start();
	i2c_write((addr & 0x7F) << 1 | 1);
}

void i2c_start_write(uint8_t addr) {
	i2c_start();
	i2c_write((addr & 0x7F) << 1 | 0);
}

void i2c_start() {
	i2c_sda_hi();
	i2c_scl_hi();
	
	i2c_delay();
	
	i2c_sda_lo();
	i2c_delay();
	
	i2c_scl_lo();
	i2c_delay();
}

void i2c_stop() {
	i2c_scl_hi();
	i2c_delay();
	
	i2c_sda_hi();
	i2c_delay();
}

void i2c_writebit(uint32_t c) {
	if (c > 0) {
		i2c_sda_hi();
	} else {
		i2c_sda_lo();
	}
	
	i2c_scl_hi();
	i2c_delay();
	
	i2c_scl_lo();
	i2c_delay();
	
	if (c > 0)
		i2c_sda_lo();
	
	i2c_delay();
}

uint32_t i2c_readbit() {
	i2c_sda_hi();
	i2c_scl_hi();
	i2c_delay();
	
	uint32_t bit = gpio_get(GPIO_I2C_SDA);
	i2c_scl_lo();
	i2c_delay();
	
	return bit;
}

uint8_t i2c_write(uint8_t c) {
	for (uint8_t i = 0; i < 8; ++i) {
		i2c_writebit(c & 0x80);
		c <<= 1;
	}
	return i2c_readbit();
}

uint8_t i2c_read(bool ack) {
    uint8_t res = 0;
	
	for (uint8_t i = 0; i < 8; ++i) {
		res <<= 1;
		res |= i2c_readbit();
	}
	
	i2c_writebit(ack ? 1 : 0);
	i2c_delay();
	
    return res;
}

/* I2C SMBUS */
void i2c_smbus_write_byte(uint32_t addr, uint8_t reg, uint8_t value) {
	i2c_smbus_write(addr, reg, 1, &value);
}

uint8_t i2c_smbus_read_byte(uint32_t addr, uint8_t reg) {
	uint8_t value = 0;
	i2c_smbus_read(addr, reg, 2, &value);
	return value;
}

void i2c_smbus_write(uint32_t addr, uint8_t reg, uint32_t size, uint8_t *value) {
	i2c_start_write(addr);
	i2c_write(reg);
	for (uint32_t i = 0; i < size; i++)
		i2c_write(value[i]);
	i2c_stop();
}

void i2c_smbus_read(uint32_t addr, uint8_t reg, uint32_t size, uint8_t *value) {
	i2c_start_write(addr);
	i2c_write(reg);
	i2c_stop();

	i2c_start_read(addr);
	for (uint32_t i = 0; i < size; i++)
		value[i] = i2c_read(i != size - 1);
	i2c_stop();
}
