#include <pmb887x.h>

#include "test.h"

#define CAPCOM_EVENT_TIMEOUT_MS 100
#define CAPCOM_NO_EVENT_US 1000

typedef struct {
	const char *name;
	uint32_t base;
} capcom_t;

static volatile uint32_t *capcom_cc_src(uint32_t base, uint32_t channel) {
	switch (channel) {
		case 0:
			return &CAPCOM_CC0_SRC(base);
		case 1:
			return &CAPCOM_CC1_SRC(base);
		case 2:
			return &CAPCOM_CC2_SRC(base);
		case 3:
			return &CAPCOM_CC3_SRC(base);
		case 4:
			return &CAPCOM_CC4_SRC(base);
		case 5:
			return &CAPCOM_CC5_SRC(base);
		case 6:
			return &CAPCOM_CC6_SRC(base);
		case 7:
			return &CAPCOM_CC7_SRC(base);
	}

	return NULL;
}

static void capcom_clear_sources(uint32_t base) {
	CAPCOM_T0_SRC(base) = MOD_SRC_CLRR;
	CAPCOM_T1_SRC(base) = MOD_SRC_CLRR;
	for (uint32_t channel = 0; channel < 8; channel++)
		*capcom_cc_src(base, channel) = MOD_SRC_CLRR;
}

static void capcom_configure(uint32_t base) {
	CAPCOM_CLC(base) = (1 << MOD_CLC_RMC_SHIFT);
	CAPCOM_T01CON(base) = 0;
	CAPCOM_PISEL(base) = 0;
	CAPCOM_CCM0(base) = 0;
	CAPCOM_CCM1(base) = 0;
	CAPCOM_IOC(base) = 0;
	CAPCOM_SEM(base) = 0;
	CAPCOM_SEE(base) = 0;
	CAPCOM_DRM(base) = CAPCOM_DRM_DR0M_DIS | CAPCOM_DRM_DR1M_DIS |
		CAPCOM_DRM_DR2M_DIS | CAPCOM_DRM_DR3M_DIS;
	CAPCOM_OUT(base) = 0;
	capcom_clear_sources(base);
}

static bool wait_request(volatile uint32_t *src) {
	stopwatch_t start = stopwatch_get();

	while ((*src & MOD_SRC_SRR) == 0 && stopwatch_elapsed_ms(start) < CAPCOM_EVENT_TIMEOUT_MS)
		test_watchdog_serve();

	return (*src & MOD_SRC_SRR) != 0;
}

static bool wait_timer_at_least(uint32_t base, uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (CAPCOM_T0(base) < value && stopwatch_elapsed_ms(start) < CAPCOM_EVENT_TIMEOUT_MS)
		test_watchdog_serve();

	return CAPCOM_T0(base) >= value;
}

static bool request_stays_clear(volatile uint32_t *src) {
	stopwatch_usleep_wd(CAPCOM_NO_EVENT_US);

	return (*src & MOD_SRC_SRR) == 0;
}

static void test_compare_mode0(const capcom_t *capcom) {
	test_category("Compare mode 0");
	capcom_configure(capcom->base);
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool first = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	bool timer_advanced = wait_timer_at_least(capcom->base, 0x110);
	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC(capcom->base, 0) = 0x4000;
	bool second = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t output = CAPCOM_OUT(capcom->base);

	test_check("mode 0 accepts another compare value in the same timer period",
		first && timer_advanced && second);
	test_eq_u32("mode 0 does not change the output latch", 0, output & CAPCOM_OUT_O0);
}

static void test_compare_mode1(const capcom_t *capcom) {
	test_category("Compare mode 1");
	capcom_configure(capcom->base);
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE1;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool first = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	uint32_t first_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;
	bool timer_advanced = wait_timer_at_least(capcom->base, 0x110);
	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC(capcom->base, 0) = 0x4000;
	bool second = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t second_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;

	test_check("mode 1 accepts another compare value in the same timer period",
		first && timer_advanced && second);
	test_eq_u32("first mode 1 match toggles the output high", CAPCOM_OUT_O0, first_output);
	test_eq_u32("second mode 1 match toggles the output low", 0, second_output);
}

static void test_compare_mode2(const capcom_t *capcom) {
	test_category("Compare mode 2");
	capcom_configure(capcom->base);
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE2;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool first = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC(capcom->base, 0) = 0x300;
	bool passed_second_value = wait_timer_at_least(capcom->base, 0x400);
	bool suppressed = (CAPCOM_CC0_SRC(capcom->base) & MOD_SRC_SRR) == 0;
	bool next_period = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;

	test_check("mode 2 generates its first match", first);
	test_check("mode 2 suppresses a second match in the same period", passed_second_value && suppressed);
	test_check("mode 2 rearms after timer overflow", next_period);
	test_eq_u32("mode 2 does not change the output latch", 0, CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0);
}

