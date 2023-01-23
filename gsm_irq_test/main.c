#include <pmb887x.h>

static volatile stopwatch_t last = 0;
static volatile uint32_t period = 0;
static volatile uint32_t cnt = 0;

const bool PLL_RECLOCK = false;

#define swap(x,y) { x = x + y; y = x - y; x = x - y; }

unsigned long gcd(unsigned long a, unsigned long b) {
	unsigned long r = a | b;

	if (!a || !b)
		return r;

	/* Isolate lsbit of r */
	r &= -r;

	while (!(b & r))
		b >>= 1;
	if (b == r)
		return r;

	for (;;) {
		while (!(a & r))
			a >>= 1;
		if (a == r)
			return r;
		if (a == b)
			return a;

		if (a < b)
			swap(a, b);
		a -= b;
		a >>= 1;
		if (a & r)
			a += b;
		a >>= 1;
	}
}

unsigned long lcm(unsigned long a, unsigned long b) {
	if (a && b)
		return (a / gcd(a, b)) * b;
	else
		return 0;
}

int main(void) {
	wdt_init();
	
	cpu_enable_irq(true);
	NVIC_CON(NVIC_TPU_INT0_IRQ) = 1;
	NVIC_CON(NVIC_TPU_INT1_IRQ) = 1;
	
	printf("usart clc: %d\n", (USART_CLC(USART0) & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT);
	
	if (PLL_RECLOCK) {
		PLL_OSC = 0x00030101;
		PLL_CON0 = 0x1120080B;
		PLL_CON1 = 0x00020002;
		PLL_CON2 = 0x1000F377;
		PLL_CON3 = 0x10000303;
		USART_CLC(USART0) = 1 << MOD_CLC_RMC_SHIFT;
		/*
		PLL_OSC = 0x00030101;
		PLL_CON0 = 0x1120080B;
		PLL_CON1 = 0x00220002;
		PLL_CON2 = 0x0000E070;
		//PLL_CON3 = 0x10000302;
		USART_CLC(USART0) = 2 << MOD_CLC_RMC_SHIFT;
		STM_CLC = 0x001A1A14;
		*/
	}
	
	uint32_t ftpu = 26000000;
	uint32_t freq = 1000000;
	
	uint64_t v = (freq * 6) / gcd((freq * 6), ftpu) * (uint64_t) ftpu;
	uint32_t L = v / (freq * 6);
	uint32_t K = v / ftpu;
	
	uint32_t real_freq = L && K ? (ftpu / L * K) / 6 : ftpu / 6;
	
	printf("L=%08X (%u)\r\n", L, L);
	printf("K=%08X (%u)\r\n", K, K);
	printf("real_freq=%u\r\n", real_freq);
	
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_PLLCON0 = K << TPU_PLLCON0_K_DIV_SHIFT;
	TPU_PLLCON1 = L << TPU_PLLCON1_L_DIV_SHIFT;
	TPU_PLLCON2 = TPU_PLLCON2_INIT | TPU_PLLCON2_LOAD;
	
	/*
	for (int i = 0; i < 512; i++)
		TPU_RAM(i) = 0;
	
	TPU_UNK5 = 0;
	TPU_UNK4 = 6;
	TPU_UNK6 = 0x80000000;
	*/
	
	TPU_OVERFLOW = 32768-1;
	TPU_OFFSET = 0;
	TPU_INT(0) = 0;
	TPU_INT(1) = 30000;
	
	last = stopwatch_get();
	
	TPU_SRC(0) = MOD_SRC_CLRR;
	//TPU_SRC(1) = MOD_SRC_CLRR;
	
	TPU_SRC(0) = MOD_SRC_SRE;
	//TPU_SRC(1) = MOD_SRC_SRE;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	
	while (true) {
		printf("period: %d us [%d]\n", period, cnt);
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

