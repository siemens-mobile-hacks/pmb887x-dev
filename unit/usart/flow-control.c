#include <pmb887x.h>
#include <stopwatch.h>

#include "test.h"

#define FLOW_TIMEOUT_MS 100
#define RTS_TRIGGER_LEVEL 4

static bool wait_for_status(uint32_t mask) {
	stopwatch_t start = stopwatch_get();

	while ((USART_RIS(USART1) & mask) != mask && stopwatch_elapsed_ms(start) < FLOW_TIMEOUT_MS)
		test_watchdog_serve();
	return (USART_RIS(USART1) & mask) == mask;
}

static bool wait_for_rx_level(uint32_t level) {
	stopwatch_t start = stopwatch_get();

	while (usart_get_rx_fifo_level(USART1) != level && stopwatch_elapsed_ms(start) < FLOW_TIMEOUT_MS)
		test_watchdog_serve();
	return usart_get_rx_fifo_level(USART1) == level;
}

static void configure_loopback(void) {
	USART_CLC(USART1) = 1 << MOD_CLC_RMC_SHIFT;
	usart_set_speed(USART1, 115200);
	USART_CON(USART1) = (
		USART_CON_M_ASYNC_8BIT | USART_CON_FDE | USART_CON_LB |
		USART_CON_REN | USART_CON_CON_R
	);
	USART_WHBCON(USART1) = USART_WHBCON_SETREN;
	USART_RXFCON(USART1) = (
		USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU |
		(1 << USART_RXFCON_RXFITL_SHIFT)
	);
	USART_TXFCON(USART1) = 0;
	USART_ICR(USART1) = 0xFF;

	if (!test_is_qemu()) {
		GPIO_PIN(GPIO_USART1_RTS) = GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_DIR_IN;
		GPIO_PIN(GPIO_USART1_CTS) = GPIO_IS_ALT0 | GPIO_PS_ALT | GPIO_DIR_IN;
	}
}

static void test_cts(void) {
	test_category("CTS transmission gating and status");
	USART_FCCON(USART1) = USART_FCCON_CTSEN;
	stopwatch_usleep_wd(1000);
	if (!test_is_qemu())
		test_eq_u32("software RTS inactive drives RTS_N high", GPIO_DATA, GPIO_PIN(GPIO_USART1_RTS) & GPIO_DATA);
	test_eq_u32("looped-back inactive CTS is reported", 0, USART_FCSTAT(USART1) & USART_FCSTAT_CTS);

	USART_TXB(USART1) = 0x5A;
	stopwatch_usleep_wd(1000);
	test_eq_u32("inactive CTS holds queued frame", 0, USART_RIS(USART1) & USART_RIS_TX);
	test_eq_u32("inactive CTS produces no loopback byte", 0, usart_get_rx_fifo_level(USART1));

	USART_FCCON(USART1) = USART_FCCON_CTSEN | USART_FCCON_RTS;
	test_check("active CTS releases queued frame", wait_for_status(USART_RIS_TX));
	test_check("released frame reaches receiver", wait_for_rx_level(1));
	test_eq_u32("released frame data", 0x5A, USART_RXB(USART1));
	if (!test_is_qemu())
		test_eq_u32("software RTS active drives RTS_N low", 0, GPIO_PIN(GPIO_USART1_RTS) & GPIO_DATA);
	test_eq_u32("looped-back active CTS is reported", USART_FCSTAT_CTS, USART_FCSTAT(USART1) & USART_FCSTAT_CTS);
}

static void test_rts_threshold(void) {
	test_category("Automatic RTS receive threshold");
	USART_RXFCON(USART1) |= USART_RXFCON_RXFFLU;
	USART_FCCON(USART1) = (
		USART_FCCON_RTSEN |
		(RTS_TRIGGER_LEVEL << USART_FCCON_RTS_TRIGGER_SHIFT)
	);
	stopwatch_usleep_wd(1000);
	if (!test_is_qemu())
		test_eq_u32("empty RX FIFO keeps RTS_N active", 0, GPIO_PIN(GPIO_USART1_RTS) & GPIO_DATA);
	test_eq_u32("empty RX FIFO reports looped-back active CTS", USART_FCSTAT_CTS,
		USART_FCSTAT(USART1) & USART_FCSTAT_CTS);

	for (uint32_t i = 0; i < RTS_TRIGGER_LEVEL; i++) {
		USART_ICR(USART1) = USART_ICR_TX | USART_ICR_TB;
		USART_TXB(USART1) = 0x40 + i;
		test_check("loopback frame transmits below RTS threshold", wait_for_status(USART_RIS_TX));
	}
	test_check("RX FIFO reaches RTS threshold", wait_for_rx_level(RTS_TRIGGER_LEVEL));
	if (!test_is_qemu())
		test_eq_u32("RTS_N becomes inactive at RX threshold", GPIO_DATA, GPIO_PIN(GPIO_USART1_RTS) & GPIO_DATA);
	test_eq_u32("RX threshold reports looped-back inactive CTS", 0,
		USART_FCSTAT(USART1) & USART_FCSTAT_CTS);

	test_eq_u32("first threshold byte", 0x40, USART_RXB(USART1));
	stopwatch_usleep_wd(1000);
	if (!test_is_qemu())
		test_eq_u32("RTS_N becomes active below RX threshold", 0, GPIO_PIN(GPIO_USART1_RTS) & GPIO_DATA);
	test_eq_u32("draining below threshold reports looped-back active CTS", USART_FCSTAT_CTS,
		USART_FCSTAT(USART1) & USART_FCSTAT_CTS);
}

int main(void) {
	test_start("USART1 flow-control loopback test");

	configure_loopback();
	test_cts();
	test_rts_threshold();

	return test_finish();
}
