#include <pmb887x.h>

#include "test.h"

#define POLL_ROUNDS 20

static __TCM uint32_t stm_poll(uint32_t ticks, uint32_t *backward_steps) {
	uint32_t start = STM_TIM0;
	uint32_t previous = start;
	uint32_t reads = 0;
	uint32_t backward = 0;

	while (previous - start < ticks) {
		uint32_t now = STM_TIM0;
		if ((int32_t) (now - previous) < 0)
			backward++;
		previous = now;
		reads++;
	}

	*backward_steps = backward;
	return reads;
}

int main(void) {
	test_start("STM polling from TCM");

	uint32_t ticks_per_second = test_stm_ticks_per_ms() * 1000;
	for (uint32_t round = 0; round < POLL_ROUNDS; round++) {
		uint32_t backward_steps;
		uint32_t reads = stm_poll(ticks_per_second, &backward_steps);

		printf("# round %lu: %lu TIM0 reads in one second\n", round, reads);
		test_eq_u32("TIM0 never steps back while polled", 0, backward_steps);
	}

	return test_finish();
}
