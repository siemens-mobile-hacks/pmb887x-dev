#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#define USART_WAIT_TIMEOUT_MS 100
#define USART_TIMEOUT_CHECK_INTERVAL 256
#define LOOPBACK_FRAMES 16
#define USART_FRAME_BITS 10
/* Slow framing makes IRQ entry latency negligible in the clock measurement. */
#define USART_BG_RELOAD 0x3E7
#define USART_FDV_VALUE 0x1D8
#define USART_FDV_SCALE 512
#define USART_OVERSAMPLING 16

struct usart_clock_case {
	const char *name;
	enum cgu_fpi1_source source;
	uint32_t divider;
	enum cgu_fsys_source fsys;
	uint32_t rmc;
};

static const struct usart_clock_case CASES[] = {
	{ "Baud rate follows FPI1 OSC", CGU_FPI1_OSC, 0, CGU_FSYS_OSC, 1 },
	{ "USART1 RMC=4 divides FPI1 OSC", CGU_FPI1_OSC, 0, CGU_FSYS_OSC, 4 },
	{ "Baud rate follows FPI1 PLL/2", CGU_FPI1_PLL_DIV_2, 0, CGU_FSYS_OSC, 1 },
	{ "Baud rate follows the FPI1 divider", CGU_FPI1_PLL_DIV_2, 2, CGU_FSYS_OSC, 1 },
	{ "fSYS PLL does not change the baud rate", CGU_FPI1_PLL_DIV_2, 0, CGU_FSYS_PLL, 1 },
};

static volatile uint8_t received_byte;
static volatile bool receive_complete;

static void configure_usart(uint32_t rmc) {
	uint32_t control = USART_CON_M_ASYNC_8BIT | USART_CON_FDE | USART_CON_LB;
	USART_CLC(USART1) = (rmc << MOD_CLC_RMC_SHIFT);
	USART_CON(USART1) = control;
	USART_BG(USART1) = USART_BG_RELOAD;
	USART_FDV(USART1) = USART_FDV_VALUE;
	USART_TMO(USART1) = 0;
	USART_DMAE(USART1) = 0;
	USART_IMSC(USART1) = USART_IMSC_RX;
	USART_ICR(USART1) = 0xFF;
	USART_RXFCON(USART1) = 0;
	USART_TXFCON(USART1) = 0;
	USART_WHBCON(USART1) = USART_WHBCON_CLRPE | USART_WHBCON_CLRFE | USART_WHBCON_CLROE;
	USART_CON(USART1) = control | USART_CON_CON_R;
	USART_WHBCON(USART1) = USART_WHBCON_SETREN;
}

static bool loopback_byte(uint8_t value) {
	receive_complete = false;
	USART_ICR(USART1) = 0xFF;
	USART_TXB(USART1) = value;
	stopwatch_t deadline = stopwatch_get() + (stopwatch_t) USART_WAIT_TIMEOUT_MS * stopwatch_ticks_per_ms();
	uint32_t polls = 0;
	while (!receive_complete) {
		polls++;
		if (polls % USART_TIMEOUT_CHECK_INTERVAL == 0 && stopwatch_get() >= deadline)
			return false;
	}

	return received_byte == value;
}

static bool measure_loopback(uint32_t *ticks) {
	if (!loopback_byte(0xA5))
		return false;

	uint32_t start = STM_TIM0;
	for (uint32_t frame = 0; frame < LOOPBACK_FRAMES; frame++) {
		if (!loopback_byte(0x40 + frame))
			return false;
	}
	*ticks = STM_TIM0 - start;
	return true;
}

static uint32_t get_expected_ticks(const struct usart_clock_case *usart_case) {
	uint64_t frame_bits = (uint64_t) LOOPBACK_FRAMES * USART_FRAME_BITS;
	uint64_t module_hz = cpu_get_fpi1_freq() / usart_case->rmc;
	uint64_t bit_cycles = (uint64_t) USART_OVERSAMPLING * (USART_BG_RELOAD + 1) * USART_FDV_SCALE / USART_FDV_VALUE;
	return frame_bits * bit_cycles * stopwatch_ticks_per_s() / module_hz;
}

static void test_usart_case(const struct usart_clock_case *usart_case) {
	cgu_fpi1_select(usart_case->source, usart_case->divider);
	cgu_fsys_select(usart_case->fsys);
	configure_usart(usart_case->rmc);
	uint32_t expected_ticks = get_expected_ticks(usart_case);
	uint32_t ticks = 0;
	bool completed = measure_loopback(&ticks);
	cgu_fsys_select(CGU_FSYS_OSC);
	cgu_fpi1_select(CGU_FPI1_OSC, 0);

	printf("# %s: %s, %lu STM ticks, expected %lu\n", usart_case->name, completed ? "ok" : "timeout",
		ticks, expected_ticks);
	test_check(usart_case->name,
		completed && test_u32_in_interval(ticks, expected_ticks * 98 / 100, expected_ticks * 102 / 100));
}

int main(void) {
	test_start("USART clock test");

	cgu_fpi1_select(CGU_FPI1_DISABLED, 0);
	cgu_pll_set(3, 0);
	cgu_fpi1_select(CGU_FPI1_OSC, 0);
	VIC_CON(VIC_USART1_RX_IRQ) = 1;
	cpu_enable_irq(true);

	for (uint32_t index = 0; index < ARRAY_SIZE(CASES); index++)
		test_usart_case(&CASES[index]);

	cpu_enable_irq(false);
	VIC_CON(VIC_USART1_RX_IRQ) = 0;
	return test_finish();
}

__IRQ void irq_handler(void) {
	if (VIC_IRQ_CURRENT == VIC_USART1_RX_IRQ) {
		received_byte = USART_RXB(USART1);
		USART_ICR(USART1) = USART_ICR_RX;
		receive_complete = true;
	}

	VIC_IRQ_ACK = 1;
}
