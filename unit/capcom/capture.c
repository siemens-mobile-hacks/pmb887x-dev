#include <pmb887x.h>

#include "test.h"

#define CAPCOM_CAPTURE_TIMEOUT_MS 100
#define CAPCOM_NO_EVENT_US 1000

typedef struct {
	const char *name;
	uint32_t base;
	uint32_t gpio;
	uint32_t channel;
	uint32_t rising_mode;
	uint32_t falling_mode;
	uint32_t both_edges_mode;
	uint32_t t1_allocation;
} capcom_t;

static volatile uint32_t *capcom_cc_src(const capcom_t *capcom) {
	switch (capcom->channel) {
		case 5:
			return &CAPCOM_CC5_SRC(capcom->base);
		case 6:
			return &CAPCOM_CC6_SRC(capcom->base);
	}

	return NULL;
}

static void gpio_set_capture_level(const capcom_t *capcom, bool high) {
	GPIO_PIN(capcom->gpio) = GPIO_IS_ALT2 | GPIO_PS_MANUAL |
		(high ? GPIO_DATA_HIGH : GPIO_DATA_LOW) | GPIO_DIR_OUT |
		GPIO_PPEN_PUSHPULL | GPIO_PDPU_NONE | GPIO_ENAQ_OUTPUT_ENABLED;
	test_spin(1000);
}

static void capcom_configure(const capcom_t *capcom, uint32_t mode, bool initial_level) {
	CAPCOM_CLC(capcom->base) = (1 << MOD_CLC_RMC_SHIFT);
	CAPCOM_T01CON(capcom->base) = 0;
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
	gpio_set_capture_level(capcom, initial_level);
	CAPCOM_PISEL(capcom->base) = 0;
	CAPCOM_CCM1(capcom->base) = mode;
	CAPCOM_T0(capcom->base) = 0x1000;
	CAPCOM_T0REL(capcom->base) = 0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED;
}

static bool wait_capture(const capcom_t *capcom) {
	volatile uint32_t *src = capcom_cc_src(capcom);
	stopwatch_t start = stopwatch_get();

	while ((*src & MOD_SRC_SRR) == 0 && stopwatch_elapsed_ms(start) < CAPCOM_CAPTURE_TIMEOUT_MS)
		test_watchdog_serve();

	return (*src & MOD_SRC_SRR) != 0;
}

static bool request_stays_clear(const capcom_t *capcom) {
	stopwatch_usleep_wd(CAPCOM_NO_EVENT_US);

	return (*capcom_cc_src(capcom) & MOD_SRC_SRR) == 0;
}

static bool captured_running_timer(uint32_t before, uint32_t captured, uint32_t after) {
	return captured >= before && captured <= after;
}

static void test_single_edge(const capcom_t *capcom, const char *name, uint32_t mode, bool initial_level) {
	test_category(name);
	capcom_configure(capcom, mode, initial_level);
	uint32_t before = CAPCOM_T0(capcom->base);
	gpio_set_capture_level(capcom, !initial_level);
	bool requested = wait_capture(capcom);
	uint32_t captured = CAPCOM_CC(capcom->base, capcom->channel);
	uint32_t after = CAPCOM_T0(capcom->base);
	*capcom_cc_src(capcom) = MOD_SRC_CLRR;
	gpio_set_capture_level(capcom, initial_level);
	bool opposite_ignored = request_stays_clear(capcom);
	CAPCOM_T01CON(capcom->base) = 0;

	printf("# capture window: before=%04lX CC%lu=%04lX after=%04lX\n",
		before, capcom->channel, captured, after);
	test_check("selected edge requests capture service", requested);
	test_check("capture register contains the running timer value",
		captured_running_timer(before, captured, after));
	test_check("opposite edge does not request capture service", opposite_ignored);
}

static void test_both_edges(const capcom_t *capcom) {
	test_category("Both-edge capture");
	capcom_configure(capcom, capcom->both_edges_mode, false);
	gpio_set_capture_level(capcom, true);
	bool rising = wait_capture(capcom);
	uint32_t rising_value = CAPCOM_CC(capcom->base, capcom->channel);
	*capcom_cc_src(capcom) = MOD_SRC_CLRR;
	gpio_set_capture_level(capcom, false);
	bool falling = wait_capture(capcom);
	uint32_t falling_value = CAPCOM_CC(capcom->base, capcom->channel);
	CAPCOM_T01CON(capcom->base) = 0;

	test_check("both-edge mode captures rising and falling transitions", rising && falling);
	test_check("falling-edge capture updates the captured timestamp", falling_value > rising_value);
}

