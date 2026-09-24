#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

static const struct {
	const char *oscillator_test;
	const char *pll_test;
	uint32_t rmc;
	uint32_t rmc2;
} DIVIDER_CASES[] = {
	{ "RMC2=0 does not affect oscillator source", "PLL divisor is RMC=1 + RMC2=0", 1, 0 },
	{ "RMC2=1 does not affect oscillator source", "PLL divisor is RMC=1 + RMC2=1", 1, 1 },
	{ "RMC2=4 does not affect oscillator source", "PLL divisor is RMC=1 + RMC2=4", 1, 4 },
	{ "RMC2=1 does not affect oscillator source with RMC=4", "PLL divisor is RMC=4 + RMC2=1", 4, 1 },
	{ "RMC2=4 does not affect oscillator source with RMC=2", "PLL divisor is RMC=2 + RMC2=4", 2, 4 },
	{ "RMC2=4 does not affect oscillator source with RMC=4", "PLL divisor is RMC=4 + RMC2=4", 4, 4 },
};

static uint32_t stm_clc_with_dividers(uint32_t clc, uint32_t rmc, uint32_t rmc2) {
	return (clc & ~(MOD_CLC_RMC | STM_CLC_RMC2)) |
		(rmc << MOD_CLC_RMC_SHIFT) | (rmc2 << STM_CLC_RMC2_SHIFT);
}

static uint32_t measure_stm_khz(void) {
	test_rtc_init();
	uint32_t first_stm = STM_TIM0;
	test_rtc_wait_second();

	return (STM_TIM0 - first_stm) / 1000;
}

static void configure_stm(uint32_t con1, uint32_t clc) {
	CGU_CON1 = con1;
	STM_CLC = MOD_CLC_DISR;
	while ((STM_CLC & MOD_CLC_DISS) == 0)
		;
	STM_CLC = clc & ~(MOD_CLC_DISR | MOD_CLC_DISS);
	while ((STM_CLC & MOD_CLC_DISS) != 0)
		;
}

int main(void) {
	test_start("CGU STM clock test");

	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_clc = STM_CLC;
	uint32_t base_con1 = initial_con1 &
		~(CGU_CON1_FPI2_OSC_DISABLE | CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV);

	test_category("Oscillator source");
	configure_stm(base_con1 | CGU_CON1_FPI2_CLKDIV_DIV8, stm_clc_with_dividers(initial_clc, 1, 0));
	uint32_t bypass_model_hz = cpu_get_stm_freq();
	uint32_t bypass_khz = measure_stm_khz();
	uint32_t bypass_expected_khz = CPU_OSC_FREQ / 1000;
	printf("# bypass: expected=%lu kHz measured=%lu kHz model=%lu Hz\n",
		bypass_expected_khz, bypass_khz, bypass_model_hz);
	test_check("FPI2 divider does not affect the oscillator source",
		test_u32_in_interval(bypass_khz, bypass_expected_khz * 98 / 100, bypass_expected_khz * 102 / 100));
	test_eq_u32("STM model selects the oscillator source", CPU_OSC_FREQ, bypass_model_hz);

	test_category("PLL source");
	for (uint32_t divider = 0; divider < 4; divider++) {
		uint32_t con1 = base_con1 | CGU_CON1_FPI2_CLKSEL_PLL | (divider << CGU_CON1_FPI2_CLKDIV_SHIFT);
		configure_stm(con1, stm_clc_with_dividers(initial_clc, 1, 1));
		uint32_t modeled_hz = cpu_get_stm_freq();
		uint32_t measured_khz = measure_stm_khz();
		uint32_t expected_hz = CPU_OSC_FREQ >> (divider + 2);
		uint32_t expected_khz = expected_hz / 1000;

		printf("# DIV=%lu: expected=%lu kHz measured=%lu kHz model=%lu Hz\n",
			divider, expected_khz, measured_khz, modeled_hz);
		test_check("FPI2 divider selects the expected STM rate",
			test_u32_in_interval(measured_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));
		test_eq_u32("STM model follows the FPI2 divider", expected_hz, modeled_hz);
	}

	test_category("STM CLC source-dependent dividers");
	cgu_pll_set(3, 0);
	uint32_t pll_con1 = base_con1 | CGU_CON1_FPI2_CLKSEL_PLL | CGU_CON1_FPI2_CLKDIV_DIV2;
	for (uint32_t index = 0; index < ARRAY_SIZE(DIVIDER_CASES); index++) {
		uint32_t clc = stm_clc_with_dividers(initial_clc, DIVIDER_CASES[index].rmc, DIVIDER_CASES[index].rmc2);
		configure_stm(base_con1, clc);
		uint32_t oscillator_model_hz = cpu_get_stm_freq();
		uint32_t oscillator_khz = measure_stm_khz();
		configure_stm(pll_con1, clc);
		uint32_t pll_model_hz = cpu_get_stm_freq();
		uint32_t pll_khz = measure_stm_khz();
		uint32_t expected_oscillator_hz = CPU_OSC_FREQ / DIVIDER_CASES[index].rmc;
		uint32_t expected_pll_hz = CPU_OSC_FREQ / (DIVIDER_CASES[index].rmc + DIVIDER_CASES[index].rmc2);
		uint32_t expected_oscillator_khz = expected_oscillator_hz / 1000;
		uint32_t expected_pll_khz = expected_pll_hz / 1000;

		printf("# RMC=%lu RMC2=%lu: oscillator=%lu/%lu kHz model=%lu Hz; PLL=%lu/%lu kHz model=%lu Hz\n",
			DIVIDER_CASES[index].rmc, DIVIDER_CASES[index].rmc2,
			oscillator_khz, expected_oscillator_khz, oscillator_model_hz,
			pll_khz, expected_pll_khz, pll_model_hz);
		test_check(DIVIDER_CASES[index].oscillator_test,
			test_u32_in_interval(oscillator_khz, expected_oscillator_khz * 98 / 100,
				expected_oscillator_khz * 102 / 100));
		test_eq_u32("STM model uses only RMC for oscillator source", expected_oscillator_hz, oscillator_model_hz);
		test_check(DIVIDER_CASES[index].pll_test,
			test_u32_in_interval(pll_khz, expected_pll_khz * 98 / 100, expected_pll_khz * 102 / 100));
		test_eq_u32("STM model uses RMC + RMC2 for PLL source", expected_pll_hz, pll_model_hz);
	}

	return test_finish();
}
