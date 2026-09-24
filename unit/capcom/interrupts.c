#include <pmb887x.h>

#include "test.h"

#define CAPCOM_IRQ_TIMEOUT_MS 100

typedef struct {
	const char *name;
	uint32_t base;
	uint32_t t0_irq;
	uint32_t t1_irq;
	uint32_t compare_irq;
	uint32_t capture_irq;
	uint32_t capture_gpio;
	uint32_t capture_channel;
	uint32_t capture_mode;
} capcom_t;

static volatile uint32_t *active_src;
static volatile uint32_t active_base;
static volatile uint32_t active_irq;
static volatile uint32_t irq_count;
static volatile uint32_t other_irq_count;
static volatile bool reprogram_timer;
static volatile uint32_t irq_elapsed[2];
static volatile uint32_t timer_start;

static volatile uint32_t *capcom_capture_src(const capcom_t *capcom) {
	switch (capcom->capture_channel) {
		case 5:
			return &CAPCOM_CC5_SRC(capcom->base);
		case 6:
			return &CAPCOM_CC6_SRC(capcom->base);
	}

	return NULL;
}

static void gpio_set_capture_level(const capcom_t *capcom, bool high) {
	GPIO_PIN(capcom->capture_gpio) = GPIO_IS_ALT2 | GPIO_PS_MANUAL |
		(high ? GPIO_DATA_HIGH : GPIO_DATA_LOW) | GPIO_DIR_OUT |
		GPIO_PPEN_PUSHPULL | GPIO_PDPU_NONE | GPIO_ENAQ_OUTPUT_ENABLED;
	test_spin(1000);
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

static bool wait_irqs(uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while (irq_count < expected && stopwatch_elapsed_ms(start) < CAPCOM_IRQ_TIMEOUT_MS)
		test_watchdog_serve();

	return irq_count == expected;
}

static void configure_irq(volatile uint32_t *src, uint32_t irq) {
	active_src = src;
	active_irq = irq;
	irq_count = 0;
	other_irq_count = 0;
	*src = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(irq) = 1;
}

static void test_reprogrammed_timer_irq(const capcom_t *capcom) {
	test_category("Reprogrammed timer interrupt");
	uint32_t saved_vic = VIC_CON(capcom->t0_irq);
	bool irq_was_disabled = cpu_enable_irq(false);
	capcom_configure(capcom);
	configure_irq(&CAPCOM_T0_SRC(capcom->base), capcom->t0_irq);
	active_base = capcom->base;
	reprogram_timer = true;
	CAPCOM_T0REL(capcom->base) = 0xFF00;
	CAPCOM_T0(capcom->base) = 0xFF00;
	timer_start = STM_TIM0;
	cpu_enable_irq(true);
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool completed = wait_irqs(2);
	cpu_enable_irq(false);
	CAPCOM_T01CON(capcom->base) = 0;
	CAPCOM_T0_SRC(capcom->base) = MOD_SRC_CLRR;
	VIC_CON(capcom->t0_irq) = saved_vic;
	cpu_enable_irq(!irq_was_disabled);
	uint32_t second_interval = completed ? irq_elapsed[1] - irq_elapsed[0] : 0;

	printf("# %s timer IRQ ticks: first=%lu second=%lu\n", capcom->name,
		irq_elapsed[0], irq_elapsed[1]);
	test_check("overflow handler receives two timer IRQs", completed);
	test_eq_u32("timer IRQ routing raises no other vector", 0, other_irq_count);
	test_check("handler-programmed longer deadline delays the second IRQ",
		second_interval > irq_elapsed[0]);
	test_eq_u32("handler clears the final timer request", 0,
		CAPCOM_T0_SRC(capcom->base) & MOD_SRC_SRR);
}

static void test_t1_irq(const capcom_t *capcom) {
	test_category("T1 interrupt");
	uint32_t saved_vic = VIC_CON(capcom->t1_irq);
	bool irq_was_disabled = cpu_enable_irq(false);
	capcom_configure(capcom);
	configure_irq(&CAPCOM_T1_SRC(capcom->base), capcom->t1_irq);
	active_base = capcom->base;
	reprogram_timer = false;
	CAPCOM_T1REL(capcom->base) = 0xFF00;
	CAPCOM_T1(capcom->base) = 0xFF00;
	cpu_enable_irq(true);
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T1R_ENABLED;
	bool completed = wait_irqs(1);
	cpu_enable_irq(false);
	CAPCOM_T01CON(capcom->base) = 0;
	CAPCOM_T1_SRC(capcom->base) = MOD_SRC_CLRR;
	VIC_CON(capcom->t1_irq) = saved_vic;
	cpu_enable_irq(!irq_was_disabled);

	test_check("T1 overflow reaches its VIC vector", completed);
	test_eq_u32("T1 IRQ routing raises no other vector", 0, other_irq_count);
	test_eq_u32("handler clears the T1 request", 0,
		CAPCOM_T1_SRC(capcom->base) & MOD_SRC_SRR);
}

static void test_compare_irq(const capcom_t *capcom) {
	test_category("Compare interrupt");
	uint32_t saved_vic = VIC_CON(capcom->compare_irq);
	bool irq_was_disabled = cpu_enable_irq(false);
	capcom_configure(capcom);
	configure_irq(&CAPCOM_CC0_SRC(capcom->base), capcom->compare_irq);
	active_base = capcom->base;
	reprogram_timer = false;
	CAPCOM_T0(capcom->base) = 0;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_CC(capcom->base, 0) = 0x100;
	CAPCOM_CCM0(capcom->base) = CAPCOM_CCM0_MOD0_MODE0;
	cpu_enable_irq(true);
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	bool completed = wait_irqs(1);
	cpu_enable_irq(false);
	CAPCOM_T01CON(capcom->base) = 0;
	CAPCOM_CC0_SRC(capcom->base) = MOD_SRC_CLRR;
	VIC_CON(capcom->compare_irq) = saved_vic;
	cpu_enable_irq(!irq_was_disabled);

	test_check("compare match reaches its VIC vector", completed);
	test_eq_u32("compare IRQ routing raises no other vector", 0, other_irq_count);
	test_eq_u32("handler clears the compare request", 0,
		CAPCOM_CC0_SRC(capcom->base) & MOD_SRC_SRR);
}

static void test_capture_irq(const capcom_t *capcom) {
	test_category("Capture interrupt");
	volatile uint32_t *src = capcom_capture_src(capcom);
	uint32_t saved_gpio = GPIO_PIN(capcom->capture_gpio);
	uint32_t saved_vic = VIC_CON(capcom->capture_irq);
	bool irq_was_disabled = cpu_enable_irq(false);
	capcom_configure(capcom);
	gpio_set_capture_level(capcom, false);
	CAPCOM_CCM1(capcom->base) = capcom->capture_mode;
	configure_irq(src, capcom->capture_irq);
	active_base = capcom->base;
	reprogram_timer = false;
	CAPCOM_T0(capcom->base) = 0x1000;
	CAPCOM_T0REL(capcom->base) = 0;
	cpu_enable_irq(true);
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
	gpio_set_capture_level(capcom, true);
	bool completed = wait_irqs(1);
	cpu_enable_irq(false);
	CAPCOM_T01CON(capcom->base) = 0;
	uint32_t captured = CAPCOM_CC(capcom->base, capcom->capture_channel);
	uint32_t stopped_timer = CAPCOM_T0(capcom->base);
	*src = MOD_SRC_CLRR;
	VIC_CON(capcom->capture_irq) = saved_vic;
	GPIO_PIN(capcom->capture_gpio) = saved_gpio;
	cpu_enable_irq(!irq_was_disabled);

	test_check("capture event reaches its VIC vector", completed);
	test_eq_u32("capture IRQ routing raises no other vector", 0, other_irq_count);
	test_check("capture IRQ records the triggering timer value",
		captured >= 0x1000 && captured <= stopped_timer);
	test_eq_u32("handler clears the capture request", 0, *src & MOD_SRC_SRR);
}

int main(void) {
	const capcom_t capcoms[] = {
#ifdef PMB8875
		{
			"CAPCOM0", CAPCOM0,
			VIC_CAPCOM0_T0_IRQ, VIC_CAPCOM0_T1_IRQ,
			VIC_CAPCOM0_CC0_IRQ, VIC_CAPCOM0_CC6_IRQ,
			GPIO_USART0_RTS, 6, CAPCOM_CCM1_MOD6_RISING_EDGE,
		},
#else
		{
			"CAPCOM0", CAPCOM0,
			VIC_CAPCOM0_T0_IRQ, VIC_CAPCOM0_T1_IRQ,
			VIC_CAPCOM0_CC0_IRQ, VIC_CAPCOM0_CC5_IRQ,
			GPIO_CLKOUT0, 5, CAPCOM_CCM1_MOD5_RISING_EDGE,
		},
#endif
		{
			"CAPCOM1", CAPCOM1,
			VIC_CAPCOM1_T0_IRQ, VIC_CAPCOM1_T1_IRQ,
			VIC_CAPCOM1_CC0_IRQ, VIC_CAPCOM1_CC6_IRQ,
			GPIO_DSPOUT0, 6, CAPCOM_CCM1_MOD6_RISING_EDGE,
		},
	};

	test_start("CAPCOM interrupt test");
	for (uint32_t index = 0; index < ARRAY_SIZE(capcoms); index++) {
		test_category(capcoms[index].name);
		test_reprogrammed_timer_irq(&capcoms[index]);
		test_t1_irq(&capcoms[index]);
		test_compare_irq(&capcoms[index]);
		test_capture_irq(&capcoms[index]);
	}

	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == active_irq) {
		if (irq_count < ARRAY_SIZE(irq_elapsed))
			irq_elapsed[irq_count] = STM_TIM0 - timer_start;
		irq_count++;
		CAPCOM_T01CON(active_base) = 0;
		*active_src = MOD_SRC_CLRR | MOD_SRC_SRE;
		if (reprogram_timer && irq_count == 1) {
			CAPCOM_T0REL(active_base) = 0xFE00;
			CAPCOM_T0(active_base) = 0xFE00;
			CAPCOM_T01CON(active_base) = CAPCOM_T01CON_T0R_ENABLED;
		}
	} else {
		other_irq_count++;
	}
	VIC_IRQ_ACK = 1;
}
