#pragma once

#include <pmb887x.h>

typedef unsigned long long int stopwatch_t;

void stopwatch_init(void);

void stopwatch_usleep(uint32_t us);
void stopwatch_usleep_wd(uint32_t us);

stopwatch_t stopwatch_get(void);
stopwatch_t stopwatch_elapsed(stopwatch_t start);

uint32_t stopwatch_elapsed_us(stopwatch_t start);
uint32_t stopwatch_elapsed_ms(stopwatch_t start);
uint32_t stopwatch_elapsed_s(stopwatch_t start);

uint32_t stopwatch_ticks_per_us(void);
uint32_t stopwatch_ticks_per_ms(void);
uint32_t stopwatch_ticks_per_s(void);

inline void stopwatch_msleep(uint32_t ms) {
	stopwatch_usleep(ms * 1000);
}

inline void stopwatch_msleep_wd(uint32_t ms) {
	stopwatch_usleep_wd(ms * 1000);
}

inline void stopwatch_sleep(uint32_t s) {
	stopwatch_usleep(s * 1000000);
}

inline void stopwatch_sleep_wd(uint32_t s) {
	stopwatch_usleep_wd(s * 1000000);
}
