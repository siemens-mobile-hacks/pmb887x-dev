#include <pmb887x.h>
#include <stopwatch.h>

#include "test.h"

#define RX_TIMEOUT_MS 1000
#define RX_IRQ_DRAIN_LIMIT 2
#define HOST_TRIGGER '!'

static const uint8_t HOST_FRAME[] = "0123456789ABCDEFfedcba9876543210";

struct usart_configuration {
	uint32_t pin_select;
	uint32_t baud_reload;
	uint32_t fractional_divider;
	uint32_t rx_fifo_control;
	uint32_t tx_fifo_control;
	uint32_t interrupt_mask;
};

static volatile uint8_t received[sizeof(HOST_FRAME) - 1];
static volatile uint32_t received_length;
static volatile uint32_t rx_irqs;
static volatile uint32_t software_retriggers;

static void wait_for_host_trigger(void) {
	stopwatch_t start = stopwatch_get();

	while (usart_get_rx_fifo_level(USART0) == 0 && stopwatch_elapsed_ms(start) < RX_TIMEOUT_MS)
		test_watchdog_serve();
	if (usart_get_rx_fifo_level(USART0) != 0)
		(void) USART_RXB(USART0);
	USART_ICR(USART0) = USART_ICR_RX;
}

static struct usart_configuration get_usart_configuration(void) {
	return (struct usart_configuration) {
		.pin_select = USART_PISEL(USART0),
		.baud_reload = USART_BG(USART0),
		.fractional_divider = USART_FDV(USART0),
		.rx_fifo_control = USART_RXFCON(USART0),
		.tx_fifo_control = USART_TXFCON(USART0),
		.interrupt_mask = USART_IMSC(USART0),
	};
}

static void restore_usart_configuration(const struct usart_configuration *configuration) {
	USART_IMSC(USART0) = 0;
	USART_BG(USART0) = configuration->baud_reload;
	USART_FDV(USART0) = configuration->fractional_divider;
	USART_RXFCON(USART0) = configuration->rx_fifo_control | USART_RXFCON_RXFFLU;
	USART_TXFCON(USART0) = configuration->tx_fifo_control | USART_TXFCON_TXFFLU;
	USART_PISEL(USART0) = configuration->pin_select;
	USART_WHBCON(USART0) = USART_WHBCON_SETREN;
	USART_IMSC(USART0) = configuration->interrupt_mask;
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
	printf("# HOST-CMD: WAIT 50\n");
	printf("# HOST-CMD: SEND \"%c\"\n", HOST_TRIGGER);
	printf("# HOST-CMD: END\n");
	usart_flush(USART0);
	wait_for_host_trigger();
}

int main(void) {
	test_start("USART0 firmware-style RX IRQ test");
	struct usart_configuration console = get_usart_configuration();

	test_category("Bounded FIFO drain and software retrigger");
	request_host_frame();
	USART_PISEL(USART0) = 0;
	usart_set_speed(USART0, 115200);
	USART_RXFCON(USART0) = (
		USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU |
		(4 << USART_RXFCON_RXFITL_SHIFT)
	);
	USART_ICR(USART0) = USART_ICR_RX;
	USART_IMSC(USART0) = USART_IMSC_RX;
	VIC_CON(VIC_USART0_RX_IRQ) = 1;
	cpu_enable_irq(true);

	stopwatch_t start = stopwatch_get();
	while (received_length < sizeof(received) && stopwatch_elapsed_ms(start) < RX_TIMEOUT_MS)
		test_watchdog_serve();

	cpu_enable_irq(false);
	uint32_t final_received_length = received_length;
	uint32_t final_rx_irqs = rx_irqs;
	uint32_t final_software_retriggers = software_retriggers;
	uint32_t final_fifo_level = USART_FSTAT(USART0) & USART_FSTAT_RXFFL;
	USART_IMSC(USART0) = 0;
	VIC_CON(VIC_USART0_RX_IRQ) = 0;
	restore_usart_configuration(&console);
	wait_for_host_trigger();

	test_eq_u32("IRQ path receives complete host burst", sizeof(received), final_received_length);
	test_eq_memory("IRQ path preserves host burst", HOST_FRAME, received, sizeof(received));
	test_check("FIFO threshold produces multiple RX IRQs", final_rx_irqs > 1);
	test_check("RX ISR retriggers while unread FIFO data remains", final_software_retriggers > 0);
	test_eq_u32("RX ISR drains FIFO", 0, final_fifo_level);
	return test_finish();
}

__IRQ void irq_handler(void) {
	if (VIC_IRQ_CURRENT == VIC_USART0_RX_IRQ) {
		rx_irqs++;
		uint32_t drained = 0;
		while (received_length < sizeof(received) && drained < RX_IRQ_DRAIN_LIMIT &&
			(USART_FSTAT(USART0) & USART_FSTAT_RXFFL) != 0)
		{
			received[received_length++] = USART_RXB(USART0);
			drained++;
		}
		USART_ICR(USART0) = USART_ICR_RX;
		if ((USART_FSTAT(USART0) & USART_FSTAT_RXFFL) != 0) {
			software_retriggers++;
			USART_ISR(USART0) = USART_ISR_RX;
		}
	}

	VIC_IRQ_ACK = 1;
}
