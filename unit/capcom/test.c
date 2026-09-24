#include <pmb887x.h>

#include "test.h"

#define CAPCOM_EVENT_TIMEOUT_MS 100

typedef struct {
	const char *name;
	uint32_t base;
} capcom_t;

typedef struct {
	uint32_t t0;
	uint32_t t1;
} timer_counts_t;

static const capcom_t CAPCOMS[] = {
	{ "CAPCOM0", CAPCOM0 },
	{ "CAPCOM1", CAPCOM1 },
};

static void capcom_clear_sources(const capcom_t *capcom) {
	CAPCOM_T0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_T1_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC1_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC2_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC3_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC4_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC5_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC6_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC7_SRC(capcom->base) = MOD_SRC_CLRR;
}

static void capcom_configure(const capcom_t *capcom) {
	CAPCOM_CLC(capcom->base) = (1 << MOD_CLC_RMC_SHIFT);
	CAPCOM_T01CON(capcom->base) = 0;
	CAPCOM_PISEL(capcom->base) = 0;
	CAPCOM_CCM0(capcom->base) = 0;
	CAPCOM_CCM1(capcom->base) = 0;
	CAPCOM_IOC(capcom->base) = 0;
	CAPCOM_SEM(capcom->base) = 0;
	CAPCOM_SEE(capcom->base) = 0;
	CAPCOM_DRM(capcom->base) = CAPCOM_DRM_DR0M_DIS | CAPCOM_DRM_DR1M_DIS |
		CAPCOM_DRM_DR2M_DIS | CAPCOM_DRM_DR3M_DIS;
	capcom_clear_sources(capcom);
}

static void test_reset_values(void) {
	test_category("Reset values");
	const capcom_t *capcom = NULL;
	for (uint32_t index = 0; index < ARRAY_SIZE(CAPCOMS); index++) {
		if (CAPCOM_CLC(CAPCOMS[index].base) == (MOD_CLC_DISR | MOD_CLC_DISS)) {
			capcom = &CAPCOMS[index];
			break;
		}
	}
	if (capcom == NULL) {
		test_skip("register reset values", "boot firmware enabled both CAPCOM instances");
		return;
	}

	printf("# using untouched %s\n", capcom->name);
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, CAPCOM_CLC(capcom->base));
	CAPCOM_CLC(capcom->base) = (1 << MOD_CLC_RMC_SHIFT);
	test_eq_u32("PISEL reset value", 0, CAPCOM_PISEL(capcom->base));
	test_eq_u32("T01CON reset value", 0, CAPCOM_T01CON(capcom->base));
	test_eq_u32("CCM0 reset value", 0, CAPCOM_CCM0(capcom->base));
	test_eq_u32("CCM1 reset value", 0, CAPCOM_CCM1(capcom->base));
	test_eq_u32("OUT reset value", 0, CAPCOM_OUT(capcom->base));
	/* IOC.ORSEL is a read-only implementation option fixed to direct I/O. */
	test_eq_u32("IOC reset value", BIT(0), CAPCOM_IOC(capcom->base));
	test_eq_u32("SEM reset value", 0, CAPCOM_SEM(capcom->base));
	test_eq_u32("SEE reset value", 0, CAPCOM_SEE(capcom->base));
	test_eq_u32("DRM reset value", 0, CAPCOM_DRM(capcom->base));
	test_eq_u32("T0 reset value", 0, CAPCOM_T0(capcom->base));
	test_eq_u32("T0REL reset value", 0, CAPCOM_T0REL(capcom->base));
	test_eq_u32("T1 reset value", 0, CAPCOM_T1(capcom->base));
	test_eq_u32("T1REL reset value", 0, CAPCOM_T1REL(capcom->base));
	for (uint32_t channel = 0; channel < 8; channel++)
		test_eq_u32("CC reset value", 0, CAPCOM_CC(capcom->base, channel));
}

