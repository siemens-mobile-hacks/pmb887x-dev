#include <pmb887x.h>

static volatile stopwatch_t last = 0;
static volatile uint32_t period = 0;
static volatile uint32_t cnt = 0;

const bool PLL_RECLOCK = false;

int main(void) {
	wdt_init();
	
	cpu_enable_irq(true);
	NVIC_CON(NVIC_TPU_INT0_IRQ) = 1;
	NVIC_CON(NVIC_TPU_INT1_IRQ) = 1;
	
	printf("usart clc: %d\n", (USART_CLC(USART0) & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT);
	
	if (PLL_RECLOCK) {
		/*
		PLL_OSC = 0x00030101;
		PLL_CON0 = 0x1120080B;
		PLL_CON1 = 0x00020002;
		PLL_CON2 = 0x1000F377;
		PLL_CON3 = 0x10000303;
		USART_CLC(USART0) = 1 << MOD_CLC_RMC_SHIFT;
		*/
		
		PLL_OSC = 0x00030101;
		PLL_CON0 = 0x1120080B;
		PLL_CON1 = 0x00220002;
		PLL_CON2 = 0x0000E070;
		//PLL_CON3 = 0x10000302;
		USART_CLC(USART0) = 2 << MOD_CLC_RMC_SHIFT;
		STM_CLC = 0x001A1A14;
	}
	
	TPU_CLC = 2 << MOD_CLC_RMC_SHIFT;
	TPU_PLLCON0 = 1 << TPU_PLLCON0_K_DIV_SHIFT;
	TPU_PLLCON1 = 32 << TPU_PLLCON1_L_DIV_SHIFT;
	TPU_PLLCON2 = TPU_PLLCON2_LOAD | TPU_PLLCON2_INIT;
	
	for (int i = 0; i < 512; i++)
		TPU_RAM(i) = 0;
	
	TPU_UNK5 = 0;
	TPU_UNK4 = 6;
	TPU_UNK6 = 0x80000000;
	TPU_CORRECTION = 0;
	
	TPU_OVERFLOW = 9999;
	TPU_OFFSET = 0;
	TPU_INT(0) = 0;
	TPU_INT(1) = 30000;
	
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	
	TPU_SRC(0) = MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_SRE;
	
	last = stopwatch_get();
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	
	volatile uint32_t last_cnt = 0;
	while (true) {
		if (cnt != last_cnt) {
			printf("period: %d us [%d]\n", period, cnt);
			last_cnt = cnt;
		}
		
		if (cnt >= 10)
			break;
		
		// wdt_serve();
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
	
	if (irqn == NVIC_TPU_INT0_IRQ) {
		period = stopwatch_elapsed_us(last);
		last = stopwatch_get();
		
		cnt++;
		
		TPU_SRC(0) |= MOD_SRC_CLRR;
	} else if (irqn == NVIC_TPU_INT1_IRQ) {
		printf("IRQ FIRED: %X\n", irqn);
		while (1);
		TPU_SRC(1) |= MOD_SRC_CLRR;
	}
	
	NVIC_IRQ_ACK = 1;
}
