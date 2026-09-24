#include <pmb887x.h>

#include "test.h"

#define RTC_T14_PRESCALER 8
#define RTC_MEASURE_STM_TICKS (CPU_OSC_FREQ / 8)
#define RTC_ENABLE_TIMEOUT_TICKS (CPU_OSC_FREQ / 1000)
#define RTC_START_TIMEOUT_TICKS (CPU_OSC_FREQ / 100)

static void configure_rtc(void) {
	SCU_RTCIF = 0xAA;
	RTC_CLC = (1 << MOD_CLC_RMC_SHIFT);
	RTC_CTRL |= RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN;
	RTC_CON = RTC_CON_PRE;
	RTC_T14 = 0;
	RTC_REL = 0;
	RTC_ALARM = 0;
	RTC_SRC = 0;
	RTC_ISNC = 0;
	RTC_CTRL |= RTC_CTRL_CLK_SEL | RTC_CTRL_CLR_RTCBAD | RTC_CTRL_CLR_RTCINT;
}

static uint32_t get_t14_count(void) {
	return (RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT;
}

static bool configure_t14(bool prescaler) {
	uint32_t control = prescaler ? RTC_CON_PRE : 0;
	RTC_CON = control;
	uint32_t start = STM_TIM0;
	while ((RTC_CON & RTC_CON_ACCPOS) == 0 && STM_TIM0 - start < RTC_START_TIMEOUT_TICKS)
		test_watchdog_serve();
	if ((RTC_CON & RTC_CON_ACCPOS) == 0)
		return false;

	RTC_T14 = 0;
	RTC_CON = control | RTC_CON_RUN;
	start = STM_TIM0;
	while (get_t14_count() == 0 && STM_TIM0 - start < RTC_START_TIMEOUT_TICKS)
		test_watchdog_serve();
	return get_t14_count() != 0 && ((RTC_CON & RTC_CON_PRE) != 0) == prescaler;
}

static uint32_t measure_t14_ticks(uint32_t *stm_ticks) {
	uint32_t first_t14 = get_t14_count();
	uint32_t first_stm = STM_TIM0;
	while (STM_TIM0 - first_stm < RTC_MEASURE_STM_TICKS)
		test_watchdog_serve();
	*stm_ticks = STM_TIM0 - first_stm;
	return (get_t14_count() - first_t14) & 0xFFFF;
}

static uint32_t stm_ticks_to_t14_ticks(uint32_t stm_ticks, uint32_t divider) {
	return (uint64_t) stm_ticks * CPU_CLK32K_FREQ / CPU_OSC_FREQ / divider;
}

int main(void) {
	test_start("CGU RTC clock test");

	CGU_CON1 &= ~(CGU_CON1_FPI2_OSC_DISABLE | CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV);
	STM_CLC = (1 << MOD_CLC_RMC_SHIFT);
	configure_rtc();

	test_category("RTC prescaler");
	bool direct_selected = configure_t14(false);
	uint32_t stm_ticks;
	uint32_t direct_ticks = measure_t14_ticks(&stm_ticks);
	uint32_t expected_direct = stm_ticks_to_t14_ticks(stm_ticks, 1);
	bool divided_selected = configure_t14(true);
	uint32_t divided_ticks = measure_t14_ticks(&stm_ticks);
	uint32_t expected_divided = stm_ticks_to_t14_ticks(stm_ticks, RTC_T14_PRESCALER);
	printf("# RTC T14: direct=%lu/%lu prescaled=%lu/%lu ticks\n", direct_ticks,
		expected_direct, divided_ticks, expected_divided);
	test_check("RTC T14 uses the 32.768 kHz source without PRE",
		direct_selected && test_u32_in_interval(direct_ticks, expected_direct * 98 / 100,
			expected_direct * 102 / 100));
	test_check("RTC T14 PRE divides the 32.768 kHz source by eight",
		divided_selected && test_u32_in_interval(divided_ticks, expected_divided * 98 / 100,
			expected_divided * 102 / 100));

	test_category("RTC module divider");
	for (uint32_t divider = 1; divider < 4; divider++) {
		uint32_t rmc = 1 << divider;
		RTC_CLC = (rmc << MOD_CLC_RMC_SHIFT);
		uint32_t selected_rmc = (RTC_CLC & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT;
		uint32_t measured_ticks = measure_t14_ticks(&stm_ticks);
		uint32_t expected_ticks = stm_ticks_to_t14_ticks(stm_ticks, RTC_T14_PRESCALER);
		printf("# RTC RMC=%lu: T14=%lu/%lu ticks\n", rmc,
			measured_ticks, expected_ticks);
		test_check("RTC module divider does not change T14 rate",
			selected_rmc == rmc && test_u32_in_interval(measured_ticks, expected_ticks * 98 / 100,
				expected_ticks * 102 / 100));
	}
	RTC_CLC = (1 << MOD_CLC_RMC_SHIFT);

	test_category("CLK32K output enable");
	CGU_CON2 &= ~CGU_CON2_CLK32K_EN;
	uint32_t output_off_ticks = measure_t14_ticks(&stm_ticks);
	uint32_t expected_off_ticks = stm_ticks_to_t14_ticks(stm_ticks, RTC_T14_PRESCALER);
	CGU_CON2 |= CGU_CON2_CLK32K_EN;
	uint32_t output_on_ticks = measure_t14_ticks(&stm_ticks);
	uint32_t expected_on_ticks = stm_ticks_to_t14_ticks(stm_ticks, RTC_T14_PRESCALER);
	printf("# RTC T14 with CLK32K output off/on: %lu/%lu and %lu/%lu ticks\n",
		output_off_ticks, expected_off_ticks,
		output_on_ticks, expected_on_ticks);
	test_check("CLK32K output enable does not gate the RTC timebase",
		test_u32_in_interval(output_off_ticks, expected_off_ticks * 98 / 100, expected_off_ticks * 102 / 100) &&
		test_u32_in_interval(output_on_ticks, expected_on_ticks * 98 / 100, expected_on_ticks * 102 / 100));

	test_category("RTC module clock gate");
	uint32_t first_t14 = get_t14_count();
	uint32_t first_stm = STM_TIM0;
	RTC_CLC = MOD_CLC_DISR;
	uint32_t disabled_clc = RTC_CLC;
	while (STM_TIM0 - first_stm < RTC_MEASURE_STM_TICKS)
		test_watchdog_serve();
	uint32_t elapsed_stm = STM_TIM0 - first_stm;
	RTC_CLC = (1 << MOD_CLC_RMC_SHIFT);
	uint32_t enable_start = STM_TIM0;
	while ((RTC_CLC & MOD_CLC_DISS) != 0 && STM_TIM0 - enable_start < RTC_ENABLE_TIMEOUT_TICKS);
	uint32_t enabled_clc = RTC_CLC;
	uint32_t gated_t14 = get_t14_count();
	uint32_t gated_ticks = (gated_t14 - first_t14) & 0xFFFF;
	uint32_t resume_start = STM_TIM0;
	stopwatch_usleep_wd(10000);
	uint32_t resumed_t14 = get_t14_count();
	uint32_t resume_stm = STM_TIM0 - resume_start;
	uint32_t resumed_ticks = (resumed_t14 - gated_t14) & 0xFFFF;
	uint32_t expected_gated_ticks = stm_ticks_to_t14_ticks(elapsed_stm, RTC_T14_PRESCALER);
	uint32_t expected_resumed_ticks = stm_ticks_to_t14_ticks(resume_stm, RTC_T14_PRESCALER);
	printf("# RTC module clock: disabled=%08lX enabled=%08lX T14=%lu/%lu ticks; resumed=%lu/%lu ticks\n",
		disabled_clc, enabled_clc, gated_ticks,
		expected_gated_ticks, resumed_ticks, expected_resumed_ticks);
	test_check("RTC_CLC disables the RTC module clock", (disabled_clc & MOD_CLC_DISS) != 0);
	test_check("RTC T14 stops while its module clock is disabled", gated_ticks < expected_gated_ticks / 16);
	test_check("RTC T14 resumes after its module clock is enabled", (enabled_clc & MOD_CLC_DISS) == 0 &&
		(RTC_CON & RTC_CON_RUN) != 0 &&
		test_u32_in_interval(resumed_ticks, expected_resumed_ticks * 80 / 100,
			expected_resumed_ticks * 120 / 100));
	return test_finish();
}
