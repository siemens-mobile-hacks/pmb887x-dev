#include <pmb887x.h>

#include "test.h"

#define ITCM_BASE 0x01000000
#define TCM_REGION_SIZE_8K (4 << 2)
#define TCM_REGION_ENABLE BIT(0)
#define CGU_MEASURE_NOPS 512
#define CGU_MEASURE_ITERATIONS 1000
#define CGU_DIVIDER_MEASURE_ITERATIONS 1000
#define EBU_FLASH_BASE 0xA0000000
#define EBU_FLASH_PROBE_WORDS 32768
#define EBU_CLOCK_WAIT_ITERATIONS 100000
#define AHB_PER_PROBE_READS 32768
#define MCI_TIMEOUT_TARGET_HZ 200
#define MCI_TIMEOUT_MIN_CYCLES 512
#define MCI_TIMEOUT_MAX_CYCLES 262144
#define MCI_TIMEOUT_WAIT_MS 2000
#define RTC_T14_RELOAD 61440
#define RTC_T14_PERIOD (65536 - RTC_T14_RELOAD)
#define RTC_T14_MEASURE_TICKS 512
#define CLOCK_MANAGER_WAIT_ITERATIONS 100000
#define USART_IRQ_MASK 0xFF
#define FPI1_BENCHMARK_FRAMES 16
#define DMA_CHANNEL 7

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;
static volatile uint32_t tpu_frame_count;
static uint32_t dma_source[16] __attribute__((aligned(16)));
static volatile uint32_t dma_destination[ARRAY_SIZE(dma_source)] __attribute__((aligned(16)));

enum ebu_source_index {
	EBU_SOURCE_PLL,
	EBU_SOURCE_PHASE1,
	EBU_SOURCE_PHASE2,
	EBU_SOURCE_PHASE3,
	EBU_SOURCE_PHASE4,
	EBU_SOURCE_OSC,
	EBU_SOURCE_COUNT,
};

uint32_t cgu_execute_itcm(uint32_t address, uint32_t iterations);
void cgu_execute_itcm_source(uint32_t address, uint32_t iterations, uint32_t selected_con1, uint32_t restore_con1);
void cgu_execute_itcm_cpu_div(uint32_t address, uint32_t iterations, uint32_t selected_con2, uint32_t restore_con2);
extern const uint32_t cgu_cpu_div_itcm_template_start[];
extern const uint32_t cgu_cpu_div_itcm_template_end[];
static void test_measured_frequency(const char *name, uint32_t expected_khz, uint32_t measured_khz);

static uint32_t read_itcm(void) {
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c9, c1, 1" : "=r" (value));
	return value;
}

static void write_itcm(uint32_t value) {
	__asm__ volatile("mcr p15, 0, %0, c9, c1, 1" : : "r" (value) : "memory");
}

static void sync_code(void) {
	uint32_t value = 0;
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (value) : "memory");
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r" (value) : "memory");
}

static bool wait_for_irq(void) {
	stopwatch_t start = stopwatch_get();

	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return irq_count != 0;
}

static bool wait_for_pll_lock(void) {
	stopwatch_t start = stopwatch_get();

	while ((CGU_STAT & CGU_STAT_LOCK) == 0 && stopwatch_elapsed_ms(start) < 20)
		test_watchdog_serve();

	return (CGU_STAT & CGU_STAT_LOCK) != 0;
}

static bool wait_for_pll_unlock(void) {
	stopwatch_t start = stopwatch_get();

	while ((CGU_STAT & CGU_STAT_LOCK) != 0 && stopwatch_elapsed_ms(start) < 20)
		test_watchdog_serve();

	return (CGU_STAT & CGU_STAT_LOCK) == 0;
}

static bool wait_for_ebu_clock_transition(void) {
	for (uint32_t index = 0; index < EBU_CLOCK_WAIT_ITERATIONS; index++)
		if ((SCU_EBUCLC2 & SCU_EBUCLC2_READY) != 0)
			return true;

	return false;
}

static bool apply_pll_osc(uint32_t value) {
	CGU_OSC = value & ~(CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N);
	if ((value & CGU_OSC_PLL_POWER_UP) == 0)
		return true;

	CGU_OSC = (value | CGU_OSC_PLL_POWER_UP) & ~CGU_OSC_PLL_BYPASS_N;
	if (!wait_for_pll_lock())
		return false;

	CGU_OSC = value;
	return true;
}

static void test_boot_state(void) {
	/* PLL clock registers are live boot configuration and cannot be reset while executing from this clock tree. */
	/* CGU_SRC is omitted because its lock request is already pending asynchronously after boot. */
	test_check("PLL is powered after boot", (CGU_OSC & CGU_OSC_PLL_POWER_UP) != 0);
	test_eq_u32("PLL is locked after boot", CGU_STAT_LOCK, CGU_STAT & CGU_STAT_LOCK);
	printf("# CGU boot configuration: OSC=%08X CON0=%08X CON1=%08X CON2=%08X CON3=%08X\n",
		(unsigned int) CGU_OSC, (unsigned int) CGU_CON0, (unsigned int) CGU_CON1, (unsigned int) CGU_CON2,
		(unsigned int) CGU_CON3);
}

static void init_reference_rtc(void) {
	SCU_RTCIF = 0xAA;
	RTC_CLC = (1 << MOD_CLC_RMC_SHIFT);
	RTC_CTRL |= RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN;
	RTC_CON |= RTC_CON_PRE;
	RTC_T14 = (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT);
	RTC_REL = 0;
	RTC_ALARM = 0;
	RTC_SRC = 0;
	RTC_ISNC = 0;
	RTC_CTRL |= RTC_CTRL_CLK_SEL | RTC_CTRL_CLR_RTCBAD | RTC_CTRL_CLR_RTCINT;
	RTC_CON |= RTC_CON_RUN;
}

static void init_fsys_tpu(void) {
	TPU_CLC = (1 << MOD_CLC_RMC_SHIFT);
	TPU_GSMCLK1 = (1 << TPU_GSMCLK1_K_SHIFT);
	TPU_GSMCLK2 = (1 << TPU_GSMCLK2_L_SHIFT);
	TPU_GSMCLK3 = TPU_GSMCLK3_INIT;
	for (uint32_t index = 0; index < 512; index++)
		TPU_RAM(index) = 0;
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 0;
	TPU_INT(0) = 0;
	TPU_INT(1) = 30000;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	TPU_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_CLRR;
}

static void benchmark_example_clocks(uint32_t *tpu_frames, uint32_t *stm_ticks) {
	test_watchdog_serve();
	uint32_t start = RTC_CNT + 1;
	while (RTC_CNT < start)
		test_watchdog_serve();
	uint32_t first_tpu = tpu_frame_count;
	stopwatch_t first_stm = stopwatch_get();
	while (RTC_CNT - start < 1)
		test_watchdog_serve();
	*tpu_frames = tpu_frame_count - first_tpu;
	*stm_ticks = (uint32_t) stopwatch_elapsed(first_stm);
	test_watchdog_serve();
}

static uint32_t benchmark_stm_ticks(void) {
	test_watchdog_serve();
	uint32_t first_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
	stopwatch_t first_stm = stopwatch_get();
	uint32_t elapsed_t14;
	do {
		uint32_t current_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
		elapsed_t14 = (current_t14 - first_t14) & (RTC_T14_PERIOD - 1);
		test_watchdog_serve();
	} while (elapsed_t14 < RTC_T14_MEASURE_TICKS);
	return (uint32_t) stopwatch_elapsed(first_stm);
}

static uint32_t stm_ticks_to_khz(uint32_t stm_ticks) {
	return (uint64_t) stm_ticks * RTC_T14_PERIOD / RTC_T14_MEASURE_TICKS / 1000;
}

static void configure_fpi1_usart(void) {
	uint32_t control = USART_CON_M_ASYNC_8BIT | USART_CON_FDE | USART_CON_LB;
	USART_CLC(USART1) = (1 << MOD_CLC_RMC_SHIFT);
	USART_CON(USART1) = control;
	USART_BG(USART1) = 0x0C;
	USART_FDV(USART1) = 0x1D8;
	USART_TMO(USART1) = 0;
	USART_DMAE(USART1) = 0;
	USART_IMSC(USART1) = 0;
	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_RXFCON(USART1) = 0;
	USART_TXFCON(USART1) = 0;
	USART_WHBCON(USART1) = USART_WHBCON_CLRPE | USART_WHBCON_CLRFE | USART_WHBCON_CLROE;
	USART_CON(USART1) = control | USART_CON_CON_R;
	USART_WHBCON(USART1) = USART_WHBCON_SETREN;
}

