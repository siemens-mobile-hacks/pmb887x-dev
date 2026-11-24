#include <pmb887x.h>

#include "test.h"

#define KEYPAD_INPUTS 7
#define KEYPAD_OUTPUTS 4
#define KEYPAD_PINS (KEYPAD_INPUTS + KEYPAD_OUTPUTS)
#define KEYPAD_SCAN_COLUMNS 9
#define KEYPAD_PORT_COLUMNS 12
#define KEYPAD_SOURCES 4
#define KEYPAD_LOW_ALT1_INPUTS 3
#define KEYPAD_TEST_CON 0x0101
#define KEYPAD_DEBOUNCE_MS 20
#define KEYPAD_INT2_DELAY_MS 10
#define KEYPAD_TOLERANCE_MS 4
#define KEYPAD_HOLD_MS 300
#define KEYPAD_EVENT_TIMEOUT_MS 200
#define GPIO_PINS 114
#define GPIO_ALT_INPUTS 7
#define SETTLE_MS 30

#define KEYPAD_IN_CONFIG (GPIO_IS_ALT0 | GPIO_PS_ALT | GPIO_DIR_IN | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON)
#define KEYPAD_OUT_CONFIG (GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_PPEN_OPENDRAIN | GPIO_PDPU_PULLUP | GPIO_ENAQ_OFF)
#define KEYPAD_ALT1_CONFIG (GPIO_IS_ALT0 | GPIO_OS_ALT1 | GPIO_PS_ALT | \
	GPIO_PPEN_OPENDRAIN | GPIO_PDPU_PULLUP | GPIO_ENAQ_OFF)
#define PROBE_CONFIG (GPIO_PS_ALT | GPIO_DIR_IN | GPIO_ENAQ_ON)
#define MANUAL_INPUT 3
#define MANUAL_OUTPUT_CONFIG (GPIO_IS_ALT0 | GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_PPEN_PUSHPULL | GPIO_ENAQ_OFF)
#define MANUAL_PULLDOWN_CONFIG (GPIO_IS_ALT0 | GPIO_PS_MANUAL | GPIO_DIR_IN | GPIO_PDPU_PULLDOWN | GPIO_ENAQ_ON)

#ifdef PMB8876
typedef struct keypad_alt_input_t keypad_alt_input_t;
typedef struct keypad_alt_output_t keypad_alt_output_t;

struct keypad_alt_input_t {
	uint32_t pin;
	uint32_t alt;
	uint32_t input;
};

struct keypad_alt_output_t {
	uint32_t column;
	uint32_t input;
};

static const uint32_t KEYPAD_PIN_GPIOS[KEYPAD_PINS] = {
	GPIO_KP_IN0, GPIO_KP_IN1, GPIO_KP_IN2, GPIO_KP_IN3, GPIO_KP_IN4, GPIO_KP_IN5, GPIO_KP_IN6,
	GPIO_KP_OUT0, GPIO_KP_OUT1, GPIO_KP_OUT2, GPIO_KP_OUT3,
};

static volatile uint32_t *const KEYPAD_SOURCE_REGS[KEYPAD_SOURCES] = {
	&KEYPAD_INT0_SRC, &KEYPAD_INT1_SRC, &KEYPAD_INT2_SRC, &KEYPAD_INT3_SRC,
};

static const keypad_alt_input_t KEYPAD_ALT_INPUTS[] = {
	{ GPIO_SSC1_SCLK, 3, 4 },
	{ GPIO_SSC1_MRST, 3, 5 },
	{ GPIO_I2S2_CLK0, 2, 6 },
	{ GPIO_T_OUT6, 4, 7 },
	{ GPIO_T_OUT9, 4, 4 },
	{ GPIO_T_OUT11, 4, 6 },
	{ GPIO_DIF_CS2, 1, 4 },
	{ GPIO_DIF_VD, 1, 6 },
	{ GPIO_DIF_RESET2, 1, 5 },
};

static const keypad_alt_output_t KEYPAD_ALT1_OUTPUTS[] = {
	{ 5, 6 },
	{ 6, 5 },
	{ 7, 4 },
};

static void keypad_configure_pins(void) {
	for (uint32_t i = 0; i < KEYPAD_INPUTS; i++)
		GPIO_PIN(KEYPAD_PIN_GPIOS[i]) = KEYPAD_IN_CONFIG;
	for (uint32_t i = KEYPAD_INPUTS; i < KEYPAD_PINS; i++)
		GPIO_PIN(KEYPAD_PIN_GPIOS[i]) = KEYPAD_OUT_CONFIG;
}

