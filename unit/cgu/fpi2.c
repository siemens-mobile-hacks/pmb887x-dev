#include <pmb887x.h>

#include "clock.h"
#include "test.h"

#define FPI2_FIELDS (CGU_CON1_FPI2_OSC_DISABLE | CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV)

struct fpi2_case {
	const char *name;
	uint32_t source;
	uint32_t divider;
	uint32_t pll_ndiv;
	uint32_t expected_khz;
};

static const struct fpi2_case CASES[] = {
	{ "OSC", CGU_CON1_FPI2_CLKSEL_OSC, 0, 3, 26000 },
	{ "OSC with CLKDIV /2", CGU_CON1_FPI2_CLKSEL_OSC, 1, 3, 26000 },
	{ "OSC with CLKDIV /4", CGU_CON1_FPI2_CLKSEL_OSC, 2, 3, 26000 },
	{ "OSC with CLKDIV /8", CGU_CON1_FPI2_CLKSEL_OSC, 3, 3, 26000 },
	{ "PLL/2 of 104 MHz", CGU_CON1_FPI2_CLKSEL_PLL, 0, 3, 52000 },
	{ "PLL/2 of 104 MHz with CLKDIV /2", CGU_CON1_FPI2_CLKSEL_PLL, 1, 3, 26000 },
	{ "PLL/2 of 104 MHz with CLKDIV /4", CGU_CON1_FPI2_CLKSEL_PLL, 2, 3, 13000 },
	{ "PLL/2 of 104 MHz with CLKDIV /8", CGU_CON1_FPI2_CLKSEL_PLL, 3, 3, 6500 },
	{ "PLL/2 of 130 MHz with CLKDIV /2", CGU_CON1_FPI2_CLKSEL_PLL, 1, 4, 32500 },
	{ "PLL/2 of 156 MHz with CLKDIV /2", CGU_CON1_FPI2_CLKSEL_PLL, 1, 5, 39000 },
};

static void configure_fpi2(uint32_t con1, uint32_t stm_clc) {
	CGU_CON1 = con1;
	STM_CLC = MOD_CLC_DISR;
	while ((STM_CLC & MOD_CLC_DISS) == 0)
		;
	STM_CLC = stm_clc;
	while ((STM_CLC & MOD_CLC_DISS) != 0)
		;
}

static void align_rtc_second(void) {
	test_rtc_init();
	test_rtc_wait_second();
}

static uint32_t measure_stm_khz(void) {
	test_rtc_init();
	uint32_t first_stm = STM_TIM0;
	test_rtc_wait_second();
	return (STM_TIM0 - first_stm) / 1000;
}

int main(void) {
	test_start("CGU FPI2 bus clock test");

	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_stm_clc = STM_CLC;
	uint32_t base_con1 = initial_con1 & ~FPI2_FIELDS;
	uint32_t fpi2_cycle_stm_clc = (initial_stm_clc & ~(MOD_CLC_DISR | MOD_CLC_DISS | MOD_CLC_RMC | STM_CLC_RMC2)) |
		(1 << MOD_CLC_RMC_SHIFT);

	align_rtc_second();
	for (uint32_t index = 0; index < ARRAY_SIZE(CASES); index++) {
		const struct fpi2_case *fpi2 = &CASES[index];
		uint32_t con1 = base_con1 | fpi2->source | (fpi2->divider << CGU_CON1_FPI2_CLKDIV_SHIFT);

		configure_fpi2(base_con1, fpi2_cycle_stm_clc);
		cgu_pll_set(fpi2->pll_ndiv, 0);
		configure_fpi2(con1, fpi2_cycle_stm_clc);
		uint32_t modeled_hz = cpu_get_fpi2_freq();
		uint32_t measured_khz = measure_stm_khz();
		configure_fpi2(base_con1, fpi2_cycle_stm_clc);

		printf("# %s: expected=%lu kHz measured=%lu kHz model=%lu kHz\n",
			fpi2->name, fpi2->expected_khz, measured_khz, modeled_hz / 1000);
		test_check(fpi2->name, modeled_hz / 1000 == fpi2->expected_khz &&
			test_u32_in_interval(measured_khz, fpi2->expected_khz * 98 / 100, fpi2->expected_khz * 102 / 100));
	}

	configure_fpi2(initial_con1, initial_stm_clc);
	return test_finish();
}
