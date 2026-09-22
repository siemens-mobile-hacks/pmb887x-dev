#include <pmb887x.h>

void putchar_(char character);

void putchar_(char character) {
	usart_putc(USART0, character);
}