static void keypad_start(void) {
	KEYPAD_CON = KEYPAD_TEST_CON;
	keypad_configure_pins();
	stopwatch_msleep_wd(SETTLE_MS * 2);
	for (uint32_t i = 0; i < KEYPAD_SOURCES; i++)
		*KEYPAD_SOURCE_REGS[i] = MOD_SRC_CLRR;
}

static uint32_t keypad_get_column(uint32_t column) {
	return (KEYPAD_PORT(column / 4) >> ((column % 4) * 8)) & 0xFF;
}

static uint32_t keypad_count_columns_with(uint32_t pressed_inputs) {
	uint32_t count = 0;
	for (uint32_t column = 0; column < KEYPAD_PORT_COLUMNS; column++) {
		if ((~keypad_get_column(column) & 0xFF) == pressed_inputs)
			count++;
	}
	return count;
}

static void keypad_print_ports(void) {
	printf("# PORT0=%08lX PORT1=%08lX PORT2=%08lX ISR=%08lX\n",
		KEYPAD_PORT(0), KEYPAD_PORT(1), KEYPAD_PORT(2), KEYPAD_ISR);
}

static bool keypad_wait_request(uint32_t source, stopwatch_t origin, uint32_t *time_ms) {
	uint32_t deadline_ms = *time_ms + KEYPAD_EVENT_TIMEOUT_MS;
	while (stopwatch_elapsed_ms(origin) < deadline_ms) {
		if ((*KEYPAD_SOURCE_REGS[source] & MOD_SRC_SRR) != 0) {
			*KEYPAD_SOURCE_REGS[source] = MOD_SRC_CLRR;
			*time_ms = stopwatch_elapsed_ms(origin);
			return true;
		}
		test_watchdog_serve();
	}
	return false;
}

static void test_inputs_tied_low(void) {
	test_category("Inputs tied low");

	for (uint32_t input = 0; input < KEYPAD_LOW_ALT1_INPUTS; input++) {
		keypad_start();
		GPIO_PIN(KEYPAD_PIN_GPIOS[input]) = KEYPAD_ALT1_CONFIG;
		stopwatch_msleep_wd(SETTLE_MS * 2);
		keypad_print_ports();
		test_eq_u32("input tied low shows up in every scan column", KEYPAD_SCAN_COLUMNS,
			keypad_count_columns_with(BIT(input)));
		test_eq_u32("unscanned column stays released", 0xFF, keypad_get_column(KEYPAD_SCAN_COLUMNS));
	}
}

static void test_extra_output_columns(void) {
	test_category("KP_IN6..4 as scan columns 5..7");

	for (size_t i = 0; i < ARRAY_SIZE(KEYPAD_ALT1_OUTPUTS); i++) {
		uint32_t column = KEYPAD_ALT1_OUTPUTS[i].column;
		uint32_t input = KEYPAD_ALT1_OUTPUTS[i].input;
		keypad_start();
		GPIO_PIN(KEYPAD_PIN_GPIOS[input]) = KEYPAD_ALT1_CONFIG;
		stopwatch_msleep_wd(SETTLE_MS * 2);
		keypad_print_ports();
		test_eq_u32("pin driven by its own column reads pressed there", BIT(input) ^ 0xFF,
			keypad_get_column(column));
		test_eq_u32("no other column sees the key", 1, keypad_count_columns_with(BIT(input)));
	}
}

static uint32_t keypad_probe_manual_pin(uint32_t config) {
	keypad_start();
	GPIO_PIN(KEYPAD_PIN_GPIOS[MANUAL_INPUT]) = config;
	stopwatch_msleep_wd(SETTLE_MS * 2);
	keypad_print_ports();
	return keypad_count_columns_with(BIT(MANUAL_INPUT));
}

static void test_manual_gpio_input(void) {
	test_category("Input function of a pin in manual GPIO mode");

	test_eq_u32("manual output low reaches the keypad input", KEYPAD_SCAN_COLUMNS,
		keypad_probe_manual_pin(MANUAL_OUTPUT_CONFIG | GPIO_DATA_LOW));
	test_eq_u32("manual input pulled down reaches the keypad input", KEYPAD_SCAN_COLUMNS,
		keypad_probe_manual_pin(MANUAL_PULLDOWN_CONFIG));
	test_eq_u32("manual output high leaves the keypad input released", 0,
		keypad_probe_manual_pin(MANUAL_OUTPUT_CONFIG | GPIO_DATA_HIGH));
}

