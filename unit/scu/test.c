#include <pmb887x.h>

#include "test.h"
#include "watchdog.h"

#define WDT_ERROR_STATUS (SCU_WDT_SR_WDTAE | SCU_WDT_SR_WDTOE | SCU_WDT_SR_WDTPR)
#define WDT_CONFIG (SCU_WDTCON0_WDTREL | SCU_WDTCON0_WDTPW)
#define WDT_FREQUENCY_WINDOW_US 50000
#define WDT_FREQUENCY_TOLERANCE_PERCENT 10

typedef struct {
	uint32_t frequency;
	uint32_t status;
	bool counter_advanced;
	bool unlocked;
	bool modified;
} watchdog_measurement_t;

static bool frequency_matches(uint32_t actual, uint32_t expected) {
	uint32_t tolerance = expected * WDT_FREQUENCY_TOLERANCE_PERCENT / 100;

	return actual >= expected - tolerance && actual <= expected + tolerance;
}

static watchdog_measurement_t measure_watchdog_frequency(uint32_t con1) {
	struct scu_watchdog_access_result enable = scu_watchdog_configure(con1, 0, true);
	uint32_t status = SCU_WDT_SR;
	uint32_t before = (status & SCU_WDT_SR_WDTTIM) >> SCU_WDT_SR_WDTTIM_SHIFT;
	stopwatch_t start = stopwatch_get();
	stopwatch_usleep_wd(WDT_FREQUENCY_WINDOW_US);
	uint32_t elapsed_us = stopwatch_elapsed_us(start);
	uint32_t after = (SCU_WDT_SR & SCU_WDT_SR_WDTTIM) >> SCU_WDT_SR_WDTTIM_SHIFT;
	struct scu_watchdog_access_result disable = scu_watchdog_configure(con1 | SCU_WDTCON1_WDTDR, 0, true);
	uint32_t ticks = (after - before) & 0xFFFF;

	return (watchdog_measurement_t) {
		.frequency = (uint32_t) ((uint64_t) ticks * 1000000 / elapsed_us),
		.status = status,
		.counter_advanced = after != before,
		.unlocked = enable.unlocked && disable.unlocked,
		.modified = enable.modified && disable.modified,
	};
}

static void test_cpu_identification(void) {
#ifdef PMB8875
	const uint32_t EXPECTED_CHIP = 0x1A;
#elif defined(PMB8876)
	const uint32_t EXPECTED_CHIP = 0x1B;
#else
#error Unsupported CPU
#endif
	uint32_t manufacturer = (SCU_MANID & SCU_MANID_MANUF) >> SCU_MANID_MANUF_SHIFT;
	uint32_t department = (SCU_MANID & SCU_MANID_DEPT) >> SCU_MANID_DEPT_SHIFT;
	uint32_t chip = (SCU_CHIPID & SCU_CHIPID_CHIPD) >> SCU_CHIPID_CHIPD_SHIFT;
	uint32_t revision = (SCU_CHIPID & SCU_CHIPID_CHREV) >> SCU_CHIPID_CHREV_SHIFT;
	uint32_t uid0 = SCU_UID0;
	uint32_t uid1 = SCU_UID1;
	uint32_t uid2 = SCU_UID2;

	test_module_id("module ID", 0xF040C000, SCU_ID);
	test_module_clock("module clock is enabled", SCU_CLC);
	test_eq_u32("manufacturer is Infineon", 0x182, manufacturer);
	test_eq_u32("manufacturer department", 3, department);
	test_eq_u32("CPU model", EXPECTED_CHIP, chip);
	test_eq_u32("CHIPID reserved bits are zero", 0, SCU_CHIPID & GENMASK(31, 16));
	test_check("CPU UID is programmed", (uid0 | uid1 | uid2) != 0);
	test_check("CPU UID is not erased", (uid0 & uid1 & uid2) != UINT32_MAX);
	printf(
		"# CPU: CHIPID=%02X rev=%02X, MANID=%04X, UID=%08X-%08X-%08X\n",
		(unsigned int) chip,
		(unsigned int) revision,
		(unsigned int) SCU_MANID,
		(unsigned int) uid0,
		(unsigned int) uid1,
		(unsigned int) uid2
	);
}