static bool fpi1_usart_loopback(uint8_t value) {
	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_TXB(USART1) = value;
	for (uint32_t index = 0; index < CLOCK_MANAGER_WAIT_ITERATIONS && (USART_RIS(USART1) & USART_RIS_RX) == 0; index++)
		__asm__ volatile("nop");
	if ((USART_RIS(USART1) & USART_RIS_RX) == 0)
		return false;
	uint32_t actual = USART_RXB(USART1);
	USART_ICR(USART1) = USART_ICR_RX;
	return actual == value;
}

static bool benchmark_fpi1_loopback(uint32_t *ticks, uint32_t *checksum) {
	uint32_t sum = 0;
	stopwatch_t start = stopwatch_get();

	for (uint32_t frame = 0; frame < FPI1_BENCHMARK_FRAMES; frame++) {
		uint8_t value = 0x40 + frame;

		USART_ICR(USART1) = USART_IRQ_MASK;
		USART_TXB(USART1) = value;
		uint32_t wait = 0;
		while (wait < CLOCK_MANAGER_WAIT_ITERATIONS && (USART_RIS(USART1) & USART_RIS_RX) == 0) {
			__asm__ volatile("nop");
			wait++;
		}
		if ((USART_RIS(USART1) & USART_RIS_RX) == 0)
			return false;

		sum += USART_RXB(USART1) & 0xFF;
	}

	*ticks = (uint32_t) stopwatch_elapsed(start);
	*checksum = sum;
	return true;
}

static void test_fpi1_pll_div_2_frequency(void) {
	if (test_is_qemu()) {
		test_skip("FPI1 source frequency matches the CGU model", "QEMU does not model USART timing");
		return;
	}

	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_usart1_clc = USART_CLC(USART1);
	uint32_t ticks[3] = { 0 };
	uint32_t checksum[3] = { 0 };
	bool completed[3] = { false };
	uint32_t selected_osc = initial_osc | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
	bool locked = apply_pll_osc(selected_osc);
	uint32_t con1_base = initial_con1 & ~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV |
		CGU_CON1_FSYS_CLKSEL);

	if (locked) {
		CGU_CON1 = con1_base | CGU_CON1_FPI1_CLKSEL_OSC | CGU_CON1_FSYS_CLKSEL_BYPASS;
		configure_fpi1_usart();
		fpi1_usart_loopback(0xA5);
		completed[0] = benchmark_fpi1_loopback(&ticks[0], &checksum[0]);

		CGU_CON1 = con1_base | CGU_CON1_FPI1_CLKSEL_PLL_DIV_2 | CGU_CON1_FSYS_CLKSEL_BYPASS;
		fpi1_usart_loopback(0xA5);
		completed[1] = benchmark_fpi1_loopback(&ticks[1], &checksum[1]);

		CGU_CON1 = con1_base | CGU_CON1_FPI1_CLKSEL_PLL_DIV_2 | CGU_CON1_FSYS_CLKSEL_PLL;
		fpi1_usart_loopback(0xA5);
		completed[2] = benchmark_fpi1_loopback(&ticks[2], &checksum[2]);
	}

	CGU_CON1 = initial_con1;
	USART_CLC(USART1) = initial_usart1_clc;
	bool restored = apply_pll_osc(initial_osc);

	printf("# FPI1 loopback: OSC=%u PLL_DIV_2_FSYS_BYPASS=%u PLL_DIV_2_FSYS_PLL=%u STM ticks\n",
		(unsigned int) ticks[0], (unsigned int) ticks[1], (unsigned int) ticks[2]);
	test_check("PLL locks for FPI1 PLL/2 frequency measurement", locked);
	test_check("FPI1 loopback completes with PLL enabled",
		completed[0] && completed[1] && completed[2]);
	test_check("FPI1 loopback data is independent of source configuration",
		checksum[0] == checksum[1] && checksum[0] == checksum[2]);
	test_check("FPI1 PLL/2 clock is approximately twice the oscillator source at 104 MHz PLL",
		test_u32_in_interval(ticks[0], ticks[1] * 17 / 10, ticks[1] * 21 / 10));
	test_check("fSYS selection does not change the FPI1 PLL/2 frequency",
		test_u32_in_interval(ticks[2], ticks[1] * 95 / 100, ticks[1] * 105 / 100));
	test_check("FPI1 PLL/2 frequency measurement restores PLL registers", restored &&
		CGU_OSC == initial_osc && CGU_CON1 == initial_con1);
}

static void test_fpi1_pll_div_2_dependency(void) {
	if (test_is_qemu()) {
		test_skip("FPI1 frequency follows the PLL frequency", "QEMU does not model USART timing");
		return;
	}

	static const struct {
		uint32_t ndiv;
		uint32_t mdiv;
		uint32_t pll_khz;
	} CONFIGS[] = {
		{ 3, 0, 104000 },
		{ 4, 0, 130000 },
		{ 5, 0, 156000 },
	};
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_usart1_clc = USART_CLC(USART1);
	uint32_t ticks[ARRAY_SIZE(CONFIGS)] = { 0 };
	uint32_t normalized_ticks[ARRAY_SIZE(CONFIGS)] = { 0 };
	uint32_t checksum[ARRAY_SIZE(CONFIGS)] = { 0 };
	bool completed[ARRAY_SIZE(CONFIGS)] = { false };
	bool locked[ARRAY_SIZE(CONFIGS)] = { false };
	uint32_t con1_base = (initial_con1 & ~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV |
		CGU_CON1_FSYS_CLKSEL | CGU_CON1_AHB_CLKSEL)) | CGU_CON1_FSYS_CLKSEL_BYPASS |
		CGU_CON1_AHB_CLKSEL_BYPASS;

	for (size_t index = 0; index < ARRAY_SIZE(CONFIGS); index++) {
		CGU_CON1 = con1_base | CGU_CON1_FPI1_CLKSEL_DISABLE;
		uint32_t osc = (initial_osc & ~(CGU_OSC_NDIV | CGU_OSC_MDIV)) |
			(CONFIGS[index].ndiv << CGU_OSC_NDIV_SHIFT) |
			(CONFIGS[index].mdiv << CGU_OSC_MDIV_SHIFT) |
			CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
		locked[index] = apply_pll_osc(osc);
		if (!locked[index])
			continue;

		CGU_CON1 = con1_base | CGU_CON1_FPI1_CLKSEL_PLL_DIV_2;
		configure_fpi1_usart();
		fpi1_usart_loopback(0xA5);
		completed[index] = benchmark_fpi1_loopback(&ticks[index], &checksum[index]);
		normalized_ticks[index] = ticks[index] * (CONFIGS[index].pll_khz / 1000);
	}

	CGU_CON1 = con1_base | CGU_CON1_FPI1_CLKSEL_DISABLE;
	bool restored = apply_pll_osc(initial_osc);
	CGU_CON1 = initial_con1;
	USART_CLC(USART1) = initial_usart1_clc;

	printf("# FPI1 selector 2 PLL dependency: PLL=%u/%u/%u kHz ticks=%u/%u/%u\n",
		(unsigned int) CONFIGS[0].pll_khz, (unsigned int) CONFIGS[1].pll_khz,
		(unsigned int) CONFIGS[2].pll_khz, (unsigned int) ticks[0], (unsigned int) ticks[1],
		(unsigned int) ticks[2]);
	test_check("PLL locks for every FPI1 selector 2 dependency measurement",
		locked[0] && locked[1] && locked[2]);
	test_check("FPI1 loopback completes at every tested PLL frequency",
		completed[0] && completed[1] && completed[2]);
	test_check("FPI1 loopback data is independent of PLL frequency",
		checksum[0] == checksum[1] && checksum[0] == checksum[2]);
	test_check("FPI1 selector 2 frequency increases with PLL frequency",
		ticks[0] > ticks[1] && ticks[1] > ticks[2]);
	test_check("FPI1 selector 2 changes substantially between 104 and 156 MHz PLL",
		ticks[0] > ticks[2] * 5 / 4);
	test_check("FPI1 selector 2 timing is inversely proportional to PLL frequency",
		test_u32_in_interval(normalized_ticks[1], normalized_ticks[0] * 98 / 100,
			normalized_ticks[0] * 102 / 100) &&
		test_u32_in_interval(normalized_ticks[2], normalized_ticks[0] * 98 / 100,
			normalized_ticks[0] * 102 / 100));
	test_check("FPI1 PLL dependency measurement restores PLL registers", restored &&
		CGU_OSC == initial_osc && CGU_CON1 == initial_con1);
}

