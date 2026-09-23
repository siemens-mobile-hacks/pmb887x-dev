#include <pmb887x.h>

#include "test.h"

#define MEASURE_NOPS 512
#define MEASURE_ITERATIONS 1000

static __TCM void run_cpu_loop(uint32_t selected_con2, uint32_t restore_con2) {
	CGU_CON2 = selected_con2;
	for (uint32_t iteration = 0; iteration < MEASURE_ITERATIONS; iteration++)
		__asm__ volatile(".rept 512\n\tnop\n\t.endr");
	CGU_CON2 = restore_con2;
}

static uint32_t measure_cpu_khz(uint32_t selected_con2, uint32_t restore_con2) {
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	uint32_t start = STM_TIM0;
	run_cpu_loop(selected_con2, restore_con2);
	uint32_t elapsed = STM_TIM0 - start;
	cpu_enable_irq(!irq_was_disabled);
	if (elapsed == 0)
		return 0;

	return (uint64_t) MEASURE_NOPS * MEASURE_ITERATIONS * (CPU_OSC_FREQ / 1000) / elapsed;
}

int main(void) {
	test_start("CGU ARM clock divider test");
	if (test_is_qemu()) {
		test_skip("ARM instruction timing", "requires real hardware");
		return test_finish();
	}

	uint32_t initial_con2 = CGU_CON2;
	uint32_t base_con2 = initial_con2 & ~(CGU_CON2_CPU_DIV | CGU_CON2_CPU_DIV_EN);
	uint32_t baseline_khz = measure_cpu_khz(base_con2, initial_con2);
	CGU_CON2 = base_con2;
	uint32_t baseline_model_khz = cpu_get_freq() / 1000;
	CGU_CON2 = initial_con2;
	uint32_t expected_khz = CPU_OSC_FREQ / 1000;
	printf("# divider bypass: measured=%lu kHz model=%lu kHz\n", baseline_khz, baseline_model_khz);
	test_check("disabled divider keeps the 26 MHz ARM clock",
		baseline_model_khz == expected_khz &&
		test_u32_in_interval(baseline_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));

	for (uint32_t divider = 0; divider < 4; divider++) {
		uint32_t selected_con2 = base_con2 | CGU_CON2_CPU_DIV_EN | (divider << CGU_CON2_CPU_DIV_SHIFT);
		uint32_t measured_khz = measure_cpu_khz(selected_con2, initial_con2);
		CGU_CON2 = selected_con2;
		uint32_t modeled_khz = cpu_get_freq() / 1000;
		CGU_CON2 = initial_con2;
		expected_khz = CPU_OSC_FREQ / (divider + 1) / 1000;
		printf("# DIV=%lu: expected=%lu kHz measured=%lu kHz model=%lu kHz\n",
			divider, expected_khz, measured_khz, modeled_khz);
		test_check("CPU divider selects the expected ARM rate",
			modeled_khz == expected_khz &&
			test_u32_in_interval(measured_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));
	}

	return test_finish();
}
