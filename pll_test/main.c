#include <pmb887x.h>
#include <math.h>

struct bench_t {
	uint32_t cpu;
	uint32_t cpu_raw;
	uint32_t tpu;
	uint32_t tpu_raw;
	uint32_t stm;
	uint64_t stm_raw;
};

struct clock_config_t {
	uint32_t ndiv;
	uint32_t k1;
	uint32_t k2;
	uint32_t cpu_div;
	uint32_t freq;
};

static volatile uint32_t tpu_cnt = 0;
static uint32_t old_pll[5];

static void rtc_init(void) {
	SCU_RTCIF = 0xAA;
	
	RTC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	RTC_CTRL |= RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN;
	RTC_CON |= RTC_CON_PRE;
	RTC_T14 = (61440 << RTC_T14_CNT_SHIFT) | (61440 << RTC_T14_REL_SHIFT);
	RTC_REL = 0;
	RTC_ALARM = 0;
	RTC_SRC = 0;
	RTC_ISNC = 0;
	RTC_CTRL |= RTC_CTRL_CLK_SEL | RTC_CTRL_CLR_RTCBAD | RTC_CTRL_CLR_RTCINT;
	
	if (RTC_CTRL & RTC_CTRL_RTCBAD) {
		printf("RTC init error\n");
		while (true);
	}
	
	RTC_CON |= RTC_CON_RUN;
}

static void tpu_init(void) {
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_PLLCON0 = 1 << TPU_PLLCON0_K_DIV_SHIFT;
	TPU_PLLCON1 = 1 << TPU_PLLCON1_L_DIV_SHIFT;
	TPU_PLLCON2 = TPU_PLLCON2_INIT;
	
	for (int i = 0; i < 512; i++)
		TPU_RAM(i) = 0;
	
	TPU_UNK5 = 0;
	TPU_UNK4 = 6;
	TPU_UNK6 = 0x80000000;
	
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 0;
	TPU_INT(0) = 0;
	TPU_INT(1) = 30000;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	
	TPU_SRC(0) = MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_SRE;
}

static void benchmark(struct bench_t *result) {
	wdt_serve();
	
	uint32_t cycles = 100000;
	uint32_t t = stopwatch_get();
	__asm__ volatile (
		"1: \n"
		"SUBS %0, #1\n"
		"BCS 1b \n" : : "r" (cycles) : "memory"
	);
	result->cpu_raw = cycles / ((stopwatch_get() - t) / 10000);
	wdt_serve();
	
	uint32_t start = RTC_CNT + 1;
	while (RTC_CNT < start)
		wdt_serve();
	
	uint32_t timer_start = tpu_cnt;
	uint32_t timer2_start = stopwatch_get();
	while (true) {
		uint32_t now = RTC_CNT;
		uint32_t elapsed = now - start;
		
		if (elapsed >= 1) {
			result->tpu_raw = (tpu_cnt - timer_start);
			result->stm_raw = (stopwatch_get() - timer2_start);
			break;
		}
		
		wdt_serve();
	}
	
	wdt_serve();
	
	result->tpu = (result->tpu_raw * 2600 / 4333) * 10000;
	result->stm = (result->stm_raw * 2600 / 25998000) * 10000;
	result->cpu = (result->cpu_raw * 2600 / 2500) * 10000;
}

static void clock_setup(struct clock_config_t *cfg) {
	old_pll[0] = PLL_OSC;
	old_pll[1] = PLL_CON0;
	old_pll[2] = PLL_CON1;
	old_pll[3] = PLL_CON2;
	old_pll[4] = PLL_CON3;
	
	PLL_OSC = (cfg->ndiv << PLL_OSC_NDIV_SHIFT) | 0x707;
	PLL_CON0 = (cfg->k1 << PLL_CON0_PLL1_K1_SHIFT) | (cfg->k1 << PLL_CON0_PLL1_K2_SHIFT);
	PLL_CON1 = PLL_CON1_AHB_CLKSEL_PLL1 | PLL_CON1_FSYS_CLKSEL_PLL;
	PLL_CON2 = (cfg->cpu_div << PLL_CON2_CPU_DIV_SHIFT) | PLL_CON2_CPU_DIV_EN;
	
	while (!(PLL_STAT & PLL_STAT_LOCK));
}

static void clock_restore(void) {
	PLL_CON1 = old_pll[2];
	PLL_CON0 = old_pll[1];
	PLL_CON2 = old_pll[3];
	PLL_CON3 = old_pll[4];
	PLL_OSC = old_pll[0];
	
	while (!(PLL_STAT & PLL_STAT_LOCK));
}

int main(void) {
	struct bench_t b;
	
	wdt_init();
	rtc_init();
	tpu_init();
	
	cpu_enable_irq(true);
	NVIC_CON(NVIC_TPU_INT0_IRQ) = 1;
	NVIC_CON(NVIC_TPU_INT1_IRQ) = 1;
	
	struct clock_config_t clocks[] = {
		{ .ndiv = 2, .k1 = 15, .k2 = 7, .cpu_div = 3, .freq = 2 },
		{ .ndiv = 3, .k1 = 1, .k2 = 1, .cpu_div = 1, .freq = 104 },
		{ .ndiv = 3, .k1 = 1, .k2 = 1, .cpu_div = 0, .freq = 208 },
		{ .ndiv = 4, .k1 = 1, .k2 = 1, .cpu_div = 0, .freq = 260 },
		{ .ndiv = 5, .k1 = 1, .k2 = 1, .cpu_div = 0, .freq = 312 },
	};
	
	for (uint32_t i = 0; i < ARRAY_SIZE(clocks); i++) {
		printf("[clock] ndiv: %d, k1: %d, k2: %d, cpu_div: %d\n", clocks[i].ndiv, clocks[i].k1, clocks[i].k2, clocks[i].cpu_div);
		
		clock_setup(&clocks[i]);
		benchmark(&b);
		uint32_t ahb_freq = cpu_get_ahb_freq();
		uint32_t cpu_freq = cpu_get_freq();
		uint32_t sys_freq = cpu_get_sys_freq();
		uint32_t stm_freq = cpu_get_stm_freq();
		clock_restore();
		
		printf("By benchmark:\n");
		printf("fCPU: %d MHz [%d]\n", b.cpu / 1000000, b.cpu_raw);
		printf("fSYS: %d MHz [%d]\n", b.tpu / 1000000, b.tpu_raw);
		printf("fSTM: %d MHz [%d]\n", b.stm / 1000000, (int) b.stm_raw);
		printf("\n");
		
		printf("By registers:\n");
		printf("fAHB: %d MHz\n", ahb_freq / 1000000);
		printf("fCPU: %d MHz\n", cpu_freq / 1000000);
		printf("fSYS: %d MHz\n", sys_freq / 1000000);
		printf("fSTM: %d MHz\n", stm_freq / 1000000);
		printf("--------------------------\n");
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
		tpu_cnt++;
		TPU_SRC(0) |= MOD_SRC_CLRR;
	}
	
	NVIC_IRQ_ACK = 1;
}

