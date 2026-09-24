#include <pmb887x.h>
#include <stopwatch.h>

#include "test.h"

#define AUTOBAUD_STANDARD_DIVIDER_FREQUENCY 11059200
#define AUTOBAUD_NONSTANDARD_DIVIDER_FREQUENCY 9600000
#define AUTOBAUD_FDV_SCALE 512
#define AUTOBAUD_STATUS_MASK 0x1F
#define AUTOBAUD_TIMEOUT_MS 500
#define AUTOBAUD_HANDOFF_FRAME "atREADY"
#define HOST_TRIGGER '!'

struct usart_configuration {
	uint32_t pin_select;
	uint32_t control;
	uint32_t baud_reload;
	uint32_t fractional_divider;
	uint32_t rx_fifo_control;
	uint32_t tx_fifo_control;
	uint32_t interrupt_mask;
	uint32_t autobaud_control;
};

struct autobaud_case {
	uint32_t baud_rate;
	uint32_t baud_reload;
	const char *format;
	const char *frame;
	uint32_t mode;
	uint32_t status;
};

struct autobaud_result {
	bool enabled_after_start;
	bool waiting_after_start;
	bool receiver_enabled_after_start;
	bool detected;
	uint32_t control;
	uint32_t baud_reload;
	uint32_t status;
	uint32_t detect_irqs;
};

static volatile uint32_t autobaud_detect_irqs;
static volatile bool autobaud_handoff_active;
static volatile uint8_t autobaud_handoff_data[sizeof(AUTOBAUD_HANDOFF_FRAME) - 1];
static volatile uint32_t autobaud_handoff_length;

static void wait_for_host_trigger(void) {
	stopwatch_t start = stopwatch_get();

	while (usart_get_rx_fifo_level(USART0) == 0 && stopwatch_elapsed_ms(start) < AUTOBAUD_TIMEOUT_MS)
		test_watchdog_serve();
	if (usart_get_rx_fifo_level(USART0) != 0)
		(void) USART_RXB(USART0);
	USART_ICR(USART0) = USART_ICR_RX;
}

static struct usart_configuration get_usart_configuration(void) {
	return (struct usart_configuration) {
		.pin_select = USART_PISEL(USART0),
		.control = USART_CON(USART0),
		.baud_reload = USART_BG(USART0),
		.fractional_divider = USART_FDV(USART0),
		.rx_fifo_control = USART_RXFCON(USART0),
		.tx_fifo_control = USART_TXFCON(USART0),
		.interrupt_mask = USART_IMSC(USART0),
		.autobaud_control = USART_ABCON(USART0),
	};
}

static void restore_usart_configuration(const struct usart_configuration *configuration) {
	USART_IMSC(USART0) = 0;
	USART_WHBABCON(USART0) = USART_WHBABCON_CLRABEN;
	USART_ABCON(USART0) = configuration->autobaud_control;
	USART_CON(USART0) = 0;
	USART_BG(USART0) = configuration->baud_reload;
	USART_FDV(USART0) = configuration->fractional_divider;
	USART_RXFCON(USART0) = configuration->rx_fifo_control | USART_RXFCON_RXFFLU;
	USART_TXFCON(USART0) = configuration->tx_fifo_control | USART_TXFCON_TXFFLU;
	USART_PISEL(USART0) = configuration->pin_select;
	USART_CON(USART0) = configuration->control;
	USART_WHBCON(USART0) = USART_WHBCON_SETREN;
	USART_IMSC(USART0) = configuration->interrupt_mask;
}

static void send_autobaud_frame(const struct autobaud_case *test_case) {
	usart_flush(USART0);
	USART_RXFCON(USART0) |= USART_RXFCON_RXFFLU;
	printf("# HOST-CMD: BEGIN\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: FORMAT %s\n", test_case->format);
	printf("# HOST-CMD: BAUDRATE %lu\n", test_case->baud_rate);
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: SEND \"%s\"\n", test_case->frame);
	printf("# HOST-CMD: WAIT 100\n");
	printf("# HOST-CMD: RESTORE\n");
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: END\n");
	usart_flush(USART0);
	wait_for_host_trigger();
}

static void configure_autobaud(uint32_t divider_frequency, uint32_t options) {
	USART_CLC(USART0) = 1 << MOD_CLC_RMC_SHIFT;
	USART_PISEL(USART0) = 0;
	USART_IMSC(USART0) = 0;
	USART_ICR(USART0) = 0xFF;
	USART_RXFCON(USART0) = 0;
	USART_TXFCON(USART0) = 0;

	uint32_t module_frequency = cpu_get_fpi1_freq();
	USART_FDV(USART0) = ((uint64_t) divider_frequency * AUTOBAUD_FDV_SCALE +
		module_frequency / 2) / module_frequency;
	USART_BG(USART0) = 0;
	USART_CON(USART0) = USART_CON_M_ASYNC_8BIT |
		USART_CON_FDE | USART_CON_REN | USART_CON_CON_R;
	USART_WHBCON(USART0) = USART_WHBCON_SETREN;

	autobaud_detect_irqs = 0;
	USART_IMSC(USART0) = USART_IMSC_ABDET;
	USART_ABCON(USART0) = USART_ABCON_ABDETEN | options;
	USART_WHBABCON(USART0) = USART_WHBABCON_SETABEN;
}

