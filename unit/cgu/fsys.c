#include <pmb887x.h>
#include <sio.h>

#include "clock.h"
#include "test.h"

#define TPU_FRAME_CYCLES 6000

static volatile uint32_t tpu_frames;

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

static __SRAM uint32_t count_tpu_frames(void) {
	test_rtc_init();
	uint32_t first_frame = tpu_frames;
	test_rtc_wait_second();
	return tpu_frames - first_frame;
}

static __SRAM void check_pll_fsys(const char *name, uint32_t fsys_hz) {
	cgu_fsys_select(CGU_FSYS_PLL);
	uint32_t frames = count_tpu_frames();
	uint32_t modeled_hz = cpu_get_sys_freq();
	uint32_t expected_frames = fsys_hz / TPU_FRAME_CYCLES;

	printf("# %s: expected=%lu measured=%lu TPU frames per second, model=%lu Hz\n",
		name, expected_frames, frames, modeled_hz);
	test_check(name,
		modeled_hz == fsys_hz &&
		test_u32_in_interval(frames, expected_frames * 98 / 100, expected_frames * 102 / 100));
}

static __SRAM void test_fsys(void) {
	uint32_t initial_tpu_vic = VIC_CON(VIC_TPU_INT0_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	configure_tpu();
	tpu_frames = 0;
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	cpu_enable_irq(true);

	cgu_fsys_select(CGU_FSYS_OSC);
	uint32_t frames = count_tpu_frames();
	uint32_t modeled_hz = cpu_get_sys_freq();

	/* K=L=1 and OVERFLOW=999 give one TPU frame per 6000 fSYS cycles. */
	uint32_t expected_frames = CPU_OSC_FREQ / TPU_FRAME_CYCLES;
	printf("# OSC: %lu TPU frames per second, model=%lu Hz\n", frames, modeled_hz);
	test_check("fSYS bypass selects the 26 MHz oscillator",
		modeled_hz == CPU_OSC_FREQ &&
		test_u32_in_interval(frames, expected_frames * 98 / 100, expected_frames * 102 / 100));

	check_pll_fsys("PLL supplies half its frequency as fSYS", cgu_pll_enable() / 2);
	cgu_pll_output_bypass();
	check_pll_fsys("bypassed PLL output supplies half the oscillator as fSYS", CPU_OSC_FREQ / 2);
	const char *ratio_name = "PLL N=15 M=3 preserves 52 MHz fSYS";
	if (sio_is_usb()) {
		test_skip(ratio_name, "restarting the PLL core stops the USB console clock");
	} else {
		cgu_pll_set(15, 3);
		check_pll_fsys(ratio_name, 52000000);
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
