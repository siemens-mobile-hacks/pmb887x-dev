#include <pmb887x.h>

#include "test.h"

#define TPU_TIMER_RAM_BASE 512
#define TPU_RF_CONTROL_RAM_BASE 64
#define TPU_RF_TELEGRAM_WORDS 4
#define TPU_RF_TELEGRAM_BURST 0x0001
#define TPU_RF_TELEGRAM_8_BIT 0x0300
#define TPU_RF_TELEGRAM_24_BIT 0x0320
#define TPU_RF_TELEGRAM_3_25MHZ 0x0080
#define TPU_RF_TYPE1_TRIGGER_FIRST 8
#define TPU_FRAME_OVERFLOW 9999
#define TPU_EVENT_TICK 5000
#define TPU_EVENT_ARM_TICK 4900
/* The counter runs at clk_gsm_if / 6 = 13 MHz / 6. */
#define TPU_COUNTER_TICK_NS_NUMERATOR 6000
#define TPU_COUNTER_TICK_NS_DENOMINATOR 13
#define TPU_TIMEOUT_MS 100

#define POLL_LIMIT 100000
#define SHORT_CHAIN 16
#define LONG_CHAIN 48
#define STROBE_SAMPLES 4096
#define SHORT_SERIES 10
#define LONG_SERIES 40

/* Bit clocks 6.5 and 3.25 MHz; 5 clk_gsm_if (13 MHz) between telegrams of a series. */
#define BIT_PERIOD_6_5MHZ_NS 154
#define BIT_PERIOD_3_25MHZ_NS 308
#define SERIES_GAP_NS 385
#define TOLERANCE_PERCENT 10

static uint32_t strobe_samples[STROBE_SAMPLES];

static uint32_t stm_ticks_to_ns(uint32_t ticks) {
	return ticks * 1000 / stopwatch_ticks_per_us();
}

static bool within_tolerance(uint32_t actual_ns, uint32_t expected_ns) {
	uint32_t margin = expected_ns * TOLERANCE_PERCENT / 100;
	return test_u32_in_interval(actual_ns, expected_ns - margin, expected_ns + margin);
}

static void configure_frame(void) {
	TPU_PARAM = 0;
	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = 2 << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = TPU_FRAME_OVERFLOW;
	TPU_OFFSET = 0;
	TPU_RFCON1 = TPU_RFCON1_STBSEL_ACTIVE_HIGH;
	TPU_RFCON2 = 0;
	GPIO_PIN(GPIO_RF_STR0) = GPIO_OS_ALT0 | GPIO_PS_ALT;
}

static bool wait_request(void) {
	for (uint32_t i = 0; i < POLL_LIMIT; i++) {
		if ((TPU_RFSSC_SRC & MOD_SRC_SRR) != 0)
			return true;
	}
	return false;
}

static bool wait_complete(void) {
	for (uint32_t i = 0; i < POLL_LIMIT; i++) {
		if ((TPU_RFCON2 & TPU_RFCON2_SSCEN) == 0)
			return true;
	}
	return false;
}

/* STM ticks of a continuous transfer, each word written while the previous one shifts; 0 on timeout. */
static uint32_t measure_chain_ticks(uint32_t control, uint32_t words) {
	TPU_RFSSC_SRC = MOD_SRC_SRE | MOD_SRC_CLRR;
	TPU_RFCON2 = control | TPU_RFCON2_SSCEN;
	uint32_t start = STM_TIM0;
	TPU_RFSSCTB = 0;
	for (uint32_t i = 1; i < words; i++) {
		if (!wait_request())
			return 0;
		TPU_RFSSC_SRC = MOD_SRC_SRE | MOD_SRC_CLRR;
		TPU_RFSSCTB = 0;
	}
	if (!wait_complete())
		return 0;
	uint32_t elapsed = STM_TIM0 - start;

	TPU_RFSSC_SRC = MOD_SRC_CLRR;
	test_watchdog_serve();
	return elapsed;
}

/* Polling cannot resolve the end of one transfer, so compare two chain lengths. */
static uint32_t measure_word_ns(uint32_t control) {
	uint32_t short_chain = measure_chain_ticks(control, SHORT_CHAIN);
	uint32_t long_chain = measure_chain_ticks(control, LONG_CHAIN);

	if (short_chain == 0 || long_chain <= short_chain)
		return 0;
	return stm_ticks_to_ns(long_chain - short_chain) / (LONG_CHAIN - SHORT_CHAIN);
}

/* C code cannot refill the buffer within a 6.5 MHz word (PMB7870 data sheet 10.12.3.6), so only 3.25 MHz. */
static void test_direct_transfers(void) {
	test_category("direct continuous transfers");

	uint32_t word_ns = measure_word_ns(TPU_RFCON2_SSCBM_16 | TPU_RFCON2_SSCHB_MSB | TPU_RFCON2_SSCFB_3_25MHZ);

	printf("# direct: continuous 16-bit word takes %u ns at 3.25 MHz\n", (unsigned) word_ns);
	test_check("16-bit word at 3.25 MHz takes 16 bit periods", within_tolerance(word_ns, 16 * BIT_PERIOD_3_25MHZ_NS));
}

