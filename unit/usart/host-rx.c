#include <pmb887x.h>
#include <stopwatch.h>

#include "test.h"

#define RECEIVE_TIMEOUT_MS 2500
#define MAX_PAYLOAD_SIZE 32768
#define CRC32_POLYNOMIAL 0xEDB88320
#define CRC32_INITIAL_VALUE 0xFFFFFFFF
#define RECEIVER_DISABLED_FRAME "ignored"
#define HOST_TRIGGER '!'

struct usart_configuration {
	uint32_t baud_reload;
	uint32_t fractional_divider;
	uint32_t rx_fifo_control;
	uint32_t tx_fifo_control;
};

struct receive_case {
	uint32_t baud_rate;
	uint32_t length;
	uint32_t seed;
	uint32_t crc32;
};

struct receive_result {
	uint32_t length;
	uint32_t crc32;
	uint32_t errors;
	uint32_t fifo_level;
};

static uint32_t crc32_table[256];
static uint8_t payload[MAX_PAYLOAD_SIZE];

static struct usart_configuration get_usart_configuration(void) {
	return (struct usart_configuration) {
		.baud_reload = USART_BG(USART0),
		.fractional_divider = USART_FDV(USART0),
		.rx_fifo_control = USART_RXFCON(USART0),
		.tx_fifo_control = USART_TXFCON(USART0),
	};
}

static void restore_usart_configuration(const struct usart_configuration *configuration) {
	USART_BG(USART0) = configuration->baud_reload;
	USART_FDV(USART0) = configuration->fractional_divider;
	USART_RXFCON(USART0) = configuration->rx_fifo_control | USART_RXFCON_RXFFLU;
	USART_TXFCON(USART0) = configuration->tx_fifo_control | USART_TXFCON_TXFFLU;
	USART_WHBCON(USART0) = USART_WHBCON_SETREN;
}

static void crc32_init(void) {
	for (uint32_t value = 0; value < ARRAY_SIZE(crc32_table); value++) {
		uint32_t crc = value;
		for (uint32_t bit = 0; bit < 8; bit++) {
			bool low_bit = (crc & 1) != 0;
			crc >>= 1;
			if (low_bit)
				crc ^= CRC32_POLYNOMIAL;
		}
		crc32_table[value] = crc;
	}
}

static uint32_t crc32_update_byte(uint32_t crc, uint8_t value) {
	return crc32_table[(crc ^ value) & UINT8_MAX] ^ (crc >> 8);
}

static uint32_t crc32(const void *data, uint32_t size) {
	const uint8_t *bytes = data;
	uint32_t crc = CRC32_INITIAL_VALUE;

	for (uint32_t i = 0; i < size; i++)
		crc = crc32_update_byte(crc, bytes[i]);

	return ~crc;
}

static void wait_for_host_trigger(void) {
	stopwatch_t start = stopwatch_get();

	while (usart_get_rx_fifo_level(USART0) == 0 && stopwatch_elapsed_ms(start) < RECEIVE_TIMEOUT_MS)
		test_watchdog_serve();
	if (usart_get_rx_fifo_level(USART0) != 0)
		(void) USART_RXB(USART0);
	USART_ICR(USART0) = USART_ICR_RX;
}

static void send_payload(const struct receive_case *test_case) {
	usart_flush(USART0);
	USART_RXFCON(USART0) |= USART_RXFCON_RXFFLU;
	printf("# HOST-CMD: BEGIN\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: FORMAT 8N1\n");
	printf("# HOST-CMD: BAUDRATE %lu\n", test_case->baud_rate);
	printf("# HOST-CMD: WAIT 50\n");
	printf(
		"# HOST-CMD: SEND_XORSHIFT32 %lu 0x%08lX 0x%08lX\n",
		test_case->length,
		test_case->seed,
		test_case->crc32
	);
	printf("# HOST-CMD: WAIT 500\n");
	printf("# HOST-CMD: RESTORE\n");
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: END\n");
	usart_flush(USART0);
	wait_for_host_trigger();
}

static void send_text(uint32_t baud_rate, const char *text) {
	usart_flush(USART0);
	USART_RXFCON(USART0) |= USART_RXFCON_RXFFLU;
	printf("# HOST-CMD: BEGIN\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: FORMAT 8N1\n");
	printf("# HOST-CMD: BAUDRATE %lu\n", baud_rate);
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: SEND \"%s\"\n", text);
	printf("# HOST-CMD: WAIT 500\n");
	printf("# HOST-CMD: RESTORE\n");
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: END\n");
	usart_flush(USART0);
	wait_for_host_trigger();
}

