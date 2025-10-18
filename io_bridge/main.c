#include <pmb887x.h>

volatile uint32_t current_irq = 0;
volatile uint32_t i = 0;

static int command_handler(int irq);

static void onerr(char c) {
	while (1) {
		usart_putc(USART0, c);
	}
}

int main(void) {
	wdt_init();
	
	i2c_init();
	//i2c_smbus_write_byte(0x31, 0xE, 0b11);
	
	GPIO_PIN(GPIO_I2C_SCL) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(GPIO_I2C_SDA) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;

#ifdef PMB8876
	usart_set_speed(USART0, UART_SPEED_1600000);
#else
	usart_set_speed(USART0, UART_SPEED_1600000);
#endif
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
	uint32_t value = 0, addr;
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
			
			bool skip = false;
			if (addr == 0xFFFFFFFF)
				skip = true;

			if (addr == (uint32_t) &VIC_IRQ_CURRENT) {
				value = current_irq;
			} else if (!skip) {
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
			
		//	if (addr == (uint32_t) &(GPTU_SRC(GPTU0))) {
		//		value = 0;
		//	}
			
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

			if (addr == 0xFFFFFFFF)
				skip = true;
			
#ifdef PMB8876
			// Low freq i2c clock
			if (addr == (uint32_t) &I2C_FDIVCFG) {
				//value = 0x00010090;
			}
			
			// Low freq i2c clock
			if (addr == (uint32_t) &I2C_CLC) {
				value = 0xFF << MOD_CLC_RMC_SHIFT;
			}
			
			if (addr == (uint32_t) &I2C_TXD) {
				//if (value == 0x00140E62)
				//	value = 0x005F4662;
			}
#endif

#ifdef PMB8875
	if (addr == (uint32_t) &I2C_CLC) {
		if ((value & MOD_CLC_RMC)) {
			value &= ~MOD_CLC_RMC;
			value |= 8 << MOD_CLC_RMC_SHIFT;
		}
	}



	if (addr == (uint32_t) &DIF_TB) {
		if (value == 0x1FF0) {
			value = i++ % 2 ? 0x1FF0 : 0x1000;
		}
	}

	if (addr == (uint32_t) &DIF_CLC) {
		if ((value & MOD_CLC_RMC)) {
			//value &= ~MOD_CLC_RMC;
			//value |= 0xFF << MOD_CLC_RMC_SHIFT;
		}
	}

	if (addr == (uint32_t) &I2C_BUSCON) {
	//	value &= ~I2C_BUSCON_BRPMOD;
	//	value |= I2C_BUSCON_BRPMOD_MODE0;
	}
#endif

// WRITE[4] F4800014: A0007E11 (I2C_BUSCON): SDAEN0 | SCLEN0 | BRP(0x7E) | PREDIV(8) | BRPMOD(MODE1) (PC: A0A95FDC, LR: A0A95FCC)


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
				if (addr == (uint32_t) &VIC_IRQ_ACK)
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
	current_irq = VIC_IRQ_CURRENT;
	while (command_handler(1) != 2); // Ждём IRQ_ACK
}
