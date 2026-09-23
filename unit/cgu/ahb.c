#include <pmb887x.h>

#include "clock.h"
#include "test.h"

/* Matches the NOP block in cgu_itcm_source_loop() in measure.S. */
#define MEASURE_NOPS 512
#define MEASURE_ITERATIONS 1000

struct phase_case {
	const char *name;
	uint32_t phase;
	uint32_t k1;
	uint32_t k2;
	uint32_t source;
	uint32_t expected_khz;
};

static const struct phase_case PHASE_CASES[] = {
	{ "phase 1 K1=2 K2=1", 1, 2, 1, CGU_CON1_AHB_CLKSEL_PHASE1, 96000 },
	{ "phase 1 K1=4 K2=2", 1, 4, 2, CGU_CON1_AHB_CLKSEL_PHASE1, 48000 },
	{ "phase 1 K1=4 K2=3", 1, 4, 3, CGU_CON1_AHB_CLKSEL_PHASE1, 46222 },
	{ "phase 2 K1=4 K2=2", 2, 4, 2, CGU_CON1_AHB_CLKSEL_PHASE2, 48000 },
	{ "phase 3 K1=4 K2=3", 3, 4, 3, CGU_CON1_AHB_CLKSEL_PHASE3, 46222 },
	{ "phase 4 K1=5 K2=0", 4, 5, 0, CGU_CON1_AHB_CLKSEL_PHASE4, 41600 },
	{ "phase 1 K1=2 K2=0 bypasses the divider", 1, 2, 0, CGU_CON1_AHB_CLKSEL_PHASE1, 104000 },
};

static const struct {
	const char *name;
	uint32_t ndiv;
	uint32_t mdiv;
} PLL_RATIO_CASES[] = {
	{ "PLL N=7 M=1 preserves 104 MHz", 7, 1 },
	{ "PLL N=11 M=2 preserves 104 MHz", 11, 2 },
	{ "PLL N=15 M=3 preserves 104 MHz", 15, 3 },
};

void cgu_itcm_source_loop(void);
void cgu_execute_itcm_source(void (*loop)(void), uint32_t iterations, uint32_t selected_con1, uint32_t restore_con1);

static uint32_t measure_ahb_khz(uint32_t selected_con1, uint32_t restore_con1) {
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	uint32_t start_ticks = STM_TIM0;
	cgu_execute_itcm_source(cgu_itcm_source_loop, MEASURE_ITERATIONS, selected_con1, restore_con1);
	uint32_t elapsed_ticks = STM_TIM0 - start_ticks;
	cpu_enable_irq(!irq_was_disabled);

	if (elapsed_ticks == 0)
		return 0;
	return (uint64_t) MEASURE_NOPS * MEASURE_ITERATIONS * (CPU_OSC_FREQ / 1000) / elapsed_ticks;
}

static void check_frequency(const char *name, uint32_t expected_khz, uint32_t measured_khz, uint32_t modeled_hz) {
	printf("# %s: expected=%lu kHz measured=%lu kHz model=%lu kHz\n",
		name, expected_khz, measured_khz, modeled_hz / 1000);
	test_check(name, modeled_hz / 1000 == expected_khz &&
		test_u32_in_interval(measured_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));
}

int main(void) {
	test_start("CGU AHB source clock test");
	if (test_is_qemu()) {
		test_skip("AHB instruction timing", "requires real hardware");
		return test_finish();
	}

	uint32_t base_con1 = CGU_CON1 & ~CGU_CON1_AHB_CLKSEL;
	check_frequency("AHB bypass selects the 26 MHz oscillator", 26000,
		measure_ahb_khz(base_con1, base_con1), cpu_get_ahb_freq());

	cgu_pll_set(3, 0);
	cgu_phases_enable(CGU_PHASE_ALL);

	check_frequency("AHB PLL source", 104000,
		measure_ahb_khz(base_con1 | CGU_CON1_AHB_CLKSEL_PLL, base_con1), cpu_get_pll_freq());

	for (uint32_t index = 0; index < ARRAY_SIZE(PHASE_CASES); index++) {
		const struct phase_case *phase = &PHASE_CASES[index];
		cgu_phase_set_divider(phase->phase, phase->k1, phase->k2);
		check_frequency(phase->name, phase->expected_khz,
			measure_ahb_khz(base_con1 | phase->source, base_con1), cpu_get_phase_freq(phase->phase));
	}

	for (uint32_t index = 0; index < ARRAY_SIZE(PLL_RATIO_CASES); index++) {
		cgu_pll_set(PLL_RATIO_CASES[index].ndiv, PLL_RATIO_CASES[index].mdiv);
		uint32_t measured_khz = measure_ahb_khz(base_con1 | CGU_CON1_AHB_CLKSEL_PLL, base_con1);
		check_frequency(PLL_RATIO_CASES[index].name, 104000, measured_khz, cpu_get_pll_freq());
	}

	return test_finish();
}
