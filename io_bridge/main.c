#include <pmb887x.h>

volatile uint32_t current_irq = 0;

static int command_handler(int irq);

static void onerr(char c) {
	while (1) {
		usart_putc(USART0, c);
	}
}

int main(void) {
	wdt_init();
	
	i2c_init();
	i2c_smbus_write_byte(0x31, 0xE, 0b11);
	
	usart_set_speed(USART0, UART_SPEED_1600000);
	while (usart_getc(USART0) != 'O');
	while (usart_getc(USART0) != 'K');
	usart_putc(USART0, '.');
	
	cpu_enable_irq(true);
	cpu_enable_fiq(true);
	
	while (true) {
		if (usart_has_byte(USART0)) {
			__asm__ volatile("swi 0");
		}
	}
	
	return 0;
}

static int command_handler(int irq) {
	uint32_t value, addr;
	if (usart_has_byte(USART0)) {
		char c = usart_getc(USART0);
		
		if (c == '.') {
			wdt_serve();
			
			if (irq == 1) {
				usart_putc(USART0, ',');
				usart_putc(USART0, current_irq);
			} else {
				usart_putc(USART0, '.');
			}
			
			return 1;
		} else if (c == 'R' || c == 'r' || c == 'I' || c == 'i') { // 4, 2, 3, 1 bytes
			addr = usart_getc(USART0) << 24 | usart_getc(USART0) << 16 | usart_getc(USART0) << 8 | usart_getc(USART0);
			
			wdt_serve();
			
			if (addr == (uint32_t) &NVIC_CURRENT_IRQ) {
				value = current_irq;
			} else {
				if (c == 'R') { // 4
					value = REG(addr);
				} else if (c == 'r') { // 2
					value = REG_SHORT(addr);
				} else if (c == 'I') { // 3
					value = REG_SHORT(addr) << 8 | REG_BYTE(addr);
				} else if (c == 'i') { // 1
					value = REG_BYTE(addr);
				}
			}
			
			if (addr == (uint32_t) &PLL_STAT) {
				value = 0x2000;
			}
			
			if (addr == (uint32_t) &TPU_COUNTER && value >= 0x875) {
				// value = 0x875;
			}
			
			if (addr == (uint32_t) &SCU_RST_SR) {
				// value = (value & ~SCU_RST_SR_PWDRST) | SCU_RST_SR_HDRST;
			}
			
			usart_putc(USART0, (value >> 0 ) & 0xFF);
			usart_putc(USART0, (value >> 8 ) & 0xFF);
			usart_putc(USART0, (value >> 16) & 0xFF);
			usart_putc(USART0, (value >> 24) & 0xFF);
			
			if (irq == 1) {
				usart_putc(USART0, '!');
				usart_putc(USART0, current_irq);
			} else {
				usart_putc(USART0, ';');
			}
			
			wdt_serve();
			
			return 1;
		} else if (c == 'W' || c == 'w' || c == 'O' || c == 'o') { // 4, 2, 3, 1 bytes
			addr = usart_getc(USART0) << 24 | usart_getc(USART0) << 16 | usart_getc(USART0) << 8 | usart_getc(USART0);
			value = usart_getc(USART0) << 24 | usart_getc(USART0) << 16 | usart_getc(USART0) << 8 | usart_getc(USART0);
			
			wdt_serve();
			
			bool skip = false;
			
			// Low freq i2c clock
			if (addr == (uint32_t) &I2C_FDIVCFG) {
				value = 0x00010090;
			}
			
			if (!skip) {
				if (c == 'W') { // 4
					REG(addr) = value;
				} else if (c == 'w') { // 2
					REG_SHORT(addr) = value & 0xFFFF;
				} else if (c == 'O') { // 3
					REG_BYTE(addr) = value & 0xFF;
					REG_SHORT(addr) = (value >> 8) & 0xFFFF;
				} else if (c == 'o') { // 1
					REG_BYTE(addr) = value & 0xFF;
				}
			}
			
			if (irq == 1) {
				if (addr == (uint32_t) &NVIC_IRQ_ACK)
					current_irq = 0;
			}
			
			if (irq == 1) {
				usart_putc(USART0, '!');
				usart_putc(USART0, current_irq);
			} else {
				usart_putc(USART0, ';');
			}
			
			wdt_serve();
			
			if (irq && current_irq == 0)
				return 2;
			
			return 1;
		} else {
			onerr(0xE6);
		}
	}
	return 0;
}

__IRQ void data_abort_handler(void) {
	onerr(0xE1);
}

__IRQ void undef_handler(void) {
	onerr(0xE2);
}

__IRQ void prefetch_abort_handler(void) {
	onerr(0xE3);
}

__IRQ void fiq_handler(void) {
	onerr(0xE4);
}

__IRQ void reserved_handler(void) {
	onerr(0xE5);
}

__IRQ void swi_handler(void) {
	command_handler(0);
}

__IRQ void irq_handler(void) {
	current_irq = NVIC_CURRENT_IRQ;
	while (command_handler(1) != 2); // Ждём IRQ_ACK
}
