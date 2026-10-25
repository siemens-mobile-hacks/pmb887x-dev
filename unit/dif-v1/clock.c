#include <pmb887x.h>

#include "cgu/clock.h"
#include "dif-v1.h"
#include "test.h"

#ifdef PMB8875

#define WORD_RUNS 8
#define BURST_WORDS 4
#define WORD_BITS 16
#define BAUD_RELOAD 0xFF
#define RECEIVE_TIMEOUT_US 100000

struct clock_case {
	const char *name;
	uint32_t con1_mask;
	uint32_t con1;
	bool pll;
	uint32_t rmc;
	uint32_t percent_of_boot_time;
};

static uint32_t initial_osc;
static uint32_t initial_con1;

static void restore_clocks(void) {
	CGU_CON1 = initial_con1;
	CGU_OSC = initial_osc;
	stopwatch_update();
	stopwatch_usleep_wd(2000);
}

static uint32_t burst_ticks(uint32_t words) {
	uint32_t best = UINT32_MAX;

	for (uint32_t i = 0; i < WORD_RUNS; i++) {
		while ((DIF_FSTAT & DIF_FSTAT_RXFFL) != 0)
			(void) DIF_RB;
		stopwatch_t start = stopwatch_get();
		for (uint32_t word = 0; word < words; word++)
			DIF_TB = 0xA55A;
		while (((DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT) < words) {
			if (stopwatch_elapsed_us(start) > RECEIVE_TIMEOUT_US)
				return 0;
		}
		uint32_t ticks = (uint32_t) stopwatch_elapsed(start);
		if (ticks < best)
			best = ticks;
		wdt_serve();
	}
	return best;
}

static uint32_t measure_case(const struct clock_case *clock) {
	if (clock->pll)
		cgu_pll_set(3, 0);
	CGU_CON1 = (CGU_CON1 & ~clock->con1_mask) | clock->con1;
	stopwatch_update();
	stopwatch_usleep_wd(2000);
	uint32_t ahb_hz = cpu_get_ahb_freq();
	uint32_t sys_hz = cpu_get_sys_freq();
	uint32_t fpi1_hz = cpu_get_fpi1_freq();
	uint32_t fpi2_hz = cpu_get_fpi2_freq();
	uint32_t ticks_per_s = stopwatch_ticks_per_s();

	DIF_CLC = clock->rmc << MOD_CLC_RMC_SHIFT;
	dif_v1_configure(WORD_BITS, DIF_RXFCON_RXFEN, DIF_TXFCON_TXFEN, 0);
	DIF_BR = BAUD_RELOAD;
	uint32_t one = burst_ticks(1);
	uint32_t many = burst_ticks(BURST_WORDS);
	restore_clocks();

	bool ok = one != 0 && many > one;
	uint32_t burst_delta_ticks = ok ? many - one : 0;
	uint32_t word_ns = (uint32_t) ((uint64_t) burst_delta_ticks * 1000000000ULL / ticks_per_s / (BURST_WORDS - 1));
	printf("# %s: %s, %lu ns per word; AHB %lu Hz, FSYS %lu Hz, FPI1 %lu Hz, FPI2 %lu Hz\n", clock->name,
		ok ? "transfer ok" : "transfer FAILED", word_ns, ahb_hz, sys_hz, fpi1_hz, fpi2_hz);
	return word_ns;
}

static void test_sources(void) {
	static const struct clock_case CASES[] = {
		{.name = "FSYS PLL", .con1_mask = CGU_CON1_FSYS_CLKSEL, .con1 = CGU_CON1_FSYS_CLKSEL_PLL, .pll = true,
			.rmc = 1, .percent_of_boot_time = 100},
		{.name = "FPI1 OSC /2 (divider bypassed)", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_OSC | CGU_CON1_FPI1_CLKDIV_DIV2, .rmc = 1, .percent_of_boot_time = 100},
		{.name = "FPI1 OSC /4 (divider bypassed)", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_OSC | CGU_CON1_FPI1_CLKDIV_DIV4, .rmc = 1, .percent_of_boot_time = 100},
		{.name = "FPI1 PLL/2", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_PLL_DIV_2, .pll = true, .rmc = 1, .percent_of_boot_time = 50},
		{.name = "FPI1 PLL/2 /2", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_PLL_DIV_2 | CGU_CON1_FPI1_CLKDIV_DIV2, .pll = true,
			.rmc = 1, .percent_of_boot_time = 100},
		{.name = "FPI1 PLL/2 /4", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_PLL_DIV_2 | CGU_CON1_FPI1_CLKDIV_DIV4, .pll = true,
			.rmc = 1, .percent_of_boot_time = 200},
		{.name = "FPI2 PLL/2", .con1_mask = CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV,
			.con1 = CGU_CON1_FPI2_CLKSEL_PLL, .pll = true, .rmc = 1, .percent_of_boot_time = 100},
		{.name = "RMC=2", .rmc = 2, .percent_of_boot_time = 200},
		{.name = "FPI1 PLL/2, RMC=2", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_PLL_DIV_2, .pll = true, .rmc = 2, .percent_of_boot_time = 100},
	};
	static const struct clock_case BOOT = {.name = "boot clocks", .rmc = 1};

	test_category("CGU clock sources");
	uint32_t boot_ns = measure_case(&BOOT);
	test_check("boot clocks", boot_ns != 0);
	for (uint32_t i = 0; i < ARRAY_SIZE(CASES); i++) {
		uint64_t expected_ns = (uint64_t) boot_ns * CASES[i].percent_of_boot_time / 100;
		uint32_t word_ns = measure_case(&CASES[i]);

		test_check(CASES[i].name, test_u32_in_interval(word_ns, expected_ns * 90 / 100, expected_ns * 110 / 100));
	}
}

int main(void) {
	test_start("DIFv1 clock source test");
	if (test_is_qemu()) {
		test_skip("DIFv1 clock source", "QEMU does not model transfer time");
		return test_finish();
	}

	cpu_enable_irq(false);
	initial_osc = CGU_OSC;
	initial_con1 = CGU_CON1;
	printf("# boot CGU: OSC=%08lX CON0=%08lX CON1=%08lX; STM %lu ticks/us\n", initial_osc, CGU_CON0, initial_con1,
		stopwatch_ticks_per_us());

	test_sources();

	return test_finish();
}

#else

int main(void) {
	test_start("DIFv1 clock source test");
	test_skip("DIFv1 clock source", "unsupported");

	return test_finish();
}

#endif
