#include <pmb887x.h>
#include <stopwatch.h>

#include "test.h"

#define RECEIVE_TIMEOUT_MS 1000
#define RX_TIMEOUT_SHIFT_CLOCKS 58
#define IDLE_AFTER_FRAME_MS 5
#define HOST_TRIGGER '!'

static const char HOST_FRAME[] = "01234567";

struct usart_configuration {
	uint32_t baud_reload;
	uint32_t fractional_divider;
	uint32_t rx_fifo_control;
	uint32_t tx_fifo_control;
	uint32_t interrupt_mask;
	uint32_t timeout_reload;
};

struct rx_snapshot {
	uint32_t level;
	uint32_t ris;
};

static volatile uint8_t received[sizeof(HOST_FRAME) - 1];
static volatile uint32_t received_length;

static struct usart_configuration get_usart_configuration(void) {
	return (struct usart_configuration) {
		.baud_reload = USART_BG(USART0),
		.fractional_divider = USART_FDV(USART0),
		.rx_fifo_control = USART_RXFCON(USART0),
		.tx_fifo_control = USART_TXFCON(USART0),
		.interrupt_mask = USART_IMSC(USART0),
		.timeout_reload = USART_TMO(USART0),
	};
}

static void restore_usart_configuration(const struct usart_configuration *configuration) {
	USART_IMSC(USART0) = 0;
	USART_BG(USART0) = configuration->baud_reload;
	USART_FDV(USART0) = configuration->fractional_divider;
	USART_RXFCON(USART0) = configuration->rx_fifo_control | USART_RXFCON_RXFFLU;
	USART_TXFCON(USART0) = configuration->tx_fifo_control | USART_TXFCON_TXFFLU;
	USART_TMO(USART0) = configuration->timeout_reload;
	USART_WHBCON(USART0) = USART_WHBCON_SETREN;
	USART_ICR(USART0) = USART_ICR_RX | USART_ICR_TMO;
	USART_IMSC(USART0) = configuration->interrupt_mask;
}

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

static void request_host_frame(void) {
	usart_flush(USART0);
	USART_RXFCON(USART0) |= USART_RXFCON_RXFFLU;
	printf("# HOST-CMD: BEGIN\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: FORMAT 8N1\n");
	printf("# HOST-CMD: BAUDRATE 115200\n");
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: SEND \"%s\"\n", HOST_FRAME);
	printf("# HOST-CMD: WAIT 100\n");
	printf("# HOST-CMD: RESTORE\n");
	printf("# HOST-CMD: END\n");
	usart_flush(USART0);
	wait_for_host_trigger();
}

static void configure_receiver(void) {
	USART_PISEL(USART0) = 0;
	usart_set_speed(USART0, 115200);
	USART_RXFCON(USART0) = USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU | (1 << USART_RXFCON_RXFITL_SHIFT);
	USART_TMO(USART0) = RX_TIMEOUT_SHIFT_CLOCKS;
	USART_ICR(USART0) = USART_ICR_RX | USART_ICR_TMO;
}

static void print_rx_snapshot(const char *name, const struct rx_snapshot *snapshot) {
	printf(
		"# %s: FIFO level %lu bytes, RX request %s, timeout request %s\n",
		name,
		snapshot->level,
		(snapshot->ris & USART_RIS_RX) ? "set" : "clear",
		(snapshot->ris & USART_RIS_TMO) ? "set" : "clear"
	);
}

static struct rx_snapshot get_rx_snapshot(void) {
	return (struct rx_snapshot) {
		.level = usart_get_rx_fifo_level(USART0),
		.ris = USART_RIS(USART0) & (USART_RIS_RX | USART_RIS_TMO),
	};
}

static void wait_for_host_frame(void) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < RECEIVE_TIMEOUT_MS) {
		if (usart_get_rx_fifo_level(USART0) + received_length >= ARRAY_SIZE(received))
			break;
		test_watchdog_serve();
	}
	stopwatch_msleep_wd(IDLE_AFTER_FRAME_MS);
}

static void test_cleared_requests_stay_clear(const struct usart_configuration *console) {
	test_category("Cleared requests with unread FIFO data");
	request_host_frame();
	configure_receiver();
	wait_for_host_frame();
	struct rx_snapshot full = get_rx_snapshot();

	USART_ICR(USART0) = USART_ICR_RX | USART_ICR_TMO;
	(void) USART_RXB(USART0);
	struct rx_snapshot after_read = get_rx_snapshot();
	stopwatch_msleep_wd(IDLE_AFTER_FRAME_MS);
	struct rx_snapshot idle = get_rx_snapshot();

	restore_usart_configuration(console);
	request_host_trigger();

	print_rx_snapshot("full FIFO", &full);
	print_rx_snapshot("after cleared requests and RXB read", &after_read);
	print_rx_snapshot("after idle line", &idle);
	test_eq_u32("host frame fills RX FIFO", ARRAY_SIZE(received), full.level);
	test_eq_u32("full FIFO raises RX and timeout requests", USART_RIS_RX | USART_RIS_TMO, full.ris);
	test_eq_u32("RXB read keeps cleared requests clear", 0, after_read.ris);
	test_eq_u32("idle line keeps cleared requests clear", 0, idle.ris);
	test_eq_u32("unread bytes stay in FIFO", ARRAY_SIZE(received) - 1, idle.level);
}

static void test_one_byte_per_interrupt(const struct usart_configuration *console) {
	test_category("One byte per RX interrupt");
	request_host_frame();
	configure_receiver();
	received_length = 0;
	USART_IMSC(USART0) = USART_IMSC_RX;
	VIC_CON(VIC_USART0_RX_IRQ) = 1;
	cpu_enable_irq(true);

	wait_for_host_frame();

	cpu_enable_irq(false);
	VIC_CON(VIC_USART0_RX_IRQ) = 0;
	uint32_t final_received_length = received_length;
	restore_usart_configuration(console);
	request_host_trigger();

	printf("# received %lu of %u bytes, one per RX interrupt\n", final_received_length, ARRAY_SIZE(received));
	test_eq_u32("each host byte raises its own RX interrupt", ARRAY_SIZE(received), final_received_length);
	test_eq_memory("RX interrupts preserve host bytes", HOST_FRAME, received, ARRAY_SIZE(received));
}

int main(void) {
	test_start("USART0 RX FIFO request test");
	USART_CLC(USART0) = 1 << MOD_CLC_RMC_SHIFT;
	struct usart_configuration console = get_usart_configuration();
	test_cleared_requests_stay_clear(&console);
	test_one_byte_per_interrupt(&console);
	return test_finish();
}

__IRQ void irq_handler(void) {
	if (VIC_IRQ_CURRENT == VIC_USART0_RX_IRQ) {
		if (received_length < ARRAY_SIZE(received) && usart_get_rx_fifo_level(USART0) != 0)
			received[received_length++] = USART_RXB(USART0);
		USART_ICR(USART0) = USART_ICR_RX;
	}
	VIC_IRQ_ACK = 1;
}
