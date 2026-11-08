#include <pmb887x.h>
#include <stopwatch.h>

#include "test.h"

#define RECEIVE_TIMEOUT_MS 1000
#define BREAK_MS 20
#define BREAK_WINDOW_MS 400
#define HOST_TRIGGER '!'
#define RX_ERROR_FLAGS (USART_CON_PE | USART_CON_FE | USART_CON_OE)

struct break_snapshot {
	uint32_t errors;
	uint32_t ris;
	uint32_t level;
	uint32_t first_word;
};

static void wait_for_host_trigger(void) {
	stopwatch_t start = stopwatch_get();

	while (usart_get_rx_fifo_level(USART0) == 0 && stopwatch_elapsed_ms(start) < RECEIVE_TIMEOUT_MS)
		test_watchdog_serve();
	if (usart_get_rx_fifo_level(USART0) != 0)
		(void) USART_RXB(USART0);
	USART_ICR(USART0) = USART_ICR_RX;
}

static void request_host_trigger(void) {
	printf("# HOST-CMD: BEGIN\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: END\n");
	usart_flush(USART0);
	wait_for_host_trigger();
}

static void request_host_break(void) {
	usart_flush(USART0);
	USART_RXFCON(USART0) |= USART_RXFCON_RXFFLU;
	printf("# HOST-CMD: BEGIN\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: WAIT 100\n");
	printf("# HOST-CMD: BREAK %d\n", BREAK_MS);
	printf("# HOST-CMD: END\n");
	usart_flush(USART0);
	wait_for_host_trigger();
}

static void configure_receiver(void) {
	USART_IMSC(USART0) = 0;
	USART_RXFCON(USART0) = USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU | (1 << USART_RXFCON_RXFITL_SHIFT);
	USART_WHBCON(USART0) = USART_WHBCON_CLRPE | USART_WHBCON_CLRFE | USART_WHBCON_CLROE;
	USART_ICR(USART0) = USART_ICR_RX | USART_ICR_ERR | USART_ICR_TMO;
}

static struct break_snapshot get_break_snapshot(void) {
	struct break_snapshot snapshot = {
		.errors = USART_CON(USART0) & RX_ERROR_FLAGS,
		.ris = USART_RIS(USART0) & (USART_RIS_RX | USART_RIS_ERR),
		.level = usart_get_rx_fifo_level(USART0),
	};
	snapshot.first_word = snapshot.level != 0 ? USART_RXB(USART0) : 0;
	return snapshot;
}

static void test_break_reception(void) {
	test_category("Break on RX");
	uint32_t console_rx_fifo_control = USART_RXFCON(USART0);
	request_host_break();
	configure_receiver();
	stopwatch_msleep_wd(BREAK_WINDOW_MS);
	struct break_snapshot snapshot = get_break_snapshot();

	USART_RXFCON(USART0) = console_rx_fifo_control | USART_RXFCON_RXFFLU;
	request_host_trigger();

	printf(
		"# break: parity error %s, framing error %s, overrun %s, RX request %s, error request %s, "
		"%lu word(s) in RX FIFO, first word 0x%03lX\n",
		(snapshot.errors & USART_CON_PE) ? "set" : "clear",
		(snapshot.errors & USART_CON_FE) ? "set" : "clear",
		(snapshot.errors & USART_CON_OE) ? "set" : "clear",
		(snapshot.ris & USART_RIS_RX) ? "set" : "clear",
		(snapshot.ris & USART_RIS_ERR) ? "set" : "clear",
		snapshot.level,
		snapshot.first_word
	);
	test_eq_u32("break sets no receive error flag", 0, snapshot.errors);
	test_eq_u32("break raises only the RX request", USART_RIS_RX, snapshot.ris);
	test_eq_u32("break stores one word", 1, snapshot.level);
	test_eq_u32("break word is zero", 0, snapshot.first_word);
}

int main(void) {
	test_start("USART0 break test");
	USART_CLC(USART0) = 1 << MOD_CLC_RMC_SHIFT;
	test_break_reception();
	return test_finish();
}
