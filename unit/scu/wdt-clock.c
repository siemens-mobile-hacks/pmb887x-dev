#include <pmb887x.h>

#include "cgu/clock.h"
#include "scu/watchdog.h"
#include "test.h"

#define WDT_CLOCK_DIVIDER 16384
#define WDT_FPI2_DIVIDER 2048
#define RTC_T14_RELOAD 61440
#define RTC_T14_PERIOD 4096
#define RTC_MEASURE_COUNTS 512
#define RTC_POLL_LIMIT 10000000

static const struct {
	const char *name;
	uint32_t ndiv;
	uint32_t fsys_source;
} CLOCK_CASES[] = {
	{ "WDT uses OSC with fSYS bypass", 3, CGU_CON1_FSYS_CLKSEL_BYPASS },
	{ "WDT uses OSC with 52 MHz fSYS", 3, CGU_CON1_FSYS_CLKSEL_PLL },
	{ "WDT uses OSC with 65 MHz fSYS", 4, CGU_CON1_FSYS_CLKSEL_PLL },
	{ "WDT uses OSC with 78 MHz fSYS", 5, CGU_CON1_FSYS_CLKSEL_PLL },
};

static const struct {
	const char *name;
	uint32_t fpi2_config;
	uint32_t source_divider;
	uint32_t stm_rmc;
} DIVIDER_CASES[] = {
	{ "FPI2 oscillator", 0, 8, 1 },
	{ "FPI2 oscillator with STM RMC /4", 0, 8, 4 },
	{ "FPI2 PLL /2", CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV1, 4, 1 },
	{ "FPI2 PLL /4", CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV2, 8, 1 },
	{ "FPI2 PLL /8", CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV4, 16, 1 },
	{ "FPI2 PLL /16", CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV8, 32, 1 },
	{ "FPI2 PLL /16 with STM RMC /4", CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV8, 32, 4 },
};

static __TCM uint32_t count_watchdog_ticks(uint32_t selected_con1, uint32_t restore_con1) {
	CGU_CON1 = selected_con1;
	uint32_t first = (SCU_WDT_SR & SCU_WDT_SR_WDTTIM) >> SCU_WDT_SR_WDTTIM_SHIFT;
	uint32_t start = STM_TIM0;
	while (STM_TIM0 - start < CPU_OSC_FREQ / 20);
	uint32_t last = (SCU_WDT_SR & SCU_WDT_SR_WDTTIM) >> SCU_WDT_SR_WDTTIM_SHIFT;
	CGU_CON1 = restore_con1;

	return (last - first) & 0xFFFF;
}

