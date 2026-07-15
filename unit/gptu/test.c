#include <pmb887x.h>

#include "test.h"

#define GPTU_RMC_MAX 0xFF
#define GPTU_T2_RC_RELOAD_OVERFLOW 4
#define FREQUENCY_WINDOW_US 2000
#define FREQUENCY_TOLERANCE_PERCENT 5
#define GPTU_T2AIS_UNUSED_TRIGGER 0x03333333
#define WAIT_ITERATIONS 100000
#define GPTU_EVENT_TIMEOUT_MS 100
#define GPTU_IRQ_TIMEOUT_MS 100

typedef struct {
	const char *name;
	uint32_t base;
} gptu_t;

static volatile uint32_t gptu0_src7_irqs;

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

static bool frequency_matches(uint32_t actual, uint32_t expected) {
	uint32_t tolerance = expected * FREQUENCY_TOLERANCE_PERCENT / 100;

	return actual >= expected - tolerance && actual <= expected + tolerance;
}

static void clear_gptu0_sources(void) {
	for (uint32_t index = 0; index < 8; index++)
		GPTU_SRC(GPTU0, index) = MOD_SRC_CLRR;
}

static void configure_gptu0_pending_request(void) {
	GPTU_CLC(GPTU0) = 1 << MOD_CLC_RMC_SHIFT;
	GPTU_T01IRS(GPTU0) = 0;
	GPTU_T01OTS(GPTU0) = GPTU_T01OTS_SSR00_A;
	GPTU_SRSEL(GPTU0) = GPTU_SRSEL_SSR7_SR00;
	GPTU_T0RDCBA(GPTU0) = 0;
	GPTU_T0DCBA(GPTU0) = 0xF0;
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T0ARUN;
}

static void disable_gptu0(void) {
	GPTU_T012RUN(GPTU0) = 0;
	GPTU_T01IRS(GPTU0) = 0;
	GPTU_T01OTS(GPTU0) = 0;
	GPTU_SRSEL(GPTU0) = 0;
	GPTU_T0RDCBA(GPTU0) = 0;
	GPTU_T0DCBA(GPTU0) = 0;
	GPTU_CLC(GPTU0) = MOD_CLC_DISR;
}

