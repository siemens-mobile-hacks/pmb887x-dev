#include <stopwatch.h>
#include <printf.h>

static uint32_t ticks_per_s;
static uint32_t ticks_per_ms;
static uint32_t ticks_per_us;

void stopwatch_init() {
	uint32_t clock = (STM_CLC & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT;
	
	ticks_per_s = PMB8876_SYSTEM_FREQ / clock;
	ticks_per_ms = ticks_per_s / 1000;
	ticks_per_us = ticks_per_s / 1000000;
}

void stopwatch_usleep(uint32_t us) {
	stopwatch_t end = stopwatch_get() + us * ticks_per_us;
	while (stopwatch_get() <= end);
}

void stopwatch_usleep_wd(uint32_t us) {
	stopwatch_t end = stopwatch_get() + us * ticks_per_us;
	while (stopwatch_get() <= end)
		wdt_serve();
}

stopwatch_t stopwatch_get() {
	return ((stopwatch_t) STM_TIM6 << 32) | (stopwatch_t) STM_TIM0;
}

stopwatch_t stopwatch_elapsed(stopwatch_t start) {
	return stopwatch_get() - start;
}

uint32_t stopwatch_elapsed_us(stopwatch_t start) {
	return stopwatch_elapsed(start) / ticks_per_us;
}

uint32_t stopwatch_elapsed_ms(stopwatch_t start) {
	return stopwatch_elapsed(start) / ticks_per_ms;
}

uint32_t stopwatch_elapsed_s(stopwatch_t start) {
	return stopwatch_elapsed(start) / ticks_per_s;
}

uint32_t stopwatch_ticks_per_us() {
	return ticks_per_us;
}

uint32_t stopwatch_ticks_per_ms() {
	return ticks_per_ms;
}

uint32_t stopwatch_ticks_per_s() {
	return ticks_per_s;
}
