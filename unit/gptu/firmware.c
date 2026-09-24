#include <pmb887x.h>

#include "test.h"

#define GPTU_EVENT_TIMEOUT_MS 100
#define GPTU_RMC_SL98_PULSE 26
#define GPTU_T2_RC_RELOAD_DIRECTIONAL 6

typedef struct {
	const char *name;
	uint32_t base;
} gptu_t;

static void gptu_enable(const gptu_t *gptu, uint32_t rmc) {
	GPTU_CLC(gptu->base) = rmc << MOD_CLC_RMC_SHIFT;
}

static void gptu_stop(const gptu_t *gptu) {
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ACLRR | GPTU_T012RUN_T2BCLRR;
	GPTU_T012RUN(gptu->base) = 0;
}

static void gptu_clear_sources(const gptu_t *gptu) {
	for (uint32_t index = 0; index < 8; index++)
		GPTU_SRC(gptu->base, index) = MOD_SRC_CLRR;
}

static uint32_t gptu_pending_sources(const gptu_t *gptu) {
	uint32_t pending = 0;

	for (uint32_t index = 0; index < 8; index++) {
		if ((GPTU_SRC(gptu->base, index) & MOD_SRC_SRR) != 0)
			pending |= BIT(index);
	}

	return pending;
}

static bool wait_source(const gptu_t *gptu, uint32_t index) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < GPTU_EVENT_TIMEOUT_MS) {
		if ((GPTU_SRC(gptu->base, index) & MOD_SRC_SRR) != 0)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static bool wait_t2_stopped(const gptu_t *gptu, uint32_t running) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < GPTU_EVENT_TIMEOUT_MS) {
		if ((GPTU_T012RUN(gptu->base) & running) == 0)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static void configure_el71_system_timers(const gptu_t *gptu) {
	GPTU_T01IRS(gptu->base) =
		GPTU_T01IRS_T01IN0_POS_IN0 |
		GPTU_T01IRS_T01IN1_POS_IN1 |
		GPTU_T01IRS_T0AREL | GPTU_T01IRS_T0BREL | GPTU_T01IRS_T0CREL |
		GPTU_T01IRS_T1AREL | GPTU_T01IRS_T1BREL | GPTU_T01IRS_T1CREL |
		GPTU_T01IRS_T0BINS_CONCAT | GPTU_T01IRS_T0CINS_CONCAT | GPTU_T01IRS_T0DINS_CONCAT |
		GPTU_T01IRS_T1BINS_CONCAT | GPTU_T01IRS_T1CINS_CONCAT | GPTU_T01IRS_T1DINS_CONCAT;
	GPTU_T01OTS(gptu->base) =
		GPTU_T01OTS_SOUT00_D | GPTU_T01OTS_SOUT01_D |
		GPTU_T01OTS_STRG00_D | GPTU_T01OTS_STRG01_D |
		GPTU_T01OTS_SSR00_D | GPTU_T01OTS_SSR01_D |
		GPTU_T01OTS_SOUT10_D | GPTU_T01OTS_SOUT11_D |
		GPTU_T01OTS_STRG10_D | GPTU_T01OTS_STRG11_D |
		GPTU_T01OTS_SSR10_D | GPTU_T01OTS_SSR11_D;
	GPTU_SRSEL(gptu->base) = GPTU_SRSEL_SSR7_SR00 |
		GPTU_SRSEL_SSR6_SR10 | GPTU_SRSEL_SSR5_OUV_T2B;
}

static void test_bootrom_timeout(const gptu_t *gptu) {
	test_category("BootROM 24-bit timeout");
	gptu_enable(gptu, 1);
	gptu_stop(gptu);
	gptu_clear_sources(gptu);
	GPTU_T01IRS(gptu->base) = GPTU_T01IRS_T0AREL | GPTU_T01IRS_T0BREL |
		GPTU_T01IRS_T0BINS_CONCAT | GPTU_T01IRS_T0CINS_CONCAT;
	GPTU_T01OTS(gptu->base) = GPTU_T01OTS_SSR00_C;
	GPTU_SRSEL(gptu->base) = GPTU_SRSEL_SSR7_SR00;
	GPTU_T0RCBA(gptu->base) = 0;
	GPTU_T0CBA(gptu->base) = 0xFFFF00;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T0ARUN |
		GPTU_T012RUN_T0BRUN | GPTU_T012RUN_T0CRUN;

	test_check("T0C overflow reaches BootROM SRC7", wait_source(gptu, 7));
	gptu_stop(gptu);
	test_eq_u32("only the routed BootROM source is pending", BIT(7), gptu_pending_sources(gptu));
	test_check("24-bit counter crossed its overflow boundary", GPTU_T0CBA(gptu->base) < 0x010000);
	gptu_clear_sources(gptu);
}

static void test_el71_t0_timer(const gptu_t *gptu) {
	gptu_stop(gptu);
	gptu_clear_sources(gptu);
	GPTU_T0RDCBA(gptu->base) = 0;
	GPTU_T0DCBA(gptu->base) = 0xFFFFFF00;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T0BRUN |
		GPTU_T012RUN_T0CRUN | GPTU_T012RUN_T0DRUN;

	test_check("T0D overflow reaches EL71 SRC7", wait_source(gptu, 7));
	gptu_stop(gptu);
	test_eq_u32("T0 stages A-C do not raise early requests", BIT(7), gptu_pending_sources(gptu));
	test_check("T0 crossed the full 32-bit boundary", GPTU_T0DCBA(gptu->base) < 0x00010000);
}

static void test_el71_t1_timer(const gptu_t *gptu) {
	gptu_stop(gptu);
	gptu_clear_sources(gptu);
	GPTU_T1RDCBA(gptu->base) = 0;
	GPTU_T1DCBA(gptu->base) = 0xFFFFFF00;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T1ARUN | GPTU_T012RUN_T1BRUN |
		GPTU_T012RUN_T1CRUN | GPTU_T012RUN_T1DRUN;

	test_check("T1D overflow reaches EL71 SRC6", wait_source(gptu, 6));
	gptu_stop(gptu);
	test_eq_u32("T1 stages A-C do not raise early requests", BIT(6), gptu_pending_sources(gptu));
	test_check("T1 crossed the full 32-bit boundary", GPTU_T1DCBA(gptu->base) < 0x00010000);
}

static void test_el71_t2_timer(const gptu_t *gptu) {
	gptu_stop(gptu);
	gptu_clear_sources(gptu);
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2ACOS | GPTU_T2CON_T2ACDIR_COUNT_DOWN;
	GPTU_T2RCCON(gptu->base) = 0;
	GPTU_T2(gptu->base) = 0x100;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2BSETR;

	test_check("T2 underflow reaches EL71 SRC5", wait_source(gptu, 5));
	test_check("EL71 combined one-shot stops", wait_t2_stopped(gptu, GPTU_T012RUN_T2ARUN));
	test_eq_u32("combined underflow raises only OUV_T2B source", BIT(5), gptu_pending_sources(gptu));
	test_eq_u32("combined countdown stops at underflow", 0xFFFFFFFF, GPTU_T2(gptu->base));
}

static void test_el71_system_timers(const gptu_t *gptu) {
	test_category("EL71 system timer sequence");
	gptu_enable(gptu, 1);
	configure_el71_system_timers(gptu);
	test_el71_t0_timer(gptu);
	test_el71_t1_timer(gptu);
	test_el71_t2_timer(gptu);
	gptu_clear_sources(gptu);
}

static void test_clc_retention(const gptu_t *gptu) {
	test_category("Firmware configuration retention across CLC disable");
	gptu_enable(gptu, GPTU_RMC_SL98_PULSE);
	gptu_stop(gptu);
	configure_el71_system_timers(gptu);
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2SPLIT | GPTU_T2CON_T2ACOS | GPTU_T2CON_T2BCOS;
	GPTU_T2RCCON(gptu->base) =
		(GPTU_T2_RC_RELOAD_DIRECTIONAL << GPTU_T2RCCON_T2AMRC0_SHIFT) |
		(GPTU_T2_RC_RELOAD_DIRECTIONAL << GPTU_T2RCCON_T2BMRC0_SHIFT);
	GPTU_T2AIS(gptu->base) = 0x60;
	GPTU_T2BIS(gptu->base) = 0;
	GPTU_T2ES(gptu->base) = 0x00040004;
	GPTU_OSEL(gptu->base) = GPTU_OSEL_SO0_OUV_T2A | GPTU_OSEL_SO6_OUV_T2B;
	GPTU_OUT(gptu->base) = 0xFF << 8;
	GPTU_OUT(gptu->base) = GPTU_OUT_SETO2 | GPTU_OUT_SETO5;
	GPTU_T0RDCBA(gptu->base) = 0x12345678;
	GPTU_T1RDCBA(gptu->base) = 0x89ABCDEF;
	GPTU_T2(gptu->base) = 0x13572468;
	GPTU_T2RC0(gptu->base) = 0x24681357;

	GPTU_CLC(gptu->base) = MOD_CLC_DISR;
	stopwatch_usleep_wd(100);
	gptu_enable(gptu, GPTU_RMC_SL98_PULSE);

	test_eq_u32("T01IRS survives CLC disable", 0x5077FCFC, GPTU_T01IRS(gptu->base));
	test_eq_u32("T01OTS survives CLC disable", 0x0FFF0FFF, GPTU_T01OTS(gptu->base));
	test_eq_u32("T2CON survives CLC disable",
		GPTU_T2CON_T2SPLIT | GPTU_T2CON_T2ACOS | GPTU_T2CON_T2BCOS, GPTU_T2CON(gptu->base));
	test_eq_u32("T2RCCON survives CLC disable", 0x00060006, GPTU_T2RCCON(gptu->base));
	test_eq_u32("T2AIS survives CLC disable", 0x00000060, GPTU_T2AIS(gptu->base));
	test_eq_u32("T2BIS survives CLC disable", 0, GPTU_T2BIS(gptu->base));
	test_eq_u32("T2ES survives CLC disable", 0x00040004, GPTU_T2ES(gptu->base));
	test_eq_u32("OSEL survives CLC disable", 0x05000004, GPTU_OSEL(gptu->base));
	test_eq_u32("OUT survives CLC disable", BIT(2) | BIT(5), GPTU_OUT(gptu->base));
	test_eq_u32("T0 reload survives CLC disable", 0x12345678, GPTU_T0RDCBA(gptu->base));
	test_eq_u32("T1 reload survives CLC disable", 0x89ABCDEF, GPTU_T1RDCBA(gptu->base));
	test_eq_u32("T2 count survives CLC disable", 0x13572468, GPTU_T2(gptu->base));
	test_eq_u32("T2 reload survives CLC disable", 0x24681357, GPTU_T2RC0(gptu->base));
	test_eq_u32("SRSEL survives CLC disable", 0x000007EC, GPTU_SRSEL(gptu->base));
}

static void test_sl98_split_pulse(const gptu_t *gptu) {
	test_category("SL98 split T2 pulse configuration");
	gptu_enable(gptu, GPTU_RMC_SL98_PULSE);
	gptu_stop(gptu);
	GPTU_OUT(gptu->base) = 0xFF << 8;
	GPTU_T2CON(gptu->base) = GPTU_T2CON_T2SPLIT | GPTU_T2CON_T2ACOS | GPTU_T2CON_T2BCOS;
	GPTU_T2RCCON(gptu->base) =
		(GPTU_T2_RC_RELOAD_DIRECTIONAL << GPTU_T2RCCON_T2AMRC0_SHIFT) |
		(GPTU_T2_RC_RELOAD_DIRECTIONAL << GPTU_T2RCCON_T2BMRC0_SHIFT);
	GPTU_OSEL(gptu->base) = GPTU_OSEL_SO0_OUV_T2A | GPTU_OSEL_SO6_OUV_T2B;
	GPTU_T2RC0(gptu->base) = 0x24681357;
	GPTU_T2(gptu->base) = 0xFFF0FFF8;
	GPTU_T012RUN(gptu->base) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2BSETR;

	test_check("both split one-shot timers complete", wait_t2_stopped(
		gptu, GPTU_T012RUN_T2ARUN | GPTU_T012RUN_T2BRUN
	));
	test_eq_u32("split timers use their directional reload halves", 0x24681357, GPTU_T2(gptu->base));
	test_eq_u32("OUV_T2A and OUV_T2B drive firmware outputs 0 and 6", BIT(0) | BIT(6), GPTU_OUT(gptu->base));
}

static void test_instance(const gptu_t *gptu) {
	test_category(gptu->name);
	test_bootrom_timeout(gptu);
	test_el71_system_timers(gptu);
	test_clc_retention(gptu);
	test_sl98_split_pulse(gptu);
	gptu_stop(gptu);
	gptu_clear_sources(gptu);
}

int main(void) {
	const gptu_t instances[] = {
		{ "GPTU0", GPTU0 },
		{ "GPTU1", GPTU1 },
	};

	test_start("GPTU firmware contract test");
	for (uint32_t index = 0; index < ARRAY_SIZE(instances); index++)
		test_instance(&instances[index]);

	return test_finish();
}
