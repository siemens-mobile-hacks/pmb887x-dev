#include <pmb887x.h>

#include "test.h"

#define GPTU_RMC_MAX 0xFF
#define GPTU_T2_RC_RELOAD_OVERFLOW 4
#define WAIT_ITERATIONS 100000

typedef struct {
	const char *name;
	uint32_t base;
} gptu_t;

static void gptu_stop(const gptu_t *gptu) {
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ACLRR | GPTU_T012RUN_T2BCLRR;
	GPTU_T012RUN(gptu->base) = 0;
}

static bool wait_mask_changed(volatile uint32_t *reg, uint32_t mask, uint32_t value) {
	for (unsigned int i = 0; i < WAIT_ITERATIONS; i++) {
		if ((*reg & mask) != value)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static bool wait_mask_equal(volatile uint32_t *reg, uint32_t mask, uint32_t value) {
	for (unsigned int i = 0; i < WAIT_ITERATIONS; i++) {
		if ((*reg & mask) == value)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static void test_t01_basic(const gptu_t *gptu) {
	test_category("T0 and T1 independent timers");
	gptu_stop(gptu);
	GPTU_T01IRS(gptu->base) = 0;
	GPTU_T0RDCBA(gptu->base) = 0;
	GPTU_T1RDCBA(gptu->base) = 0;
	GPTU_T0DCBA(gptu->base) = 0x11223300;
	GPTU_T1DCBA(gptu->base) = 0x55667700;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T1ARUN;

	test_check("T0A counts", wait_mask_changed(&GPTU_T0DCBA(gptu->base), 0xFF, 0));
	test_check("T1A counts", wait_mask_changed(&GPTU_T1DCBA(gptu->base), 0xFF, 0));
	gptu_stop(gptu);
	test_eq_u32("stopped T0B-T0D stay unchanged", 0x11223300, GPTU_T0DCBA(gptu->base) & 0xFFFFFF00);
	test_eq_u32("stopped T1B-T1D stay unchanged", 0x55667700, GPTU_T1DCBA(gptu->base) & 0xFFFFFF00);
}

static void test_concat(const gptu_t *gptu, bool t1, uint32_t input, uint32_t run, const char *name) {
	gptu_stop(gptu);
	GPTU_T01IRS(gptu->base) = input;
	if (t1) {
		GPTU_T1RDCBA(gptu->base) = 0;
		GPTU_T1DCBA(gptu->base) = 0xFFFFFFF0;
		GPTU_T012RUN(gptu->base) = run;
		test_check(name, wait_mask_changed(&GPTU_T1DCBA(gptu->base), 0xFFFFFF00, 0xFFFFFF00));
	} else {
		GPTU_T0RDCBA(gptu->base) = 0;
		GPTU_T0DCBA(gptu->base) = 0xFFFFFFF0;
		GPTU_T012RUN(gptu->base) = run;
		test_check(name, wait_mask_changed(&GPTU_T0DCBA(gptu->base), 0xFFFFFF00, 0xFFFFFF00));
	}
	gptu_stop(gptu);
}

static void test_t01_concat(const gptu_t *gptu) {
	test_category("T0 and T1 concatenation");
	test_concat(gptu, false, GPTU_T01IRS_T0BINS_CONCAT,
		GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T0BRUN, "T0A+B form a 16-bit timer");
	test_concat(gptu, false, GPTU_T01IRS_T0BINS_CONCAT | GPTU_T01IRS_T0CINS_CONCAT,
		GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T0BRUN | GPTU_T012RUN_T0CRUN, "T0A+B+C form a 24-bit timer");
	test_concat(gptu, false,
		GPTU_T01IRS_T0BINS_CONCAT | GPTU_T01IRS_T0CINS_CONCAT | GPTU_T01IRS_T0DINS_CONCAT,
		GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T0BRUN | GPTU_T012RUN_T0CRUN | GPTU_T012RUN_T0DRUN,
		"T0A+B+C+D form a 32-bit timer");
	test_concat(gptu, true,
		GPTU_T01IRS_T1BINS_CONCAT | GPTU_T01IRS_T1CINS_CONCAT | GPTU_T01IRS_T1DINS_CONCAT,
		GPTU_T012RUN_T1ARUN | GPTU_T012RUN_T1BRUN | GPTU_T012RUN_T1CRUN | GPTU_T012RUN_T1DRUN,
		"T1A+B+C+D form a 32-bit timer");
}

static void test_t01_reload_and_request(const gptu_t *gptu) {
	test_category("T0 reload and service request");
	gptu_stop(gptu);
	GPTU_SRC(gptu->base, 0) = MOD_SRC_CLRR;
	GPTU_T01IRS(gptu->base) = 0;
	GPTU_T01OTS(gptu->base) = GPTU_T01OTS_SSR00_A;
	GPTU_SRSEL(gptu->base) = GPTU_SRSEL_SSR0_SR00;
	GPTU_T0RDCBA(gptu->base) = 0xF0;
	GPTU_T0DCBA(gptu->base) = 0xF0;
	GPTU_SRC(gptu->base, 0) = MOD_SRC_SRE;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T0ARUN;

	test_check(
		"T0A overflow reaches SRC0",
		wait_mask_equal(&GPTU_SRC(gptu->base, 0), MOD_SRC_SRR, MOD_SRC_SRR)
	);
	gptu_stop(gptu);
	test_check("T0A reload value is used", (GPTU_T0DCBA(gptu->base) & 0xFF) >= 0xF0);
	GPTU_SRC(gptu->base, 0) = MOD_SRC_CLRR;
	test_eq_u32("SRC0 request clears", 0, GPTU_SRC(gptu->base, 0) & MOD_SRC_SRR);
}

static void test_t2_combined(const gptu_t *gptu) {
	test_category("T2 combined 32-bit timer");
	gptu_stop(gptu);
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACDIR_COUNT_UP;
	GPTU_T2(gptu->base) = 0xFFFFFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("combined T2 crosses 32-bit overflow", wait_mask_equal(&GPTU_T2(gptu->base), 0x80000000, 0));
	gptu_stop(gptu);

	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACDIR_COUNT_DOWN;
	GPTU_T2(gptu->base) = 0x10;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("combined T2 crosses 32-bit underflow", wait_mask_equal(&GPTU_T2(gptu->base), 0x80000000, 0x80000000));
	gptu_stop(gptu);

	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS;
	GPTU_T2RCCON(gptu->base) = GPTU_T2_RC_RELOAD_OVERFLOW << GPTU_T2RCCON_T2AMRC0_SHIFT;
	GPTU_T2RC0(gptu->base) = 0x12345678;
	GPTU_T2(gptu->base) = 0xFFFFFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("combined T2 one-shot stops", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("combined T2 reloads from RC0", 0x12345678, GPTU_T2(gptu->base));
}

static void test_t2_split(const gptu_t *gptu) {
	test_category("T2 split 16-bit timers");
	gptu_stop(gptu);
	GPTU_T2CON(gptu->base) = (
		GPTU_T2CON_T2SPLIT |
		GPTU_T2CON_T2ACDIR_COUNT_UP |
		GPTU_T2CON_T2BCDIR_COUNT_DOWN
	);
	GPTU_T2(gptu->base) = 0x0010FFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2BSETR;
	test_check("split T2A crosses 16-bit overflow", wait_mask_equal(&GPTU_T2(gptu->base), BIT(15), 0));
	test_check("split T2B crosses 16-bit underflow", wait_mask_equal(&GPTU_T2(gptu->base), BIT(31), BIT(31)));
	gptu_stop(gptu);

	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2SPLIT;
	GPTU_T2(gptu->base) = 0x40004000;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2BSETR;
	test_check("split T2A and T2B start together", wait_mask_changed(
		&GPTU_T2(gptu->base), 0xFFFFFFFF, 0x40004000
	));
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ACLRR;
	uint32_t stopped = GPTU_T2(gptu->base);
	test_spin(1024);
	uint32_t running = GPTU_T2(gptu->base);
	test_eq_u32("split T2A stops independently", stopped & 0xFFFF, running & 0xFFFF);
	test_check("split T2B keeps running", (stopped & 0xFFFF0000) != (running & 0xFFFF0000));
	gptu_stop(gptu);
}

static void test_instance(const gptu_t *gptu) {
	test_category(gptu->name);
	GPTU_CLC(gptu->base) = GPTU_RMC_MAX << MOD_CLC_RMC_SHIFT;
	test_module_id("module ID", 0x0001C002, GPTU_ID(gptu->base));
	test_module_clock("module clock is enabled", GPTU_CLC(gptu->base));
	test_t01_basic(gptu);
	test_t01_concat(gptu);
	test_t01_reload_and_request(gptu);
	test_t2_combined(gptu);
	test_t2_split(gptu);
}

int main(void) {
	const gptu_t instances[] = {
		{ "GPTU0", GPTU0 },
		{ "GPTU1", GPTU1 },
	};

	test_start("GPTU peripheral test");
	for (unsigned int i = 0; i < sizeof(instances) / sizeof(instances[0]); i++)
		test_instance(&instances[i]);

	return test_finish();
}
