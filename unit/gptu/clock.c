#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#define RTC_T14_RELOAD 61440
#define RTC_T14_PERIOD (65536 - RTC_T14_RELOAD)
#define RTC_MEASURE_COUNTS 512
#define RTC_POLL_LIMIT 10000000

static const struct {
	const char *name;
	uint32_t ndiv;
	enum cgu_fsys_source fsys_source;
} CLOCK_CASES[] = {
	{ "GPTU uses OSC with PLL 104 and fSYS bypass", 3, CGU_FSYS_OSC },
	{ "GPTU uses OSC with PLL 104 and fSYS PLL", 3, CGU_FSYS_PLL },
	{ "GPTU uses OSC with PLL 130 and fSYS bypass", 4, CGU_FSYS_OSC },
	{ "GPTU uses OSC with PLL 130 and fSYS PLL", 4, CGU_FSYS_PLL },
	{ "GPTU uses OSC with PLL 156 and fSYS bypass", 5, CGU_FSYS_OSC },
	{ "GPTU uses OSC with PLL 156 and fSYS PLL", 5, CGU_FSYS_PLL },
};

struct clock_measurement {
	uint32_t t0_hz;
	uint32_t t1_hz;
	uint32_t t2_hz;
	uint32_t stm_hz;
};

static bool has_clock_rates(struct clock_measurement result, uint32_t gptu_hz, uint32_t stm_hz) {
	return test_u32_in_interval(result.t0_hz, gptu_hz * 98 / 100, gptu_hz * 102 / 100) &&
		test_u32_in_interval(result.t1_hz, gptu_hz * 98 / 100, gptu_hz * 102 / 100) &&
		test_u32_in_interval(result.t2_hz, gptu_hz * 98 / 100, gptu_hz * 102 / 100) &&
		test_u32_in_interval(result.stm_hz, stm_hz * 98 / 100, stm_hz * 102 / 100);
}

