#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	cpu_enable_irq(true);
	
	GPIO_CLC = 0x200;
	
	// Init gpios
	#if defined(PMB8876)
	GPIO_PIN(GPIO_KP_IN0) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN1) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN2) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN3) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN4) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN5) = GPIO_OS_ALT1 | GPIO_PPEN_OPENDRAIN | GPIO_ENAQ_OFF;
	GPIO_PIN(GPIO_KP_IN6) = GPIO_PS_MANUAL | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	
	GPIO_PIN(GPIO_KP_OUT0) = GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN;
	GPIO_PIN(GPIO_KP_OUT1) = GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN;
	GPIO_PIN(GPIO_KP_OUT2) = GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN;
	GPIO_PIN(GPIO_KP_OUT3) = GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN;
	#elif defined(PMB8875)
	GPIO_PIN(GPIO_KP_IN0) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN1) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN2) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN3) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	GPIO_PIN(GPIO_KP_IN4) = GPIO_IS_ALT0 | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
	
	GPIO_PIN(GPIO_KP_OUT0) = GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN;
	GPIO_PIN(GPIO_KP_OUT1) = GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN;
	GPIO_PIN(GPIO_KP_OUT2) = GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN;
	GPIO_PIN(GPIO_KP_OUT3) = GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN;
	#else
		#error "WTF"
	#endif
	
	// Configure
	KEYPAD_CON = 0x101;
	
	// Enable IRQ
	KEYPAD_PRESS_SRC = MOD_SRC_SRE;
	KEYPAD_RELEASE_SRC = MOD_SRC_SRE;
	
	NVIC_CON(NVIC_KEYPAD_PRESS_IRQ) = 1;
	NVIC_CON(NVIC_KEYPAD_RELEASE_IRQ) = 1;
	
	printf("Hello?\n");
	
	while (true) {
		printf("%08X\n", KEYPAD_PORT(0));
		wdt_serve();
	}
	
	printf("done\n");
	
	return 0;
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

static void read_keycodes(void) {
	uint32_t ports[3] = {KEYPAD_PORT(0), KEYPAD_PORT(1), KEYPAD_PORT(2)};
	
	for (int kp_in = 0; kp_in < 8; kp_in++) {
		uint32_t mask1 = (1 << (kp_in + 24)) | (1 << (kp_in + 16)) | (1 << (kp_in + 8)) | (1 << kp_in);
		uint32_t mask2 = (1 << kp_in);
		
		if (!(ports[0] & mask1) && !(ports[1] & mask1) && !(ports[2] & mask2)) {
			uint32_t keycode = 0x1FF00 | (1 << kp_in);
			
			ports[0] |= mask1;
			ports[1] |= mask1;
			ports[2] |= mask2;
			
			printf("keycode=%08X [KP_IN%d + KP_OUT0-KP_OUT9]\n", keycode, kp_in);
		}
	}
	
	for (int port_n = 0; port_n < 3; port_n++) {
		uint32_t port = ports[port_n];
		if (port == 0xFFFFFFFF)
			continue;
		
		for (int i = 0; i < 4; i++) {
			int kp_out = i + (port_n * 4);
			
			uint8_t kp_bits = (port >> (i * 8)) & 0xFF;
			if (kp_bits == 0xFF)
				continue;
			
			for (int kp_in = 0; kp_in < 8; kp_in++) {
				if (!(kp_bits & (1 << kp_in))) {
					uint32_t keycode = (1 << (8 + kp_out)) | (1 << kp_in);
					printf("keycode=%08X [KP_IN%d + KP_OUT%d]\n", keycode, kp_in, kp_out);
				}
			}
		}
	}
}

__IRQ void irq_handler(void) {
	int irqn = NVIC_CURRENT_IRQ;
	
	printf("irqn=%d\n", irqn);
	
	if (irqn == NVIC_KEYPAD_PRESS_IRQ) {
		KEYPAD_PRESS_SRC |= MOD_SRC_CLRR;
		printf("key press\n");
		read_keycodes();
	}
	
	if (irqn == NVIC_KEYPAD_RELEASE_IRQ) {
		KEYPAD_RELEASE_SRC |= MOD_SRC_CLRR;
		printf("key release\n");
	}
	
	NVIC_IRQ_ACK = 1;
}
