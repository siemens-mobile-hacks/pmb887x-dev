#include <pmb887x.h>

#include "test.h"

#define CAPCOM_PWM_EVENT_COUNT 6
#define CAPCOM_PWM_TIMEOUT_MS 100

typedef struct {
	const char *name;
	uint32_t base;
	uint32_t timer_irq;
	uint32_t compare_irq;
} capcom_pwm_t;

typedef struct {
	stopwatch_t timestamp;
	uint32_t irq;
	uint32_t output;
} pwm_event_t;

static const capcom_pwm_t CAPCOMS[] = {
	{ "CAPCOM0 CC0IO", CAPCOM0, VIC_CAPCOM0_T0_IRQ, VIC_CAPCOM0_CC0_IRQ },
	{ "CAPCOM1 CC0IO", CAPCOM1, VIC_CAPCOM1_T0_IRQ, VIC_CAPCOM1_CC0_IRQ },
};

static const capcom_pwm_t *volatile active_capcom;
static volatile pwm_event_t pwm_events[CAPCOM_PWM_EVENT_COUNT];
static volatile size_t pwm_event_count;
static volatile uint32_t other_irq_count;

static void capcom_configure(const capcom_pwm_t *capcom) {
	CAPCOM_CLC(capcom->base) = 1 << MOD_CLC_RMC_SHIFT;
	CAPCOM_T01CON(capcom->base) = 0;
	CAPCOM_CCM0(capcom->base) = 0;
	CAPCOM_CCM1(capcom->base) = 0;
	CAPCOM_IOC(capcom->base) = 0;
	CAPCOM_SEM(capcom->base) = 0;
	CAPCOM_SEE(capcom->base) = 0;
	CAPCOM_DRM(capcom->base) = CAPCOM_DRM_DR0M_DIS | CAPCOM_DRM_DR1M_DIS |
		CAPCOM_DRM_DR2M_DIS | CAPCOM_DRM_DR3M_DIS;
	CAPCOM_OUT(capcom->base) = 0;
	CAPCOM_T0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;
}

static bool wait_events(void) {
	stopwatch_t start = stopwatch_get();

	while (pwm_event_count < CAPCOM_PWM_EVENT_COUNT &&
		stopwatch_elapsed_ms(start) < CAPCOM_PWM_TIMEOUT_MS) {
		test_watchdog_serve();
	}

	return pwm_event_count == CAPCOM_PWM_EVENT_COUNT;
}

static uint32_t event_interval_us(size_t first, size_t second) {
	stopwatch_t ticks = pwm_events[second].timestamp - pwm_events[first].timestamp;
	return (uint32_t) (ticks * 1000000ULL / stopwatch_ticks_per_s());
}

static void test_pwm(const capcom_pwm_t *capcom) {
	test_category(capcom->name);
	uint32_t saved_timer_vic = VIC_CON(capcom->timer_irq);
	uint32_t saved_compare_vic = VIC_CON(capcom->compare_irq);
	bool irq_was_disabled = cpu_enable_irq(false);

	capcom_configure(capcom);
	active_capcom = capcom;
	pwm_event_count = 0;
	other_irq_count = 0;
	CAPCOM_T0(capcom->base) = 0xC000;
	CAPCOM_T0REL(capcom->base) = 0xC000;
	CAPCOM_CC(capcom->base, 0) = 0xE000;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE3;
	CAPCOM_T0_SRC(capcom->base) = MOD_SRC_CLRR | MOD_SRC_SRE;
	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(capcom->timer_irq) = 1;
	VIC_CON(capcom->compare_irq) = 1;

	cpu_enable_irq(true);
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool completed = wait_events();

	cpu_enable_irq(false);
	CAPCOM_T01CON(capcom->base) = 0;
	CAPCOM_T0_SRC(capcom->base) = MOD_SRC_CLRR;
	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;

	VIC_CON(capcom->timer_irq) = saved_timer_vic;
	VIC_CON(capcom->compare_irq) = saved_compare_vic;
	cpu_enable_irq(!irq_was_disabled);

	test_check("PWM produces three compare/overflow cycles", completed);
	if (!completed)
		return;

	bool sequence_matches = true;
	bool output_matches = true;
	for (size_t index = 0; index < CAPCOM_PWM_EVENT_COUNT; index++) {
		uint32_t expected_irq = index % 2 == 0 ? capcom->compare_irq : capcom->timer_irq;
		uint32_t expected_output = index % 2 == 0 ? CAPCOM_OUT_O0 : 0;
		sequence_matches = sequence_matches && pwm_events[index].irq == expected_irq;
		output_matches = output_matches && pwm_events[index].output == expected_output;
	}
	test_check("PWM IRQs alternate compare then overflow", sequence_matches);
	test_check("mode 3 output is high between compare and overflow", output_matches);
	test_eq_u32("PWM raises no unrelated IRQ", 0, other_irq_count);

	uint32_t high_us = event_interval_us(2, 3);
	uint32_t low_us = event_interval_us(3, 4);
	uint32_t period_us = event_interval_us(2, 4);
	printf("# %s PWM after update: high=%lu us low=%lu us period=%lu us\n",
		capcom->name, high_us, low_us, period_us);
	test_check("updated PWM high phase is one quarter period",
		test_u32_in_interval(high_us, 1150, 1400));
	test_check("updated PWM low phase is three quarters period",
		test_u32_in_interval(low_us, 3500, 4050));
	test_check("updated PWM keeps the configured period",
		test_u32_in_interval(period_us, 4750, 5350));
}

int main(void) {
	test_start("CAPCOM PWM event test");
	for (size_t index = 0; index < ARRAY_SIZE(CAPCOMS); index++)
		test_pwm(&CAPCOMS[index]);

	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == active_capcom->compare_irq || irq == active_capcom->timer_irq) {
		if (pwm_event_count < ARRAY_SIZE(pwm_events)) {
			pwm_events[pwm_event_count].timestamp = stopwatch_get();
			pwm_events[pwm_event_count].irq = irq;
			pwm_events[pwm_event_count].output = CAPCOM_OUT(active_capcom->base) & CAPCOM_OUT_O0;
			pwm_event_count++;
		}
		if (irq == active_capcom->compare_irq) {
			CAPCOM_CC0_SRC(active_capcom->base) = MOD_SRC_CLRR | MOD_SRC_SRE;
		} else {
			CAPCOM_T0_SRC(active_capcom->base) = MOD_SRC_CLRR | MOD_SRC_SRE;
			if (pwm_event_count == 2)
				CAPCOM_CC(active_capcom->base, 0) = 0xF000;
		}
	} else {
		other_irq_count++;
	}

	VIC_IRQ_ACK = 1;
}
