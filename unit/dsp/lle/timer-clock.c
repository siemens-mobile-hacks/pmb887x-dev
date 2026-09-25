#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "test.h"

#define TIMER_SAMPLE_US 5000
#define TIMER1_DIVIDER 384
#define TIMER2_DIVIDER 96

struct timer_measurement {
	uint32_t timer_ticks;
	uint32_t stm_ticks;
	bool completed;
};

static struct timer_measurement measure_timer1(void) {
	struct timer_measurement result = { 0 };
	uint16_t first;
	uint16_t last;
	bool started = dsp_hw_reset() &&
		dsp_hw_write_reg(TEAK_TMR1_INT0, 0x0FFF) &&
		dsp_hw_write_reg(TEAK_TMR1_INT1, 0x0FFF) &&
		dsp_hw_write_reg(TEAK_TMR1_CTRL, TEAK_TMR1_CTRL_DT1ENA) &&
		dsp_hw_write_reg(TEAK_TMR1_CTRL, (TEAK_TMR1_CTRL_DT1ENA | TEAK_TMR1_CTRL_RESTART)) &&
		dsp_hw_read_reg(TEAK_TMR1_CNT, &first);
	if (!started)
		return result;

	uint32_t start = STM_TIM0;
	stopwatch_usleep_wd(TIMER_SAMPLE_US);
	bool read = dsp_hw_read_reg(TEAK_TMR1_CNT, &last);
	result.stm_ticks = STM_TIM0 - start;
	bool stopped = dsp_hw_write_reg(TEAK_TMR1_CTRL, 0);
	if (!read || !stopped)
		return result;

	result.timer_ticks = (last - first) & TEAK_TMR1_CNT_T1CNT;
	result.completed = true;
	return result;
}

static struct timer_measurement measure_timer2(void) {
	struct timer_measurement result = { 0 };
	uint16_t first;
	uint16_t last;
	bool started = dsp_hw_reset() &&
		dsp_hw_write_reg(TEAK_TMR2_CTRL, 0) &&
		dsp_hw_write_reg(TEAK_TMR2_MAX, UINT16_MAX) &&
		dsp_hw_write_reg(TEAK_TMR2_CNT, 0) &&
		dsp_hw_write_reg(TEAK_TMR2_CTRL, TEAK_TMR2_CTRL_DT2ACT) &&
		dsp_hw_read_reg(TEAK_TMR2_CNT, &first);
	if (!started)
		return result;

	uint32_t start = STM_TIM0;
	stopwatch_usleep_wd(TIMER_SAMPLE_US);
	bool read = dsp_hw_read_reg(TEAK_TMR2_CNT, &last);
	result.stm_ticks = STM_TIM0 - start;
	bool stopped = dsp_hw_write_reg(TEAK_TMR2_CTRL, 0);
	if (!read || !stopped)
		return result;

	result.timer_ticks = (uint16_t) (last - first);
	result.completed = true;
	return result;
}

static uint32_t timer_rate_hz(struct timer_measurement measurement) {
	if (measurement.stm_ticks == 0)
		return 0;
	return (uint64_t) measurement.timer_ticks * stopwatch_ticks_per_s() / measurement.stm_ticks;
}

