#include "wdt.h"

static stopwatch_t last_wdt_serve = 0;
static uint32_t wdt_interval = 0;

static void _set_einit(bool flag) {
	uint32_t tmp = (((SCU_WDTCON0 & ~0x0E) | 0xf0));
	tmp |= (SCU_WDTCON1 & 0x0c);
	SCU_WDTCON0 = tmp;
	SCU_WDTCON0 = (tmp & ~0x0d) | 2 | (flag ? 1 : 0);
}

void wdt_init() {
	wdt_init_custom(550);
}

void wdt_init_custom(uint32_t interval) {
	wdt_interval = interval;
	
	stopwatch_init();
	
	// Init external watchdog gpio (dialog)
	#ifdef BOOT_EXTRAM
		extern uint32_t _last_wdt_serve_from_boot;
		last_wdt_serve = _last_wdt_serve_from_boot << 16;
		wdt_serve();
	#else
		// Disable internal watchdog (CPU)
		_set_einit(0);
		SCU_WDTCON1 = 0x8;
		_set_einit(1);
		
		GPIO_PIN(GPIO_PM_WADOG) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;
		last_wdt_serve = stopwatch_get();
	#endif
}

void wdt_serve(void) {
	if (stopwatch_elapsed_ms(last_wdt_serve) < wdt_interval)
		return;
	gpio_toggle(GPIO_PM_WADOG);
	last_wdt_serve = stopwatch_get();
}
