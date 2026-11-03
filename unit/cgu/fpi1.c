#include <pmb887x.h>

#include "clock.h"
#include "test.h"

#define USART_WAIT_TIMEOUT_MS 1000
#define USART_TIMEOUT_CHECK_INTERVAL 256
#define LOOPBACK_FRAMES 16
#define USART_FRAME_BITS 10
#define USART_OVERSAMPLING 16
#define USART_BG_RELOAD 0x3E7
#define USART_FDV_VALUE 0x1D8
#define USART_FDV_SCALE 512

struct fpi1_case {
	const char *name;
	enum cgu_fpi1_source source;
	uint32_t divider;
	uint32_t pll_ndiv;
	uint32_t expected_khz;
};

static const struct fpi1_case CASES[] = {
	{ "OSC", CGU_FPI1_OSC, 0, 3, 26000 },
	{ "OSC with CLKDIV /2", CGU_FPI1_OSC, 1, 3, 26000 },
	{ "OSC with CLKDIV /4", CGU_FPI1_OSC, 2, 3, 26000 },
	{ "OSC with CLKDIV /8", CGU_FPI1_OSC, 3, 3, 26000 },
	{ "PLL/2 of 104 MHz", CGU_FPI1_PLL_DIV_2, 0, 3, 52000 },
	{ "PLL/2 of 104 MHz with CLKDIV /2", CGU_FPI1_PLL_DIV_2, 1, 3, 26000 },
	{ "PLL/2 of 104 MHz with CLKDIV /4", CGU_FPI1_PLL_DIV_2, 2, 3, 13000 },
	{ "PLL/2 of 104 MHz with CLKDIV /8", CGU_FPI1_PLL_DIV_2, 3, 3, 6500 },
	{ "PLL/2 of 130 MHz", CGU_FPI1_PLL_DIV_2, 0, 4, 65000 },
	{ "PLL/2 of 156 MHz", CGU_FPI1_PLL_DIV_2, 0, 5, 78000 },
};

static volatile uint8_t received_byte;
static volatile bool receive_complete;

static void configure_usart(void) {
	uint32_t control = USART_CON_M_ASYNC_8BIT | USART_CON_FDE | USART_CON_LB;
	USART_CLC(USART1) = (1 << MOD_CLC_RMC_SHIFT);
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

static uint32_t measure_fpi1_khz(void) {
	configure_usart();
	if (!loopback_byte(0xA5))
		return 0;

	uint32_t start_ticks = STM_TIM0;
	for (uint32_t frame = 0; frame < LOOPBACK_FRAMES; frame++) {
		if (!loopback_byte(0x40 + frame))
			return 0;
	}
	uint32_t elapsed_ticks = STM_TIM0 - start_ticks;
	test_watchdog_serve();

	uint64_t frame_cycles = (uint64_t) USART_FRAME_BITS * USART_OVERSAMPLING * (USART_BG_RELOAD + 1) *
		USART_FDV_SCALE / USART_FDV_VALUE;
	return LOOPBACK_FRAMES * frame_cycles * (stopwatch_ticks_per_s() / 1000) / elapsed_ticks;
}

static void test_fpi1_case(const struct fpi1_case *fpi1) {
	cgu_fpi1_select(CGU_FPI1_OSC, 0);
	cgu_pll_set(fpi1->pll_ndiv, 0);
	cgu_fpi1_select(fpi1->source, fpi1->divider);
	uint32_t modeled_hz = cpu_get_fpi1_freq();
	uint32_t measured_khz = measure_fpi1_khz();
	cgu_fpi1_select(CGU_FPI1_OSC, 0);

	printf("# %s: expected=%lu kHz measured=%lu kHz model=%lu kHz\n",
		fpi1->name, fpi1->expected_khz, measured_khz, modeled_hz / 1000);
	test_check(fpi1->name, modeled_hz / 1000 == fpi1->expected_khz &&
		test_u32_in_interval(measured_khz, fpi1->expected_khz * 98 / 100, fpi1->expected_khz * 102 / 100));
}

int main(void) {
	test_start("CGU FPI1 bus clock test");

	uint32_t initial_con1 = CGU_CON1;
	VIC_CON(VIC_USART1_RX_IRQ) = 1;
	cpu_enable_irq(true);

	for (uint32_t index = 0; index < ARRAY_SIZE(CASES); index++)
		test_fpi1_case(&CASES[index]);

	cpu_enable_irq(false);
	VIC_CON(VIC_USART1_RX_IRQ) = 0;
	USART_CON(USART1) = 0;
	CGU_CON1 = initial_con1;
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
