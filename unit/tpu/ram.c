#include <pmb887x.h>

#include "test.h"

#define TPU_TIMEOUT_MS 100
#define TPU_TIMER_RAM_BASE 512

static bool wait_counter_at_least(uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (TPU_COUNTER < value && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return TPU_COUNTER >= value;
}

static void test_ram_widths(void) {
	test_category("RAM partitioning and widths");
	TPU_PARAM = 0;
	TPU_RFCON1 = TPU_RFCON1_RAMTYPE_1;
	TPU_RAM(511) = 0x55AA33CC;
	TPU_RAM(TPU_TIMER_RAM_BASE) = 0xA5C35A3C;
	TPU_RAM(1023) = 0x55AAAA55;
	test_eq_u32("last RF RAM word is 11 bit", 0x03CC, TPU_RAM(511));
	test_eq_u32("first Timer RAM word is 16 bit", 0x5A3C, TPU_RAM(TPU_TIMER_RAM_BASE));
	test_eq_u32("last Timer RAM word is 16 bit", 0xAA55, TPU_RAM(1023));

	TPU_RFCON1 = TPU_RFCON1_RAMTYPE_2;
	TPU_RAM(511) = 0x12345678;
	TPU_RAM(TPU_TIMER_RAM_BASE) = 0x89ABCDEF;
	test_eq_u32("Type 2 keeps RF RAM width", 0x0678, TPU_RAM(511));
	test_eq_u32("Type 2 keeps Timer RAM width", 0xCDEF, TPU_RAM(TPU_TIMER_RAM_BASE));
}

static void test_event_pointers(void) {
	test_category("Timer event address pointers");
	TPU_PARAM = 0;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_GSMCLK1 = 1;
	TPU_GSMCLK2 = 32;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 0;
	TPU_TGER = BIT(0);
	TPU_EAPB = 0;
	TPU_EAPT = 6;
	TPU_RAM(TPU_TIMER_RAM_BASE + 0) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + 1) = 100;
	TPU_RAM(TPU_TIMER_RAM_BASE + 2) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + 3) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + 4) = 200;
	TPU_RAM(TPU_TIMER_RAM_BASE + 5) = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("event engine reaches first compare", wait_counter_at_least(50));
	test_eq_u32("CEAP points to current first event", 0, TPU_CEAP);
	test_check("event engine executes first compare", wait_counter_at_least(150));
	test_eq_u32("CEAP advances to second event", 3, TPU_CEAP);
	test_check("event engine executes second compare", wait_counter_at_least(250));
	test_eq_u32("CEAP skips expired event after wrapping", 3, TPU_CEAP);
	TPU_PARAM = 0;
}

int main(void) {
	test_start("TPU RAM test");
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_id("module ID", 0xF021C000, TPU_ID);
	test_module_clock("module clock is enabled", TPU_CLC);
	test_ram_widths();
	test_event_pointers();

	return test_finish();
}
