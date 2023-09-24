#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	//cpu_enable_irq(true);
	
	GPIO_PIN(GPIO_DIF_VD) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_LOW;
	GPIO_PIN(GPIO_DIF_RESET1) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;

	GPIO_PIN(GPIO_DIF_CD) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_CS1) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;

	GPIO_PIN(GPIO_DIF_RD) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_WR) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;

	GPIO_PIN(GPIO_DIF_D0) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D1) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D2) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D3) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D4) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D5) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D6) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D7) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	
	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
	
	DIF_RUNCTRL = 0;
	DIF_CON1 = DIF_CON1 & 0xfffffffd;
	DIF_CON2 = DIF_CON2 | 0x10;
	DIF_CON2 = DIF_CON2 & 0xffffff9f;
	DIF_RUNCTRL = 1;
	
	DIF_RUNCTRL = 0;
	DIF_PROG(0) = 0x14830820;
	DIF_PROG(1) = 0x2d4920e6;
	DIF_PROG(2) = 0x460f39ac;
	DIF_PROG(3) = 0x5ed55272;
	DIF_PROG(4) = 0x779b6b38;
	DIF_PROG(5) = 0x3fe;
	DIF_RUNCTRL = 1;
	
	while (true) {
		DIF_RUNCTRL = 0;
		DIF_CON2 = DIF_CON2 & 0xfffffffe;
		DIF_RUNCTRL = 1;
		DIF_FIFO = 0;
		while (DIF_STAT & 1);
		
		printf(".\n");
	}
	
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

__IRQ void irq_handler(void) {
	int irqn = NVIC_CURRENT_IRQ;
	
	printf("irqn=%d\n", irqn);
	
	NVIC_IRQ_ACK = 1;
}