static void test_fsys_frequencies(void) {
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con0 = CGU_CON0;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_con2 = CGU_CON2;
	uint32_t initial_con3 = CGU_CON3;
	uint32_t initial_tpu_vic = VIC_CON(VIC_TPU_INT0_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	init_reference_rtc();
	init_fsys_tpu();
	tpu_frame_count = 0;
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	cpu_enable_irq(true);

	static const struct {
		uint32_t clksel;
		const char *name;
	} FSYS_SOURCES[] = {
		{ CGU_CON1_FSYS_CLKSEL_BYPASS, "fSYS bypass selects the oscillator" },
		{ CGU_CON1_FSYS_CLKSEL_PLL, "fSYS PLL source divides PLL by two" },
	};
	CGU_OSC =
		(3 << CGU_OSC_NDIV_SHIFT) |
		CGU_OSC_PLL_POWER_UP |
		CGU_OSC_PHASE1_POWER_UP |
		CGU_OSC_PLL_BYPASS_N |
		CGU_OSC_PHASE1_BYPASS_N;
	CGU_CON0 = (1 << CGU_CON0_PHASE1_K1_SHIFT) | (1 << CGU_CON0_PHASE1_K2_SHIFT);
	CGU_CON2 = (1 << CGU_CON2_CPU_DIV_SHIFT) | CGU_CON2_CPU_DIV_EN;
	bool locked = wait_for_pll_lock();

	for (uint32_t index = 0; index < ARRAY_SIZE(FSYS_SOURCES); index++) {
		CGU_CON1 = CGU_CON1_AHB_CLKSEL_PHASE1 | FSYS_SOURCES[index].clksel;
		uint32_t model_hz = cpu_get_sys_freq();
		uint32_t tpu_frames = 0;
		uint32_t stm_ticks = 0;
		if (locked)
			benchmark_example_clocks(&tpu_frames, &stm_ticks);
		uint32_t measured_khz = (uint64_t) tpu_frames * 26000 / 4333;

		printf("# %s: TPU=%u frames\n", FSYS_SOURCES[index].name, (unsigned int) tpu_frames);
		test_measured_frequency(FSYS_SOURCES[index].name, model_hz / 1000, measured_khz);
	}

	CGU_CON1 = initial_con1;
	CGU_CON0 = initial_con0;
	CGU_CON2 = initial_con2;
	CGU_CON3 = initial_con3;
	bool restored = apply_pll_osc(initial_osc);

	cpu_enable_irq(false);
	TPU_PARAM = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	TPU_CLC = MOD_CLC_DISR;
	VIC_CON(VIC_TPU_INT0_IRQ) = initial_tpu_vic;
	cpu_enable_irq(!irq_was_disabled);

	test_check("PLL locks for fSYS measurements", locked);
	test_check("fSYS measurements restore CGU registers", restored && CGU_OSC == initial_osc &&
		CGU_CON0 == initial_con0 && CGU_CON1 == initial_con1 && CGU_CON2 == initial_con2 &&
		CGU_CON3 == initial_con3);
}

static void write_itcm_measurement_loop(void) {
	for (uint32_t index = 0; index < CGU_MEASURE_NOPS; index++)
		MMIO32(ITCM_BASE + index * sizeof(uint32_t)) = 0xE1A00000;

	MMIO32(ITCM_BASE + CGU_MEASURE_NOPS * sizeof(uint32_t)) = 0xE2500001;
	uint32_t branch_offset = (uint32_t) (-(CGU_MEASURE_NOPS + 3)) & 0x00FFFFFF;
	MMIO32(ITCM_BASE + (CGU_MEASURE_NOPS + 1) * sizeof(uint32_t)) = 0x1A000000 | branch_offset;
	MMIO32(ITCM_BASE + (CGU_MEASURE_NOPS + 2) * sizeof(uint32_t)) = 0xE12FFF14;
	sync_code();
}

static void write_itcm_source_measurement_loop(void) {
	MMIO32(ITCM_BASE) = 0xE5831000;
	MMIO32(ITCM_BASE + sizeof(uint32_t)) = 0xE593C000;
	for (uint32_t index = 0; index < CGU_MEASURE_NOPS; index++)
		MMIO32(ITCM_BASE + (index + 2) * sizeof(uint32_t)) = 0xE1A00000;

	MMIO32(ITCM_BASE + (CGU_MEASURE_NOPS + 2) * sizeof(uint32_t)) = 0xE2500001;
	uint32_t branch_offset = (uint32_t) (-(CGU_MEASURE_NOPS + 3)) & 0x00FFFFFF;
	MMIO32(ITCM_BASE + (CGU_MEASURE_NOPS + 3) * sizeof(uint32_t)) = 0x1A000000 | branch_offset;
	MMIO32(ITCM_BASE + (CGU_MEASURE_NOPS + 4) * sizeof(uint32_t)) = 0xE5832000;
	MMIO32(ITCM_BASE + (CGU_MEASURE_NOPS + 5) * sizeof(uint32_t)) = 0xE593C000;
	MMIO32(ITCM_BASE + (CGU_MEASURE_NOPS + 6) * sizeof(uint32_t)) = 0xE12FFF14;
	sync_code();
}

static void write_itcm_cpu_div_measurement_loop(void) {
	uint32_t words = (uint32_t) (cgu_cpu_div_itcm_template_end - cgu_cpu_div_itcm_template_start);
	for (uint32_t index = 0; index < words; index++)
		MMIO32(ITCM_BASE + index * sizeof(uint32_t)) = cgu_cpu_div_itcm_template_start[index];
	sync_code();
}

static uint32_t measured_frequency_khz(uint32_t elapsed_ticks, uint32_t iterations) {
	uint64_t work_cycles = (uint64_t) CGU_MEASURE_NOPS * iterations;
	return work_cycles * (CPU_OSC_FREQ / 1000) / elapsed_ticks;
}

static uint32_t measure_ahb_source_khz(uint32_t selected_con1, uint32_t restore_con1) {
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	stopwatch_t start = stopwatch_get();
	cgu_execute_itcm_source(ITCM_BASE, CGU_MEASURE_ITERATIONS, selected_con1, restore_con1);
	uint32_t elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	cpu_enable_irq(!irq_was_disabled);

	return measured_frequency_khz(elapsed_ticks, CGU_MEASURE_ITERATIONS);
}

static uint32_t measure_cpu_divider_khz(uint32_t selected_con2, uint32_t restore_con2) {
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	stopwatch_t start = stopwatch_get();
	cgu_execute_itcm_cpu_div(ITCM_BASE, CGU_DIVIDER_MEASURE_ITERATIONS, selected_con2, restore_con2);
	uint32_t elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	cpu_enable_irq(!irq_was_disabled);

	return measured_frequency_khz(elapsed_ticks, CGU_DIVIDER_MEASURE_ITERATIONS);
}

static void test_measured_frequency(const char *name, uint32_t expected_khz, uint32_t measured_khz) {
	printf("# %s: expected=%u kHz measured=%u kHz\n", name, (unsigned int) expected_khz,
		(unsigned int) measured_khz);
	test_check(name, test_u32_in_interval(measured_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));
}

static uint32_t benchmark_ebu_flash_reads(uint32_t *checksum) {
	/* cgu_loader copies the test into internal RAM before any EBU clock changes. */
	const volatile uint32_t *flash = (const volatile uint32_t *) EBU_FLASH_BASE;
	uint32_t value = 0;
	stopwatch_t start = stopwatch_get();

	for (uint32_t index = 0; index < EBU_FLASH_PROBE_WORDS; index++)
		value ^= flash[index];

	*checksum = value;
	return (uint32_t) stopwatch_elapsed(start);
}

#ifdef PMB8876
static uint32_t benchmark_ahb_per_reads(uint32_t *checksum) {
	uint32_t sum = 0;
	stopwatch_t start = stopwatch_get();

	for (uint32_t index = 0; index < AHB_PER_PROBE_READS; index++)
		sum += MMICIF_ID;

	*checksum = sum;
	return (uint32_t) stopwatch_elapsed(start);
}

static bool measure_mmci_frequency(uint32_t expected_hz, uint32_t *measured_hz, uint32_t *elapsed_ticks) {
	uint32_t timeout_cycles = expected_hz / MCI_TIMEOUT_TARGET_HZ;
	if (timeout_cycles < MCI_TIMEOUT_MIN_CYCLES)
		timeout_cycles = MCI_TIMEOUT_MIN_CYCLES;
	if (timeout_cycles > MCI_TIMEOUT_MAX_CYCLES)
		timeout_cycles = MCI_TIMEOUT_MAX_CYCLES;

	MCI_DATACTRL = 0;
	stopwatch_usleep_wd(1000);
	MCI_CLEAR = MCI_CLEAR_DATATIMEOUTCLR;
	stopwatch_usleep_wd(1000);
	MCI_DATATIMER = timeout_cycles;
	MCI_DATALENGTH = 1;
	stopwatch_usleep_wd(1000);
	stopwatch_t start = stopwatch_get();
	MCI_DATACTRL = MCI_DATACTRL_DIRECTION_READ | MCI_DATACTRL_ENABLE;
	while ((MCI_STATUS & MCI_STATUS_DATATIMEOUT) == 0 && stopwatch_elapsed_ms(start) < MCI_TIMEOUT_WAIT_MS)
		test_watchdog_serve();

	*elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	bool completed = (MCI_STATUS & MCI_STATUS_DATATIMEOUT) != 0;
	MCI_DATACTRL = 0;
	MCI_CLEAR = MCI_CLEAR_DATATIMEOUTCLR;
	if (!completed)
		return false;

	*measured_hz = (uint64_t) timeout_cycles * cpu_get_stm_freq() / *elapsed_ticks;
	return true;
}

static bool probe_mci_command_progress(uint32_t *elapsed_ticks) {
	MCI_COMMAND = 0;
	MCI_CLEAR = MCI_CLEAR_CMDSENTCLR;
	stopwatch_t start = stopwatch_get();
	MCI_COMMAND = MCI_COMMAND_ENABLE;
	while ((MCI_STATUS & MCI_STATUS_CMDSENT) == 0 && stopwatch_elapsed_ms(start) < MCI_TIMEOUT_WAIT_MS)
		test_watchdog_serve();
	*elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	bool completed = (MCI_STATUS & MCI_STATUS_CMDSENT) != 0;
	MCI_COMMAND = 0;
	MCI_CLEAR = MCI_CLEAR_CMDSENTCLR;
	return completed;
}

static void test_mmci_source_frequencies(void) {
	if (test_is_qemu()) {
		test_skip("MMCI source and divider frequencies match the CGU model", "QEMU does not implement the MCI core");
		return;
	}

	static const struct {
		const char *name;
		uint32_t source;
	} SOURCES[] = {
		{ "OSC", CGU_CON3_MMCI_CLKSEL_OSC },
		{ "CLK32K", CGU_CON3_MMCI_CLKSEL_CLK32K },
		{ "PHASE4", CGU_CON3_MMCI_CLKSEL_PHASE4 },
	};
	static const uint32_t DIVIDERS[] = {
		CGU_CON3_MMCI_CLKDIV_DIV1,
		CGU_CON3_MMCI_CLKDIV_DIV2,
		CGU_CON3_MMCI_CLKDIV_DIV4,
		CGU_CON3_MMCI_CLKDIV_DIV8,
	};
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con0 = CGU_CON0;
	uint32_t initial_con3 = CGU_CON3;
	uint32_t initial_mmci_clc = MMCI_CLC;
	uint32_t selected_osc =
		(initial_osc & ~(CGU_OSC_NDIV | CGU_OSC_MDIV)) |
		CGU_OSC_PLL_POWER_UP |
		CGU_OSC_PHASE4_POWER_UP |
		CGU_OSC_PLL_BYPASS_N |
		CGU_OSC_PHASE4_BYPASS_N |
		(7 << CGU_OSC_NDIV_SHIFT) |
		(1 << CGU_OSC_MDIV_SHIFT);
	bool locked = apply_pll_osc(selected_osc);

	CGU_CON0 = (initial_con0 & ~CGU_CON0_PHASE4_CONFIG) | (0x11 << CGU_CON0_PHASE4_CONFIG_SHIFT);
	MMCI_CLC = (1 << MOD_CLC_RMC_SHIFT);
	MCI_POWER = MCI_POWER_CTRL_POWER_UP;
	MCI_CLOCK = MCI_CLOCK_BYPASS | MCI_CLOCK_ENABLE;
	MCI_POWER = MCI_POWER_CTRL_POWER_ON;
	MCI_MASK0 = 0;
	MCI_MASK1 = 0;
	MCI_TCR = MCI_TCR_ITEN;
	MCI_ITIP = MCI_ITIP_DATIN | MCI_ITIP_CMDIN;

	bool frequencies_match = locked;
	bool slowest_mode_stopped = false;
	for (size_t source = 0; source < ARRAY_SIZE(SOURCES); source++) {
		for (size_t divider = 0; divider < ARRAY_SIZE(DIVIDERS); divider++) {
			CGU_CON3 = (initial_con3 & ~(CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV)) |
				SOURCES[source].source | DIVIDERS[divider];
			stopwatch_usleep_wd(2000);
			uint32_t mmci_hz = cpu_get_mmci_freq();
			uint32_t rmc = (MMCI_CLC & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT;
			uint32_t mmci_clc_hz = mmci_hz / rmc;
			uint32_t expected_hz = mmci_clc_hz / 8;
			uint32_t measured_hz = 0;
			uint32_t elapsed_ticks = 0;
			bool measured = measure_mmci_frequency(expected_hz, &measured_hz, &elapsed_ticks);

			printf("# MMCI %s DIV%u: CGU=%u Hz CLC=%u Hz MCLK expected=%u Hz measured=%u Hz elapsed=%u STM ticks\n",
				SOURCES[source].name, (1U << divider), (unsigned int) mmci_hz, (unsigned int) mmci_clc_hz,
				(unsigned int) expected_hz, (unsigned int) measured_hz, (unsigned int) elapsed_ticks);
			bool is_slowest_mode = SOURCES[source].source == CGU_CON3_MMCI_CLKSEL_CLK32K &&
				DIVIDERS[divider] == CGU_CON3_MMCI_CLKDIV_DIV8;
			if (is_slowest_mode) {
				uint32_t command_ticks = 0;
				bool command_completed = probe_mci_command_progress(&command_ticks);
				printf("# MCI CLK32K DIV8 command progress: completed=%u elapsed=%u STM ticks\n",
					command_completed ? 1U : 0U, (unsigned int) command_ticks);
				slowest_mode_stopped = !measured && !command_completed;
				continue;
			}
			uint32_t tolerance_hz = expected_hz / 50;
			bool frequency_matches = measured &&
				test_u32_in_interval(measured_hz, expected_hz - tolerance_hz, expected_hz + tolerance_hz);
			frequencies_match = frequencies_match && frequency_matches;
		}
	}

	MCI_DATACTRL = 0;
	MCI_COMMAND = 0;
	MCI_CLEAR = MCI_CLEAR_DATATIMEOUTCLR | MCI_CLEAR_CMDSENTCLR;
	MCI_CLOCK = 0;
	MCI_POWER = MCI_POWER_CTRL_POWER_OFF;
	MCI_MASK0 = 0;
	MCI_MASK1 = 0;
	MCI_TCR = 0;
	MCI_ITIP = 0;
	MCI_DATATIMER = 0;
	MCI_DATALENGTH = 0;
	CGU_CON3 = initial_con3;
	MMCI_CLC = initial_mmci_clc;
	CGU_CON0 = initial_con0;
	bool restored = apply_pll_osc(initial_osc);

	test_check("MMCI source and divider frequencies match the CGU model", frequencies_match);
	test_check("MMCI CLK32K DIV8 stops the MCI core", slowest_mode_stopped);
	test_check("MMCI frequency measurement restores PLL and module registers", restored && CGU_OSC == initial_osc &&
		CGU_CON0 == initial_con0 && CGU_CON3 == initial_con3 && MMCI_CLC == initial_mmci_clc);
}

static void test_ahb_per_pll_div_2_source(void) {
	if (test_is_qemu()) {
		test_skip("AHB_PER source frequency matches the CGU model", "QEMU does not model MMICIF timing");
		return;
	}

	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con0 = CGU_CON0;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_con2 = CGU_CON2;
	uint32_t initial_con3 = CGU_CON3;
	uint32_t initial_mmicif_clc = MMICIF_CLC;
	uint32_t checksum[4] = { 0 };
	uint32_t ticks[4] = { 0 };
	uint32_t selected_osc =
		(initial_osc & ~(CGU_OSC_NDIV | CGU_OSC_MDIV)) |
		CGU_OSC_PLL_POWER_UP |
		CGU_OSC_PHASE1_POWER_UP |
		CGU_OSC_PLL_BYPASS_N |
		CGU_OSC_PHASE1_BYPASS_N |
		(7 << CGU_OSC_NDIV_SHIFT) |
		(1 << CGU_OSC_MDIV_SHIFT);
	bool locked = apply_pll_osc(selected_osc);

	CGU_CON0 = (initial_con0 & ~CGU_CON0_PHASE1_CONFIG) | (0x22 << CGU_CON0_PHASE1_CONFIG_SHIFT);
	MMICIF_CLC = (1 << MOD_CLC_RMC_SHIFT);
	uint32_t con1_bypass = initial_con1 & ~CGU_CON1_AHB_CLKSEL;
	uint32_t con1_phase1 = con1_bypass | CGU_CON1_AHB_CLKSEL_PHASE1;
	uint32_t con2_div2 =
		(initial_con2 & ~(CGU_CON2_CPU_DIV | CGU_CON2_CPU_DIV_EN)) |
		CGU_CON2_CPU_DIV_EN |
		(1 << CGU_CON2_CPU_DIV_SHIFT);
	uint32_t con3_osc = initial_con3 & ~(CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV);
	uint32_t con3_pll_div_2 = con3_osc | CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2;

	if (locked) {
		CGU_CON2 = initial_con2;
		CGU_CON1 = con1_bypass;
		CGU_CON3 = con3_osc;
		ticks[0] = benchmark_ahb_per_reads(&checksum[0]);
		CGU_CON2 = con2_div2;
		CGU_CON1 = con1_phase1;
		ticks[1] = benchmark_ahb_per_reads(&checksum[1]);
		CGU_CON2 = initial_con2;
		CGU_CON1 = con1_bypass;
		CGU_CON3 = con3_pll_div_2;
		ticks[2] = benchmark_ahb_per_reads(&checksum[2]);
		CGU_CON2 = con2_div2;
		CGU_CON1 = con1_phase1;
		ticks[3] = benchmark_ahb_per_reads(&checksum[3]);
	}

	CGU_CON1 = initial_con1;
	CGU_CON2 = initial_con2;
	CGU_CON3 = initial_con3;
	MMICIF_CLC = initial_mmicif_clc;
	CGU_CON0 = initial_con0;
	bool restored = apply_pll_osc(initial_osc);

	printf("# AHB_PER source: OSC/AHB_OSC=%u OSC/AHB_PHASE1=%u PLL_DIV_2/AHB_OSC=%u PLL_DIV_2/AHB_PHASE1=%u ticks\n",
		(unsigned int) ticks[0], (unsigned int) ticks[1], (unsigned int) ticks[2], (unsigned int) ticks[3]);
	test_check("PLL locks for AHB_PER PLL/2 source probe", locked);
	test_check("AHB_PER source probe returns stable MMICIF data",
		checksum[0] == checksum[1] && checksum[0] == checksum[2] && checksum[0] == checksum[3]);
	test_check("AHB_PER source probe restores PLL registers", restored && CGU_OSC == initial_osc &&
		CGU_CON0 == initial_con0 && CGU_CON1 == initial_con1 && CGU_CON2 == initial_con2 &&
		CGU_CON3 == initial_con3);
}

static void test_ahb_per_pll_div_2_dependency(void) {
	if (test_is_qemu()) {
		test_skip("AHB_PER frequency follows the PLL frequency", "QEMU does not model MMICIF timing");
		return;
	}

	static const struct {
		uint32_t ndiv;
		uint32_t mdiv;
		uint32_t pll_khz;
	} CONFIGS[] = {
		{ 2, 0, 78000 },
		{ 6, 1, 91000 },
		{ 3, 0, 104000 },
	};
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_con3 = CGU_CON3;
	uint32_t initial_mmicif_clc = MMICIF_CLC;
	uint32_t ticks[ARRAY_SIZE(CONFIGS)] = { 0 };
	uint32_t checksum[ARRAY_SIZE(CONFIGS)] = { 0 };
	bool locked[ARRAY_SIZE(CONFIGS)] = { false };
	uint32_t con1_bypass = initial_con1 & ~(CGU_CON1_FSYS_CLKSEL | CGU_CON1_AHB_CLKSEL);
	uint32_t con3_base = initial_con3 & ~(CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV);

	CGU_CON1 = con1_bypass;
	MMICIF_CLC = (1 << MOD_CLC_RMC_SHIFT);
	for (size_t index = 0; index < ARRAY_SIZE(CONFIGS); index++) {
		CGU_CON3 = con3_base | CGU_CON3_AHB_PER_CLKSEL_DISABLE;
		uint32_t osc = (initial_osc & ~(CGU_OSC_NDIV | CGU_OSC_MDIV)) |
			(CONFIGS[index].ndiv << CGU_OSC_NDIV_SHIFT) |
			(CONFIGS[index].mdiv << CGU_OSC_MDIV_SHIFT) |
			CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
		locked[index] = apply_pll_osc(osc);
		if (!locked[index])
			continue;

		CGU_CON3 = con3_base | CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2;
		ticks[index] = benchmark_ahb_per_reads(&checksum[index]);
	}

	CGU_CON3 = con3_base | CGU_CON3_AHB_PER_CLKSEL_DISABLE;
	bool restored = apply_pll_osc(initial_osc);
	CGU_CON3 = initial_con3;
	CGU_CON1 = initial_con1;
	MMICIF_CLC = initial_mmicif_clc;
	uint32_t low_frequency_delta = ticks[0] > ticks[1] ? ticks[0] - ticks[1] : 0;
	uint32_t high_frequency_delta = ticks[1] > ticks[2] ? ticks[1] - ticks[2] : 0;

	printf("# AHB_PER PLL/2 dependency: PLL=%u/%u/%u kHz ticks=%u/%u/%u\n",
		(unsigned int) CONFIGS[0].pll_khz, (unsigned int) CONFIGS[1].pll_khz,
		(unsigned int) CONFIGS[2].pll_khz, (unsigned int) ticks[0], (unsigned int) ticks[1],
		(unsigned int) ticks[2]);
	test_check("PLL locks for every AHB_PER PLL/2 dependency measurement",
		locked[0] && locked[1] && locked[2]);
	test_check("AHB_PER PLL/2 reads return stable MMICIF data",
		checksum[0] == checksum[1] && checksum[0] == checksum[2]);
	test_check("AHB_PER PLL/2 timing decreases as PLL frequency increases",
		ticks[0] > ticks[1] && ticks[1] > ticks[2]);
	test_check("AHB_PER PLL/2 latency drops by one wait state per 13 MHz PLL step",
		test_u32_in_interval(low_frequency_delta, AHB_PER_PROBE_READS * 98 / 100,
			AHB_PER_PROBE_READS * 102 / 100) &&
		test_u32_in_interval(high_frequency_delta, AHB_PER_PROBE_READS * 98 / 100,
			AHB_PER_PROBE_READS * 102 / 100));
	test_check("AHB_PER PLL/2 dependency measurement restores PLL registers", restored &&
		CGU_OSC == initial_osc && CGU_CON1 == initial_con1 && CGU_CON3 == initial_con3);
}
#endif

static void test_ebu_source_frequencies(void) {
	if (test_is_qemu()) {
		test_skip("asynchronous EBU clock transition becomes ready", "QEMU does not model EBU access timing");
		test_skip("all EBU sources read identical flash data", "QEMU does not model EBU access timing");
		test_skip("EBU access timing follows the selected source", "QEMU does not model EBU access timing");
		return;
	}

	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con0 = CGU_CON0;
	uint32_t initial_con2 = CGU_CON2;
	uint32_t initial_ebuclc2 = SCU_EBUCLC2;
	uint32_t selected_osc =
		(initial_osc & ~(CGU_OSC_NDIV | CGU_OSC_MDIV)) |
		CGU_OSC_PLL_POWER_UP |
		CGU_OSC_PHASE1_POWER_UP |
		CGU_OSC_PHASE2_POWER_UP |
		CGU_OSC_PHASE3_POWER_UP |
		CGU_OSC_PHASE4_POWER_UP |
		CGU_OSC_PLL_BYPASS_N |
		CGU_OSC_PHASE1_BYPASS_N |
		CGU_OSC_PHASE2_BYPASS_N |
		CGU_OSC_PHASE3_BYPASS_N |
		CGU_OSC_PHASE4_BYPASS_N |
		(7 << CGU_OSC_NDIV_SHIFT) |
		(1 << CGU_OSC_MDIV_SHIFT);
	uint32_t con2_base = initial_con2 & ~CGU_CON2_EBU_CLKSEL;
	bool irq_was_disabled = cpu_enable_irq(false);
	bool pll_locked = apply_pll_osc(selected_osc);

	uint32_t selected_con0 =
		(CGU_CON0 & ~(CGU_CON0_PHASE1_CONFIG | CGU_CON0_PHASE2_CONFIG | CGU_CON0_PHASE3_CONFIG |
			CGU_CON0_PHASE4_CONFIG)) |
		(0x11 << CGU_CON0_PHASE1_CONFIG_SHIFT) |
		(0x22 << CGU_CON0_PHASE2_CONFIG_SHIFT) |
		(0x23 << CGU_CON0_PHASE3_CONFIG_SHIFT) |
		(0x28 << CGU_CON0_PHASE4_CONFIG_SHIFT);
	CGU_CON0 = selected_con0;
	CGU_CON2 = con2_base | CGU_CON2_EBU_CLKSEL_PLL;
	SCU_EBUCLC2 = initial_ebuclc2 | (1 << SCU_EBUCLC2_FLAG1_SHIFT);
	bool transition_ready = wait_for_ebu_clock_transition();

	static const uint32_t EBU_SOURCES[EBU_SOURCE_COUNT] = {
		CGU_CON2_EBU_CLKSEL_PLL,
		CGU_CON2_EBU_CLKSEL_PHASE1,
		CGU_CON2_EBU_CLKSEL_PHASE2,
		CGU_CON2_EBU_CLKSEL_PHASE3,
		CGU_CON2_EBU_CLKSEL_PHASE4,
		CGU_CON2_EBU_CLKSEL_OSC,
	};
	uint32_t checksums[EBU_SOURCE_COUNT] = { 0 };
	uint32_t ticks[EBU_SOURCE_COUNT] = { 0 };
	if (pll_locked && transition_ready) {
		for (uint32_t index = 0; index < ARRAY_SIZE(EBU_SOURCES); index++) {
			CGU_CON2 = con2_base | EBU_SOURCES[index];
			ticks[index] = benchmark_ebu_flash_reads(&checksums[index]);
		}
	}

	CGU_CON2 = initial_con2;
	SCU_EBUCLC2 = initial_ebuclc2;
	CGU_CON0 = initial_con0;
	bool restored = apply_pll_osc(initial_osc);
	cpu_enable_irq(!irq_was_disabled);

	test_check("PLL locks before EBU source measurements", pll_locked);
	test_check("asynchronous EBU clock transition becomes ready", transition_ready);
	if (pll_locked && transition_ready) {
		bool data_matches = true;
		bool source_order = true;
		for (uint32_t index = 1; index < ARRAY_SIZE(EBU_SOURCES); index++) {
			data_matches = data_matches && checksums[index] == checksums[EBU_SOURCE_PLL];
			source_order = source_order && ticks[index - 1] < ticks[index];
		}

		printf("# EBU flash reads: OSC=%u PLL=%u PHASE1=%u PHASE2=%u PHASE3=%u PHASE4=%u ticks, checksum=%08X\n",
			(unsigned int) ticks[EBU_SOURCE_OSC], (unsigned int) ticks[EBU_SOURCE_PLL],
			(unsigned int) ticks[EBU_SOURCE_PHASE1], (unsigned int) ticks[EBU_SOURCE_PHASE2],
			(unsigned int) ticks[EBU_SOURCE_PHASE3], (unsigned int) ticks[EBU_SOURCE_PHASE4],
			(unsigned int) checksums[EBU_SOURCE_PLL]);
		test_check("all EBU sources read identical flash data", data_matches);
		test_check("EBU access timing follows the selected source", source_order);
	} else {
		test_skip("all EBU sources read identical flash data", "EBU source measurement is unavailable");
		test_skip("EBU access timing follows the selected source", "EBU source measurement is unavailable");
	}

	bool registers_restored = restored && CGU_OSC == initial_osc && CGU_CON0 == initial_con0 &&
		CGU_CON2 == initial_con2 && SCU_EBUCLC2 == initial_ebuclc2;
	test_check("EBU source measurement restores PLL and SCU registers", registers_restored);
}

static void test_ahb_frequency_measurement(void) {
	if (test_is_qemu()) {
		test_skip("AHB frequency matches the CGU model", "QEMU does not clock ARM instructions from CGU");
		return;
	}

	uint32_t initial_itcm = read_itcm();
	bool can_measure =
		(CGU_CON1 & CGU_CON1_AHB_CLKSEL) == CGU_CON1_AHB_CLKSEL_BYPASS &&
		(CGU_CON1 & CGU_CON1_FSTM_DIV_EN) == 0 &&
		(CGU_CON2 & CGU_CON2_CPU_DIV_EN) == 0 &&
		(initial_itcm & TCM_REGION_ENABLE) == 0;

	if (!can_measure) {
		test_skip("ITCM timing measures the 26 MHz AHB clock", "boot clock or ITCM configuration is incompatible");
		return;
	}

	write_itcm(ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_itcm_measurement_loop();
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	stopwatch_t start = stopwatch_get();
	cgu_execute_itcm(ITCM_BASE, CGU_MEASURE_ITERATIONS);
	uint32_t elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	cpu_enable_irq(!irq_was_disabled);
	write_itcm(initial_itcm);
	sync_code();

	uint32_t measured_khz = measured_frequency_khz(elapsed_ticks, CGU_MEASURE_ITERATIONS);
	test_measured_frequency("AHB frequency matches the CGU model", cpu_get_ahb_freq() / 1000, measured_khz);
}

static void test_pll_source_frequencies(void) {
	if (test_is_qemu()) {
		test_skip("PLL and phase frequencies match the CGU model", "QEMU does not clock ARM instructions from CGU");
		return;
	}

	uint32_t initial_itcm = read_itcm();
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con0 = CGU_CON0;
	uint32_t initial_con1 = CGU_CON1;
	bool can_measure =
		(initial_con1 & CGU_CON1_AHB_CLKSEL) == CGU_CON1_AHB_CLKSEL_BYPASS &&
		(initial_con1 & CGU_CON1_FSYS_CLKSEL) == CGU_CON1_FSYS_CLKSEL_BYPASS &&
		(initial_con1 & CGU_CON1_FSTM_DIV_EN) == 0 &&
		(CGU_CON2 & CGU_CON2_CPU_DIV_EN) == 0 &&
		(initial_itcm & TCM_REGION_ENABLE) == 0;

	if (!can_measure) {
		test_skip("PLL frequency follows N/M divider", "boot clock configuration is incompatible");
		test_skip("phase 1 frequency follows K1/K2 divider", "boot clock or ITCM configuration is incompatible");
		test_skip("phase 2 frequency follows K1/K2 divider", "boot clock or ITCM configuration is incompatible");
		test_skip("phase 3 frequency follows K1/K2 divider", "boot clock or ITCM configuration is incompatible");
		test_skip("phase 4 frequency follows K1/K2 divider", "boot clock or ITCM configuration is incompatible");
		return;
	}

	write_itcm(ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_itcm_source_measurement_loop();
	uint32_t safe_con1 = initial_con1;
	bool irq_was_disabled = cpu_enable_irq(false);
	CGU_CON0 = 0x22222222;
	uint32_t selected_con1 =
		(safe_con1 & ~CGU_CON1_AHB_CLKSEL) | CGU_CON1_AHB_CLKSEL_PLL;
	static const uint32_t PLL_DIVIDERS[][2] = {
		{ 3, 0 },
		{ 7, 1 },
		{ 11, 2 },
		{ 15, 3 },
	};
	static const char *const PLL_DIVIDER_TEST_NAMES[] = {
		"N=3 M=0 preserves the 104 MHz PLL ratio",
		"N=7 M=1 preserves the 104 MHz PLL ratio",
		"N=11 M=2 preserves the 104 MHz PLL ratio",
		"N=15 M=3 preserves the 104 MHz PLL ratio",
	};
	bool divider_locked[ARRAY_SIZE(PLL_DIVIDERS)];
	uint32_t divider_calculated_hz[ARRAY_SIZE(PLL_DIVIDERS)];
	uint32_t divider_measured_khz[ARRAY_SIZE(PLL_DIVIDERS)];
	for (uint32_t index = 0; index < ARRAY_SIZE(PLL_DIVIDERS); index++) {
		uint32_t ndiv = PLL_DIVIDERS[index][0];
		uint32_t mdiv = PLL_DIVIDERS[index][1];
		uint32_t osc =
			(CGU_OSC & ~(CGU_OSC_NDIV | CGU_OSC_MDIV)) |
			CGU_OSC_PLL_POWER_UP |
			CGU_OSC_PLL_BYPASS_N |
			(ndiv << CGU_OSC_NDIV_SHIFT) |
			(mdiv << CGU_OSC_MDIV_SHIFT);
		divider_locked[index] = apply_pll_osc(osc);
		CGU_CON1 = selected_con1;
		divider_calculated_hz[index] = cpu_get_ahb_freq();
		CGU_CON1 = safe_con1;
		divider_measured_khz[index] = divider_locked[index]
			? measure_ahb_source_khz(selected_con1, safe_con1)
			: 0;
	}

	uint32_t phase_osc =
		(CGU_OSC & ~(CGU_OSC_NDIV | CGU_OSC_MDIV)) |
		CGU_OSC_PLL_POWER_UP |
		CGU_OSC_PLL_BYPASS_N |
		(7 << CGU_OSC_NDIV_SHIFT) |
		(1 << CGU_OSC_MDIV_SHIFT);
	bool phase_pll_locked = apply_pll_osc(phase_osc);

	static const uint32_t PHASE_CONFIGS[] = {
		0x11,
		0x22,
		0x23,
		0x22,
	};
	static const uint32_t PHASE_SOURCES[] = {
		CGU_CON1_AHB_CLKSEL_PHASE1,
		CGU_CON1_AHB_CLKSEL_PHASE1,
		CGU_CON1_AHB_CLKSEL_PHASE1,
		CGU_CON1_AHB_CLKSEL_PHASE2,
	};
	static const char *const PHASE_TEST_NAMES[] = {
		"phase 1 K1=2 K2=1 produces 96 MHz",
		"phase 1 K1=4 K2=2 produces 48 MHz",
		"phase 1 K1=4 K2=3 produces 46.222 MHz",
		"phase 2 K1=4 K2=2 produces 48 MHz",
	};
	uint32_t phase_calculated_hz[ARRAY_SIZE(PHASE_SOURCES)] = { 0 };
	uint32_t phase_measured_khz[ARRAY_SIZE(PHASE_SOURCES)] = { 0 };
	if (phase_pll_locked) {
		CGU_OSC |= CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE2_POWER_UP;
		CGU_OSC |= CGU_OSC_PHASE1_BYPASS_N | CGU_OSC_PHASE2_BYPASS_N;
		for (uint32_t index = 0; index < ARRAY_SIZE(PHASE_SOURCES); index++) {
			uint32_t config_mask = index < 3 ? CGU_CON0_PHASE1_CONFIG : CGU_CON0_PHASE2_CONFIG;
			uint32_t config_shift = index < 3 ? CGU_CON0_PHASE1_CONFIG_SHIFT : CGU_CON0_PHASE2_CONFIG_SHIFT;
			CGU_CON0 = (CGU_CON0 & ~config_mask) | (PHASE_CONFIGS[index] << config_shift);
			selected_con1 = (safe_con1 & ~CGU_CON1_AHB_CLKSEL) | PHASE_SOURCES[index];
			CGU_CON1 = selected_con1;
			phase_calculated_hz[index] = cpu_get_ahb_freq();
			CGU_CON1 = safe_con1;
			phase_measured_khz[index] = measure_ahb_source_khz(selected_con1, safe_con1);
		}
	}

	CGU_CON0 = initial_con0;
	bool restore_locked = apply_pll_osc(initial_osc);
	write_itcm(initial_itcm);
	sync_code();
	cpu_enable_irq(!irq_was_disabled);

	for (uint32_t index = 0; index < ARRAY_SIZE(PLL_DIVIDERS); index++) {
		test_check("PLL locks after changing N/M", divider_locked[index]);
		if (divider_locked[index]) {
			test_measured_frequency(PLL_DIVIDER_TEST_NAMES[index], divider_calculated_hz[index] / 1000,
				divider_measured_khz[index]);
		} else {
			test_skip(PLL_DIVIDER_TEST_NAMES[index], "PLL did not lock");
		}
	}
	test_check("PLL locks before phase measurements", phase_pll_locked);
	for (uint32_t index = 0; index < ARRAY_SIZE(PHASE_SOURCES); index++) {
		if (phase_pll_locked) {
			test_measured_frequency(PHASE_TEST_NAMES[index], phase_calculated_hz[index] / 1000,
				phase_measured_khz[index]);
		} else {
			test_skip(PHASE_TEST_NAMES[index], "PLL did not lock");
		}
	}
	test_skip("phase 3 frequency follows K1/K2 divider", "C81 source 5 stops AHB and firmware never enables 0x0808");
	test_skip("phase 4 frequency follows K1/K2 divider", "C81 source 6 stops AHB");
	test_check("original PLL configuration is restored and locked", restore_locked);
}

static void test_cpu_divider_frequencies(void) {
	if (test_is_qemu()) {
		test_skip("CPU divider frequencies match the CGU model", "QEMU does not clock ARM instructions from CGU");
		return;
	}

	uint32_t initial_itcm = read_itcm();
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_con2 = CGU_CON2;
	bool can_measure =
		(initial_con1 & CGU_CON1_AHB_CLKSEL) == CGU_CON1_AHB_CLKSEL_BYPASS &&
		(initial_con1 & CGU_CON1_FSTM_DIV_EN) == 0 &&
		(initial_itcm & TCM_REGION_ENABLE) == 0;
	if (!can_measure) {
		test_skip("CPU divider frequencies match the CGU model", "boot clock or ITCM is incompatible");
		return;
	}

	write_itcm(ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_itcm_cpu_div_measurement_loop();
	uint32_t selected_con2 = initial_con2 & ~(CGU_CON2_CPU_DIV | CGU_CON2_CPU_DIV_EN);
	CGU_CON2 = selected_con2;
	uint32_t model_khz = cpu_get_freq() / 1000;
	CGU_CON2 = initial_con2;
	uint32_t baseline_khz = measure_cpu_divider_khz(selected_con2, initial_con2);
	test_measured_frequency("disabled CPU divider matches the CGU model", model_khz, baseline_khz);

	for (uint32_t divider = 0; divider < 4; divider++) {
		selected_con2 =
			(initial_con2 & ~(CGU_CON2_CPU_DIV | CGU_CON2_CPU_DIV_EN)) |
			CGU_CON2_CPU_DIV_EN |
			(divider << CGU_CON2_CPU_DIV_SHIFT);
		CGU_CON2 = selected_con2;
		model_khz = cpu_get_freq() / 1000;
		CGU_CON2 = initial_con2;
		uint32_t measured_khz = measure_cpu_divider_khz(selected_con2, initial_con2);
		printf("# CPU divider %u selects divide-by-%u\n", (unsigned int) divider,
			(unsigned int) divider + 1);
		test_measured_frequency("CPU divider frequency matches the CGU model", model_khz, measured_khz);
	}

	write_itcm(initial_itcm);
	sync_code();
}

static void test_stm_divider_frequencies(void) {
	uint32_t initial_con1 = CGU_CON1;
	uint32_t selected_con1 =
		(initial_con1 & ~(CGU_CON1_FSTM_DIV | CGU_CON1_FSTM_DIV_EN)) |
		(3 << CGU_CON1_FSTM_DIV_SHIFT);

	init_reference_rtc();
	CGU_CON1 = selected_con1;
	uint32_t model_hz = cpu_get_stm_freq();
	uint32_t measured_khz = stm_ticks_to_khz(benchmark_stm_ticks());
	CGU_CON1 = initial_con1;
	test_measured_frequency("disabled fSTM divider leaves the oscillator clock unchanged", model_hz / 1000,
		measured_khz);

	for (uint32_t divider = 0; divider < 4; divider++) {
		selected_con1 =
			(initial_con1 & ~(CGU_CON1_FSTM_DIV | CGU_CON1_FSTM_DIV_EN)) |
			CGU_CON1_FSTM_DIV_EN |
			(divider << CGU_CON1_FSTM_DIV_SHIFT);
		CGU_CON1 = selected_con1;
		model_hz = cpu_get_stm_freq();
		measured_khz = stm_ticks_to_khz(benchmark_stm_ticks());
		CGU_CON1 = initial_con1;
		printf("# STM divider %u selects divide-by-%u\n", (unsigned int) divider, (4U << divider));
		test_measured_frequency("fSTM frequency matches the CGU model", model_hz / 1000, measured_khz);
	}
}

static bool wait_for_dma_completion(void) {
	stopwatch_t start = stopwatch_get();

	while ((DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0 && stopwatch_elapsed_ms(start) < 20)
		test_watchdog_serve();

	return (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) != 0;
}

static void start_dma_transfer(void) {
	DMAC_CH_CONFIG(DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
	DMAC_ERR_CLEAR = BIT(DMA_CHANNEL);
	DMAC_CH_SRC_ADDR(DMA_CHANNEL) = (uint32_t) dma_source;
	DMAC_CH_DST_ADDR(DMA_CHANNEL) = (uint32_t) dma_destination;
	DMAC_CH_LLI(DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(DMA_CHANNEL) = ARRAY_SIZE(dma_source) | DMAC_CH_CONTROL_SB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD |
		DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI |
		DMAC_CH_CONTROL_I;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_CH_CONFIG(DMA_CHANNEL) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_ENABLE;
}

static void test_dma_clock(void) {
	if (test_is_qemu()) {
		test_skip("DMA clock gate matches the CGU model", "QEMU does not model the DMA clock gate");
		return;
	}

	uint32_t initial_con3 = CGU_CON3;
	for (uint32_t index = 0; index < ARRAY_SIZE(dma_source); index++) {
		dma_source[index] = 0x12340000 | index;
		dma_destination[index] = 0;
	}

	CGU_CON3 = initial_con3 | CGU_CON3_DMA_CLK_DISABLE;
	uint32_t disabled_frequency = cpu_get_dma_freq();
	DMAC_CONFIG = 0;
	start_dma_transfer();
	stopwatch_usleep_wd(1000);
	bool gated = (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0;

	CGU_CON3 = initial_con3 & ~CGU_CON3_DMA_CLK_DISABLE;
	uint32_t enabled_frequency = cpu_get_dma_freq();
	start_dma_transfer();
	bool completed = wait_for_dma_completion();
	bool data_matches = true;
	for (uint32_t index = 0; index < ARRAY_SIZE(dma_source); index++)
		data_matches = data_matches && dma_destination[index] == dma_source[index];

	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
	DMAC_ERR_CLEAR = BIT(DMA_CHANNEL);
	CGU_CON3 = initial_con3;

	test_eq_u32("disabled DMA clock is zero in the CGU model", 0, disabled_frequency);
	test_check("DMA transfer remains pending while its CGU clock is disabled", gated);
	test_check("enabled DMA clock follows the PLL in the CGU model", enabled_frequency == cpu_get_pll_freq());
	test_check("DMA transfer completes after its CGU clock is enabled", completed && data_matches);
}

static void test_pll_lock_transition(void) {
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_src = CGU_SRC;
	uint32_t initial_vic_con = VIC_CON(VIC_CGU_IRQ);
	bool safe_clock_tree =
		(CGU_CON1 & CGU_CON1_AHB_CLKSEL) == CGU_CON1_AHB_CLKSEL_BYPASS &&
		(CGU_CON1 & CGU_CON1_FSYS_CLKSEL) == CGU_CON1_FSYS_CLKSEL_BYPASS;
	if (!safe_clock_tree) {
		test_skip("power down clears PLL lock", "boot clock tree depends on the PLL");
		test_skip("power up reacquires PLL lock", "boot clock tree depends on the PLL");
		test_skip("PLL lock transition raises its service request", "boot clock tree depends on the PLL");
		test_skip("PLL lock transition reaches the expected VIC line", "boot clock tree depends on the PLL");
		test_skip("PLL transition restores the boot configuration", "boot clock tree depends on the PLL");
		return;
	}

	bool irq_was_disabled = cpu_enable_irq(false);
	irq_count = 0;
	irq_number = 0;
	CGU_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_CGU_IRQ) = 1;
	CGU_OSC = initial_osc & ~(CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N);
	bool unlocked = wait_for_pll_unlock();
	CGU_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	irq_count = 0;
	irq_number = 0;
	cpu_enable_irq(true);
	stopwatch_t lock_start = stopwatch_get();
	CGU_OSC = (initial_osc | CGU_OSC_PLL_POWER_UP) & ~CGU_OSC_PLL_BYPASS_N;
	bool locked = wait_for_pll_lock();
	uint32_t lock_ticks = (uint32_t) stopwatch_elapsed(lock_start);
	bool lock_irq = wait_for_irq();
	cpu_enable_irq(false);

	CGU_OSC = initial_osc;
	bool restored = wait_for_pll_lock();
	VIC_CON(VIC_CGU_IRQ) = initial_vic_con;
	CGU_SRC = MOD_SRC_CLRR | (initial_src & (MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE));
	if ((initial_src & MOD_SRC_SRR) != 0)
		CGU_SRC |= MOD_SRC_SETR;
	cpu_enable_irq(!irq_was_disabled);

	printf("# PLL lock acquisition took %u STM ticks\n", (unsigned int) lock_ticks);
	test_check("power down clears PLL lock", unlocked);
	test_check("power up reacquires PLL lock", locked);
	test_check("PLL lock transition raises its service request", lock_irq);
	test_eq_u32("PLL lock transition reaches the expected VIC line", VIC_CGU_IRQ, irq_number);
	test_check("PLL transition restores the boot configuration", restored && CGU_OSC == initial_osc);
}

static void test_interrupt_routing(void) {
	uint32_t initial_src = CGU_SRC;
	uint32_t initial_vic_con = VIC_CON(VIC_CGU_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	irq_count = 0;
	irq_number = 0;
	CGU_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_CGU_IRQ) = 1;
	cpu_enable_irq(true);
	CGU_SRC |= MOD_SRC_SETR;
	test_check("software request raises CGU IRQ", wait_for_irq());
	cpu_enable_irq(false);
	test_eq_u32("CGU SRC is routed to the expected VIC line", VIC_CGU_IRQ, irq_number);
	test_eq_u32("CGU IRQ handler clears the request", 0, CGU_SRC & MOD_SRC_SRR);

	VIC_CON(VIC_CGU_IRQ) = initial_vic_con;
	CGU_SRC = MOD_SRC_CLRR | (initial_src & (MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE));
	if ((initial_src & MOD_SRC_SRR) != 0)
		CGU_SRC |= MOD_SRC_SETR;
	cpu_enable_irq(!irq_was_disabled);
}

int main(void) {
	test_start("CGU peripheral test");

	test_category("Boot state");
	test_boot_state();
	test_category("FPI1 PLL/2 frequency");
	test_fpi1_pll_div_2_frequency();
	test_category("FPI1 selector 2 PLL dependency");
	test_fpi1_pll_div_2_dependency();
	test_category("fSYS frequencies");
	test_fsys_frequencies();
	test_category("AHB frequency measurement");
	test_ahb_frequency_measurement();
	test_category("PLL source frequencies");
	test_pll_source_frequencies();
	test_category("EBU source frequencies");
	test_ebu_source_frequencies();
#ifdef PMB8876
	test_category("MMCI source frequencies");
	test_mmci_source_frequencies();
	test_category("AHB_PER PLL/2 source");
	test_ahb_per_pll_div_2_source();
	test_category("AHB_PER PLL/2 dependency");
	test_ahb_per_pll_div_2_dependency();
#endif
	test_category("CPU divider frequencies");
	test_cpu_divider_frequencies();
	test_category("STM divider frequencies");
	test_stm_divider_frequencies();
	test_category("DMA clock");
	test_dma_clock();
	test_category("PLL lock transition");
	test_pll_lock_transition();
	test_category("Interrupt routing");
	test_interrupt_routing();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	if (irq_number == VIC_TPU_INT0_IRQ) {
		tpu_frame_count++;
		TPU_SRC(0) |= MOD_SRC_CLRR;
		VIC_IRQ_ACK = 1;
		return;
	}
	irq_count++;
	CGU_SRC |= MOD_SRC_CLRR;
	VIC_IRQ_ACK = 1;
}
