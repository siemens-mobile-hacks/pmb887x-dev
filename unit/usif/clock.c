#include <pmb887x.h>

#include "test.h"

#define USIF_DIVIDER_TX_BYTES 16
#define USIF_TX_TIMEOUT_MS 2000
#define USIF_REFERENCE_CLOCK_HZ 96000000
#define USIF_REFERENCE_BAUD_HZ 115200
#define USIF_UART_FRAME_BITS 10

static const uint32_t DIVIDERS[] = {
	CGU_CON3_MMCI_CLKDIV_DIV1,
	CGU_CON3_MMCI_CLKDIV_DIV2,
	CGU_CON3_MMCI_CLKDIV_DIV4,
	CGU_CON3_MMCI_CLKDIV_DIV8,
};

static bool measure_usif_tx(uint32_t *elapsed_us) {
	USIF_RUN = USIF_RUN_RUN;
	stopwatch_t start = stopwatch_get();
	USIF_TPS = USIF_DIVIDER_TX_BYTES;
	for (uint32_t index = 0; index < USIF_DIVIDER_TX_BYTES / 4; index++)
		USIF_TXD = 0x55AA55AA;
	while ((USIF_FIFO_STAT & (USIF_FIFO_STAT_FILL | USIF_FIFO_STAT_BUSY)) != 0 &&
		stopwatch_elapsed_ms(start) < USIF_TX_TIMEOUT_MS)
		test_watchdog_serve();
	*elapsed_us = stopwatch_elapsed_us(start);
	bool completed = (USIF_FIFO_STAT & (USIF_FIFO_STAT_FILL | USIF_FIFO_STAT_BUSY)) == 0;
	USIF_RUN = 0;
	return completed;
}

int main(void) {
	test_start("CGU USIF clock test");

	USIF_CLC = (1U << MOD_CLC_RMC_SHIFT);
	uint32_t elapsed_us[ARRAY_SIZE(DIVIDERS)] = { 0 };
	bool completed[ARRAY_SIZE(DIVIDERS)] = { false };
	/* The 705p firmware's 115200-baud/96-MHz tuple matches the E71 USIF boot profile. */
	USIF_PROTO = 0x806;
	USIF_MODE = 8;
	USIF_BAUD = 5;
	USIF_FBAUD = 0x02290048;
	USIF_RXSMP = 0x3A;
	for (uint32_t index = 0; index < ARRAY_SIZE(DIVIDERS); index++) {
		CGU_CON3 = (CGU_CON3 & ~(CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV)) |
			CGU_CON3_MMCI_CLKSEL_OSC | DIVIDERS[index];
		stopwatch_usleep_wd(2000);
		completed[index] = measure_usif_tx(&elapsed_us[index]);
	}

	printf("# USIF TX with MMCI/USIF OSC /1/2/4/8: %lu/%lu/%lu/%lu us\n",
		elapsed_us[0], elapsed_us[1], elapsed_us[2], elapsed_us[3]);
	uint32_t baud_hz = (uint64_t) USIF_REFERENCE_BAUD_HZ * CPU_OSC_FREQ / USIF_REFERENCE_CLOCK_HZ;
	uint32_t expected_us = USIF_DIVIDER_TX_BYTES * USIF_UART_FRAME_BITS * 1000000 / baud_hz;
	bool rate_matches = true;
	for (uint32_t index = 0; index < ARRAY_SIZE(DIVIDERS); index++) {
		uint32_t expected = expected_us << index;
		rate_matches = rate_matches && completed[index] &&
			test_u32_in_interval(elapsed_us[index], expected * 97 / 100, expected * 103 / 100);
	}
	test_check("USIF TX rate follows all OSC dividers", rate_matches);
	return test_finish();
}
