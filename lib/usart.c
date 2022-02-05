#include "usart.h"

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
