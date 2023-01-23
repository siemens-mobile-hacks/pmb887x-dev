#include "main.h"

// A0200000
// a04f9c04 i2c_transfer
// a04f9c58 i2c_receive

// #define DEBUG 1

DLL_PUBLIC int i2c_transfer(I2C_MSG *msg) {
	#ifdef DEBUG
	printf("i2c_transfer 0x%02X, size: %d\n", msg->chip_addr, msg->size);
	uint8_t *ptr = msg->data;
	for (int i = 0; i < msg->size; i++)
		printf(" data[%d] = %02X\n", i, ptr[i]);
	#endif
	
	i2c_smbus_write(msg->chip_addr, msg->nRegister, msg->size, msg->data);
	
	msg->tf = 1;
	msg->callback(msg, 0);
	return 0;
}

DLL_PUBLIC int i2c_receive(I2C_MSG *msg) {
	#ifdef DEBUG
	printf("i2c_receive 0x%02X, size: %d\n", msg->chip_addr, msg->size);
	i2c_smbus_read(msg->chip_addr, msg->nRegister, msg->size, msg->data);
	
	uint8_t *ptr = msg->data;
	for (int i = 0; i < msg->size; i++)
		printf(" data[%d] = %02X\n", i, ptr[i]);
	#endif
	
	msg->tf = 2;
	msg->callback(msg, 0);
	return 0;
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

void usart_set_speed(uint32_t usart, enum usart_speed_t speed) {
	USART_BG(usart) = (speed >> 16) & 0xFFFF;
	USART_FDV(usart) = speed & 0xFFFF;
}

void usart_print(uint32_t usart, const char *data) {
	while (*data)
		usart_putc(usart, *data++);
}

bool usart_has_byte(uint32_t usart) {
	return (USART_RIS(usart) & USART_RIS_RX) != 0;
}

void usart_putc(uint32_t usart, char c) {
	USART_TXB(usart) = c;
	while (!(USART_RIS(usart) & USART_RIS_TX));
	USART_ICR(usart) |= USART_ICR_TX;
}

char usart_getc(uint32_t usart) {
	while (!(USART_RIS(usart) & USART_RIS_RX));
	USART_ICR(usart) |= USART_ICR_RX;
	return USART_RXB(usart);
}
