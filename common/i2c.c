#include <pmb8876.h>
#include <gpio.h>
#include "i2c.h"

/* I2C */
void i2c_init() {
	REG(GPIO_I2C_SCL) = PMB8876_GPIO(NO_ALT,	NO_ALT,	MANUAL,	OUT,	LOW,	OPENDRAIN,	PULLUP,		NO_ENAQ);
	REG(GPIO_I2C_SDA) = PMB8876_GPIO(NO_ALT,	NO_ALT,	MANUAL,	OUT,	LOW,	OPENDRAIN,	PULLUP,		NO_ENAQ);
}

static void i2c_scl_lo() {
	unsigned int reg = REG(GPIO_I2C_SCL);
	pmb8876_gpio_reg_set_pdpu(reg, PMB8876_GPIO_PDPU_NONE);
	pmb8876_gpio_reg_set_data(reg, 0);
	pmb8876_gpio_reg_set_dir(reg, PMB8876_GPIO_DIR_OUT);
	REG(GPIO_I2C_SCL) = reg;
}

static void i2c_scl_hi() {
	unsigned int reg = REG(GPIO_I2C_SCL);
	pmb8876_gpio_reg_set_pdpu(reg, PMB8876_GPIO_PDPU_PULLUP);
	pmb8876_gpio_reg_set_data(reg, 0);
	pmb8876_gpio_reg_set_dir(reg, PMB8876_GPIO_DIR_IN);
	REG(GPIO_I2C_SCL) = reg;
}

static void i2c_sda_lo() {
	unsigned int reg = REG(GPIO_I2C_SDA);
	pmb8876_gpio_reg_set_pdpu(reg, PMB8876_GPIO_PDPU_NONE);
	pmb8876_gpio_reg_set_data(reg, 0);
	pmb8876_gpio_reg_set_dir(reg, PMB8876_GPIO_DIR_OUT);
	REG(GPIO_I2C_SDA) = reg;
}

static void i2c_sda_hi() {
	unsigned int reg = REG(GPIO_I2C_SDA);
	pmb8876_gpio_reg_set_pdpu(reg, PMB8876_GPIO_PDPU_PULLUP);
	pmb8876_gpio_reg_set_data(reg, 0);
	pmb8876_gpio_reg_set_dir(reg, PMB8876_GPIO_DIR_IN);
	REG(GPIO_I2C_SDA) = reg;
}

void i2c_start_read(unsigned char addr) {
	i2c_start();
	i2c_write((addr & 0x7F) << 1 | 1);
}

void i2c_start_write(unsigned char addr) {
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

void i2c_writebit(unsigned int c) {
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

unsigned int i2c_readbit() {
	unsigned int bit;
	
	i2c_sda_hi();
	i2c_scl_hi();
	i2c_delay();
	
	bit = pmb8876_gpio_reg_get_data(REG(GPIO_I2C_SDA));
	i2c_scl_lo();
	i2c_delay();
	
	return bit;
}

unsigned char i2c_write(unsigned char c) {
	unsigned char i;
	for (i = 0; i < 8; ++i) {
		i2c_writebit(c & 0x80);
		c <<= 1;
	}
	return i2c_readbit();
}

unsigned char i2c_read(unsigned char ack) {
    unsigned char res = 0, i;
	
	for (i = 0; i < 8; ++i) {
		res <<= 1;
		res |= i2c_readbit();
	}
	
	i2c_writebit(ack ? 0 : 1);
	i2c_delay();
	
    return res;
}

/* I2C SMBUS*/
int i2c_smbus_write_byte(unsigned int addr, unsigned char reg, unsigned char value) {
	i2c_start_write(addr);
	i2c_write(reg);
	i2c_write(value);
	i2c_stop();
}
