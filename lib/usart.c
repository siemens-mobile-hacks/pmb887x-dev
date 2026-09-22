#include "usart.h"

#include <string.h>

#define USART_BAUD_DIVIDER_MAX	8192
#define USART_FDV_SCALE			512

void usart_init(uint32_t usart, uint32_t baud_rate) {
	USART_CLC(usart) = (1 << MOD_CLC_RMC_SHIFT);
	usart_set_speed(usart, baud_rate);
	USART_CON(usart) = (
		USART_CON_M_ASYNC_8BIT | USART_CON_STP_ONE | USART_CON_REN |
		USART_CON_FDE | USART_CON_CON_R
	);
	USART_WHBCON(usart) = USART_WHBCON_SETREN;
	USART_RXFCON(usart) = USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU;
	USART_TXFCON(usart) = USART_TXFCON_TXFEN | USART_TXFCON_TXFFLU;

	uint32_t rxd_gpio, txd_gpio;
	if (usart == USART1) {
		rxd_gpio = GPIO_USART1_RXD;
		txd_gpio = GPIO_USART1_TXD;
	} else {
		rxd_gpio = GPIO_USART0_RXD;
		txd_gpio = GPIO_USART0_TXD;
	}

	GPIO_PIN(rxd_gpio) = GPIO_IS_ALT0 | GPIO_OS_NONE | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(txd_gpio) = GPIO_IS_NONE | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_DIR_IN;
}

void usart_set_speed(uint32_t usart, uint32_t baud_rate) {
	uint32_t max_baud_rate = (cpu_get_sys_freq() >> 4);
	uint32_t divider = MIN(max_baud_rate / baud_rate, USART_BAUD_DIVIDER_MAX);
	uint64_t scaled_baud_rate = (uint64_t) baud_rate * divider * USART_FDV_SCALE;
	uint32_t fractional_divider = (scaled_baud_rate + max_baud_rate / 2) / max_baud_rate;

	USART_BG(usart) = divider - 1;
	USART_FDV(usart) = fractional_divider == USART_FDV_SCALE ? 0 : fractional_divider;
}

void usart_read(uint32_t usart, void *data, uint32_t size) {
	uint8_t *bytes = data;
	uint32_t offset = 0;

	while (offset < size) {
		uint32_t fifo_level;
		while ((fifo_level = usart_get_rx_fifo_level(usart)) == 0);
		uint32_t batch_size = MIN(size - offset, fifo_level);

		for (uint32_t i = 0; i < batch_size; i++)
			bytes[offset++] = (uint8_t) USART_RXB(usart);
	}
}

void usart_write(uint32_t usart, const void *data, uint32_t size) {
	const uint8_t *bytes = data;
	uint32_t offset = 0;

	while (offset < size) {
		while (usart_get_tx_fifo_level(usart) != 0)
			(void) USART_RIS(usart);
		uint32_t batch_size = MIN(size - offset, USART_FIFO_SIZE);

		for (uint32_t i = 0; i < batch_size; i++)
			USART_TXB(usart) = bytes[offset++];
	}
}

void usart_flush(uint32_t usart) {
	while (usart_get_tx_fifo_level(usart) != 0)
		(void) USART_RIS(usart);
	while ((USART_RIS(usart) & USART_RIS_TX) == 0);
	USART_ICR(usart) = USART_ICR_TX;
}

void usart_print(uint32_t usart, const char *data) {
	usart_write(usart, data, strlen(data));
}
