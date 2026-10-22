#include <pmb887x.h>

#include "bus-timing.h"
#include "cgu/clock.h"
#include "test.h"

#ifdef PMB8876

/* ACCESSCYCLE=16 keeps every tested DIF clock slower than the DMA feed limit. */
#define ACCESS_CYCLE 16
#define BYTE_CYCLES 17

struct clock_case {
	const char *name;
	uint32_t con1_mask;
	uint32_t con1;
	uint32_t con3_mask;
	uint32_t con3;
	bool pll;
};

static uint32_t initial_osc;
static uint32_t initial_con1;
static uint32_t initial_con3;

static struct dif_bus_timing measure_dif(void) {
	return dif_bus_timing_measure(&DIF_LANE_2x8, DIF_BLIT_SRAM, 1 << MOD_CLC_RMC_SHIFT,
		dif_lcdtim1(0, ACCESS_CYCLE, 0), 0);
}

static void restore_clocks(void) {
	CGU_CON3 = initial_con3;
	CGU_CON1 = initial_con1;
	CGU_OSC = initial_osc;
	stopwatch_usleep_wd(2000);
}

/* DIF module clock implied by the measured byte period, in Hz. */
static uint32_t dif_clock_hz(const struct dif_bus_timing *timing) {
	if (!timing->ok || timing->byte_ns100 == 0)
		return 0;
	return (uint32_t) ((uint64_t) BYTE_CYCLES * 100000000000ULL / timing->byte_ns100);
}

/* Measures with the case's clock tree and prints only after the console clock is restored. */
static uint32_t measure_case(const struct clock_case *clock) {
	if (clock->pll)
		cgu_pll_set(3, 0);
	CGU_CON1 = (CGU_CON1 & ~clock->con1_mask) | clock->con1;
	CGU_CON3 = (CGU_CON3 & ~clock->con3_mask) | clock->con3;
	stopwatch_usleep_wd(2000);
	uint32_t ahb_hz = cpu_get_ahb_freq();
	uint32_t fpi1_hz = cpu_get_fpi1_freq();
	uint32_t ahb_per_hz = cpu_get_ahb_per_freq();
	struct dif_bus_timing timing = measure_dif();
	restore_clocks();

	uint32_t hz = dif_clock_hz(&timing);
	printf("# %s: DIF clock %u Hz (%u.%02u ns per byte); AHB %u Hz, FPI1 %u Hz, AHB_PER %u Hz\n",
		clock->name, (unsigned int) hz,
		(unsigned int) (timing.byte_ns100 / 100), (unsigned int) (timing.byte_ns100 % 100),
		(unsigned int) ahb_hz, (unsigned int) fpi1_hz, (unsigned int) ahb_per_hz);
	dif_bus_timing_print_failure(&timing);
	return hz;
}

static bool is_rate(uint32_t measured_hz, uint32_t expected_hz) {
	return test_u32_in_interval(measured_hz, (uint64_t) expected_hz * 99 / 100, (uint64_t) expected_hz * 101 / 100);
}

static void test_clock_case(const struct clock_case *clock, const char *check, uint32_t expected_hz) {
	uint32_t hz = measure_case(clock);

	test_check(check, is_rate(hz, expected_hz));
}

static struct clock_case ahb_per_case(const char *name, uint32_t source, uint32_t divider) {
	return (struct clock_case) {
		.name = name,
		.con3_mask = CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		.con3 = source | (divider << CGU_CON3_AHB_PER_CLKDIV_SHIFT),
		.pll = source == CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2,
	};
}

static void test_ahb_per_pll(void) {
	char name[64];

	test_category("AHB_PER PLL source and divider");
	for (uint32_t divider = 0; divider < 4; divider++) {
		snprintf(name, sizeof(name), "AHB_PER PLL 104 MHz / 2 /%u", (unsigned int) (1 << divider));
		struct clock_case clock = ahb_per_case(name, CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2, divider);
		uint32_t hz = measure_case(&clock);
		char check[96];

		snprintf(check, sizeof(check), "DIF follows %s", name);
		test_check(check, is_rate(hz, 52000000 >> divider));
		/* The AHB stays on the 26 MHz oscillator, so a faster DIF clock cannot come from it. */
		if (divider == 0)
			test_check("DIF runs faster than the AHB clock", hz > cpu_get_ahb_freq());
	}
}

static void test_ahb_per_osc(void) {
	char name[64];

	/* With the oscillator source the AHB_PER divider does not slow the DIF bus. */
	test_category("AHB_PER oscillator source");
	for (uint32_t divider = 1; divider < 4; divider++) {
		snprintf(name, sizeof(name), "AHB_PER OSC /%u", (unsigned int) (1 << divider));
		struct clock_case clock = ahb_per_case(name, CGU_CON3_AHB_PER_CLKSEL_OSC, divider);
		char check[96];

		snprintf(check, sizeof(check), "DIF ignores the divider of %s", name);
		test_clock_case(&clock, check, CPU_OSC_FREQ);
	}
}

static void test_fpi1(void) {
	static const struct clock_case FPI1_CASES[] = {
		{ "FPI1 OSC /2", CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV, CGU_CON1_FPI1_CLKSEL_OSC |
			CGU_CON1_FPI1_CLKDIV_DIV2, 0, 0, false },
		{ "FPI1 OSC /4", CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV, CGU_CON1_FPI1_CLKSEL_OSC |
			CGU_CON1_FPI1_CLKDIV_DIV4, 0, 0, false },
	};

	test_category("FPI1 clock");
	test_clock_case(&FPI1_CASES[0], "DIF ignores FPI1 OSC /2", CPU_OSC_FREQ);
	test_clock_case(&FPI1_CASES[1], "DIF ignores FPI1 OSC /4", CPU_OSC_FREQ);
}

int main(void) {
	test_start("CGU DIFv2 clock test");

	cpu_enable_irq(false);
	if (!test_check("SRAM source buffer fits", dif_bus_timing_init()))
		return test_finish();
	initial_osc = CGU_OSC;
	initial_con1 = CGU_CON1;
	initial_con3 = CGU_CON3 & ~(CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV);
	initial_con3 |= CGU_CON3_AHB_PER_CLKSEL_OSC | CGU_CON3_AHB_PER_CLKDIV_DIV1;
	printf("# boot CGU: OSC=%08X CON1=%08X CON3=%08X\n",
		(unsigned int) initial_osc, (unsigned int) initial_con1, (unsigned int) CGU_CON3);
	restore_clocks();

	static const struct clock_case BASELINE = { "OSC baseline", 0, 0, 0, 0, false };
	test_category("Oscillator baseline");
	test_clock_case(&BASELINE, "DIF runs from the 26 MHz oscillator", CPU_OSC_FREQ);

	test_ahb_per_pll();
	test_ahb_per_osc();
	test_fpi1();
	/*
	 * CLKSEL_DISABLE is not covered: with AHB_PER stopped the first DIF register access never completes
	 * and the phone hangs until the watchdog expires.
	 */

	return test_finish();
}

#else

int main(void) {
	test_start("CGU DIFv2 clock test");
	test_skip("DIFv2 is available", "unsupported");

	return test_finish();
}

#endif
