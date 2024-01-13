#pragma once

#include <pmb887x.h>

void wdt_init(void);
void wdt_init_custom(uint32_t interval);
void wdt_serve(void);
void wdt_set_max_execution_time(uint32_t ms);
