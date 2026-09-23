#include <pmb887x.h>

#include "test.h"

#define CAPCOM_RMC 128
#define CAPCOM_START 0xFF00
#define CAPCOM_T0_RELOAD 0x8000
#define CAPCOM_T1_RELOAD 0x9000
#define CAPCOM_TIMEOUT_TICKS (CPU_OSC_FREQ / 10)
/* With the boot CGU profile, CAPCOM runs at fOSC / 8 before its CLC divider. */
#define CAPCOM_EXPECTED_TICKS ((0x10000 - CAPCOM_START) * 8 * CAPCOM_RMC)

struct timer_result {
	uint32_t elapsed_ticks;
	uint32_t value;
};

static void test_timers(uint32_t capcom, const char *name) {
	uint32_t initial_clc = CAPCOM_CLC(capcom);
	CAPCOM_CLC(capcom) = (CAPCOM_RMC << MOD_CLC_RMC_SHIFT);
	uint32_t initial_t01con = CAPCOM_T01CON(capcom);
	uint32_t initial_t0 = CAPCOM_T0(capcom);
	uint32_t initial_t1 = CAPCOM_T1(capcom);
	uint32_t initial_t0rel = CAPCOM_T0REL(capcom);
	uint32_t initial_t1rel = CAPCOM_T1REL(capcom);

	CAPCOM_T01CON(capcom) = 0;
	CAPCOM_T0REL(capcom) = CAPCOM_T0_RELOAD;
	CAPCOM_T1REL(capcom) = CAPCOM_T1_RELOAD;
	CAPCOM_T0(capcom) = CAPCOM_START;
	CAPCOM_T1(capcom) = CAPCOM_START;
	uint32_t start = STM_TIM0;
	CAPCOM_T01CON(capcom) = CAPCOM_T01CON_T0R_ENABLED | CAPCOM_T01CON_T1R_ENABLED;
	struct timer_result t0 = { 0 };
	struct timer_result t1 = { 0 };
	while ((t0.elapsed_ticks == 0 || t1.elapsed_ticks == 0) && STM_TIM0 - start < CAPCOM_TIMEOUT_TICKS) {
		uint32_t value0 = CAPCOM_T0(capcom) & CAPCOM_T0_T0;
		uint32_t value1 = CAPCOM_T1(capcom) & CAPCOM_T1_T1;
		uint32_t elapsed = STM_TIM0 - start;
		if (t0.elapsed_ticks == 0 && value0 < CAPCOM_START) {
			t0.elapsed_ticks = elapsed;
			t0.value = value0;
		}
		if (t1.elapsed_ticks == 0 && value1 < CAPCOM_START) {
			t1.elapsed_ticks = elapsed;
			t1.value = value1;
		}
	}
	CAPCOM_T01CON(capcom) = 0;
	CAPCOM_T0(capcom) = initial_t0;
	CAPCOM_T1(capcom) = initial_t1;
	CAPCOM_T0REL(capcom) = initial_t0rel;
	CAPCOM_T1REL(capcom) = initial_t1rel;
	CAPCOM_T01CON(capcom) = initial_t01con;
	CAPCOM_CLC(capcom) = initial_clc;

	printf("# %s overflow: T0=%lu ticks value=%04lX T1=%lu ticks value=%04lX\n", name,
		t0.elapsed_ticks, t0.value, t1.elapsed_ticks, t1.value);
	test_check("T0 and T1 overflow at the expected clock rate",
		test_u32_in_interval(t0.elapsed_ticks, CAPCOM_EXPECTED_TICKS * 95 / 100,
			CAPCOM_EXPECTED_TICKS * 105 / 100) &&
		test_u32_in_interval(t1.elapsed_ticks, CAPCOM_EXPECTED_TICKS * 95 / 100,
			CAPCOM_EXPECTED_TICKS * 105 / 100));
	test_check("T0 and T1 reload independently after overflow",
		t0.value >= CAPCOM_T0_RELOAD && t0.value < CAPCOM_T0_RELOAD + 0x100 &&
		t1.value >= CAPCOM_T1_RELOAD && t1.value < CAPCOM_T1_RELOAD + 0x100);
}

int main(void) {
	test_start("CAPCOM timer overflow and reload test");
	test_category("CAPCOM0");
	test_timers(CAPCOM0, "CAPCOM0");
	test_category("CAPCOM1");
	test_timers(CAPCOM1, "CAPCOM1");
	return test_finish();
}