static void write_series(uint16_t control, uint32_t count) {
	for (uint32_t index = 0; index < count; index++) {
		uint32_t word = TPU_RF_CONTROL_RAM_BASE + index * TPU_RF_TELEGRAM_WORDS;
		bool last = index + 1 == count;

		TPU_RAM(word) = control | (last ? 0 : TPU_RF_TELEGRAM_BURST);
		for (uint32_t byte = 1; byte < TPU_RF_TELEGRAM_WORDS; byte++)
			TPU_RAM(word + byte) = 0;
	}

	TPU_TGER = BIT(0);
	TPU_EAPB = 0;
	TPU_EAPT = 3;
	TPU_RAM(TPU_TIMER_RAM_BASE + 0) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + 1) = TPU_EVENT_TICK;
	TPU_RAM(TPU_TIMER_RAM_BASE + 2) = TPU_RF_TYPE1_TRIGGER_FIRST;
}

static bool wait_event_window(void) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS) {
		uint32_t counter = TPU_COUNTER;
		if (counter >= TPU_EVENT_ARM_TICK && counter < TPU_EVENT_TICK)
			return true;
		test_watchdog_serve();
	}
	return false;
}

/* Strobe end in ns after the event compare; 0 when no strobe was seen or the window cut it off. */
static uint32_t measure_series_end_ns(uint16_t control, uint32_t count) {
	write_series(control, count);
	TPU_RFSSC_SRC = MOD_SRC_CLRR;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	if (!wait_event_window()) {
		TPU_PARAM = 0;
		return 0;
	}

	uint32_t lead_ticks = TPU_EVENT_TICK - TPU_COUNTER;
	uint32_t start = STM_TIM0;
	for (uint32_t i = 0; i < STROBE_SAMPLES; i++)
		strobe_samples[i] = GPIO_PIN(GPIO_RF_STR0);
	uint32_t elapsed = STM_TIM0 - start;

	TPU_PARAM = 0;
	TPU_TGER = 0;
	TPU_RFSSC_SRC = MOD_SRC_CLRR;

	uint32_t end = 0;
	for (uint32_t i = 0; i < STROBE_SAMPLES; i++) {
		if ((strobe_samples[i] & GPIO_DATA) != 0)
			end = i + 1;
	}
	uint32_t lead_ns = lead_ticks * TPU_COUNTER_TICK_NS_NUMERATOR / TPU_COUNTER_TICK_NS_DENOMINATOR;
	uint32_t end_ns = stm_ticks_to_ns(elapsed * end / STROBE_SAMPLES);
	if (end == 0 || end == STROBE_SAMPLES || end_ns <= lead_ns)
		return 0;
	return end_ns - lead_ns;
}

/* Polling cannot resolve the start of a series, so compare two series lengths. */
static uint32_t measure_telegram_ns(uint16_t control) {
	uint32_t short_end = measure_series_end_ns(control, SHORT_SERIES);
	uint32_t long_end = measure_series_end_ns(control, LONG_SERIES);

	if (short_end == 0 || long_end <= short_end)
		return 0;
	return (long_end - short_end) / (LONG_SERIES - SHORT_SERIES);
}

static void test_control_block_series(void) {
	test_category("control block telegram series");

	uint32_t fast_8 = measure_telegram_ns(TPU_RF_TELEGRAM_8_BIT);
	uint32_t fast_24 = measure_telegram_ns(TPU_RF_TELEGRAM_24_BIT);
	uint32_t slow_8 = measure_telegram_ns(TPU_RF_TELEGRAM_8_BIT | TPU_RF_TELEGRAM_3_25MHZ);
	uint32_t slow_24 = measure_telegram_ns(TPU_RF_TELEGRAM_24_BIT | TPU_RF_TELEGRAM_3_25MHZ);

	printf("# series period: 8 bit %u ns, 24 bit %u ns at 6.5 MHz; 8 bit %u ns, 24 bit %u ns at 3.25 MHz\n",
		(unsigned) fast_8, (unsigned) fast_24, (unsigned) slow_8, (unsigned) slow_24);
	test_check(
		"8-bit telegram at 6.5 MHz takes 8 bits and the series gap",
		within_tolerance(fast_8, 8 * BIT_PERIOD_6_5MHZ_NS + SERIES_GAP_NS)
	);
	test_check(
		"24-bit telegram at 6.5 MHz takes 24 bits and the series gap",
		within_tolerance(fast_24, 24 * BIT_PERIOD_6_5MHZ_NS + SERIES_GAP_NS)
	);
	test_check(
		"8-bit telegram at 3.25 MHz takes 8 bits and the series gap",
		within_tolerance(slow_8, 8 * BIT_PERIOD_3_25MHZ_NS + SERIES_GAP_NS)
	);
	test_check(
		"24-bit telegram at 3.25 MHz takes 24 bits and the series gap",
		within_tolerance(slow_24, 24 * BIT_PERIOD_3_25MHZ_NS + SERIES_GAP_NS)
	);
}

int main(void) {
	test_start("TPU RF SSC timing test");
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	configure_frame();
	test_direct_transfers();
	test_control_block_series();

	return test_finish();
}
