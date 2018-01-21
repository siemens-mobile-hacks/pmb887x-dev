#include <stopwatch.h>
#include <printf.h>

static uint32_t ticks_per_s;
static uint32_t ticks_per_ms;
static uint32_t ticks_per_us;

void stopwatch_init() {
	ticks_per_s = PMB8876_SYSTEM_FREQ / STM->CLC.b.RMC;
	ticks_per_ms = ticks_per_s / 1000;
	ticks_per_us = ticks_per_s / 1000000;
	
	if (sizeof(stopwatch_t) != 8) {
		printf("stopwatch_init failed, long long int has size %d\r\n", sizeof(long long int));
		while (1);
	}
}

void stopwatch_usleep(uint32_t us) {
	stopwatch_t end = stopwatch_start() + us * ticks_per_us;
	while (stopwatch_start() <= end);
}

void stopwatch_usleep_wd(uint32_t us) {
	stopwatch_t end = stopwatch_start() + us * ticks_per_us;
	while (stopwatch_start() <= end)
		serve_watchdog();
}

stopwatch_t stopwatch_start() {
	return ((stopwatch_t) STM->TIM[6].v << 32) | (stopwatch_t) STM->TIM[0].v;
}

stopwatch_t stopwatch_elapsed(stopwatch_t start) {
	return stopwatch_start(start) - start;
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