static void test_registers(const capcom_t *capcom) {
	test_category(capcom->name);
	capcom_configure(capcom);
	test_module_id("module ID", 0x00005000, CAPCOM_ID(capcom->base));
	test_module_clock("module clock is enabled", CAPCOM_CLC(capcom->base));

	CAPCOM_T0(capcom->base) = 0x12345678;
	CAPCOM_T0REL(capcom->base) = 0x89ABCDEF;
	CAPCOM_T1(capcom->base) = 0x76543210;
	CAPCOM_T1REL(capcom->base) = 0xFEDCBA98;
	test_eq_u32("T0 stores 16 bits", 0x5678, CAPCOM_T0(capcom->base));
	test_eq_u32("T0REL stores 16 bits", 0xCDEF, CAPCOM_T0REL(capcom->base));
	test_eq_u32("T1 stores 16 bits", 0x3210, CAPCOM_T1(capcom->base));
	test_eq_u32("T1REL stores 16 bits", 0xBA98, CAPCOM_T1REL(capcom->base));

	for (uint32_t channel = 0; channel < 8; channel++)
		CAPCOM_CC(capcom->base, channel) = 0x12340000 | (0x5500 + channel);
	bool cc_values_match = true;
	for (uint32_t channel = 0; channel < 8; channel++)
		cc_values_match = cc_values_match && CAPCOM_CC(capcom->base, channel) == 0x5500 + channel;
	test_check("disabled CC channels store independent 16-bit values", cc_values_match);

	CAPCOM_OUT(capcom->base) = CAPCOM_OUT_O0 | CAPCOM_OUT_O2 | CAPCOM_OUT_O4 | CAPCOM_OUT_O6;
	CAPCOM_WHBSOUT(capcom->base) = CAPCOM_WHBSOUT_SET1O | CAPCOM_WHBSOUT_SET7O;
	test_eq_u32("WHBSOUT sets selected output latches", 0xD7, CAPCOM_OUT(capcom->base));
	CAPCOM_WHBCOUT(capcom->base) = CAPCOM_WHBCOUT_CLR2O | CAPCOM_WHBCOUT_CLR4O;
	test_eq_u32("WHBCOUT clears selected output latches", 0xC3, CAPCOM_OUT(capcom->base));

	CAPCOM_SEE(capcom->base) = CAPCOM_SEE_SEE1 | CAPCOM_SEE_SEE6;
	test_eq_u32("SEE arms selected channels", CAPCOM_SEE_SEE1 | CAPCOM_SEE_SEE6,
		CAPCOM_SEE(capcom->base));
	CAPCOM_SEE(capcom->base) = CAPCOM_SEE_SEE6;
	test_eq_u32("SEE disarms selected channels", CAPCOM_SEE_SEE6, CAPCOM_SEE(capcom->base));
}

static void test_run_control(const capcom_t *capcom) {
	test_category("Timer run control");
	capcom_configure(capcom);
	CAPCOM_T0(capcom->base) = 0x1000;
	CAPCOM_T1(capcom->base) = 0x2000;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED | CAPCOM_T01CON_T1R_ENABLED;
	stopwatch_usleep_wd(1000);
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t stopped_t0 = CAPCOM_T0(capcom->base);
	uint32_t stopped_t1 = CAPCOM_T1(capcom->base);
	stopwatch_usleep_wd(1000);

	test_check("T0 and T1 advance while enabled", stopped_t0 > 0x1000 && stopped_t1 > 0x2000);
	test_eq_u32("T0 stops when T0R is cleared", stopped_t0, CAPCOM_T0(capcom->base));
	test_eq_u32("T1 stops when T1R is cleared", stopped_t1, CAPCOM_T1(capcom->base));
}

static bool wait_timer_request(volatile uint32_t *src) {
	stopwatch_t start = stopwatch_get();

	while ((*src & MOD_SRC_SRR) == 0 && stopwatch_elapsed_ms(start) < CAPCOM_EVENT_TIMEOUT_MS)
		test_watchdog_serve();

	return (*src & MOD_SRC_SRR) != 0;
}

