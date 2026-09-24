#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

struct timer_counts {
	uint32_t t0;
	uint32_t t1;
};

static const struct {
	const char *name;
	uint32_t expected_hz;
} CLOCK_CASES[] = {
	{ "CAPCOM timers run at oscillator / 8", 3250000 },
	{ "fSYS PLL does not change CAPCOM timers", 3250000 },
	{ "AHB_PER PLL/2 does not change CAPCOM timers", 3250000 },
	{ "FPI2 PLL /2 selects 6.5 MHz CAPCOM timers", 6500000 },
	{ "FPI2 PLL /4 selects 3.25 MHz CAPCOM timers", 3250000 },
	{ "FPI2 PLL /8 selects 1.625 MHz CAPCOM timers", 1625000 },
	{ "FPI2 PLL /16 selects 812.5 kHz CAPCOM timers", 812500 },
	{ "CAPCOM RMC/2 halves the timer clock", 1625000 },
	{ "CAPCOM RMC/4 quarters the timer clock", 812500 },
	{ "CAPCOM RMC/8 divides the timer clock by eight", 406250 },
	{ "CAPCOM RMC/4 divides the selected FPI2 PLL /4 clock", 812500 },
};

static struct timer_counts measure_timers(uint32_t capcom) {
	CAPCOM_T01CON(capcom) = 0;
	CAPCOM_T0(capcom) = 0;
	CAPCOM_T1(capcom) = 0;
	CAPCOM_T0REL(capcom) = 0;
	CAPCOM_T1REL(capcom) = 0;
	test_rtc_init();
	CAPCOM_T01CON(capcom) = CAPCOM_T01CON_T0R_ENABLED | CAPCOM_T01CON_T1R_ENABLED;
	/* Count 16-bit overflows during the RTC measurement window. */
	uint32_t previous_t0 = 0;
	uint32_t previous_t1 = 0;
	uint32_t wraps_t0 = 0;
	uint32_t wraps_t1 = 0;
	while (!test_rtc_second_elapsed()) {
		uint32_t t0 = CAPCOM_T0(capcom) & CAPCOM_T0_T0;
		uint32_t t1 = CAPCOM_T1(capcom) & CAPCOM_T1_T1;
		wraps_t0 += t0 < previous_t0;
		wraps_t1 += t1 < previous_t1;
		previous_t0 = t0;
		previous_t1 = t1;
	}
	CAPCOM_T01CON(capcom) = 0;
	uint32_t t0 = CAPCOM_T0(capcom) & CAPCOM_T0_T0;
	uint32_t t1 = CAPCOM_T1(capcom) & CAPCOM_T1_T1;
	wraps_t0 += t0 < previous_t0;
	wraps_t1 += t1 < previous_t1;
	test_watchdog_serve();

	struct timer_counts counts = {
		(wraps_t0 << 16) | t0,
		(wraps_t1 << 16) | t1,
	};
	return counts;
}

int main(void) {
	test_start("CGU CAPCOM timer clock test");

	uint32_t capcoms[] = { CAPCOM0, CAPCOM1 };
	struct timer_counts counts[2][ARRAY_SIZE(CLOCK_CASES)] = { 0 };
	uint32_t bypass_con1 = CGU_CON1 & ~CGU_CON1_FSYS_CLKSEL;
	uint32_t fpi2_con1 = bypass_con1 &
		~(CGU_CON1_FPI2_OSC_DISABLE | CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV);
	uint32_t osc_con3 = CGU_CON3 & ~(CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV);
	uint32_t pll_con3 = osc_con3 | CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2;
	cgu_pll_set(3, 0);
	bool width_matches = true;

	for (uint32_t index = 0; index < ARRAY_SIZE(capcoms); index++) {
		CAPCOM_CLC(capcoms[index]) = (1 << MOD_CLC_RMC_SHIFT);
		CAPCOM_T0(capcoms[index]) = 0x12345678;
		CAPCOM_T1(capcoms[index]) = 0x12345678;
		CAPCOM_T0REL(capcoms[index]) = 0x12345678;
		CAPCOM_T1REL(capcoms[index]) = 0x12345678;
		width_matches = width_matches && CAPCOM_T0(capcoms[index]) == 0x5678 &&
			CAPCOM_T1(capcoms[index]) == 0x5678 && CAPCOM_T0REL(capcoms[index]) == 0x5678 &&
			CAPCOM_T1REL(capcoms[index]) == 0x5678;
		CGU_CON1 = bypass_con1;
		CGU_CON3 = osc_con3;
		counts[index][0] = measure_timers(capcoms[index]);
		CGU_CON1 = bypass_con1 | CGU_CON1_FSYS_CLKSEL_PLL;
		counts[index][1] = measure_timers(capcoms[index]);
		CGU_CON1 = bypass_con1;
		CGU_CON3 = pll_con3;
		counts[index][2] = measure_timers(capcoms[index]);
		CGU_CON3 = osc_con3;
		CGU_CON1 = fpi2_con1 | CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV1;
		counts[index][3] = measure_timers(capcoms[index]);
		CGU_CON1 = fpi2_con1 | CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV2;
		counts[index][4] = measure_timers(capcoms[index]);
		CGU_CON1 = fpi2_con1 | CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV4;
		counts[index][5] = measure_timers(capcoms[index]);
		CGU_CON1 = fpi2_con1 | CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV8;
		counts[index][6] = measure_timers(capcoms[index]);
		CGU_CON1 = bypass_con1;
		CAPCOM_CLC(capcoms[index]) = (2 << MOD_CLC_RMC_SHIFT);
		counts[index][7] = measure_timers(capcoms[index]);
		CAPCOM_CLC(capcoms[index]) = (4 << MOD_CLC_RMC_SHIFT);
		counts[index][8] = measure_timers(capcoms[index]);
		CAPCOM_CLC(capcoms[index]) = (8 << MOD_CLC_RMC_SHIFT);
		counts[index][9] = measure_timers(capcoms[index]);
		CGU_CON1 = fpi2_con1 | CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV2;
		CAPCOM_CLC(capcoms[index]) = (4 << MOD_CLC_RMC_SHIFT);
		counts[index][10] = measure_timers(capcoms[index]);
	}

	test_check("CAPCOM0/1 timers and reload registers are 16-bit", width_matches);
	for (uint32_t mode = 0; mode < ARRAY_SIZE(CLOCK_CASES); mode++) {
		uint32_t expected_hz = CLOCK_CASES[mode].expected_hz;
		bool matched = true;
		for (uint32_t index = 0; index < ARRAY_SIZE(capcoms); index++) {
			struct timer_counts result = counts[index][mode];
			uint32_t t0_hz = result.t0;
			uint32_t t1_hz = result.t1;
			printf("# CAPCOM%lu %s: T0=%lu Hz T1=%lu Hz\n", index,
				CLOCK_CASES[mode].name, t0_hz, t1_hz);
			matched = matched && test_u32_in_interval(t0_hz, expected_hz * 97 / 100, expected_hz * 103 / 100) &&
				test_u32_in_interval(t1_hz, expected_hz * 97 / 100, expected_hz * 103 / 100);
		}
		test_check(CLOCK_CASES[mode].name, matched);
	}
	return test_finish();
}
