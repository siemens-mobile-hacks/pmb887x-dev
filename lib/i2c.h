#pragma once

#include <pmb887x.h>

/* I2C */
void i2c_init(void);

void i2c_start(void);
void i2c_stop(void);

void i2c_start_read(uint8_t addr);
void i2c_start_write(uint8_t addr);

uint32_t i2c_readbit(void);
void i2c_writebit(uint32_t c);

uint8_t i2c_read(bool ack);
uint8_t i2c_write(uint8_t c);

static inline void i2c_delay(void) {
	__asm__ volatile("NOP");
}

static inline void i2c_scl_lo(void) {
	gpio_init_output(GPIO_I2C_SCL, GPIO_OS_NONE, GPIO_PS_MANUAL, false, GPIO_PPEN_OPENDRAIN, GPIO_PDPU_NONE, false);
}

static inline void i2c_scl_hi(void) {
	gpio_init_input(GPIO_I2C_SCL, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_PULLUP, false);
}

static inline void i2c_sda_lo(void) {
	gpio_init_output(GPIO_I2C_SDA, GPIO_OS_NONE, GPIO_PS_MANUAL, false, GPIO_PPEN_OPENDRAIN, GPIO_PDPU_NONE, false);
}

static inline void i2c_sda_hi(void) {
	gpio_init_input(GPIO_I2C_SDA, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_PULLUP, false);
}

/* SMBUS */
void i2c_smbus_write_byte(uint32_t addr, uint8_t reg, uint8_t value);
uint8_t i2c_smbus_read_byte(uint32_t addr, uint8_t reg);
void i2c_smbus_write(uint32_t addr, uint8_t reg, uint32_t size, uint8_t *value);
void i2c_smbus_read(uint32_t addr, uint8_t reg, uint32_t size, uint8_t *value);
