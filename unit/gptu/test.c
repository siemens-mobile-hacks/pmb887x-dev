#include <pmb887x.h>

#include "test.h"

#define GPTU_RMC_MAX 0xFF
#define GPTU_T2_RC_CAPTURE_EXTERNAL 3
#define GPTU_T2_RC_RELOAD_OVERFLOW_UNDERFLOW 4
#define GPTU_T2_RC_RELOAD_EXTERNAL 5
#define GPTU_T2_RC_RELOAD_DIRECTIONAL 6
#define FREQUENCY_WINDOW_US 2000
#define FREQUENCY_TOLERANCE_PERCENT 5
#define GPTU_T2AIS_UNUSED_TRIGGER 0x03333333
#define GPTU_EVENT_TIMEOUT_MS 100

typedef struct {
	const char *name;
	uint32_t base;
} gptu_t;

static void gptu_stop(const gptu_t *gptu) {
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ACLRR | GPTU_T012RUN_T2BCLRR;
	GPTU_T012RUN(gptu->base) = 0;
}

static bool wait_mask_changed(volatile uint32_t *reg, uint32_t mask, uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < GPTU_EVENT_TIMEOUT_MS) {
		if ((*reg & mask) != value)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static bool wait_mask_equal(volatile uint32_t *reg, uint32_t mask, uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < GPTU_EVENT_TIMEOUT_MS) {
		if ((*reg & mask) == value)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static bool frequency_matches(uint32_t actual, uint32_t expected) {
	uint32_t tolerance = expected * FREQUENCY_TOLERANCE_PERCENT / 100;

	return actual >= expected - tolerance && actual <= expected + tolerance;
}

static void test_reset_values(uint32_t base) {
	test_category("GPTU1 reset values");
	uint32_t clc = GPTU_CLC(base);
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, clc);
	if (clc != (MOD_CLC_DISR | MOD_CLC_DISS)) {
		test_skip("register reset values", "module was enabled before the test");
		return;
	}

	GPTU_CLC(base) = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("T01IRS reset value", 0, GPTU_T01IRS(base));
	test_eq_u32("T01OTS reset value", 0, GPTU_T01OTS(base));
	test_eq_u32("T2CON reset value", 0, GPTU_T2CON(base));
	test_eq_u32("T2RCCON reset value", 0, GPTU_T2RCCON(base));
	test_eq_u32("T2AIS reset value", 0, GPTU_T2AIS(base));
	test_eq_u32("T2BIS reset value", 0, GPTU_T2BIS(base));
	test_eq_u32("T2ES reset value", 0, GPTU_T2ES(base));
	test_eq_u32("OSEL reset value", 0, GPTU_OSEL(base));
	test_eq_u32("OUT reset value", 0, GPTU_OUT(base));
	test_eq_u32("T0DCBA reset value", 0, GPTU_T0DCBA(base));
	test_eq_u32("T0CBA reset value", 0, GPTU_T0CBA(base));
	test_eq_u32("T0RDCBA reset value", 0, GPTU_T0RDCBA(base));
	test_eq_u32("T0RCBA reset value", 0, GPTU_T0RCBA(base));
	test_eq_u32("T1DCBA reset value", 0, GPTU_T1DCBA(base));
	test_eq_u32("T1CBA reset value", 0, GPTU_T1CBA(base));
	test_eq_u32("T1RDCBA reset value", 0, GPTU_T1RDCBA(base));
	test_eq_u32("T1RCBA reset value", 0, GPTU_T1RCBA(base));
	test_eq_u32("T2 reset value", 0, GPTU_T2(base));
	test_eq_u32("T2RC0 reset value", 0, GPTU_T2RC0(base));
	test_eq_u32("T2RC1 reset value", 0, GPTU_T2RC1(base));
	test_eq_u32("T012RUN reset value", 0, GPTU_T012RUN(base));
	test_eq_u32("SRSEL reset value", 0, GPTU_SRSEL(base));
	uint32_t src_config = MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE;
	for (uint32_t index = 0; index < 8; index++)
		test_eq_u32("SRC routing reset value", 0, GPTU_SRC(base, index) & src_config);
}

static void test_register_aliases(const gptu_t *gptu) {
	test_category("T0 and T1 24-bit register aliases");
	gptu_stop(gptu);

	GPTU_T0DCBA(gptu->base) = 0x11223344;
	test_eq_u32("T0CBA reads only T0C-T0A", 0x00223344, GPTU_T0CBA(gptu->base));
	GPTU_T0CBA(gptu->base) = 0xFFEEDDCC;
	test_eq_u32("T0CBA write preserves T0D", 0x11EEDDCC, GPTU_T0DCBA(gptu->base));
	GPTU_T0RDCBA(gptu->base) = 0x55667788;
	test_eq_u32("T0RCBA reads only T0RC-T0RA", 0x00667788, GPTU_T0RCBA(gptu->base));
	GPTU_T0RCBA(gptu->base) = 0xFFAA9988;
	test_eq_u32("T0RCBA write preserves T0RD", 0x55AA9988, GPTU_T0RDCBA(gptu->base));

	GPTU_T1DCBA(gptu->base) = 0x99AABBCC;
	test_eq_u32("T1CBA reads only T1C-T1A", 0x00AABBCC, GPTU_T1CBA(gptu->base));
	GPTU_T1CBA(gptu->base) = 0xFF443322;
	test_eq_u32("T1CBA write preserves T1D", 0x99443322, GPTU_T1DCBA(gptu->base));
	GPTU_T1RDCBA(gptu->base) = 0xDDEEFF00;
	test_eq_u32("T1RCBA reads only T1RC-T1RA", 0x00EEFF00, GPTU_T1RCBA(gptu->base));
	GPTU_T1RCBA(gptu->base) = 0xFF123456;
	test_eq_u32("T1RCBA write preserves T1RD", 0xDD123456, GPTU_T1RDCBA(gptu->base));
}

static void test_run_control(const gptu_t *gptu) {
	test_category("T2 run control commands");
	gptu_stop(gptu);
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2SPLIT;
	GPTU_T2(gptu->base) = 0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2BSETR;
	test_eq_u32(
		"set commands start both split timers",
		GPTU_T012RUN_T2ARUN | GPTU_T012RUN_T2BRUN,
		GPTU_T012RUN(gptu->base)
	);

	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ARUN | GPTU_T012RUN_T2BRUN;
	test_eq_u32(
		"writing run status bits has no effect",
		GPTU_T012RUN_T2ARUN | GPTU_T012RUN_T2BRUN,
		GPTU_T012RUN(gptu->base)
	);
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2ACLRR |
		GPTU_T012RUN_T2BSETR | GPTU_T012RUN_T2BCLRR;
	test_eq_u32(
		"simultaneous set and clear leaves run state unchanged",
		GPTU_T012RUN_T2ARUN | GPTU_T012RUN_T2BRUN,
		GPTU_T012RUN(gptu->base)
	);

	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ACLRR;
	test_eq_u32("T2A clear command leaves T2B running", GPTU_T012RUN_T2BRUN, GPTU_T012RUN(gptu->base));
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2BCLRR;
	test_eq_u32("T2B clear command stops the remaining timer", 0, GPTU_T012RUN(gptu->base));
}

static void test_output_control(const gptu_t *gptu) {
	test_category("Output control");
	gptu_stop(gptu);
	GPTU_OUT(gptu->base) = 0xFF << 8;
	GPTU_OUT(gptu->base) = GPTU_OUT_SETO0 | GPTU_OUT_SETO3 | GPTU_OUT_SETO7;
	test_eq_u32("software set commands update selected outputs", BIT(0) | BIT(3) | BIT(7), GPTU_OUT(gptu->base));
	GPTU_OUT(gptu->base) = GPTU_OUT_OUT1 | GPTU_OUT_OUT3;
	test_eq_u32("writing output status bits has no effect", BIT(0) | BIT(3) | BIT(7), GPTU_OUT(gptu->base));
	GPTU_OUT(gptu->base) = GPTU_OUT_SETO0 | GPTU_OUT_CLRO0 | GPTU_OUT_CLRO3 | GPTU_OUT_SETO4;
	test_eq_u32("set and clear commands have documented priority", BIT(0) | BIT(4) | BIT(7), GPTU_OUT(gptu->base));

	GPTU_OUT(gptu->base) = 0xFF << 8;
	GPTU_OSEL(gptu->base) = GPTU_OSEL_SO0_OUV_T2A | GPTU_OSEL_SO1_OUV_T2B;
	GPTU_T2RCCON(gptu->base) = 0;
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS;
	GPTU_T2(gptu->base) = 0xFFFFFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("T2 overflow event completes", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("T2A and T2B overflow events toggle their selected outputs", BIT(0) | BIT(1), GPTU_OUT(gptu->base));

	GPTU_T2(gptu->base) = 0xFFFFFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("second T2 overflow event completes", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("second overflow toggles the selected outputs back", 0, GPTU_OUT(gptu->base));
}

static uint32_t measure_t2_frequency(const gptu_t *gptu, uint32_t rmc) {
	gptu_stop(gptu);
	GPTU_CLC(gptu->base) = rmc << MOD_CLC_RMC_SHIFT;
	GPTU_T2CON(gptu->base) = 0;
	GPTU_T2RCCON(gptu->base) = 0;
	GPTU_T2(gptu->base) = 0;
	stopwatch_t start = stopwatch_get();
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	stopwatch_usleep_wd(FREQUENCY_WINDOW_US);
	gptu_stop(gptu);
	uint32_t count = GPTU_T2(gptu->base);
	uint32_t elapsed_us = stopwatch_elapsed_us(start);

	return (uint32_t) ((uint64_t) count * 1000000 / elapsed_us);
}

static uint32_t measure_t01_frequency(const gptu_t *gptu, bool t1, uint32_t rmc) {
	uint32_t input;
	uint32_t run;

	if (t1) {
		input = GPTU_T01IRS_T1BINS_CONCAT | GPTU_T01IRS_T1CINS_CONCAT | GPTU_T01IRS_T1DINS_CONCAT;
		run = GPTU_T012RUN_T1ARUN | GPTU_T012RUN_T1BRUN | GPTU_T012RUN_T1CRUN | GPTU_T012RUN_T1DRUN;
	} else {
		input = GPTU_T01IRS_T0BINS_CONCAT | GPTU_T01IRS_T0CINS_CONCAT | GPTU_T01IRS_T0DINS_CONCAT;
		run = GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T0BRUN | GPTU_T012RUN_T0CRUN | GPTU_T012RUN_T0DRUN;
	}

	gptu_stop(gptu);
	GPTU_CLC(gptu->base) = rmc << MOD_CLC_RMC_SHIFT;
	GPTU_T01IRS(gptu->base) = input;
	if (t1) {
		GPTU_T1RDCBA(gptu->base) = 0;
		GPTU_T1DCBA(gptu->base) = 0;
	} else {
		GPTU_T0RDCBA(gptu->base) = 0;
		GPTU_T0DCBA(gptu->base) = 0;
	}
	stopwatch_t start = stopwatch_get();
	GPTU_T012RUN(gptu->base) = run;
	stopwatch_usleep_wd(FREQUENCY_WINDOW_US);
	gptu_stop(gptu);
	uint32_t count = t1 ? GPTU_T1DCBA(gptu->base) : GPTU_T0DCBA(gptu->base);
	uint32_t elapsed_us = stopwatch_elapsed_us(start);

	return (uint32_t) ((uint64_t) count * 1000000 / elapsed_us);
}

static void test_frequency(const gptu_t *gptu) {
	test_category("Clock frequency");
	uint32_t t2_rmc1 = measure_t2_frequency(gptu, 1);
	uint32_t t2_rmc2 = measure_t2_frequency(gptu, 2);
	uint32_t t2_rmc4 = measure_t2_frequency(gptu, 4);
	uint32_t t2_rmc26 = measure_t2_frequency(gptu, 26);
	uint32_t t2_rmc130 = measure_t2_frequency(gptu, 130);
	printf(
		"# T2: RMC=1 %u Hz, RMC=2 %u Hz, RMC=4 %u Hz, RMC=26 %u Hz, RMC=130 %u Hz\n",
		(unsigned int) t2_rmc1,
		(unsigned int) t2_rmc2,
		(unsigned int) t2_rmc4,
		(unsigned int) t2_rmc26,
		(unsigned int) t2_rmc130
	);
	test_check("T2 frequency with RMC=1", frequency_matches(t2_rmc1, PMB8876_SYSTEM_FREQ));
	test_check("T2 frequency with RMC=2", frequency_matches(t2_rmc2, PMB8876_SYSTEM_FREQ / 2));
	test_check("T2 frequency with RMC=4", frequency_matches(t2_rmc4, PMB8876_SYSTEM_FREQ / 4));
	test_check("T2 frequency with firmware RMC=26", frequency_matches(t2_rmc26, PMB8876_SYSTEM_FREQ / 26));
	test_check("T2 frequency with firmware RMC=130", frequency_matches(t2_rmc130, PMB8876_SYSTEM_FREQ / 130));
	test_check("RMC 1:2 ratio", frequency_matches(t2_rmc1, t2_rmc2 * 2));
	test_check("RMC 2:4 ratio", frequency_matches(t2_rmc2, t2_rmc4 * 2));

	uint32_t t0 = measure_t01_frequency(gptu, false, 4);
	uint32_t t1 = measure_t01_frequency(gptu, true, 4);
	printf("# T0: %u Hz, T1: %u Hz at RMC=4\n", (unsigned int) t0, (unsigned int) t1);
	test_check("T0 frequency matches T2", frequency_matches(t0, t2_rmc4));
	test_check("T1 frequency matches T2", frequency_matches(t1, t2_rmc4));
	GPTU_CLC(gptu->base) = GPTU_RMC_MAX << MOD_CLC_RMC_SHIFT;
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

	gptu_stop(gptu);
	GPTU_T01IRS(gptu->base) = (
		GPTU_T01IRS_T0BINS_CONCAT |
		GPTU_T01IRS_T0CINS_CONCAT |
		GPTU_T01IRS_T0DINS_CONCAT |
		GPTU_T01IRS_T1AINS_CONCAT |
		GPTU_T01IRS_T1BINS_CONCAT |
		GPTU_T01IRS_T1CINS_CONCAT |
		GPTU_T01IRS_T1DINS_CONCAT |
		GPTU_T01IRS_T1INC
	);
	GPTU_T0DCBA(gptu->base) = 0xFFFFFFF0;
	GPTU_T1DCBA(gptu->base) = 0x12345678;
	GPTU_T012RUN(gptu->base) = 0xFF;
	test_check("T0 and T1 form a 64-bit timer", wait_mask_changed(
		&GPTU_T1DCBA(gptu->base), 0xFFFFFFFF, 0x12345678
	));
	gptu_stop(gptu);
}

static void fire_t0a_trigger(const gptu_t *gptu) {
	GPTU_T01IRS(gptu->base) = 0;
	GPTU_T01OTS(gptu->base) = GPTU_T01OTS_STRG00_A | GPTU_T01OTS_SSR00_A;
	GPTU_SRSEL(gptu->base) = GPTU_SRSEL_SSR0_SR00;
	GPTU_SRC(gptu->base, 0) = MOD_SRC_CLRR;
	wait_mask_equal(&GPTU_SRC(gptu->base, 0), MOD_SRC_SRR, 0);
	GPTU_SRC(gptu->base, 0) = MOD_SRC_SRE;
	GPTU_T012RUN(gptu->base) &= ~GPTU_T012RUN_T0ARUN;
	GPTU_T0RDCBA(gptu->base) = 0;
	GPTU_T0DCBA(gptu->base) = 0xFE;
	test_spin(16);
	GPTU_T012RUN(gptu->base) |= GPTU_T012RUN_T0ARUN;
	wait_mask_equal(&GPTU_SRC(gptu->base, 0), MOD_SRC_SRR, MOD_SRC_SRR);
	GPTU_T012RUN(gptu->base) &= ~GPTU_T012RUN_T0ARUN;
	GPTU_SRC(gptu->base, 0) = MOD_SRC_CLRR;
}

static void test_internal_triggers(const gptu_t *gptu) {
	test_category("T0 to T2 internal triggers");
	gptu_stop(gptu);
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACSRC_EXT_COUNT;
	GPTU_T2RCCON(gptu->base) = 0;
	GPTU_T2AIS(gptu->base) = GPTU_T2AIS_UNUSED_TRIGGER & ~GPTU_T2AIS_T2AICNT;
	GPTU_T2ES(gptu->base) = 0;
	GPTU_T2(gptu->base) = 0x100;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	fire_t0a_trigger(gptu);
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ACLRR;
	uint32_t triggered_count = GPTU_T2(gptu->base);
	printf("# internal count result: %08X\n", (unsigned int) triggered_count);
	test_eq_u32("one T0 overflow increments T2 once", 0x101, triggered_count);

	gptu_stop(gptu);
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACCLR_CP0_T2;
	GPTU_T2RCCON(gptu->base) = GPTU_T2_RC_CAPTURE_EXTERNAL << GPTU_T2RCCON_T2AMRC0_SHIFT;
	GPTU_T2AIS(gptu->base) = GPTU_T2AIS_UNUSED_TRIGGER & ~GPTU_T2AIS_T2AIRC0;
	GPTU_T2ES(gptu->base) = 0;
	GPTU_T2RC0(gptu->base) = 0;
	GPTU_T2(gptu->base) = 0x13579BDF;
	fire_t0a_trigger(gptu);
	test_eq_u32("T0 overflow captures the stopped T2 value into RC0", 0x13579BDF, GPTU_T2RC0(gptu->base));
	test_eq_u32("capture 0 clears T2 after preserving its value", 0, GPTU_T2(gptu->base));

	GPTU_T2CON(gptu->base) = 0;
	GPTU_T2RCCON(gptu->base) = GPTU_T2_RC_RELOAD_EXTERNAL << GPTU_T2RCCON_T2AMRC0_SHIFT;
	GPTU_T2AIS(gptu->base) = GPTU_T2AIS_UNUSED_TRIGGER & ~GPTU_T2AIS_T2AIRC0;
	GPTU_T2ES(gptu->base) = 0;
	GPTU_T2RC0(gptu->base) = 0x1234;
	GPTU_T2(gptu->base) = 0x10000000;
	fire_t0a_trigger(gptu);
	test_eq_u32("T0 overflow reloads stopped T2 from RC0", 0x1234, GPTU_T2(gptu->base));
	gptu_stop(gptu);
	GPTU_T2AIS(gptu->base) = 0;
	GPTU_T2ES(gptu->base) = 0;
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
	GPTU_T2RCCON(gptu->base) =
		GPTU_T2_RC_RELOAD_OVERFLOW_UNDERFLOW << GPTU_T2RCCON_T2AMRC0_SHIFT;
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

static void test_t2_to_t01_counting(const gptu_t *gptu) {
	test_category("T2 overflow inputs to T0 and T1");
	gptu_stop(gptu);
	GPTU_T01IRS(gptu->base) = GPTU_T01IRS_T0AINS_CNT0 | GPTU_T01IRS_T1AINS_CNT1;
	GPTU_T0DCBA(gptu->base) = 0;
	GPTU_T1DCBA(gptu->base) = 0;
	GPTU_T2RCCON(gptu->base) = 0;
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS;
	GPTU_T2(gptu->base) = 0xFFFFFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T1ARUN | GPTU_T012RUN_T2ASETR;
	test_check("combined T2 overflow completes", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	GPTU_T012RUN(gptu->base) = 0;

	test_eq_u32("OUV_T2A supplies one CNT0 count to T0A", 1, GPTU_T0DCBA(gptu->base));
	test_eq_u32("OUV_T2B supplies one CNT1 count to T1A", 1, GPTU_T1DCBA(gptu->base));
}

static void test_t2_reload_modes(const gptu_t *gptu) {
	test_category("T2 reload modes");
	gptu_stop(gptu);
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS;
	GPTU_T2RCCON(gptu->base) = GPTU_T2_RC_RELOAD_DIRECTIONAL << GPTU_T2RCCON_T2AMRC0_SHIFT;
	GPTU_T2RC0(gptu->base) = 0x12345678;
	GPTU_T2(gptu->base) = 0xFFFFFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("RC0 overflow reload completes", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("RC0 mode 6 reloads only on overflow", 0x12345678, GPTU_T2(gptu->base));

	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS | GPTU_T2CON_T2ACDIR_COUNT_DOWN;
	GPTU_T2RCCON(gptu->base) = GPTU_T2_RC_RELOAD_DIRECTIONAL << GPTU_T2RCCON_T2AMRC1_SHIFT;
	GPTU_T2RC1(gptu->base) = 0x89ABCDEF;
	GPTU_T2(gptu->base) = 0x10;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("RC1 underflow reload completes", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("RC1 mode 6 reloads only on underflow", 0x89ABCDEF, GPTU_T2(gptu->base));

	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS;
	GPTU_T2RCCON(gptu->base) =
		(GPTU_T2_RC_RELOAD_OVERFLOW_UNDERFLOW << GPTU_T2RCCON_T2AMRC0_SHIFT) |
		(GPTU_T2_RC_RELOAD_OVERFLOW_UNDERFLOW << GPTU_T2RCCON_T2AMRC1_SHIFT);
	GPTU_T2RC0(gptu->base) = 0x11223344;
	GPTU_T2RC1(gptu->base) = 0x55667788;
	GPTU_T2(gptu->base) = 0xFFFFFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("simultaneous RC0 and RC1 reload completes", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("RC1 has priority when both reload conditions match", 0x55667788, GPTU_T2(gptu->base));

	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2SPLIT | GPTU_T2CON_T2ACOS |
		GPTU_T2CON_T2BCOS | GPTU_T2CON_T2BCDIR_COUNT_DOWN;
	GPTU_T2RCCON(gptu->base) =
		(GPTU_T2_RC_RELOAD_OVERFLOW_UNDERFLOW << GPTU_T2RCCON_T2AMRC0_SHIFT) |
		(GPTU_T2_RC_RELOAD_OVERFLOW_UNDERFLOW << GPTU_T2RCCON_T2BMRC1_SHIFT);
	GPTU_T2RC0(gptu->base) = 0x00001234;
	GPTU_T2RC1(gptu->base) = 0xABCD0000;
	GPTU_T2(gptu->base) = 0x0010FFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2BSETR;
	test_check("split T2A and T2B reloads complete", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN | GPTU_T012RUN_T2BRUN, 0
	));
	test_eq_u32("split timers use their own reload halves", 0xABCD1234, GPTU_T2(gptu->base));
}

static void test_t2_events(const gptu_t *gptu) {
	test_category("T2 event boundaries and service requests");
	gptu_stop(gptu);
	GPTU_T2RCCON(gptu->base) = 0;
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS | GPTU_T2CON_T2ACOV_MODE1;
	GPTU_T2(gptu->base) = 0xFFFFFFFD;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("early overflow stops T2", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("early overflow occurs at maximum", 0xFFFFFFFF, GPTU_T2(gptu->base));

	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS | GPTU_T2CON_T2ACDIR_COUNT_DOWN | GPTU_T2CON_T2ACOV_MODE2;
	GPTU_T2(gptu->base) = 2;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("early underflow stops T2", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("early underflow occurs at zero", 0, GPTU_T2(gptu->base));

	GPTU_SRC(gptu->base, 5) = MOD_SRC_CLRR;
	GPTU_SRC(gptu->base, 6) = MOD_SRC_CLRR;
	GPTU_SRSEL(gptu->base) = GPTU_SRSEL_SSR5_OUV_T2B | GPTU_SRSEL_SSR6_OUV_T2A;
	GPTU_SRC(gptu->base, 5) = MOD_SRC_SRE;
	GPTU_SRC(gptu->base, 6) = MOD_SRC_SRE;
	GPTU_T2CON(gptu->base) = 0;
	GPTU_T2(gptu->base) = 0xFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("combined T2 crosses the 16-bit boundary", wait_mask_changed(
		&GPTU_T2(gptu->base), 0xFFFF0000, 0
	));
	gptu_stop(gptu);
	test_eq_u32("combined T2 boundary does not raise OUV_T2B",
		0, GPTU_SRC(gptu->base, 5) & MOD_SRC_SRR);
	test_eq_u32("combined T2 boundary raises OUV_T2A", MOD_SRC_SRR, GPTU_SRC(gptu->base, 6) & MOD_SRC_SRR);
	GPTU_SRC(gptu->base, 5) = MOD_SRC_CLRR;
	GPTU_SRC(gptu->base, 6) = MOD_SRC_CLRR;
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS;
	GPTU_T2(gptu->base) = 0xFFFFFFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	test_check("combined T2 full overflow stops the timer", wait_mask_equal(
		&GPTU_T012RUN(gptu->base), GPTU_T012RUN_T2ARUN, 0
	));
	test_eq_u32("combined T2 full overflow raises OUV_T2B",
		MOD_SRC_SRR, GPTU_SRC(gptu->base, 5) & MOD_SRC_SRR);
	test_eq_u32("combined T2 full overflow raises OUV_T2A",
		MOD_SRC_SRR, GPTU_SRC(gptu->base, 6) & MOD_SRC_SRR);
	GPTU_SRC(gptu->base, 5) = MOD_SRC_CLRR;
	GPTU_SRC(gptu->base, 6) = MOD_SRC_CLRR;

	GPTU_SRC(gptu->base, 0) = MOD_SRC_CLRR;
	GPTU_SRC(gptu->base, 1) = MOD_SRC_CLRR;
	GPTU_SRSEL(gptu->base) = GPTU_SRSEL_SSR0_OUV_T2A | GPTU_SRSEL_SSR1_OUV_T2B;
	GPTU_SRC(gptu->base, 0) = MOD_SRC_SRE;
	GPTU_SRC(gptu->base, 1) = MOD_SRC_SRE;
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2SPLIT;
	GPTU_T2(gptu->base) = 0xFFF0FFF0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2BSETR;
	test_check("T2A overflow reaches SRC0", wait_mask_equal(
		&GPTU_SRC(gptu->base, 0), MOD_SRC_SRR, MOD_SRC_SRR
	));
	test_check("T2B overflow reaches SRC1", wait_mask_equal(
		&GPTU_SRC(gptu->base, 1), MOD_SRC_SRR, MOD_SRC_SRR
	));
	gptu_stop(gptu);
	GPTU_SRC(gptu->base, 0) = MOD_SRC_CLRR;
	GPTU_SRC(gptu->base, 1) = MOD_SRC_CLRR;
}

static void test_instance(const gptu_t *gptu) {
	test_category(gptu->name);
	GPTU_CLC(gptu->base) = GPTU_RMC_MAX << MOD_CLC_RMC_SHIFT;
	test_module_id("module ID", 0x0001C002, GPTU_ID(gptu->base));
	test_module_clock("module clock is enabled", GPTU_CLC(gptu->base));
	test_register_aliases(gptu);
	test_run_control(gptu);
	test_output_control(gptu);
	test_frequency(gptu);
	test_t01_basic(gptu);
	test_t01_concat(gptu);
	test_t01_reload_and_request(gptu);
	test_internal_triggers(gptu);
	test_t2_combined(gptu);
	test_t2_split(gptu);
	test_t2_to_t01_counting(gptu);
	test_t2_reload_modes(gptu);
	test_t2_events(gptu);
}

int main(void) {
	const gptu_t instances[] = {
		{ "GPTU0", GPTU0 },
		{ "GPTU1", GPTU1 },
	};

	test_start("GPTU peripheral test");
	/* GPTU has no SCU_RST_REQ bit; only untouched GPTU1 provides reliable reset-state evidence. */
	test_reset_values(GPTU1);
	for (uint32_t i = 0; i < ARRAY_SIZE(instances); i++)
		test_instance(&instances[i]);

	return test_finish();
}