static void test_reload(const capcom_t *capcom) {
	test_category("Timer overflow and reload");
	capcom_configure(capcom);
	CAPCOM_T0REL(capcom->base) = 0x8000;
	CAPCOM_T1REL(capcom->base) = 0x9000;
	CAPCOM_T0(capcom->base) = 0xFF00;
	CAPCOM_T1(capcom->base) = 0xFF00;
	CAPCOM_T0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_T1_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED | CAPCOM_T01CON_T1R_ENABLED;
	bool t0_requested = wait_timer_request(&CAPCOM_T0_SRC(capcom->base));
	bool t1_requested = wait_timer_request(&CAPCOM_T1_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t t0 = CAPCOM_T0(capcom->base);
	uint32_t t1 = CAPCOM_T1(capcom->base);

	test_check("T0 and T1 overflow requests are independent", t0_requested && t1_requested);
	test_check("T0 reloads from T0REL", t0 >= 0x8000 && t0 < 0x9000);
	test_check("T1 reloads from T1REL", t1 >= 0x9000 && t1 < 0xA000);
}

static uint32_t measure_timer(const capcom_t *capcom, uint32_t t0i, uint32_t ioc) {
	capcom_configure(capcom);
	CAPCOM_IOC(capcom->base) = ioc;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	test_rtc_init();
	CAPCOM_T01CON(capcom->base) = t0i | CAPCOM_T01CON_T0R_ENABLED;
	uint32_t previous = 0;
	uint32_t wraps = 0;
	while (!test_rtc_second_elapsed()) {
		uint32_t current = CAPCOM_T0(capcom->base);
		wraps += current < previous;
		previous = current;
	}
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t current = CAPCOM_T0(capcom->base);
	wraps += current < previous;

	return (wraps << 16) | current;
}

static timer_counts_t measure_timer_pair(const capcom_t *capcom, uint32_t timer_inputs) {
	capcom_configure(capcom);
	CAPCOM_IOC(capcom->base) = 0;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T1(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_T1REL(capcom->base) = 0;
	test_rtc_init();
	CAPCOM_T01CON(capcom->base) = timer_inputs |
		CAPCOM_T01CON_T0R_ENABLED | CAPCOM_T01CON_T1R_ENABLED;
	uint32_t previous_t0 = 0;
	uint32_t previous_t1 = 0;
	uint32_t wraps_t0 = 0;
	uint32_t wraps_t1 = 0;
	while (!test_rtc_second_elapsed()) {
		uint32_t t0 = CAPCOM_T0(capcom->base);
		uint32_t t1 = CAPCOM_T1(capcom->base);
		wraps_t0 += t0 < previous_t0;
		wraps_t1 += t1 < previous_t1;
		previous_t0 = t0;
		previous_t1 = t1;
	}
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t t0 = CAPCOM_T0(capcom->base);
	uint32_t t1 = CAPCOM_T1(capcom->base);
	wraps_t0 += t0 < previous_t0;
	wraps_t1 += t1 < previous_t1;

	timer_counts_t counts = {
		(wraps_t0 << 16) | t0,
		(wraps_t1 << 16) | t1,
	};
	return counts;
}

static bool ratio_matches(uint32_t faster, uint32_t slower, uint32_t ratio) {
	uint32_t expected = slower * ratio;

	return test_u32_in_interval(faster, expected * 97 / 100, expected * 103 / 100);
}

static void test_timer_prescaler(const capcom_t *capcom) {
	test_category("Timer prescaler and stagger mode");
	uint32_t stagger_counts[8];
	for (uint32_t t0i = 0; t0i < ARRAY_SIZE(stagger_counts); t0i++) {
		stagger_counts[t0i] = measure_timer(capcom, t0i, 0);
		printf("# %s timer counts/s: stagger TxI%lu=%lu\n", capcom->name, t0i, stagger_counts[t0i]);
	}
	uint32_t direct_t0i0 = measure_timer(capcom, 0, CAPCOM_IOC_STAG);
	printf("# %s timer counts/s: direct TxI0=%lu\n", capcom->name, direct_t0i0);
	timer_counts_t slow_t1 = measure_timer_pair(capcom, (3 << CAPCOM_T01CON_T1I_SHIFT));
	timer_counts_t slow_t0 = measure_timer_pair(capcom, 3);
	printf("# %s independent prescalers: T0i0=%lu T1i3=%lu; T0i3=%lu T1i0=%lu\n",
		capcom->name, slow_t1.t0, slow_t1.t1, slow_t0.t0, slow_t0.t1);

	for (uint32_t t0i = 1; t0i < ARRAY_SIZE(stagger_counts); t0i++)
		test_check("each TxI step divides the timer clock by two",
			ratio_matches(stagger_counts[t0i - 1], stagger_counts[t0i], 2));
	test_check("disabling stagger multiplies the timer clock by eight",
		ratio_matches(direct_t0i0, stagger_counts[0], 8));
	test_check("T1 prescaler does not affect T0",
		ratio_matches(slow_t1.t0, slow_t1.t1, 8));
	test_check("T0 prescaler does not affect T1",
		ratio_matches(slow_t0.t1, slow_t0.t0, 8));
}

int main(void) {
	test_start("CAPCOM peripheral test");
	test_reset_values();
	for (uint32_t index = 0; index < ARRAY_SIZE(CAPCOMS); index++) {
		test_registers(&CAPCOMS[index]);
		test_run_control(&CAPCOMS[index]);
		test_reload(&CAPCOMS[index]);
		test_timer_prescaler(&CAPCOMS[index]);
	}

	return test_finish();
}
