#pragma once

/* I2C */
void i2c_start();
void i2c_stop();

void i2c_start_read(unsigned char addr);
void i2c_start_write(unsigned char addr);

unsigned int i2c_readbit();
void i2c_writebit(unsigned int c);

unsigned char i2c_read(unsigned char ack);
unsigned char i2c_write(unsigned char c);

static inline void i2c_delay() {
	asm volatile("NOP"); // Хватает o_O
}

static void i2c_scl_lo();
static void i2c_scl_hi();
static void i2c_sda_lo();
static void i2c_sda_hi();
static void i2c_delay();

/* SMBUS */
int i2c_smbus_write_byte(unsigned int addr, unsigned char reg, unsigned char value);