int main(void) {
	test_start("CGU DSP timer clock test");

	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con0 = CGU_CON0;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_con2 = CGU_CON2;
	uint32_t pll_ndiv = (initial_osc & CGU_OSC_NDIV) >> CGU_OSC_NDIV_SHIFT;
	uint32_t pll_mdiv = (initial_osc & CGU_OSC_MDIV) >> CGU_OSC_MDIV_SHIFT;
	uint32_t pll_hz = CPU_OSC_FREQ * (pll_ndiv + 1) / (pll_mdiv + 1);
	bool safe_clock_tree = (initial_osc & CGU_OSC_PLL_POWER_UP) != 0 &&
		(CGU_STAT & CGU_STAT_LOCK) != 0 &&
		(initial_con1 & CGU_CON1_AHB_CLKSEL) != CGU_CON1_AHB_CLKSEL_PHASE1 &&
		(initial_con2 & CGU_CON2_EBU_CLKSEL) != CGU_CON2_EBU_CLKSEL_PHASE1;
	test_check("PLL and clock tree permit phase 1 measurement", safe_clock_tree);
	if (!safe_clock_tree)
		return test_finish();

	DSP_CLC = (1 << MOD_CLC_RMC_SHIFT);
	CGU_OSC = initial_osc | CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE1_BYPASS_N;
	CGU_CON2 = (initial_con2 & ~CGU_CON2_DSP_CLKSEL) | CGU_CON2_DSP_CLKSEL_PHASE1;
	uint32_t pll_phase_con0 = (initial_con0 & ~CGU_CON0_PHASE1_CONFIG) | (2 << CGU_CON0_PHASE1_K1_SHIFT);
	CGU_CON0 = pll_phase_con0;
	uint32_t modeled_phase_hz = cpu_get_phase_freq(1);
	struct timer_measurement timer1_pll_rate = measure_timer1();
	struct timer_measurement pll_rate = measure_timer2();

	CGU_CON0 = (initial_con0 & ~CGU_CON0_PHASE1_CONFIG) |
		(1 << CGU_CON0_PHASE1_K1_SHIFT) | (2 << CGU_CON0_PHASE1_K2_SHIFT);
	struct timer_measurement timer1_faster_rate = measure_timer1();
	struct timer_measurement faster_rate = measure_timer2();

	CGU_CON0 = pll_phase_con0;
	DSP_CLC = (4 << MOD_CLC_RMC_SHIFT);
	uint32_t rmc4_clc_before = DSP_CLC;
	struct timer_measurement timer1_rmc4_rate = measure_timer1();
	struct timer_measurement rmc4_rate = measure_timer2();
	uint32_t rmc4_clc_after = DSP_CLC;

	uint32_t pll_timer_hz = timer_rate_hz(pll_rate);
	uint32_t faster_timer_hz = timer_rate_hz(faster_rate);
	uint32_t rmc4_timer_hz = timer_rate_hz(rmc4_rate);
	uint32_t timer1_pll_hz = timer_rate_hz(timer1_pll_rate);
	uint32_t timer1_faster_hz = timer_rate_hz(timer1_faster_rate);
	uint32_t timer1_rmc4_hz = timer_rate_hz(timer1_rmc4_rate);
	uint32_t expected_timer1_hz = pll_hz / TIMER1_DIVIDER;
	uint32_t expected_timer1_faster_hz = (uint64_t) pll_hz * 3 / 2 / TIMER1_DIVIDER;
	uint32_t expected_pll_hz = pll_hz / TIMER2_DIVIDER;
	uint32_t expected_faster_hz = (uint64_t) pll_hz * 3 / 2 / TIMER2_DIVIDER;

	printf("# DSP Timer1 phase1 1x/1.5x: %lu/%lu timer ticks in %lu/%lu STM ticks (%lu/%lu Hz, expected %lu/%lu)\n",
		timer1_pll_rate.timer_ticks, timer1_faster_rate.timer_ticks,
		timer1_pll_rate.stm_ticks, timer1_faster_rate.stm_ticks,
		timer1_pll_hz, timer1_faster_hz, expected_timer1_hz, expected_timer1_faster_hz);
	printf("# DSP Timer1 at DSP_CLC.RMC=4: %lu timer ticks in %lu STM ticks (%lu Hz)\n",
		timer1_rmc4_rate.timer_ticks, timer1_rmc4_rate.stm_ticks, timer1_rmc4_hz);
	printf("# DSP Timer2 phase1 1x/1.5x: %lu/%lu timer ticks in %lu/%lu STM ticks (%lu/%lu Hz, expected %lu/%lu)\n",
		pll_rate.timer_ticks, faster_rate.timer_ticks, pll_rate.stm_ticks, faster_rate.stm_ticks,
		pll_timer_hz, faster_timer_hz, expected_pll_hz, expected_faster_hz);
	printf("# DSP Timer2 RMC=4: CLC=%08lX/%08lX, %lu timer ticks in %lu STM ticks (%lu Hz)\n",
		rmc4_clc_before, rmc4_clc_after, rmc4_rate.timer_ticks, rmc4_rate.stm_ticks, rmc4_timer_hz);
	test_check("DSP Timer1 runs at PLL/384", timer1_pll_rate.completed &&
		test_u32_in_interval(timer1_pll_hz, expected_timer1_hz * 98 / 100, expected_timer1_hz * 102 / 100));
	test_eq_u32("phase 1 calculation uses the PLL core", pll_hz, modeled_phase_hz);
	test_check("DSP Timer1 follows faster phase 1 /384", timer1_faster_rate.completed &&
		test_u32_in_interval(timer1_faster_hz, expected_timer1_faster_hz * 98 / 100,
			expected_timer1_faster_hz * 102 / 100));
	test_check("DSP module divider does not change Timer1 rate", timer1_rmc4_rate.completed &&
		test_u32_in_interval(timer1_rmc4_hz, expected_timer1_hz * 98 / 100,
			expected_timer1_hz * 102 / 100));
	test_check("DSP Timer2 runs at PLL/96", pll_rate.completed &&
		test_u32_in_interval(pll_timer_hz, expected_pll_hz * 98 / 100, expected_pll_hz * 102 / 100));
	test_check("DSP Timer2 follows faster phase 1 /96", faster_rate.completed &&
		test_u32_in_interval(faster_timer_hz, expected_faster_hz * 98 / 100,
			expected_faster_hz * 102 / 100));
	test_check("DSP module divider does not change Timer2 rate", rmc4_rate.completed &&
		(rmc4_clc_before & MOD_CLC_RMC) == (4 << MOD_CLC_RMC_SHIFT) &&
		(rmc4_clc_after & MOD_CLC_RMC) == (4 << MOD_CLC_RMC_SHIFT) &&
		test_u32_in_interval(rmc4_timer_hz, expected_pll_hz * 98 / 100,
			expected_pll_hz * 102 / 100));
	return test_finish();
}
