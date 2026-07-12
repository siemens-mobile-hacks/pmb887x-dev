#include <pmb887x.h>

#include "test.h"

#define STM_COUNTER_MASK ULL(0x00FFFFFFFFFFFFFF)

/* Reading TIM0 latches the upper 24 bits in CAP. */
static uint64_t stm_read_counter(void) {
	uint64_t counter = STM_TIM0;
	counter |= (uint64_t) (STM_CAP & 0x00FFFFFF) << 32;

	return counter & STM_COUNTER_MASK;
}

static bool test_window(volatile uint32_t *reg, unsigned int shift) {
	uint64_t before = stm_read_counter();
	uint32_t value = *reg;
	uint64_t after = stm_read_counter();

	return test_u32_in_interval(value, (uint32_t) (before >> shift), (uint32_t) (after >> shift));
}

static bool test_tim6_window(void) {
	uint64_t before = stm_read_counter();
	uint32_t value = STM_TIM6;
	uint64_t after = stm_read_counter();

	return (
		(value & 0xFF000000) == 0 &&
		test_u32_in_interval(value, (uint32_t) (before >> 32) & 0x00FFFFFF, (uint32_t) (after >> 32) & 0x00FFFFFF)
	);
}

int main(void) {
	test_start("STM peripheral test");

	test_module_id("module ID", 0x0000C000, STM_ID);
	test_module_clock("module clock is enabled", STM_CLC);

	uint32_t start = STM_TIM0;
	test_spin(1024);
	test_check("counter advances", STM_TIM0 != start);

	uint32_t tim6 = STM_TIM6;
	test_eq_u32("CAP keeps the TIM6 capture", tim6, STM_CAP);
	test_check("TIM6 is 24 bit", (tim6 & 0xFF000000) == 0);

	test_check("TIM1 is counter bits 4..35", test_window(&STM_TIM1, 4));
	test_check("TIM2 is counter bits 8..39", test_window(&STM_TIM2, 8));
	test_check("TIM3 is counter bits 12..43", test_window(&STM_TIM3, 12));
	test_check("TIM4 is counter bits 16..47", test_window(&STM_TIM4, 16));
	test_check("TIM5 is counter bits 20..51", test_window(&STM_TIM5, 20));
	test_check("TIM6 is counter bits 32..55", test_tim6_window());

	return test_finish();
}