static void test_pending_request(const capcom_t *capcom) {
	test_category("Capture while request is pending");
	capcom_configure(capcom, capcom->both_edges_mode, false);
	gpio_set_capture_level(capcom, true);
	bool first = wait_capture(capcom);
	uint32_t first_value = CAPCOM_CC(capcom->base, capcom->channel);
	gpio_set_capture_level(capcom, false);
	uint32_t second_value = CAPCOM_CC(capcom->base, capcom->channel);
	uint32_t request = *capcom_cc_src(capcom) & MOD_SRC_SRR;
	CAPCOM_T01CON(capcom->base) = 0;

	test_check("first edge leaves the capture request pending", first && request != 0);
	test_check("a pending request does not block a later capture", second_value > first_value);
}

static void test_timer_allocation(const capcom_t *capcom) {
	test_category("Capture timer allocation");
	capcom_configure(capcom, capcom->rising_mode | capcom->t1_allocation, false);
	CAPCOM_T01CON(capcom->base) = 0;
	CAPCOM_T0(capcom->base) = 0x1000;
	CAPCOM_T1(capcom->base) = 0x4000;
	CAPCOM_T1REL(capcom->base) = 0;
	CAPCOM_T01CON(capcom->base) = CAPCOM_T01CON_T0R_ENABLED | CAPCOM_T01CON_T1R_ENABLED;
	uint32_t before = CAPCOM_T1(capcom->base);
	gpio_set_capture_level(capcom, true);
	bool requested = wait_capture(capcom);
	uint32_t captured = CAPCOM_CC(capcom->base, capcom->channel);
	uint32_t after = CAPCOM_T1(capcom->base);
	CAPCOM_T01CON(capcom->base) = 0;

	printf("# T1 capture window: before=%04lX CC%lu=%04lX after=%04lX\n",
		before, capcom->channel, captured, after);
	test_check("T1-allocated channel requests capture service", requested);
	test_check("T1-allocated channel captures T1 rather than T0",
		captured_running_timer(before, captured, after));
}

static void test_instance(const capcom_t *capcom) {
	test_category(capcom->name);
	uint32_t saved_gpio = GPIO_PIN(capcom->gpio);
	test_single_edge(capcom, "Rising-edge capture", capcom->rising_mode, false);
	test_single_edge(capcom, "Falling-edge capture", capcom->falling_mode, true);
	test_both_edges(capcom);
	test_pending_request(capcom);
	test_timer_allocation(capcom);
	GPIO_PIN(capcom->gpio) = saved_gpio;
}

int main(void) {
	const capcom_t capcoms[] = {
#ifdef PMB8875
		{
			"CAPCOM0", CAPCOM0, GPIO_USART0_RTS, 6,
			CAPCOM_CCM1_MOD6_RISING_EDGE,
			CAPCOM_CCM1_MOD6_FALLING_EDGE,
			CAPCOM_CCM1_MOD6_BOTH_EDGES,
			CAPCOM_CCM1_ACC6_T1,
		},
#else
		{
			"CAPCOM0", CAPCOM0, GPIO_CLKOUT0, 5,
			CAPCOM_CCM1_MOD5_RISING_EDGE,
			CAPCOM_CCM1_MOD5_FALLING_EDGE,
			CAPCOM_CCM1_MOD5_BOTH_EDGES,
			CAPCOM_CCM1_ACC5_T1,
		},
#endif
		{
			"CAPCOM1", CAPCOM1, GPIO_DSPOUT0, 6,
			CAPCOM_CCM1_MOD6_RISING_EDGE,
			CAPCOM_CCM1_MOD6_FALLING_EDGE,
			CAPCOM_CCM1_MOD6_BOTH_EDGES,
			CAPCOM_CCM1_ACC6_T1,
		},
	};

	test_start("CAPCOM capture test");
	for (uint32_t index = 0; index < ARRAY_SIZE(capcoms); index++)
		test_instance(&capcoms[index]);

	return test_finish();
}
