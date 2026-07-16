#include "wdt.h"

static stopwatch_t start_execution;
static uint32_t wdt_timeout;
static stopwatch_t last_wdt_serve;
static uint32_t wdt_interval;

static void cpu_wdt_update(bool endinit) {
	uint32_t config = SCU_WDTCON0 & (SCU_WDTCON0_WDTREL | SCU_WDTCON0_WDTPW);

	SCU_WDTCON0 = (
		config |
		(SCU_WDTCON0 & SCU_WDTCON0_ENDINIT) |
		SCU_WDTCON0_WDTHPW1 |
		(SCU_WDTCON1 & (SCU_WDTCON1_WDTIR | SCU_WDTCON1_WDTDR))
	);
	SCU_WDTCON0 = (
		config |
		SCU_WDTCON0_WDTHPW1 |
		SCU_WDTCON0_WDTLCK |
		(endinit ? SCU_WDTCON0_ENDINIT : 0)
	);
}

static void cpu_wdt_set_mode(uint32_t con1) {
	cpu_wdt_update(false);
	SCU_WDTCON1 = con1;
	cpu_wdt_update(true);
}

void wdt_init(void) {
	wdt_init_custom(550);
}

void wdt_set_interval(uint32_t interval) {
	wdt_interval = interval != 0 ? interval : 1;
}

void wdt_init_custom(uint32_t interval) {
	stopwatch_init();
	wdt_set_interval(interval);

#ifdef GPIO_PM_WADOG
#ifdef BOOT_EXTRAM
	extern uint32_t _last_wdt_serve_from_boot;
	last_wdt_serve = _last_wdt_serve_from_boot << 16;
	wdt_serve();
#else
	cpu_wdt_set_mode(SCU_WDTCON1_WDTDR);
	GPIO_PIN(GPIO_PM_WADOG) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;
	last_wdt_serve = stopwatch_get();
#endif
#else
	last_wdt_serve = stopwatch_get();
	cpu_wdt_set_mode(0);
#endif
}

void wdt_set_max_execution_time(uint32_t ms) {
	start_execution = stopwatch_get();
	wdt_timeout = ms;
}

void wdt_serve(void) {
	if (stopwatch_elapsed_ms(last_wdt_serve) < wdt_interval)
		return;
	if (wdt_timeout != 0 && stopwatch_elapsed_ms(start_execution) >= wdt_timeout)
		return;

#ifdef GPIO_PM_WADOG
	gpio_toggle(GPIO_PM_WADOG);
#else
	cpu_wdt_update(true);
#endif
	last_wdt_serve = stopwatch_get();
}
