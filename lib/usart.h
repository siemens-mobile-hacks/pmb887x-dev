#pragma once

#include <pmb887x.h>

enum usart_speed_t {
	UART_SPEED_12000 = 0x003f00f2, 
	UART_SPEED_57600 = 0x001901d8, 
	UART_SPEED_115200 = 0x000c01d8, 
	UART_SPEED_230400 = 0x000501b4, 
	UART_SPEED_460800 = 0x00000092, 
	UART_SPEED_614400 = 0x000000c3, 
	UART_SPEED_921600 = 0x00000127, 
	UART_SPEED_1228800 = 0x0000018a, 
	UART_SPEED_1600000 = 0x00000000, 
	UART_SPEED_1500000 = 0x000001d0
};

// UART
void usart_set_speed(uint32_t usart, enum usart_speed_t speed);
void usart_putc(uint32_t usart, char c);
char usart_getc(uint32_t usart);
void usart_print(uint32_t usart, const char *data);
bool usart_has_byte(uint32_t usart);