static void test_events(void) {
	test_category("Press, hold and release events");

	keypad_start();
	stopwatch_t origin = stopwatch_get();
	GPIO_PIN(GPIO_KP_IN6) = KEYPAD_ALT1_CONFIG;

	uint32_t int0_ms = 0;
	test_check("press raises INT0", keypad_wait_request(0, origin, &int0_ms));
	uint32_t int2_ms = int0_ms;
	test_check("press raises INT2", keypad_wait_request(2, origin, &int2_ms));
	printf("# INT0 %lu ms after the press, INT2 %lu ms after INT0\n", int0_ms, int2_ms - int0_ms);
	test_check("INT0 follows the debounce delay",
		test_u32_in_interval(int0_ms, KEYPAD_DEBOUNCE_MS, KEYPAD_DEBOUNCE_MS + KEYPAD_TOLERANCE_MS));
	test_check("INT2 follows INT0 by the first-repeat delay",
		test_u32_in_interval(int2_ms - int0_ms, KEYPAD_INT2_DELAY_MS - 1, KEYPAD_INT2_DELAY_MS + KEYPAD_TOLERANCE_MS));
	test_eq_u32("ISR while held", KEYPAD_ISR_INT0, KEYPAD_ISR);

	stopwatch_msleep_wd(KEYPAD_HOLD_MS);
	test_eq_u32("holding the key raises nothing more", 0, KEYPAD_INT0_SRC & MOD_SRC_SRR);

	origin = stopwatch_get();
	GPIO_PIN(GPIO_KP_IN6) = KEYPAD_IN_CONFIG;
	uint32_t int3_ms = 0;
	test_check("release raises INT3", keypad_wait_request(3, origin, &int3_ms));
	printf("# INT3 %lu ms after the release\n", int3_ms);
	test_check("INT3 follows the debounce delay",
		test_u32_in_interval(int3_ms, KEYPAD_DEBOUNCE_MS, KEYPAD_DEBOUNCE_MS + KEYPAD_TOLERANCE_MS));
	test_eq_u32("release does not raise INT0", 0, KEYPAD_INT0_SRC & MOD_SRC_SRR);
	test_eq_u32("PORT0 is released", UINT32_MAX, KEYPAD_PORT(0));
	test_eq_u32("PORT1 is released", UINT32_MAX, KEYPAD_PORT(1));
	test_eq_u32("ISR after release", KEYPAD_ISR_INT1, KEYPAD_ISR);
}

static bool probe_is_skipped(uint32_t pin) {
	bool is_keypad = pin <= GPIO_KP_OUT3;
	bool is_console = pin >= GPIO_USART0_RXD && pin <= GPIO_USART0_CTS;
	/* The E71 dies when pin 60 is reconfigured. */
	return is_keypad || is_console || pin == GPIO_DSPOUT1 || pin == 60;
}

static uint32_t keypad_get_expected_input(uint32_t pin, uint32_t alt) {
	for (size_t i = 0; i < ARRAY_SIZE(KEYPAD_ALT_INPUTS); i++) {
		if (KEYPAD_ALT_INPUTS[i].pin == pin && KEYPAD_ALT_INPUTS[i].alt == alt)
			return BIT(KEYPAD_ALT_INPUTS[i].input);
	}
	return 0;
}

static void test_alternate_inputs(void) {
	test_category("Pins whose alternate input function is a KEYPAD input");

	keypad_start();
	for (uint32_t pin = 0; pin < GPIO_PINS; pin++) {
		if (probe_is_skipped(pin))
			continue;

		uint32_t saved = GPIO_PIN(pin);
		uint32_t mismatches = 0;
		for (uint32_t alt = 0; alt < GPIO_ALT_INPUTS; alt++) {
			GPIO_PIN(pin) = PROBE_CONFIG | ((alt + 1) << GPIO_IS_SHIFT) | GPIO_PDPU_PULLDOWN;
			stopwatch_msleep_wd(SETTLE_MS);
			uint32_t pressed = ~KEYPAD_PORT(0) & 0xFF;
			uint32_t expected = keypad_get_expected_input(pin, alt);
			if (pressed != expected) {
				printf("# pin %lu IS=ALT%lu: pressed inputs %02lX, expected %02lX\n", pin, alt, pressed, expected);
				mismatches++;
			}
		}
		GPIO_PIN(pin) = saved;
		stopwatch_msleep_wd(SETTLE_MS);
		test_eq_u32("pin pulled low reaches exactly its KEYPAD inputs", 0, mismatches);
	}
}

#endif

int main(void) {
	test_start("KEYPAD scan test");

#ifdef PMB8876
	test_inputs_tied_low();
	test_extra_output_columns();
	test_manual_gpio_input();
	test_events();
	test_alternate_inputs();
#else
	test_skip("KEYPAD scan", "PMB8876 pin functions only");
#endif

	return test_finish();
}
