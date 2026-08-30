#include "wdt.h"

#include "cpu.h"

// The watchdog counts up at f_sys/16384 (WDTCON1.WDTIR would select /256) and resets the CPU
// when it wraps past 0xFFFF, so the timeout is (0x10000 - WDTREL) input clocks.
#define WDT_CLOCK_DIVIDER	16384
#define WDT_MAX_TICKS		0x10000
#define WDT_MIN_TICKS		4
// How much longer than the serve interval the hardware timeout is.
#define WDT_TIMEOUT_FACTOR	4

static stopwatch_t start_execution;
static uint32_t wdt_timeout;
static stopwatch_t last_wdt_serve;
static uint32_t wdt_interval;
static bool wdt_off;

static uint32_t cpu_wdt_reload_for_ms(uint32_t ms) {
	uint32_t clock = cpu_get_sys_freq() / WDT_CLOCK_DIVIDER;
	uint32_t ticks = clock != 0 && ms < 60000 ? clock * ms / 1000 : WDT_MAX_TICKS;

	if (ticks < WDT_MIN_TICKS)
		ticks = WDT_MIN_TICKS;
	if (ticks > WDT_MAX_TICKS)
		ticks = WDT_MAX_TICKS;

	return (WDT_MAX_TICKS - ticks) & 0xFFFF;
}

static uint32_t cpu_wdt_get_reload(void) {
	return (SCU_WDTCON0 & SCU_WDTCON0_WDTREL) >> SCU_WDTCON0_WDTREL_SHIFT;
}

// Password access: unlocks WDTCON0/WDTCON1 and puts the watchdog into time-out mode, where it
// counts from 0xFFFC - i.e. the whole unlock/modify sequence has to fit in 4 input clocks.
static void cpu_wdt_password_access(void) {
	SCU_WDTCON0 = (
		(SCU_WDTCON0 & (SCU_WDTCON0_WDTREL | SCU_WDTCON0_WDTPW | SCU_WDTCON0_ENDINIT)) |
		SCU_WDTCON0_WDTHPW1 |
		(SCU_WDTCON1 & (SCU_WDTCON1_WDTIR | SCU_WDTCON1_WDTDR))
	);
}

// Modify access: relocks WDTCON0. Setting ENDINIT applies WDTCON1 and reloads the counter.
static void cpu_wdt_modify_access(uint32_t reload, bool endinit) {
	SCU_WDTCON0 = (
		(reload << SCU_WDTCON0_WDTREL_SHIFT) |
		(SCU_WDTCON0 & SCU_WDTCON0_WDTPW) |
		SCU_WDTCON0_WDTHPW1 |
		SCU_WDTCON0_WDTLCK |
		(endinit ? SCU_WDTCON0_ENDINIT : 0)
	);
}

static void cpu_wdt_set_mode(uint32_t reload, uint32_t con1) {
	bool irq_was_disabled = cpu_enable_irq(false);

	cpu_wdt_password_access();
	cpu_wdt_modify_access(reload, false);
	SCU_WDTCON1 = con1;
	cpu_wdt_password_access();
	cpu_wdt_modify_access(reload, true);

	cpu_enable_irq(!irq_was_disabled);
}

static void cpu_wdt_reload(void) {
	bool irq_was_disabled = cpu_enable_irq(false);
	uint32_t reload = cpu_wdt_get_reload();

	cpu_wdt_password_access();
	cpu_wdt_modify_access(reload, true);

	cpu_enable_irq(!irq_was_disabled);
}

static void cpu_wdt_disable(void) {
	cpu_wdt_set_mode(cpu_wdt_get_reload(), SCU_WDTCON1_WDTDR);
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
	wdt_off = false;

#ifdef GPIO_PM_WADOG
#ifdef BOOT_EXTRAM
	extern uint32_t _last_wdt_serve_from_boot;
	last_wdt_serve = _last_wdt_serve_from_boot << 16;
	wdt_serve();
#else
	cpu_wdt_disable();
	GPIO_PIN(GPIO_PM_WADOG) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;
	last_wdt_serve = stopwatch_get();
#endif
#else
	last_wdt_serve = stopwatch_get();
	// WDTREL is 0xFFFC out of reset (~2.5 ms): enabling the watchdog without programming a
	// reload value first resets the phone long before the first wdt_serve().
	cpu_wdt_set_mode(cpu_wdt_reload_for_ms(wdt_interval * WDT_TIMEOUT_FACTOR), 0);
#endif
}

void wdt_disable(void) {
#ifndef GPIO_PM_WADOG
	wdt_off = true;
	cpu_wdt_disable();
#else
	// todo: implement it for PMIC watchdog?
#endif
}

void wdt_set_max_execution_time(uint32_t ms) {
	start_execution = stopwatch_get();
	wdt_timeout = ms;
}

void wdt_serve(void) {
	if (wdt_off)
		return;
	if (stopwatch_elapsed_ms(last_wdt_serve) < wdt_interval)
		return;
	if (wdt_timeout != 0 && stopwatch_elapsed_ms(start_execution) >= wdt_timeout)
		return;

#ifdef GPIO_PM_WADOG
	gpio_toggle(GPIO_PM_WADOG);
#else
	cpu_wdt_reload();
#endif
	last_wdt_serve = stopwatch_get();
}
