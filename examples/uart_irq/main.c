#include <pmb887x.h>
#include <printf.h>

static char buffer[0x4000] = {0};
static uint32_t buffer_size = 0;

int main(void) {
	wdt_init();
	
	cpu_enable_irq(true);
	
	// set async mode 
	USART_CON(USART0) = (USART_CON(USART0) & ~USART_CON_M) | USART_CON_M_ASYNC_8BIT;
	
	// set fifo trigger
	USART_RXFCON(USART0) = USART_RXFCON_RXFEN | (1 << USART_RXFCON_RXFITL_SHIFT);
	USART_TXFCON(USART0) = USART_TXFCON_TXFEN | (1 << USART_TXFCON_TXFITL_SHIFT);
	
	// enable RX irq
	USART_IMSC(USART0) = USART_IMSC_RX;
	
	for (int i = 0; i < 0xFF; i++) {
		VIC_CON(i) = 1; // RX
	}
	
	printf("Xuj!\r\n");
	
	while (true) {
		printf("buffer_size=%d\r\n", buffer_size);
		
		if (buffer_size >= sizeof(buffer) - 1) {
			printf("read done!\r\n");
		}
		wdt_serve();
	}
}

__IRQ void irq_handler(void) {
	int irqn = VIC_IRQ_CURRENT;
	
	// RX
	if (irqn == 0x6) {
		while ((USART_FSTAT(USART0) & USART_FSTAT_RXFFL)) {
			char ch = USART_RXB(USART0) & 0xff;
			if (buffer_size < sizeof(buffer) - 1) {
				buffer[buffer_size++] = ch;
				buffer[buffer_size] = 0;
			}
		}
		USART_ICR(USART0) = USART_ICR_ERR | USART_ICR_RX;
	}
	
	VIC_IRQ_ACK = 1;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}
