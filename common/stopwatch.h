#pragma once

#include <stdint.h>
#include <pmb8876.h>
#include <pmb8876_io.h>

#define stopwatch_msleep(s)		stopwatch_usleep((s) * 1000)
#define stopwatch_msleep_wd(s)	stopwatch_usleep_wd((s) * 1000)

#define stopwatch_sleep(s)		stopwatch_usleep((s) * 1000000)
#define stopwatch_sleep_wd(s)	stopwatch_usleep_wd((s) * 1000000)

typedef unsigned long long int stopwatch_t;

void stopwatch_init();

void stopwatch_usleep(uint32_t us);
void stopwatch_usleep_wd(uint32_t us);

stopwatch_t stopwatch_start();
stopwatch_t stopwatch_elapsed(stopwatch_t start);

uint32_t stopwatch_elapsed_us(stopwatch_t start);
uint32_t stopwatch_elapsed_ms(stopwatch_t start);
uint32_t stopwatch_elapsed_s(stopwatch_t start);

uint32_t stopwatch_ticks_per_us();
uint32_t stopwatch_ticks_per_ms();
uint32_t stopwatch_ticks_per_s();