static void configure_rtc(void) {
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

static struct clock_measurement measure_gptu(uint32_t gptu, uint32_t rmc) {
	struct clock_measurement result = { 0 };
	uint32_t initial_clc = GPTU_CLC(gptu);
	GPTU_CLC(gptu) = (rmc << MOD_CLC_RMC_SHIFT);
	GPTU_T01IRS(gptu) = GPTU_T01IRS_T0BINS_CONCAT | GPTU_T01IRS_T0CINS_CONCAT |
		GPTU_T01IRS_T0DINS_CONCAT | GPTU_T01IRS_T1BINS_CONCAT |
		GPTU_T01IRS_T1CINS_CONCAT | GPTU_T01IRS_T1DINS_CONCAT;
	GPTU_T0DCBA(gptu) = 0;
	GPTU_T1DCBA(gptu) = 0;
	GPTU_T2CON(gptu) = 0;
	GPTU_T2RCCON(gptu) = 0;
	GPTU_T2(gptu) = 0;
	uint32_t first_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
	uint32_t first_stm = STM_TIM0;
	GPTU_T012RUN(gptu) = GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T0BRUN |
		GPTU_T012RUN_T0CRUN | GPTU_T012RUN_T0DRUN | GPTU_T012RUN_T1ARUN |
		GPTU_T012RUN_T1BRUN | GPTU_T012RUN_T1CRUN | GPTU_T012RUN_T1DRUN |
		GPTU_T012RUN_T2ASETR;

	uint32_t elapsed_t14;
	uint32_t polls = 0;
	do {
		uint32_t current_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
		elapsed_t14 = (current_t14 - first_t14) & (RTC_T14_PERIOD - 1);
		test_watchdog_serve();
		polls++;
	} while (elapsed_t14 < RTC_MEASURE_COUNTS && polls < RTC_POLL_LIMIT);
	GPTU_T012RUN(gptu) = GPTU_T012RUN_T2ACLRR | GPTU_T012RUN_T2BCLRR;
	uint32_t elapsed_stm = STM_TIM0 - first_stm;
	uint32_t elapsed_t0 = GPTU_T0DCBA(gptu);
	uint32_t elapsed_t1 = GPTU_T1DCBA(gptu);
	uint32_t elapsed_t2 = GPTU_T2(gptu);
	GPTU_T012RUN(gptu) = 0;
	GPTU_CLC(gptu) = initial_clc;
	if (elapsed_t14 < RTC_MEASURE_COUNTS)
		return result;

	result.t0_hz = (uint64_t) elapsed_t0 * RTC_T14_PERIOD / elapsed_t14;
	result.t1_hz = (uint64_t) elapsed_t1 * RTC_T14_PERIOD / elapsed_t14;
	result.t2_hz = (uint64_t) elapsed_t2 * RTC_T14_PERIOD / elapsed_t14;
	result.stm_hz = (uint64_t) elapsed_stm * RTC_T14_PERIOD / elapsed_t14;
	return result;
}

int main(void) {
	test_start("CGU GPTU clock source test");

	uint32_t initial_stm_clc = STM_CLC;
	configure_rtc();
	struct clock_measurement gptu0[ARRAY_SIZE(CLOCK_CASES)] = { 0 };
	struct clock_measurement gptu1[ARRAY_SIZE(CLOCK_CASES)] = { 0 };
	for (uint32_t index = 0; index < ARRAY_SIZE(CLOCK_CASES); index++) {
		cgu_pll_set(CLOCK_CASES[index].ndiv, 0);
		cgu_fsys_select(CLOCK_CASES[index].fsys_source);
		gptu0[index] = measure_gptu(GPTU0, 1);
		gptu1[index] = measure_gptu(GPTU1, 1);
		test_watchdog_reset();
	}

	cgu_fsys_select(CGU_FSYS_OSC);
	CGU_OSC &= ~CGU_OSC_PLL_BYPASS_N;
	uint32_t fpi2_mask = CGU_CON1_FPI2_OSC_DISABLE | CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV;
	CGU_CON1 = (CGU_CON1 & ~fpi2_mask) |
		CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV8;
	struct clock_measurement divided_osc_gptu0 = measure_gptu(GPTU0, 1);
	struct clock_measurement divided_osc_gptu1 = measure_gptu(GPTU1, 1);
	struct clock_measurement divided_rmc_gptu0 = measure_gptu(GPTU0, 4);
	struct clock_measurement divided_rmc_gptu1 = measure_gptu(GPTU1, 4);
	STM_CLC = (initial_stm_clc & ~MOD_CLC_RMC) | (4 << MOD_CLC_RMC_SHIFT);
	struct clock_measurement divided_stm_gptu0 = measure_gptu(GPTU0, 1);
	struct clock_measurement divided_stm_gptu1 = measure_gptu(GPTU1, 1);
	STM_CLC = initial_stm_clc;

	CGU_OSC |= CGU_OSC_PLL_BYPASS_N;
	struct clock_measurement divided_pll_gptu0 = measure_gptu(GPTU0, 1);
	struct clock_measurement divided_pll_gptu1 = measure_gptu(GPTU1, 1);
	uint32_t modeled_fpi2_hz = cpu_get_fpi2_freq();
	uint32_t modeled_stm_hz = cpu_get_stm_freq();
	CGU_CON1 |= CGU_CON1_FPI2_OSC_DISABLE;
	struct clock_measurement disabled_osc_gptu0 = measure_gptu(GPTU0, 1);
	struct clock_measurement disabled_osc_gptu1 = measure_gptu(GPTU1, 1);

	for (uint32_t index = 0; index < ARRAY_SIZE(CLOCK_CASES); index++) {
		printf("# %s: GPTU0 T0/T1/T2=%lu/%lu/%lu Hz GPTU1=%lu/%lu/%lu Hz STM=%lu/%lu Hz\n", CLOCK_CASES[index].name,
			gptu0[index].t0_hz, gptu0[index].t1_hz,
			gptu0[index].t2_hz, gptu1[index].t0_hz,
			gptu1[index].t1_hz, gptu1[index].t2_hz,
			gptu0[index].stm_hz, gptu1[index].stm_hz);
		test_check(CLOCK_CASES[index].name,
			has_clock_rates(gptu0[index], CPU_OSC_FREQ, CPU_OSC_FREQ) &&
			has_clock_rates(gptu1[index], CPU_OSC_FREQ, CPU_OSC_FREQ));
	}

	printf("# bypassed PLL /16: GPTU0 T0/T1/T2=%lu/%lu/%lu Hz GPTU1=%lu/%lu/%lu Hz STM=%lu/%lu Hz\n",
		divided_osc_gptu0.t0_hz, divided_osc_gptu0.t1_hz,
		divided_osc_gptu0.t2_hz, divided_osc_gptu1.t0_hz,
		divided_osc_gptu1.t1_hz, divided_osc_gptu1.t2_hz,
		divided_osc_gptu0.stm_hz, divided_osc_gptu1.stm_hz);
	test_check("GPTU follows divided oscillator at twice the STM rate",
		has_clock_rates(divided_osc_gptu0, CPU_OSC_FREQ / 16, CPU_OSC_FREQ / 32) &&
		has_clock_rates(divided_osc_gptu1, CPU_OSC_FREQ / 16, CPU_OSC_FREQ / 32));
	printf("# bypassed PLL /16 with GPTU RMC /4: GPTU0 T0/T1/T2=%lu/%lu/%lu Hz GPTU1=%lu/%lu/%lu Hz STM=%lu/%lu Hz\n",
		divided_rmc_gptu0.t0_hz, divided_rmc_gptu0.t1_hz,
		divided_rmc_gptu0.t2_hz, divided_rmc_gptu1.t0_hz,
		divided_rmc_gptu1.t1_hz, divided_rmc_gptu1.t2_hz,
		divided_rmc_gptu0.stm_hz, divided_rmc_gptu1.stm_hz);
	test_check("GPTU module divider scales its timers without changing STM",
		has_clock_rates(divided_rmc_gptu0, CPU_OSC_FREQ / 64, CPU_OSC_FREQ / 32) &&
		has_clock_rates(divided_rmc_gptu1, CPU_OSC_FREQ / 64, CPU_OSC_FREQ / 32));
	printf("# bypassed PLL /16 with STM RMC /4: GPTU0 T0/T1/T2=%lu/%lu/%lu Hz GPTU1=%lu/%lu/%lu Hz STM=%lu/%lu Hz\n",
		divided_stm_gptu0.t0_hz, divided_stm_gptu0.t1_hz,
		divided_stm_gptu0.t2_hz, divided_stm_gptu1.t0_hz,
		divided_stm_gptu1.t1_hz, divided_stm_gptu1.t2_hz,
		divided_stm_gptu0.stm_hz, divided_stm_gptu1.stm_hz);
	test_check("STM module divider does not change GPTU timer clocks",
		has_clock_rates(divided_stm_gptu0, CPU_OSC_FREQ / 16, CPU_OSC_FREQ / 80) &&
		has_clock_rates(divided_stm_gptu1, CPU_OSC_FREQ / 16, CPU_OSC_FREQ / 80));

	printf("# PLL 156 /16: GPTU0 T0/T1/T2=%lu/%lu/%lu Hz GPTU1=%lu/%lu/%lu Hz STM=%lu/%lu Hz\n",
		divided_pll_gptu0.t0_hz, divided_pll_gptu0.t1_hz,
		divided_pll_gptu0.t2_hz, divided_pll_gptu1.t0_hz,
		divided_pll_gptu1.t1_hz, divided_pll_gptu1.t2_hz,
		divided_pll_gptu0.stm_hz, divided_pll_gptu1.stm_hz);
	test_check("GPTU follows divided PLL at twice the STM rate",
		has_clock_rates(divided_pll_gptu0, 156000000 / 16, 156000000 / 32) &&
		has_clock_rates(divided_pll_gptu1, 156000000 / 16, 156000000 / 32));
	test_eq_u32("FPI2 calculation follows the selected PLL output", 156000000 / 16, modeled_fpi2_hz);
	test_eq_u32("STM calculation follows the selected FPI2 clock", 156000000 / 32, modeled_stm_hz);
	test_check("PLL selection overrides the disabled FPI2 oscillator source",
		has_clock_rates(disabled_osc_gptu0, 156000000 / 16, 156000000 / 32) &&
		has_clock_rates(disabled_osc_gptu1, 156000000 / 16, 156000000 / 32));
	return test_finish();
}