static uint32_t measure_selected_clock_hz(uint32_t selected_con1, uint32_t restore_con1) {
	scu_watchdog_configure(0, 0, true);
	stopwatch_usleep_wd(1000);
	uint32_t ticks = count_watchdog_ticks(selected_con1, restore_con1);
	scu_watchdog_configure(SCU_WDTCON1_WDTDR, 0, true);

	return ticks * 20;
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

static uint32_t measure_watchdog_with_rtc(void) {
	uint32_t first_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
	uint32_t first_wdt = (SCU_WDT_SR & SCU_WDT_SR_WDTTIM) >> SCU_WDT_SR_WDTTIM_SHIFT;
	uint32_t elapsed_t14;
	uint32_t polls = 0;
	do {
		uint32_t current_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
		elapsed_t14 = (current_t14 - first_t14) & (RTC_T14_PERIOD - 1);
		polls++;
	} while (elapsed_t14 < RTC_MEASURE_COUNTS && polls < RTC_POLL_LIMIT);
	if (elapsed_t14 < RTC_MEASURE_COUNTS)
		return 0;

	uint32_t last_wdt = (SCU_WDT_SR & SCU_WDT_SR_WDTTIM) >> SCU_WDT_SR_WDTTIM_SHIFT;
	return (uint64_t) ((last_wdt - first_wdt) & 0xFFFF) * RTC_T14_PERIOD / elapsed_t14;
}

int main(void) {
	test_start("CGU watchdog clock test");

	uint32_t initial_stm_clc = STM_CLC;
	uint32_t bypass_con1 = CGU_CON1 & ~CGU_CON1_FSYS_CLKSEL;
	uint32_t measured_hz[ARRAY_SIZE(CLOCK_CASES)] = { 0 };
	for (uint32_t index = 0; index < ARRAY_SIZE(CLOCK_CASES); index++) {
		cgu_pll_set(CLOCK_CASES[index].ndiv, 0);
		measured_hz[index] = measure_selected_clock_hz(bypass_con1 | CLOCK_CASES[index].fsys_source,
			bypass_con1);
		test_watchdog_reset();
	}

	uint32_t ahb_hz = measure_selected_clock_hz(bypass_con1 | CGU_CON1_AHB_CLKSEL_PLL, bypass_con1);
	uint32_t fpi1_hz = measure_selected_clock_hz(
		(bypass_con1 & ~CGU_CON1_FPI1_CLKSEL) | CGU_CON1_FPI1_CLKSEL_PLL_DIV_2, bypass_con1);

	cgu_pll_set(3, 0);
	configure_rtc();
	uint32_t base_con1 = bypass_con1 &
		~(CGU_CON1_FPI2_OSC_DISABLE | CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV);
	uint32_t rtc_wdt_hz[ARRAY_SIZE(DIVIDER_CASES)];
	for (uint32_t mode = 0; mode < ARRAY_SIZE(DIVIDER_CASES); mode++) {
		scu_watchdog_configure(0, 0, true);
		CGU_CON1 = base_con1 | DIVIDER_CASES[mode].fpi2_config;
		STM_CLC = (initial_stm_clc & ~MOD_CLC_RMC) |
			(DIVIDER_CASES[mode].stm_rmc << MOD_CLC_RMC_SHIFT);
		rtc_wdt_hz[mode] = measure_watchdog_with_rtc();
		STM_CLC = initial_stm_clc;
		CGU_CON1 = bypass_con1;
		scu_watchdog_configure(SCU_WDTCON1_WDTDR, 0, true);
	}

	cgu_pll_set(5, 0);
	scu_watchdog_configure(0, 0, true);
	CGU_CON1 = base_con1 | CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV2;
	uint32_t pll156_wdt_hz = measure_watchdog_with_rtc();
	CGU_CON1 = bypass_con1;
	scu_watchdog_configure(SCU_WDTCON1_WDTDR, 0, true);

	uint32_t expected_hz = CPU_OSC_FREQ / WDT_CLOCK_DIVIDER;
	for (uint32_t index = 0; index < ARRAY_SIZE(CLOCK_CASES); index++) {
		printf("# %s: expected=%lu Hz measured=%lu Hz\n", CLOCK_CASES[index].name,
			expected_hz, measured_hz[index]);
		test_check(CLOCK_CASES[index].name,
			test_u32_in_interval(measured_hz[index], expected_hz * 98 / 100, expected_hz * 102 / 100));
	}
	printf("# WDT with AHB=156 MHz: %lu Hz; with FPI1=78 MHz: %lu Hz\n",
		ahb_hz, fpi1_hz);
	test_check("AHB and ARM clock do not select the watchdog clock",
		test_u32_in_interval(ahb_hz, expected_hz * 98 / 100, expected_hz * 102 / 100));
	test_check("FPI1 clock does not select the watchdog clock",
		test_u32_in_interval(fpi1_hz, expected_hz * 98 / 100, expected_hz * 102 / 100));
	for (uint32_t mode = 0; mode < ARRAY_SIZE(DIVIDER_CASES); mode++) {
		uint32_t selected_hz = CPU_OSC_FREQ / (DIVIDER_CASES[mode].source_divider * WDT_FPI2_DIVIDER);
		printf("# WDT %s: expected=%lu Hz measured=%lu Hz\n", DIVIDER_CASES[mode].name,
			selected_hz, rtc_wdt_hz[mode]);
		test_check(DIVIDER_CASES[mode].name,
			test_u32_in_interval(rtc_wdt_hz[mode], selected_hz * 98 / 100, selected_hz * 102 / 100));
	}

	uint32_t expected_pll156_hz = 156000000 / (4 * 8 * WDT_FPI2_DIVIDER);
	printf("# WDT FPI2 PLL /4 with PLL 156 MHz: expected=%lu Hz measured=%lu Hz\n",
		expected_pll156_hz, pll156_wdt_hz);
	test_check("WDT FPI2 divider follows the selected PLL output",
		test_u32_in_interval(pll156_wdt_hz, expected_pll156_hz * 98 / 100, expected_pll156_hz * 102 / 100));
	return test_finish();
}
