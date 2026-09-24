#include <pmb887x.h>

#include "test.h"

#define RTC_T14_RELOAD 61440
#define RTC_T14_PERIOD (65536 - RTC_T14_RELOAD)
#define MEASURE_T14_COUNTS 512
#define RTC_POLL_LIMIT 10000000

static const struct {
	const char *name;
	uint32_t source_divider;
	uint32_t fpi2_divider;
	uint32_t rmc;
} COMBINED_CASES[] = {
	{ "FPI2 PLL /4 with STM RMC /2", 8, CGU_CON1_FPI2_CLKDIV_DIV2, 2 },
	{ "FPI2 PLL /4 with STM RMC /4", 8, CGU_CON1_FPI2_CLKDIV_DIV2, 4 },
	{ "FPI2 PLL /4 with STM RMC /8", 8, CGU_CON1_FPI2_CLKDIV_DIV2, 8 },
	{ "FPI2 PLL /8 with STM RMC /4", 16, CGU_CON1_FPI2_CLKDIV_DIV4, 4 },
};

static void rtc_init(void) {
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

static uint32_t measure_stm_khz(void) {
	uint32_t first_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
	uint32_t first_stm = STM_TIM0;
	uint32_t elapsed_t14;
	uint32_t polls = 0;

	do {
		uint32_t current_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
		elapsed_t14 = (current_t14 - first_t14) & (RTC_T14_PERIOD - 1);
		polls++;
	} while (elapsed_t14 < MEASURE_T14_COUNTS && polls < RTC_POLL_LIMIT);
	if (elapsed_t14 < MEASURE_T14_COUNTS)
		return 0;

	return (uint64_t) (STM_TIM0 - first_stm) * RTC_T14_PERIOD / MEASURE_T14_COUNTS / 1000;
}

int main(void) {
	test_start("CGU STM clock test");
	rtc_init();

	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_clc = STM_CLC;
	uint32_t base_con1 = initial_con1 &
		~(CGU_CON1_FPI2_OSC_DISABLE | CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV);

	test_category("Oscillator source");
	CGU_CON1 = base_con1 | CGU_CON1_FPI2_CLKDIV_DIV8;
	uint32_t bypass_khz = measure_stm_khz();
	CGU_CON1 = initial_con1;
	printf("# bypass: %lu kHz\n", bypass_khz);
	test_check("FPI2 divider does not affect the oscillator source",
		test_u32_in_interval(bypass_khz, CPU_OSC_FREQ / 1000 * 98 / 100, CPU_OSC_FREQ / 1000 * 102 / 100));

	test_category("PLL source");
	for (uint32_t divider = 0; divider < 4; divider++) {
		CGU_CON1 = base_con1 | CGU_CON1_FPI2_CLKSEL_PLL | (divider << CGU_CON1_FPI2_CLKDIV_SHIFT);
		uint32_t measured_khz = measure_stm_khz();
		CGU_CON1 = initial_con1;
		uint32_t expected_khz = (CPU_OSC_FREQ >> (divider + 2)) / 1000;

		printf("# DIV=%lu: expected=%lu kHz measured=%lu kHz\n",
			divider, expected_khz, measured_khz);
		test_check("FPI2 divider selects the expected STM rate",
			test_u32_in_interval(measured_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));
	}

	test_category("STM module divider");
	for (uint32_t divider = 0; divider < 4; divider++) {
		uint32_t rmc = 1 << divider;
		CGU_CON1 = base_con1;
		STM_CLC = (initial_clc & ~MOD_CLC_RMC) | (rmc << MOD_CLC_RMC_SHIFT);
		uint32_t measured_khz = measure_stm_khz();
		STM_CLC = initial_clc;
		CGU_CON1 = initial_con1;
		uint32_t expected_khz = CPU_OSC_FREQ / rmc / 1000;

		printf("# STM RMC=%lu: expected=%lu kHz measured=%lu kHz\n",
			rmc, expected_khz, measured_khz);
		test_check("STM module divider selects the expected rate",
			test_u32_in_interval(measured_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));
	}

	test_category("Combined dividers");
	for (uint32_t index = 0; index < ARRAY_SIZE(COMBINED_CASES); index++) {
		CGU_CON1 = base_con1 | CGU_CON1_FPI2_CLKSEL_PLL | COMBINED_CASES[index].fpi2_divider;
		STM_CLC = (initial_clc & ~MOD_CLC_RMC) | (COMBINED_CASES[index].rmc << MOD_CLC_RMC_SHIFT);
		uint32_t combined_khz = measure_stm_khz();
		STM_CLC = initial_clc;
		CGU_CON1 = initial_con1;
		uint32_t expected_khz = (uint64_t) CPU_OSC_FREQ * 2 /
			(COMBINED_CASES[index].source_divider * (COMBINED_CASES[index].rmc + 1)) / 1000;
		printf("# %s: expected=%lu kHz measured=%lu kHz\n", COMBINED_CASES[index].name,
			expected_khz, combined_khz);
		test_check(COMBINED_CASES[index].name,
			test_u32_in_interval(combined_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));
	}

	return test_finish();
}
