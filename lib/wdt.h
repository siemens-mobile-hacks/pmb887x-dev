#pragma once

#include <pmb887x.h>

void wdt_init(void);
void wdt_init_custom(uint32_t interval);
void wdt_serve(void);

#ifdef BOOT_EXTRAM
void save_last_wdt_from_boot(void);
#endif