static void configure_receiver(uint32_t baud_rate) {
	USART_PISEL(USART0) = 0;
	usart_set_speed(USART0, baud_rate);
	USART_RXFCON(USART0) = USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU;
	USART_CON(USART0) |= USART_CON_FEN | USART_CON_OEN;
	USART_WHBCON(USART0) = USART_WHBCON_SETREN | USART_WHBCON_CLRFE | USART_WHBCON_CLROE;
}

static struct receive_result receive_payload(
	const struct usart_configuration *console,
	const struct receive_case *test_case
) {
	send_payload(test_case);
	configure_receiver(test_case->baud_rate);

	uint32_t received = 0;
	stopwatch_t start = stopwatch_get();
	while (received < test_case->length) {
		uint32_t fifo_level = usart_get_rx_fifo_level(USART0);
		if (fifo_level == 0) {
			if (stopwatch_elapsed_ms(start) >= RECEIVE_TIMEOUT_MS)
				break;
			test_watchdog_serve();
			continue;
		}
		while (fifo_level-- != 0 && received < test_case->length)
			payload[received++] = (uint8_t) USART_RXB(USART0);
	}

	struct receive_result result = {
		.length = received,
		.crc32 = crc32(payload, received),
		.errors = USART_CON(USART0) & (USART_CON_PE | USART_CON_FE | USART_CON_OE),
		.fifo_level = usart_get_rx_fifo_level(USART0),
	};
	restore_usart_configuration(console);
	wait_for_host_trigger();

	return result;
}

static void test_receiver_enable(const struct usart_configuration *console) {
	test_category("Receiver enable");
	send_text(115200, RECEIVER_DISABLED_FRAME);
	configure_receiver(115200);
	USART_ICR(USART0) = 0xFF;
	USART_WHBCON(USART0) = USART_WHBCON_CLRREN;
	stopwatch_msleep_wd(250);
	uint32_t disabled_fifo_level = usart_get_rx_fifo_level(USART0);
	uint32_t disabled_rx_request = USART_RIS(USART0) & USART_RIS_RX;
	restore_usart_configuration(console);
	wait_for_host_trigger();

	test_eq_u32("disabled receiver rejects complete host frame", 0, disabled_fifo_level);
	test_eq_u32("disabled receiver raises no RX request", 0, disabled_rx_request);

	send_text(115200, "R");
	configure_receiver(115200);
	stopwatch_t start = stopwatch_get();
	while (usart_get_rx_fifo_level(USART0) == 0 && stopwatch_elapsed_ms(start) < RECEIVE_TIMEOUT_MS)
		test_watchdog_serve();
	uint32_t enabled_fifo_level = usart_get_rx_fifo_level(USART0);
	uint32_t enabled_byte = enabled_fifo_level != 0 ? USART_RXB(USART0) : 0;

	restore_usart_configuration(console);
	wait_for_host_trigger();
	test_eq_u32("re-enabled receiver accepts next host frame", 1, enabled_fifo_level);
	test_eq_u32("re-enabled receiver preserves next byte", 'R', enabled_byte);
}

int main(void) {
	static const struct receive_case cases[] = {
		{ 921600, 32768, 0x12345678, 0xD1BA9EC0 },
		{ 115200, 16384, 0xC001D00D, 0xAA5AD126 },
		{   9600,  1024, 0xA5A5A5A5, 0x2CFF2098 },
	};

	test_start("USART0 host receive test");
	USART_CLC(USART0) = 1 << MOD_CLC_RMC_SHIFT;
	crc32_init();
	struct usart_configuration console = get_usart_configuration();
	test_receiver_enable(&console);

	for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++) {
		test_category("Pseudorandom payload after baud-rate change");
		struct receive_result result = receive_payload(&console, &cases[i]);

		printf(
			"# baud=%lu length=%lu seed=%08lX crc32=%08lX\n",
			cases[i].baud_rate,
			cases[i].length,
			cases[i].seed,
			result.crc32
		);
		test_eq_u32("received payload length", cases[i].length, result.length);
		test_eq_u32("received payload CRC32", cases[i].crc32, result.crc32);
		test_eq_u32("payload has no parity, framing, or overrun errors", 0, result.errors);
		test_eq_u32("RX FIFO drains after payload", 0, result.fifo_level);
	}

	return test_finish();
}