static void test_watchdog(void) {
	uint32_t initial_con0 = SCU_WDTCON0;
	uint32_t initial_con1 = SCU_WDTCON1;
	uint32_t initial_status = SCU_WDT_SR;

	test_check("watchdog control is locked", (initial_con0 & SCU_WDTCON0_WDTLCK) != 0);
	test_check("ENDINIT starts set", (initial_con0 & SCU_WDTCON0_ENDINIT) != 0);
	test_eq_u32("watchdog starts without errors", 0, initial_status & WDT_ERROR_STATUS);

	watchdog_measurement_t slow = measure_watchdog_frequency(0);
	watchdog_measurement_t fast = measure_watchdog_frequency(SCU_WDTCON1_WDTIR);
	uint32_t reload = (initial_con0 & SCU_WDTCON0_WDTREL) >> SCU_WDTCON0_WDTREL_SHIFT;
	struct scu_watchdog_access_result restore = scu_watchdog_configure(initial_con1, reload, true);
	uint32_t restored_con0 = SCU_WDTCON0;
	uint32_t restored_con1 = SCU_WDTCON1;
	uint32_t restored_status = SCU_WDT_SR;
	uint32_t system_frequency = cpu_get_sys_freq();
	uint32_t expected_slow = system_frequency / 16384;
	uint32_t expected_fast = system_frequency / 256;
	printf(
		"# Watchdog: WDTIR=0 %u Hz, WDTIR=1 %u Hz, fSYS=%u Hz\n",
		(unsigned int) slow.frequency,
		(unsigned int) fast.frequency,
		(unsigned int) system_frequency
	);

	test_check("watchdog password access unlocks WDTCON0", (
		slow.unlocked && fast.unlocked && restore.unlocked
	));
	test_check("watchdog modify access updates ENDINIT", (
		slow.modified && fast.modified && restore.modified
	));
	test_eq_u32("watchdog enters normal mode", 0, slow.status & SCU_WDT_SR_WDTDS);
	test_check("watchdog counter advances", slow.counter_advanced && fast.counter_advanced);
	test_eq_u32("WDTIR selects slow clock", 0, slow.status & SCU_WDT_SR_WDTIS);
	test_eq_u32("WDTIR selects fast clock", SCU_WDT_SR_WDTIS, fast.status & SCU_WDT_SR_WDTIS);
	test_check("slow watchdog frequency is fSYS / 16384", frequency_matches(slow.frequency, expected_slow));
	test_check("fast watchdog frequency is fSYS / 256", frequency_matches(fast.frequency, expected_fast));
	test_eq_u32(
		"watchdog mode is restored",
		initial_status & SCU_WDT_SR_WDTDS,
		restored_status & SCU_WDT_SR_WDTDS
	);
	test_eq_u32("watchdog configuration is restored", initial_con0 & WDT_CONFIG, restored_con0 & WDT_CONFIG);
	test_eq_u32(
		"watchdog mode requests are restored",
		initial_con1 & (SCU_WDTCON1_WDTIR | SCU_WDTCON1_WDTDR),
		restored_con1 & (SCU_WDTCON1_WDTIR | SCU_WDTCON1_WDTDR)
	);
	test_check("watchdog finishes locked with ENDINIT", (
		(restored_con0 & (SCU_WDTCON0_WDTLCK | SCU_WDTCON0_ENDINIT)) ==
		(SCU_WDTCON0_WDTLCK | SCU_WDTCON0_ENDINIT)
	));
	test_eq_u32("watchdog access sequence has no errors", 0, restored_status & WDT_ERROR_STATUS);
}

int main(void) {
	test_start("SCU peripheral test");

	test_category("CPU identification");
	test_cpu_identification();
	test_category("Watchdog");
	test_watchdog();

	return test_finish();
}
