#include <pmb887x.h>

static volatile stopwatch_t last = 0;
static volatile uint32_t period = 0;
static volatile uint32_t cnt = 0;
static volatile uint32_t val = 0;

static volatile stopwatch_t last2 = 0;
static volatile uint32_t period2 = 0;
static volatile uint32_t cnt2 = 0;
static volatile uint32_t val2 = 0;

const bool PLL_RECLOCK = false;

int main(void) {
	wdt_init();
	
	cpu_enable_irq(true);
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	VIC_CON(VIC_TPU_INT1_IRQ) = 1;
	
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
		PLL_CON3 = 0x10000302;
		USART_CLC(USART0) = 2 << MOD_CLC_RMC_SHIFT;
		STM_CLC = 0x001A1A14;
	}
	
	uint32_t ahb_freq = cpu_get_ahb_freq();
	uint32_t cpu_freq = cpu_get_freq();
	uint32_t sys_freq = cpu_get_sys_freq();
	uint32_t stm_freq = cpu_get_stm_freq();
	printf("By registers:\n");
	printf("fAHB: %d MHz\n", ahb_freq / 1000000);
	printf("fCPU: %d MHz\n", cpu_freq / 1000000);
	printf("fSYS: %d MHz\n", sys_freq / 1000000);
	printf("fSTM: %d MHz\n", stm_freq / 1000000);
	
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
	TPU_INT(1) = 9000;
	
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	
	TPU_SRC(0) = MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_SRE;
	
	stopwatch_init();
	
	last = stopwatch_get();
	last2 = stopwatch_get();
	
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	
	volatile uint32_t last_cnt = 0;
	volatile uint32_t last_cnt2 = 0;
	
	while (true) {
		cpu_enable_irq(false);
		if (cnt != last_cnt) {
			printf("period1: %d us [%d / %d]\n", period, cnt, val);
			last_cnt = cnt;
		}
		
		if (cnt2 != last_cnt2) {
			printf("period2: %d us [%d / %d]\n", period2, cnt2, val2);
			last_cnt2 = cnt2;
		}
		cpu_enable_irq(true);
		
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
	int irqn = VIC_IRQ_CURRENT;
	
	if (irqn == VIC_TPU_INT0_IRQ) {
		period = stopwatch_elapsed_us(last);
		last = stopwatch_get();
		val = TPU_COUNTER;
		
		cnt++;
		
		TPU_SRC(0) |= MOD_SRC_CLRR;
	} else if (irqn == VIC_TPU_INT1_IRQ) {
		period2 = stopwatch_elapsed_us(last2);
		last2 = stopwatch_get();
		val2 = TPU_COUNTER;
		
		cnt2++;
		
		TPU_SRC(1) |= MOD_SRC_CLRR;
	}
	
	VIC_IRQ_ACK = 1;
}
