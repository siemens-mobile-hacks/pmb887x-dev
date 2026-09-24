#include <stopwatch.h>

static uint32_t ticks_per_s;

void stopwatch_init(void) {
	stopwatch_update();
}

void stopwatch_update(void) {
	ticks_per_s = cpu_get_stm_freq();
}

void stopwatch_usleep(uint32_t us) {
	stopwatch_t end = stopwatch_get() + (stopwatch_t) us * ticks_per_s / 1000000;

	while (stopwatch_get() <= end);
}

void stopwatch_usleep_wd(uint32_t us) {
	stopwatch_t end = stopwatch_get() + (stopwatch_t) us * ticks_per_s / 1000000;

	while (stopwatch_get() <= end)
		wdt_serve();
}

stopwatch_t stopwatch_get(void) {
	return ((stopwatch_t) STM_TIM6 << 32) | (stopwatch_t) STM_TIM0;
}

stopwatch_t stopwatch_elapsed(stopwatch_t start) {
	return stopwatch_get() - start;
}

uint32_t stopwatch_elapsed_us(stopwatch_t start) {
	return (uint32_t) (stopwatch_elapsed(start) * 1000000ULL / ticks_per_s);
}

uint32_t stopwatch_elapsed_ms(stopwatch_t start) {
	return (uint32_t) (stopwatch_elapsed(start) * 1000ULL / ticks_per_s);
}

uint32_t stopwatch_elapsed_s(stopwatch_t start) {
	return (uint32_t) (stopwatch_elapsed(start) / ticks_per_s);
}

uint32_t stopwatch_ticks_per_us(void) {
	return ticks_per_s / 1000000;
}

uint32_t stopwatch_ticks_per_ms(void) {
	return ticks_per_s / 1000;
}

uint32_t stopwatch_ticks_per_s(void) {
	return ticks_per_s;
}
