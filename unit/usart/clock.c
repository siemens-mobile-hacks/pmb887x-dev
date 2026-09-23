#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#define USART_WAIT_ITERATIONS 100000
#define LOOPBACK_FRAMES 16
#define USART_FRAME_BITS 10
/* Slow framing makes receive-polling overhead negligible in the clock measurement. */
#define USART_BG_RELOAD 0x7C
#define USART_FDV_VALUE 0x1D8
#define USART_FDV_SCALE 512

static const uint32_t PLL_NDIV_VALUES[] = { 4, 5 };

static void configure_usart(uint32_t rmc) {
	uint32_t control = USART_CON_M_ASYNC_8BIT | USART_CON_FDE | USART_CON_LB;
	USART_CLC(USART1) = (rmc << MOD_CLC_RMC_SHIFT);
	USART_CON(USART1) = control;
	USART_BG(USART1) = USART_BG_RELOAD;
	USART_FDV(USART1) = USART_FDV_VALUE;
	USART_TMO(USART1) = 0;
	USART_DMAE(USART1) = 0;
	USART_IMSC(USART1) = 0;
	USART_ICR(USART1) = 0xFF;
	USART_RXFCON(USART1) = 0;
	USART_TXFCON(USART1) = 0;
	USART_WHBCON(USART1) = USART_WHBCON_CLRPE | USART_WHBCON_CLRFE | USART_WHBCON_CLROE;
	USART_CON(USART1) = control | USART_CON_CON_R;
	USART_WHBCON(USART1) = USART_WHBCON_SETREN;
}

static bool loopback_byte(uint8_t value) {
	USART_ICR(USART1) = 0xFF;
	USART_TXB(USART1) = value;
	uint32_t wait = 0;
	while (wait < USART_WAIT_ITERATIONS && (USART_RIS(USART1) & USART_RIS_RX) == 0) {
		__asm__ volatile("nop");
		wait++;
	}
	if ((USART_RIS(USART1) & USART_RIS_RX) == 0)
		return false;

	return (USART_RXB(USART1) & 0xFF) == value;
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

int main(void) {
	test_start("CGU FPI1 clock test");

	cgu_fpi1_select(CGU_FPI1_DISABLED, 0);
	cgu_pll_set(3, 0);

	uint32_t osc_ticks = 0;
	uint32_t rmc4_ticks = 0;
	uint32_t configured_osc_ticks = 0;
	uint32_t configured_pll_ticks = 0;
	uint32_t pll_ticks[3] = { 0 };
	uint32_t divided_ticks[4] = { 0 };
	uint32_t fsys_pll_ticks = 0;
	bool completed = true;
	cgu_fpi1_select(CGU_FPI1_OSC, 0);
	configure_usart(1);
	completed = measure_loopback(&osc_ticks) && completed;
	configure_usart(4);
	completed = measure_loopback(&rmc4_ticks) && completed;
	configure_usart(1);
	usart_set_speed(USART1, 115200);
	completed = measure_loopback(&configured_osc_ticks) && completed;

	for (uint32_t divider = 0; divider < ARRAY_SIZE(divided_ticks); divider++) {
		cgu_fpi1_select(CGU_FPI1_PLL_DIV_2, divider);
		configure_usart(1);
		completed = measure_loopback(&divided_ticks[divider]) && completed;
	}
	pll_ticks[0] = divided_ticks[0];
	cgu_fpi1_select(CGU_FPI1_PLL_DIV_2, 0);
	usart_set_speed(USART1, 115200);
	completed = measure_loopback(&configured_pll_ticks) && completed;
	cgu_fsys_select(CGU_FSYS_PLL);
	configure_usart(1);
	completed = measure_loopback(&fsys_pll_ticks) && completed;

	for (uint32_t index = 0; index < ARRAY_SIZE(PLL_NDIV_VALUES); index++) {
		cgu_fpi1_select(CGU_FPI1_DISABLED, 0);
		cgu_fsys_select(CGU_FSYS_OSC);
		cgu_pll_set(PLL_NDIV_VALUES[index], 0);
		cgu_fpi1_select(CGU_FPI1_PLL_DIV_2, 0);
		configure_usart(1);
		completed = measure_loopback(&pll_ticks[index + 1]) && completed;
	}
	cgu_fpi1_select(CGU_FPI1_OSC, 0);

	printf("# OSC=%lu PLL104=%lu DIV2/4/8=%lu/%lu/%lu fSYS_PLL=%lu PLL130/156=%lu/%lu STM ticks\n",
		osc_ticks, pll_ticks[0], divided_ticks[1],
		divided_ticks[2], divided_ticks[3], fsys_pll_ticks,
		pll_ticks[1], pll_ticks[2]);
	printf("# USART1 at 115200 baud: FPI1 OSC=%lu PLL/2=%lu STM ticks\n",
		configured_osc_ticks, configured_pll_ticks);
	printf("# USART1 at FPI1 OSC: RMC1=%lu RMC4=%lu STM ticks\n",
		osc_ticks, rmc4_ticks);
	uint32_t expected_osc_ticks = (uint64_t) LOOPBACK_FRAMES * USART_FRAME_BITS * 16 *
		(USART_BG_RELOAD + 1) * USART_FDV_SCALE / USART_FDV_VALUE;
	uint32_t expected_pll_ticks = expected_osc_ticks / 2;
	test_check("USART1 loopback succeeds at every tested FPI1 rate", completed);
	test_check("FPI1 OSC and PLL/2 rates match the USART baud model",
		test_u32_in_interval(osc_ticks, expected_osc_ticks * 98 / 100, expected_osc_ticks * 102 / 100) &&
		test_u32_in_interval(pll_ticks[0], expected_pll_ticks * 98 / 100, expected_pll_ticks * 102 / 100));
	bool divider_rates_match = true;
	for (uint32_t divider = 0; divider < ARRAY_SIZE(divided_ticks); divider++) {
		uint32_t expected_ticks = expected_pll_ticks << divider;
		divider_rates_match = divider_rates_match &&
			test_u32_in_interval(divided_ticks[divider], expected_ticks * 98 / 100, expected_ticks * 102 / 100);
	}
	test_check("FPI1 /1, /2, /4 and /8 rates match the USART baud model", divider_rates_match);
	test_check("fSYS selection does not change FPI1 loopback timing",
		test_u32_in_interval(fsys_pll_ticks, pll_ticks[0] * 95 / 100, pll_ticks[0] * 105 / 100));
	test_check("USART1 module divider scales its baud rate",
		test_u32_in_interval(rmc4_ticks, expected_osc_ticks * 4 * 98 / 100,
			expected_osc_ticks * 4 * 102 / 100));
	test_check("USART1 baud rate follows FPI1 clock changes",
		completed && test_u32_in_interval(configured_pll_ticks, configured_osc_ticks * 95 / 100,
			configured_osc_ticks * 105 / 100));
	bool pll_rates_match = true;
	for (uint32_t index = 0; index < ARRAY_SIZE(PLL_NDIV_VALUES); index++) {
		uint32_t expected_ticks = expected_osc_ticks * 2 / (PLL_NDIV_VALUES[index] + 1);
		pll_rates_match = pll_rates_match &&
			test_u32_in_interval(pll_ticks[index + 1], expected_ticks * 98 / 100, expected_ticks * 102 / 100);
	}
	test_check("FPI1 PLL/2 rates match 130 and 156 MHz PLL", pll_rates_match);
	return test_finish();
}
