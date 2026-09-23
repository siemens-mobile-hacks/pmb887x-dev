#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#define SCCU_TIMEOUT_MS 300
#define SCCU_FRAMES 32
#define SCCU_NQTZ_COUNTS 151

static const struct {
	const char *name;
	uint32_t ndiv;
	enum cgu_fsys_source fsys_source;
} CLOCK_CASES[] = {
	{ "SCCU uses standby clock with fSYS bypass", 3, CGU_FSYS_OSC },
	{ "SCCU uses standby clock with 52 MHz fSYS", 3, CGU_FSYS_PLL },
	{ "SCCU uses standby clock with 65 MHz fSYS", 4, CGU_FSYS_PLL },
	{ "SCCU uses standby clock with 78 MHz fSYS", 5, CGU_FSYS_PLL },
};

static uint32_t measure_sleep_timer_us(void) {
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPRST;
	stopwatch_t start = stopwatch_get();
	while ((SCCU_TDMOUT & SCCU_TDMOUT_TDMAOUT) != 0 && stopwatch_elapsed_ms(start) < SCCU_TIMEOUT_MS)
		test_watchdog_serve();
	if ((SCCU_TDMOUT & SCCU_TDMOUT_TDMAOUT) != 0)
		return 0;

	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI;
	SCCU_NQTZ = SCCU_NQTZ_COUNTS;
	SCCU_TDMINI = SCCU_FRAMES - 1;
	start = stopwatch_get();
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPEN;
	while ((SCCU_SLPCTRL & SCCU_SLPCTRL_SLPEN) != 0 && stopwatch_elapsed_ms(start) < SCCU_TIMEOUT_MS)
		test_watchdog_serve();

	uint32_t elapsed_us = stopwatch_elapsed_us(start);
	bool completed = (SCCU_SLPCTRL & SCCU_SLPCTRL_SLPEN) == 0;
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPRST;
	if (!completed)
		return 0;

	return elapsed_us;
}

int main(void) {
	test_start("CGU SCCU standby clock test");

	SCU_CLC = (2 << MOD_CLC_RMC_SHIFT);
	uint32_t expected_us = (uint64_t) SCCU_FRAMES * SCCU_NQTZ_COUNTS * 1000000 / CPU_CLK32K_FREQ;
	uint32_t elapsed_us[ARRAY_SIZE(CLOCK_CASES)] = { 0 };
	for (uint32_t index = 0; index < ARRAY_SIZE(CLOCK_CASES); index++) {
		cgu_pll_set(CLOCK_CASES[index].ndiv, 0);
		cgu_fsys_select(CLOCK_CASES[index].fsys_source);
		elapsed_us[index] = measure_sleep_timer_us();
		test_watchdog_reset();
	}

	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPRST;
	cgu_fsys_select(CGU_FSYS_OSC);
	cgu_pll_set(3, 0);
	SCU_CLC = (1 << MOD_CLC_RMC_SHIFT);
	uint32_t module_rmc1_us = measure_sleep_timer_us();
	SCU_CLC = (4 << MOD_CLC_RMC_SHIFT);
	uint32_t module_rmc4_us = measure_sleep_timer_us();

	for (uint32_t index = 0; index < ARRAY_SIZE(CLOCK_CASES); index++) {
		printf("# %s: expected=%lu us measured=%lu us\n", CLOCK_CASES[index].name,
			expected_us, elapsed_us[index]);
		test_check(CLOCK_CASES[index].name,
			test_u32_in_interval(elapsed_us[index], expected_us * 98 / 100, expected_us * 102 / 100));
	}
	printf("# SCCU with SCU RMC /1 and /4: %lu/%lu us\n",
		module_rmc1_us, module_rmc4_us);
	test_check("SCU module divider does not change SCCU standby timing",
		test_u32_in_interval(module_rmc1_us, expected_us * 98 / 100, expected_us * 102 / 100) &&
		test_u32_in_interval(module_rmc4_us, expected_us * 98 / 100, expected_us * 102 / 100));

	return test_finish();
}
