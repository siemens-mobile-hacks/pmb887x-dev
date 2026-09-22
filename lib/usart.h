#pragma once

#include <pmb887x.h>

#define USART_FIFO_SIZE 8

void usart_init(uint32_t usart, uint32_t baud_rate);
void usart_set_speed(uint32_t usart, uint32_t baud_rate);
void usart_read(uint32_t usart, void *data, uint32_t size);
void usart_write(uint32_t usart, const void *data, uint32_t size);
void usart_flush(uint32_t usart);
void usart_print(uint32_t usart, const char *data);

static inline uint32_t usart_get_rx_fifo_level(uint32_t usart) {
	uint32_t status = USART_FSTAT(usart);
	return (status & USART_FSTAT_RXFFL) >> USART_FSTAT_RXFFL_SHIFT;
}

static inline uint32_t usart_get_tx_fifo_level(uint32_t usart) {
	uint32_t status = USART_FSTAT(usart);
	return (status & USART_FSTAT_TXFFL) >> USART_FSTAT_TXFFL_SHIFT;
}

static inline void usart_putc(uint32_t usart, char c) {
	usart_write(usart, &c, sizeof(c));
}

static inline char usart_getc(uint32_t usart) {
	char c;
	usart_read(usart, &c, sizeof(c));
	return c;
}