static struct autobaud_result run_autobaud(
	const struct usart_configuration *console,
	const struct autobaud_case *test_case,
	uint32_t divider_frequency,
	uint32_t options,
	bool abort_detection
) {
	send_autobaud_frame(test_case);
	configure_autobaud(divider_frequency, options);

	struct autobaud_result result = {
		.enabled_after_start = (USART_ABCON(USART0) & USART_ABCON_ABEN) != 0,
		.waiting_after_start = (USART_ABSTAT(USART0) & USART_ABSTAT_DETWAIT) != 0,
		.receiver_enabled_after_start = (USART_CON(USART0) & USART_CON_REN) != 0,
	};
	stopwatch_t start = stopwatch_get();
	while ((USART_ABCON(USART0) & USART_ABCON_ABEN) != 0 &&
		stopwatch_elapsed_ms(start) < AUTOBAUD_TIMEOUT_MS)
		test_watchdog_serve();

	result.detected = (USART_ABCON(USART0) & USART_ABCON_ABEN) == 0;
	result.control = USART_CON(USART0);
	result.baud_reload = USART_BG(USART0);
	result.status = USART_ABSTAT(USART0);
	result.detect_irqs = autobaud_detect_irqs;

	if (abort_detection)
		USART_WHBABCON(USART0) = USART_WHBABCON_CLRABEN;
	restore_usart_configuration(console);
	wait_for_host_trigger();

	return result;
}

static void check_detected_case(
	const struct usart_configuration *console,
	const struct autobaud_case *test_case,
	uint32_t divider_frequency
) {
	struct autobaud_result result = run_autobaud(console, test_case, divider_frequency, 0, false);

	printf("# %lu baud, %s, %s\n", test_case->baud_rate, test_case->format, test_case->frame);
	test_check("two-byte frame completes detection", result.detected);
	test_eq_u32("detected baud reload", test_case->baud_reload, result.baud_reload);
	test_eq_u32("detected frame format", test_case->mode, result.control & (USART_CON_M | USART_CON_ODD));
	test_eq_u32("detected character case", test_case->status, result.status & AUTOBAUD_STATUS_MASK);
	test_eq_u32("successful detection raises one ABDET IRQ", 1, result.detect_irqs);
}

static void test_standard_baud_rates(const struct usart_configuration *console) {
	static const struct autobaud_case cases[] = {
		{ 230400, 0x002, "8N1", "AT", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCCDET },
		{ 115200, 0x005, "8N1", "at", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCSDET },
		{  57600, 0x00B, "8N1", "AT", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCCDET },
		{  38400, 0x011, "8N1", "at", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCSDET },
		{  19200, 0x023, "8N1", "AT", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCCDET },
		{   9600, 0x047, "8N1", "at", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCSDET },
		{   4800, 0x08F, "8N1", "AT", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCCDET },
		{   2400, 0x11F, "8N1", "at", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCSDET },
		{   1200, 0x23F, "8N1", "AT", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCCDET },
	};

	test_category("Standard baudrates at fDIV = 11.0592 MHz");
	for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++)
		check_detected_case(console, &cases[i], AUTOBAUD_STANDARD_DIVIDER_FREQUENCY);
}

static void test_nonstandard_baud_rates(const struct usart_configuration *console) {
	static const struct autobaud_case cases[] = {
		{ 200000, 0x002, "8N1", "AT", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCCDET },
		{  50000, 0x00B, "8N1", "at", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCSDET },
		{   1042, 0x23F, "8N1", "AT", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCCDET },
	};

	test_category("Non-standard baudrates at fDIV = 9.6 MHz");
	for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++)
		check_detected_case(console, &cases[i], AUTOBAUD_NONSTANDARD_DIVIDER_FREQUENCY);
}

static void test_frame_formats(const struct usart_configuration *console) {
	static const struct autobaud_case cases[] = {
		{ 115200, 0x005, "7E1", "at", USART_CON_M_ASYNC_PARITY_7BIT, USART_ABSTAT_SCSDET },
		{ 115200, 0x005, "7O1", "AT", USART_CON_M_ASYNC_PARITY_7BIT | USART_CON_ODD,
			USART_ABSTAT_SCCDET },
		{ 115200, 0x005, "8E1", "at", USART_CON_M_ASYNC_PARITY_8BIT, USART_ABSTAT_SCSDET },
		{ 115200, 0x005, "8O1", "AT", USART_CON_M_ASYNC_PARITY_8BIT | USART_CON_ODD,
			USART_ABSTAT_SCCDET },
	};

	test_category("Datasheet frame formats");
	for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++)
		check_detected_case(console, &cases[i], AUTOBAUD_STANDARD_DIVIDER_FREQUENCY);
}

