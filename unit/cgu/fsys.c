#include <pmb887x.h>

#include "clock.h"
#include "test.h"

#define RTC_T14_PRESCALER 8
#define RTC_MEASURE_TICKS 1024
#define TPU_FRAME_CYCLES 6000

static const struct {
	const char *name;
	uint32_t ndiv;
	uint32_t mdiv;
	uint32_t fsys_hz;
} PLL_CONFIGS[] = {
	{ "PLL 104 MHz supplies 52 MHz fSYS", 3, 0, 52000000 },
	{ "PLL 130 MHz supplies 65 MHz fSYS", 4, 0, 65000000 },
	{ "PLL 156 MHz supplies 78 MHz fSYS", 5, 0, 78000000 },
	{ "PLL N=15 M=3 preserves 52 MHz fSYS", 15, 3, 52000000 },
};

static volatile uint32_t tpu_frames;

static void configure_rtc(void) {
	SCU_RTCIF = 0xAA;
	RTC_CLC = (1 << MOD_CLC_RMC_SHIFT);
	RTC_CTRL |= RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN;
	RTC_CON |= RTC_CON_PRE;
	RTC_T14 = 0;
	RTC_REL = 0;
	RTC_ALARM = 0;
	RTC_SRC = 0;
	RTC_ISNC = 0;
	RTC_CTRL |= RTC_CTRL_CLK_SEL | RTC_CTRL_CLR_RTCBAD | RTC_CTRL_CLR_RTCINT;
	RTC_CON |= RTC_CON_RUN;
	RTC_CTRL &= ~RTC_CTRL_CLK_SEL;
}

static void configure_tpu(void) {
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_GSMCLK1 = (1 << TPU_GSMCLK1_K_SHIFT);
	TPU_GSMCLK2 = (1 << TPU_GSMCLK2_L_SHIFT);
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	for (uint32_t index = 0; index < 512; index++)
		TPU_RAM(index) = 0;
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 0;
	TPU_INT(0) = 0;
	TPU_INT(1) = 30000;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	TPU_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_CLRR;
}

static __SRAM uint32_t get_t14_count(void) {
	return (RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT;
}

static __SRAM uint32_t count_tpu_frames(void) {
	uint32_t first_t14 = get_t14_count();
	uint32_t first_frame = tpu_frames;
	while (((get_t14_count() - first_t14) & 0xFFFF) < RTC_MEASURE_TICKS)
		test_watchdog_serve();
	return tpu_frames - first_frame;
}

static __SRAM void test_fsys(void) {
	uint32_t initial_tpu_vic = VIC_CON(VIC_TPU_INT0_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	configure_rtc();
	configure_tpu();
	tpu_frames = 0;
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	cpu_enable_irq(true);

	cgu_fsys_select(CGU_FSYS_OSC);
	uint32_t frames = count_tpu_frames();
	uint32_t modeled_hz = cpu_get_sys_freq();

	/* K=L=1 and OVERFLOW=999 give one TPU frame per 6000 fSYS cycles. */
	uint32_t expected_frames = (uint64_t) CPU_OSC_FREQ * RTC_MEASURE_TICKS * RTC_T14_PRESCALER /
		(TPU_FRAME_CYCLES * CPU_CLK32K_FREQ);
	printf("# OSC: %lu TPU frames per 250 ms, model=%lu Hz\n", frames, modeled_hz);
	test_check("fSYS bypass selects the 26 MHz oscillator",
		modeled_hz == CPU_OSC_FREQ &&
		test_u32_in_interval(frames, expected_frames * 98 / 100, expected_frames * 102 / 100));

	for (uint32_t index = 0; index < ARRAY_SIZE(PLL_CONFIGS); index++) {
		cgu_pll_set(PLL_CONFIGS[index].ndiv, PLL_CONFIGS[index].mdiv);
		cgu_fsys_select(CGU_FSYS_PLL);
		frames = count_tpu_frames();
		modeled_hz = cpu_get_sys_freq();

		expected_frames = (uint64_t) PLL_CONFIGS[index].fsys_hz * RTC_MEASURE_TICKS * RTC_T14_PRESCALER /
			(TPU_FRAME_CYCLES * CPU_CLK32K_FREQ);
		printf("# N=%lu M=%lu: expected=%lu measured=%lu TPU frames per 250 ms, model=%lu Hz\n",
			PLL_CONFIGS[index].ndiv, PLL_CONFIGS[index].mdiv, expected_frames, frames, modeled_hz);
		test_check(PLL_CONFIGS[index].name,
			modeled_hz == PLL_CONFIGS[index].fsys_hz &&
			test_u32_in_interval(frames, expected_frames * 98 / 100, expected_frames * 102 / 100));
	}

	cpu_enable_irq(false);
	TPU_PARAM = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	TPU_CLC = MOD_CLC_DISR;
	VIC_CON(VIC_TPU_INT0_IRQ) = initial_tpu_vic;
	cpu_enable_irq(!irq_was_disabled);
}

int main(void) {
	test_start("CGU fSYS clock test");

	test_fsys();
	return test_finish();
}

__IRQ void irq_handler(void) {
	if (VIC_IRQ_CURRENT == VIC_TPU_INT0_IRQ) {
		tpu_frames++;
		TPU_SRC(0) |= MOD_SRC_CLRR;
	}
	VIC_IRQ_ACK = 1;
}