static void test_compare_mode3(const capcom_t *capcom) {
	test_category("Compare mode 3");
	capcom_configure(capcom->base);
	CAPCOM_T0(capcom->base) = 0xE000;
	CAPCOM_T0REL(capcom->base) = 0xE000;
	CAPCOM_CC(capcom->base, 0) = 0xE100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE3;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool matched = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	uint32_t matched_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;
	bool overflowed = wait_request(&CAPCOM_T0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t overflow_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;

	test_check("mode 3 match requests service", matched);
	test_eq_u32("mode 3 match sets the output latch", CAPCOM_OUT_O0, matched_output);
	test_check("mode 3 period ends with a timer overflow request", overflowed);
	test_eq_u32("timer overflow clears the mode 3 output latch", 0, overflow_output);

	capcom_configure(capcom->base);
	CAPCOM_T0(capcom->base) = 0xF100;
	CAPCOM_T0REL(capcom->base) = 0xF000;
	CAPCOM_CC(capcom->base, 0) = 0xF000;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE3;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool reload_match = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;

	test_check("mode 3 compare equal to reload still requests service", reload_match);
	test_eq_u32("mode 3 compare equal to reload leaves output unchanged", 0,
		CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0);
}

static void test_timer_allocation(const capcom_t *capcom) {
	test_category("Compare timer allocation");
	capcom_configure(capcom->base);
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T1(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_T1REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 1) = 0x100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD1_MODE0 | CAPCOM_CCM0_ACC1_T1;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool t0_did_not_match = request_stays_clear(&CAPCOM_CC1_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T1R_ENABLED;
	bool t1_matched = wait_request(&CAPCOM_CC1_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;

	test_check("T1-allocated channel ignores T0", t0_did_not_match);
	test_check("T1-allocated channel matches T1", t1_matched);
}

static void test_double_register(const capcom_t *capcom) {
	test_category("Double-register compare");
	capcom_configure(capcom->base);
	CAPCOM_DRM(capcom->base) = CAPCOM_DRM_DR0M_CON | CAPCOM_DRM_DR1M_DIS |
		CAPCOM_DRM_DR2M_DIS | CAPCOM_DRM_DR3M_DIS;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T1(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_T1REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CC(capcom->base, 4) = 0x200;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE1;
	CAPCOM_CCM1(capcom->base) = CAPCOM_CCM1_MOD4_MODE0 | CAPCOM_CCM1_ACC4_T1;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED | CAPCOM_T01CON_T1R_ENABLED;
	bool first = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	uint32_t first_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;
	bool second = wait_request(&CAPCOM_CC4_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t second_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;

	test_check("double-register pair spans independently allocated timers", first && second);
	test_eq_u32("bank 1 match toggles the paired output high", CAPCOM_OUT_O0, first_output);
	test_eq_u32("bank 2 match toggles the paired output low", 0, second_output);

	capcom_configure(capcom->base);
	CAPCOM_DRM(capcom->base) = CAPCOM_DRM_DR0M_CON | CAPCOM_DRM_DR1M_DIS |
		CAPCOM_DRM_DR2M_DIS | CAPCOM_DRM_DR3M_DIS;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CC(capcom->base, 4) = 0x100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE1;
	CAPCOM_CCM1(capcom->base) = CAPCOM_CCM1_MOD4_MODE0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool bank1 = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	bool bank2 = wait_request(&CAPCOM_CC4_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;

	test_check("simultaneous pair match requests both services", bank1 && bank2);
	test_eq_u32("simultaneous pair match toggles the output only once", CAPCOM_OUT_O0,
		CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0);

	capcom_configure(capcom->base);
	CAPCOM_DRM(capcom->base) = CAPCOM_DRM_DR0M_DIS | CAPCOM_DRM_DR1M_DIS |
		CAPCOM_DRM_DR2M_DIS | CAPCOM_DRM_DR3M_DIS;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CC(capcom->base, 4) = 0x200;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE1;
	CAPCOM_CCM1(capcom->base) = CAPCOM_CCM1_MOD4_MODE0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool disabled_first = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	uint32_t disabled_first_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;
	bool disabled_second = wait_request(&CAPCOM_CC4_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t disabled_second_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;

	test_check("forced-disabled pair still requests both services", disabled_first && disabled_second);
	test_eq_u32("forced-disabled bank 1 keeps its mode 1 behavior", CAPCOM_OUT_O0,
		disabled_first_output);
	test_eq_u32("forced-disabled bank 2 does not toggle the paired output", CAPCOM_OUT_O0,
		disabled_second_output);

	capcom_configure(capcom->base);
	CAPCOM_DRM(capcom->base) = CAPCOM_DRM_DR0M_EN | CAPCOM_DRM_DR1M_DIS |
		CAPCOM_DRM_DR2M_DIS | CAPCOM_DRM_DR3M_DIS;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CC(capcom->base, 4) = 0x200;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE0;
	CAPCOM_CCM1(capcom->base) = CAPCOM_CCM1_MOD4_MODE0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool enabled_first = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	uint32_t enabled_first_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;
	bool enabled_second = wait_request(&CAPCOM_CC4_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t enabled_second_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;

	test_check("forced-enabled pair requests both services", enabled_first && enabled_second);
	test_eq_u32("forced-enabled bank 1 toggles the paired output", CAPCOM_OUT_O0,
		enabled_first_output);
	test_eq_u32("forced-enabled bank 2 toggles the paired output", 0, enabled_second_output);
}

static void test_single_event(const capcom_t *capcom) {
	test_category("Single-event compare");
	capcom_configure(capcom->base);
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE1;
	CAPCOM_SEM(capcom->base) = CAPCOM_SEM_SEM0;
	CAPCOM_SEE(capcom->base) = CAPCOM_SEE_SEE0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool first = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t armed_after_event = CAPCOM_SEE(capcom->base) & CAPCOM_SEE_SEE0;
	uint32_t first_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;

	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool remained_disarmed = request_stays_clear(&CAPCOM_CC0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t disarmed_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;

	CAPCOM_SEE(capcom->base) = CAPCOM_SEE_SEE0;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool rearmed = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t rearmed_output = CAPCOM_OUT(capcom->base) & CAPCOM_OUT_O0;

	test_check("armed single-event channel generates one request", first);
	test_eq_u32("armed single event toggles the output", CAPCOM_OUT_O0, first_output);
	test_eq_u32("hardware clears SEE after the event", 0, armed_after_event);
	test_check("single-event channel remains silent until rearmed", remained_disarmed);
	test_eq_u32("disarmed single event preserves the output", CAPCOM_OUT_O0, disarmed_output);
	test_check("SEE rearms the next single event", rearmed);
	test_eq_u32("rearmed single event toggles the output again", 0, rearmed_output);
}

static void test_two_channel_pwm(const capcom_t *capcom) {
	test_category("Two-channel mode 3 waveform update");
	capcom_configure(capcom->base);
	CAPCOM_T0(capcom->base) = 0xE000;
	CAPCOM_T0REL(capcom->base) = 0xE000;
	CAPCOM_CC(capcom->base, 0) = 0xE100;
	CAPCOM_CC(capcom->base, 4) = 0xE100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE3;
	CAPCOM_CCM1(capcom->base) = CAPCOM_CCM1_MOD4_MODE3;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool first_channel = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	bool second_channel = wait_request(&CAPCOM_CC4_SRC(capcom->base));
	uint32_t active_outputs = CAPCOM_OUT(capcom->base) & (CAPCOM_OUT_O0 | CAPCOM_OUT_O4);
	bool overflowed = wait_request(&CAPCOM_T0_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t inactive_outputs = CAPCOM_OUT(capcom->base) & (CAPCOM_OUT_O0 | CAPCOM_OUT_O4);

	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC4_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC(capcom->base, 0) = 0xE080;
	CAPCOM_CC(capcom->base, 4) = 0xE180;
	CAPCOM_T0(capcom->base) = 0xE000;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool updated_first = wait_request(&CAPCOM_CC0_SRC(capcom->base));
	bool updated_second = wait_request(&CAPCOM_CC4_SRC(capcom->base));
	CAPCOM_T01CON(capcom->base) = 0;

	test_check("simultaneous mode 3 match requests both services", first_channel && second_channel);
	test_eq_u32("both mode 3 outputs stay active until overflow", CAPCOM_OUT_O0 | CAPCOM_OUT_O4,
		active_outputs);
	test_check("two-channel mode 3 period reaches overflow", overflowed);
	test_eq_u32("overflow clears both mode 3 outputs", 0, inactive_outputs);
	test_check("updated compare values take effect in the next period", updated_first && updated_second);
}

static void test_instance(const capcom_t *capcom) {
	test_category(capcom->name);
	test_compare_mode0(capcom);
	test_compare_mode1(capcom);
	test_compare_mode2(capcom);
	test_compare_mode3(capcom);
	test_timer_allocation(capcom);
	test_double_register(capcom);
	test_single_event(capcom);
	test_two_channel_pwm(capcom);
}

int main(void) {
	const capcom_t capcoms[] = {
		{ "CAPCOM0", CAPCOM0 },
		{ "CAPCOM1", CAPCOM1 },
	};

	test_start("CAPCOM compare test");
	for (uint32_t index = 0; index < ARRAY_SIZE(capcoms); index++)
		test_instance(&capcoms[index]);

	return test_finish();
}