static bool wait_gptu0_source(uint32_t index) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < GPTU_EVENT_TIMEOUT_MS) {
		if ((GPTU_SRC(GPTU0, index) & MOD_SRC_SRR) != 0)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static bool wait_gptu0_irq(volatile uint32_t *count) {
	stopwatch_t start = stopwatch_get();

	while (*count == 0 && stopwatch_elapsed_ms(start) < GPTU_IRQ_TIMEOUT_MS)
		test_watchdog_serve();

	return *count != 0;
}

static void test_reset_values(const gptu_t *gptu) {
	test_category(gptu->name);
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, GPTU_CLC(gptu->base));
	GPTU_CLC(gptu->base) = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("T01IRS reset value", 0, GPTU_T01IRS(gptu->base));
	test_eq_u32("T01OTS reset value", 0, GPTU_T01OTS(gptu->base));
	test_eq_u32("T2CON reset value", 0, GPTU_T2CON(gptu->base));
	test_eq_u32("T2RCCON reset value", 0, GPTU_T2RCCON(gptu->base));
	test_eq_u32("T2AIS reset value", 0, GPTU_T2AIS(gptu->base));
	test_eq_u32("T2BIS reset value", 0, GPTU_T2BIS(gptu->base));
	test_eq_u32("T2ES reset value", 0, GPTU_T2ES(gptu->base));
	test_eq_u32("OSEL reset value", 0, GPTU_OSEL(gptu->base));
	test_eq_u32("T0DCBA reset value", 0, GPTU_T0DCBA(gptu->base));
	test_eq_u32("T0CBA reset value", 0, GPTU_T0CBA(gptu->base));
	test_eq_u32("T0RDCBA reset value", 0, GPTU_T0RDCBA(gptu->base));
	test_eq_u32("T0RCBA reset value", 0, GPTU_T0RCBA(gptu->base));
	test_eq_u32("T1DCBA reset value", 0, GPTU_T1DCBA(gptu->base));
	test_eq_u32("T1CBA reset value", 0, GPTU_T1CBA(gptu->base));
	test_eq_u32("T1RDCBA reset value", 0, GPTU_T1RDCBA(gptu->base));
	test_eq_u32("T1RCBA reset value", 0, GPTU_T1RCBA(gptu->base));
	test_eq_u32("T2 reset value", 0, GPTU_T2(gptu->base));
	test_eq_u32("T2RC0 reset value", 0, GPTU_T2RC0(gptu->base));
	test_eq_u32("T2RC1 reset value", 0, GPTU_T2RC1(gptu->base));
	test_eq_u32("T012RUN reset value", 0, GPTU_T012RUN(gptu->base));
	test_eq_u32("SRSEL reset value", 0, GPTU_SRSEL(gptu->base));
	uint32_t src_config = MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE;
	for (uint32_t index = 0; index < 8; index++)
		test_eq_u32("SRC routing reset value", 0, GPTU_SRC(gptu->base, index) & src_config);
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

static uint32_t measure_t01_frequency(const gptu_t *gptu, bool t1) {
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
	GPTU_T01IRS(gptu->base) = input;
	if (t1) {
		GPTU_T1DCBA(gptu->base) = 0;
	} else {
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
	printf(
		"# T2: RMC=1 %u Hz, RMC=2 %u Hz, RMC=4 %u Hz\n",
		(unsigned int) t2_rmc1,
		(unsigned int) t2_rmc2,
		(unsigned int) t2_rmc4
	);
	test_check("T2 frequency with RMC=1", frequency_matches(t2_rmc1, PMB8876_SYSTEM_FREQ));
	test_check("T2 frequency with RMC=2", frequency_matches(t2_rmc2, PMB8876_SYSTEM_FREQ / 2));
	test_check("T2 frequency with RMC=4", frequency_matches(t2_rmc4, PMB8876_SYSTEM_FREQ / 4));
	test_check("RMC 1:2 ratio", frequency_matches(t2_rmc1, t2_rmc2 * 2));
	test_check("RMC 2:4 ratio", frequency_matches(t2_rmc2, t2_rmc4 * 2));

	uint32_t t0 = measure_t01_frequency(gptu, false);
	uint32_t t1 = measure_t01_frequency(gptu, true);
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
	test_check("T0 overflow increments T2", triggered_count > 0x100 && triggered_count < 0x200);

	GPTU_T2CON(gptu->base) = 0;
	GPTU_T2RCCON(gptu->base) = 3 << GPTU_T2RCCON_T2AMRC0_SHIFT;
	GPTU_T2AIS(gptu->base) = GPTU_T2AIS_UNUSED_TRIGGER & ~GPTU_T2AIS_T2AIRC0;
	GPTU_T2ES(gptu->base) = 0;
	GPTU_T2RC0(gptu->base) = 0;
	GPTU_T2(gptu->base) = 0;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	fire_t0a_trigger(gptu);
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ACLRR;
	test_check("T0 overflow captures T2 into RC0", GPTU_T2RC0(gptu->base) != 0);
	test_check("captured value is within trigger window", GPTU_T2RC0(gptu->base) < 0x100000);

	GPTU_T2RCCON(gptu->base) = 5 << GPTU_T2RCCON_T2AMRC0_SHIFT;
	GPTU_T2AIS(gptu->base) = GPTU_T2AIS_UNUSED_TRIGGER & ~GPTU_T2AIS_T2AIRC0;
	GPTU_T2ES(gptu->base) = 0;
	GPTU_T2RC0(gptu->base) = 0x1234;
	GPTU_T2(gptu->base) = 0x10000000;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR;
	fire_t0a_trigger(gptu);
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ACLRR;
	uint32_t reloaded_count = GPTU_T2(gptu->base);
	printf("# internal reload result: %08X\n", (unsigned int) reloaded_count);
	test_check("T0 overflow reloads T2 from RC0", reloaded_count >= 0x1234 && reloaded_count < 0x100000);
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

static void test_pending_request_survives_disable(void) {
	const uint32_t source_config = MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE;
	uint32_t saved_vic7 = VIC_CON(VIC_GPTU0_SRC7_IRQ);
	uint32_t saved_src7 = GPTU_SRC(GPTU0, 7) & source_config;
	bool irq_was_disabled = cpu_enable_irq(false);

	test_category("GPTU0 pending request after module disable");
	clear_gptu0_sources();
	configure_gptu0_pending_request();
	test_check("T0A overflow reaches SRC7", wait_gptu0_source(7));
	test_eq_u32("SRC7 SRE remains disabled", 0, GPTU_SRC(GPTU0, 7) & MOD_SRC_SRE);

	disable_gptu0();
	test_check("CLC.DISR disables GPTU0", wait_mask_equal(&GPTU_CLC(GPTU0), MOD_CLC_DISS, MOD_CLC_DISS));
	test_eq_u32("SRC7 pending request survives CLC.DISR", MOD_SRC_SRR, GPTU_SRC(GPTU0, 7) & MOD_SRC_SRR);

	VIC_CON(VIC_GPTU0_SRC7_IRQ) = 1;
	gptu0_src7_irqs = 0;
	GPTU_CLC(GPTU0) = 1 << MOD_CLC_RMC_SHIFT;
	GPTU_SRC(GPTU0, 7) = MOD_SRC_SRE;
	cpu_enable_irq(true);
	test_check("Surviving SRC7 request raises IRQ92", wait_gptu0_irq(&gptu0_src7_irqs));
	stopwatch_usleep_wd(1000);
	cpu_enable_irq(false);
	test_eq_u32("Surviving request raises exactly one IRQ92", 1, gptu0_src7_irqs);
	test_eq_u32("IRQ handler clears surviving request", 0, GPTU_SRC(GPTU0, 7) & MOD_SRC_SRR);

	clear_gptu0_sources();
	disable_gptu0();
	GPTU_SRC(GPTU0, 7) = MOD_SRC_CLRR | saved_src7;
	VIC_CON(VIC_GPTU0_SRC7_IRQ) = saved_vic7;
	cpu_enable_irq(!irq_was_disabled);
}

static void test_instance(const gptu_t *gptu) {
	test_category(gptu->name);
	GPTU_CLC(gptu->base) = GPTU_RMC_MAX << MOD_CLC_RMC_SHIFT;
	test_module_id("module ID", 0x0001C002, GPTU_ID(gptu->base));
	test_module_clock("module clock is enabled", GPTU_CLC(gptu->base));
	test_frequency(gptu);
	test_t01_basic(gptu);
	test_t01_concat(gptu);
	test_t01_reload_and_request(gptu);
	test_internal_triggers(gptu);
	test_t2_combined(gptu);
	test_t2_split(gptu);
	test_t2_events(gptu);
}

int main(void) {
	const gptu_t instances[] = {
		{ "GPTU0", GPTU0 },
		{ "GPTU1", GPTU1 },
	};

	test_start("GPTU peripheral test");
	test_category("Reset values");
	for (unsigned int i = 0; i < sizeof(instances) / sizeof(instances[0]); i++)
		test_reset_values(&instances[i]);
	for (unsigned int i = 0; i < sizeof(instances) / sizeof(instances[0]); i++)
		test_instance(&instances[i]);
	test_pending_request_survives_disable();

	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == VIC_GPTU0_SRC7_IRQ) {
		gptu0_src7_irqs++;
		GPTU_SRC(GPTU0, 7) = MOD_SRC_CLRR;
	}
	VIC_IRQ_ACK = 1;
}