static void test_detection_controls(const struct usart_configuration *console) {
	static const struct autobaud_case test_case = {
		115200, 0x005, "8N1", "at", USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCSDET,
	};
	uint32_t options = USART_ABCON_AUREN | USART_ABCON_FCDETEN;
	test_category("Detection controls");
	struct autobaud_result result = run_autobaud(
		console,
		&test_case,
		AUTOBAUD_STANDARD_DIVIDER_FREQUENCY,
		options,
		false
	);

	test_check("SETABEN enables detection", result.enabled_after_start);
	test_check("SETABEN sets DETWAIT", result.waiting_after_start);
	test_check("AUREN disables receiver during detection", !result.receiver_enabled_after_start);
	test_check("successful AUREN detection restores receiver", (result.control & USART_CON_REN) != 0);
	test_check("control case completes detection", result.detected);
	test_eq_u32("second character clears DETWAIT", 0, result.status & USART_ABSTAT_DETWAIT);
	test_eq_u32("FCDETEN raises ABDET after both characters", 2, result.detect_irqs);
}

static void test_mixed_case_rejected(const struct usart_configuration *console) {
	static const struct autobaud_case test_case = {
		115200, 0x005, "8N1", "aT", USART_CON_M_ASYNC_8BIT, 0,
	};
	test_category("Invalid two-byte frame");
	struct autobaud_result result = run_autobaud(
		console,
		&test_case,
		AUTOBAUD_STANDARD_DIVIDER_FREQUENCY,
		0,
		true
	);

	test_check("mixed-case aT does not complete detection", !result.detected);
	test_eq_u32("mixed-case aT raises no ABDET IRQ", 0, result.detect_irqs);
}

static void test_receive_handoff(const struct usart_configuration *console) {
	static const struct autobaud_case test_case = {
		115200, 0x005, "8N1", AUTOBAUD_HANDOFF_FRAME,
		USART_CON_M_ASYNC_8BIT, USART_ABSTAT_SCSDET,
	};

	test_category("Firmware autobaud-to-RX handoff");
	send_autobaud_frame(&test_case);
	autobaud_handoff_length = 0;
	autobaud_handoff_active = true;
	configure_autobaud(AUTOBAUD_STANDARD_DIVIDER_FREQUENCY, 0);

	stopwatch_t start = stopwatch_get();
	while (autobaud_handoff_length < sizeof(autobaud_handoff_data) &&
		stopwatch_elapsed_ms(start) < AUTOBAUD_TIMEOUT_MS)
	{
		test_watchdog_serve();
	}
	autobaud_handoff_active = false;
	uint32_t handoff_length = autobaud_handoff_length;
	uint32_t fifo_level = USART_FSTAT(USART0) & USART_FSTAT_RXFFL;

	USART_IMSC(USART0) = 0;
	restore_usart_configuration(console);
	wait_for_host_trigger();

	test_eq_u32(
		"handoff preserves detected and trailing byte count",
		sizeof(autobaud_handoff_data),
		handoff_length
	);
	test_eq_memory(
		"handoff preserves detected and trailing bytes",
		AUTOBAUD_HANDOFF_FRAME,
		autobaud_handoff_data,
		sizeof(autobaud_handoff_data)
	);
	test_eq_u32("handoff drains RX FIFO", 0, fifo_level);
}

int main(void) {
	test_start("USART0 autobaud test");
	struct usart_configuration console = get_usart_configuration();

	VIC_CON(VIC_USART0_ABDET_IRQ) = 1;
	VIC_CON(VIC_USART0_RX_IRQ) = 1;
	cpu_enable_irq(true);

	test_standard_baud_rates(&console);
	test_nonstandard_baud_rates(&console);
	test_frame_formats(&console);
	test_detection_controls(&console);
	test_mixed_case_rejected(&console);
	test_receive_handoff(&console);

	cpu_enable_irq(false);
	USART_IMSC(USART0) = 0;
	VIC_CON(VIC_USART0_ABDET_IRQ) = 0;
	VIC_CON(VIC_USART0_RX_IRQ) = 0;

	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == VIC_USART0_ABDET_IRQ) {
		autobaud_detect_irqs++;
		if (autobaud_handoff_active) {
			uint32_t status = USART_ABSTAT(USART0);

			bool lowercase = (status & USART_ABSTAT_SCSDET) != 0;
			autobaud_handoff_data[autobaud_handoff_length++] = lowercase ? 'a' : 'A';
			autobaud_handoff_data[autobaud_handoff_length++] = lowercase ? 't' : 'T';
			USART_RXFCON(USART0) = (
				USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU |
				(1 << USART_RXFCON_RXFITL_SHIFT)
			);
		}
		USART_ICR(USART0) = USART_ICR_ABDET;
		if (autobaud_handoff_active)
			USART_IMSC(USART0) = USART_IMSC_RX;
	} else if (irq == VIC_USART0_RX_IRQ) {
		while (autobaud_handoff_length < sizeof(autobaud_handoff_data) &&
			(USART_FSTAT(USART0) & USART_FSTAT_RXFFL) != 0)
		{
			autobaud_handoff_data[autobaud_handoff_length++] = USART_RXB(USART0);
		}
		USART_ICR(USART0) = USART_ICR_RX;
	}

	VIC_IRQ_ACK = 1;
}
