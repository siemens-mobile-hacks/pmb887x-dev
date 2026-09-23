#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#define EBU_FLASH_BASE 0xA0000000
#define EBU_FLASH_PROBE_WORDS 32768
#define EBU_CLOCK_WAIT_ITERATIONS 100000

enum ebu_source_index {
	EBU_SOURCE_PLL,
	EBU_SOURCE_PHASE1,
	EBU_SOURCE_PHASE2,
	EBU_SOURCE_PHASE3,
	EBU_SOURCE_PHASE4,
	EBU_SOURCE_OSC,
	EBU_SOURCE_COUNT,
};

static __SRAM bool wait_for_ebu_clock_transition(void) {
	for (uint32_t index = 0; index < EBU_CLOCK_WAIT_ITERATIONS; index++)
		if ((SCU_EBUCLC2 & SCU_EBUCLC2_READY) != 0)
			return true;

	return false;
}

static __SRAM uint32_t measure_flash_read_ticks(void) {
	const volatile uint32_t *flash = (const volatile uint32_t *) EBU_FLASH_BASE;
	uint32_t start = STM_TIM0;

	for (uint32_t index = 0; index < EBU_FLASH_PROBE_WORDS; index++)
		(void) flash[index];

	return STM_TIM0 - start;
}

static __SRAM void test_ebu_source_read_latency(void) {
	uint32_t ebu_sources[EBU_SOURCE_COUNT] = {
		CGU_CON2_EBU_CLKSEL_PLL,
		CGU_CON2_EBU_CLKSEL_PHASE1,
		CGU_CON2_EBU_CLKSEL_PHASE2,
		CGU_CON2_EBU_CLKSEL_PHASE3,
		CGU_CON2_EBU_CLKSEL_PHASE4,
		CGU_CON2_EBU_CLKSEL_OSC,
	};
	uint32_t ticks[EBU_SOURCE_COUNT] = { 0 };
	uint32_t con2_base = CGU_CON2 & ~CGU_CON2_EBU_CLKSEL;
	bool irq_was_disabled = cpu_enable_irq(false);
	cgu_pll_set(7, 1);
	cgu_phases_enable(CGU_PHASE_ALL);
	cgu_phase_set_divider(1, 2, 1);
	cgu_phase_set_divider(2, 4, 2);
	cgu_phase_set_divider(3, 4, 3);
	cgu_phase_set_divider(4, 5, 0);

	CGU_CON2 = con2_base | CGU_CON2_EBU_CLKSEL_PLL;
	SCU_EBUCLC2 |= (1 << SCU_EBUCLC2_FLAG1_SHIFT);
	bool transition_ready = wait_for_ebu_clock_transition();

	bool source_order = true;

	if (transition_ready) {
		for (uint32_t index = 0; index < ARRAY_SIZE(ebu_sources); index++) {
			CGU_CON2 = con2_base | ebu_sources[index];
			ticks[index] = measure_flash_read_ticks();
			if (index != 0)
				source_order = source_order && ticks[index - 1] < ticks[index];
		}
	}

	CGU_CON2 = con2_base | CGU_CON2_EBU_CLKSEL_OSC;
	cpu_enable_irq(!irq_was_disabled);

	test_check("asynchronous EBU clock transition becomes ready", transition_ready);
	if (transition_ready) {
		printf("# EBU flash reads: OSC=%lu PLL=%lu PHASE1=%lu PHASE2=%lu PHASE3=%lu PHASE4=%lu ticks\n",
			ticks[EBU_SOURCE_OSC], ticks[EBU_SOURCE_PLL], ticks[EBU_SOURCE_PHASE1],
			ticks[EBU_SOURCE_PHASE2], ticks[EBU_SOURCE_PHASE3], ticks[EBU_SOURCE_PHASE4]);
		test_check("EBU access timing follows the selected source", source_order);
	}
}

int main(void) {
	test_start("EBU flash-read latency test");
	if (test_is_qemu()) {
		test_skip("EBU flash-read latency", "hardware-only measurement");
		return test_finish();
	}

	test_ebu_source_read_latency();
	return test_finish();
}
