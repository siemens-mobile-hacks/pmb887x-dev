#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#ifdef PMB8875
#include "clock-domains-8875.inc"
#define DSP_CLOCK_DOMAINS_IMAGE DSP_CLOCK_DOMAINS_8875
#else
#include "clock-domains-8876.inc"
#define DSP_CLOCK_DOMAINS_IMAGE DSP_CLOCK_DOMAINS_8876
#endif

#define DSP_CLOCK_COUNTER_OFFSET 0x0400
#define DSP_CLOCK_READY_OFFSET 0x0401
#define DSP_CLOCK_TIMER_OFFSET 0x0402
#define DSP_CLOCK_READY_MARKER 0xA55A
#define DSP_CLOCK_WINDOW_US 1000
#define DSP_CLOCK_TEST_COM_FLAG BIT(2)

enum dsp_clock_control {
	DSP_CLOCK_CONTROL_NONE,
	DSP_CLOCK_CONTROL_DISR,
	DSP_CLOCK_CONTROL_RMC,
	DSP_CLOCK_CONTROL_SOURCE_DISABLED,
};

struct dsp_clock_window {
	uint16_t count;
	uint16_t timer_count;
	uint32_t core_active;
	uint16_t shared_marker;
	uint16_t com_status;
	uint16_t restored_com_status;
};

static uint32_t initial_cgu_con2;

static void dsp_clock_control_apply(enum dsp_clock_control control, uint32_t rmc) {
	switch (control) {
		case DSP_CLOCK_CONTROL_NONE:
			break;
		case DSP_CLOCK_CONTROL_DISR:
			DSP_CLC = (1 << MOD_CLC_RMC_SHIFT) | MOD_CLC_DISR;
			break;
		case DSP_CLOCK_CONTROL_RMC:
			DSP_CLC = rmc << MOD_CLC_RMC_SHIFT;
			break;
		case DSP_CLOCK_CONTROL_SOURCE_DISABLED:
			CGU_CON2 = (initial_cgu_con2 & ~CGU_CON2_DSP_CLKSEL) | CGU_CON2_DSP_CLKSEL_DISABLE;
			break;
	}
}

static void dsp_clock_control_restore(void) {
	CGU_CON2 = initial_cgu_con2;
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
}

// Shared RAM is sampled while the module is controlled, then the module is restored before reading counters.
static struct dsp_clock_window dsp_clock_measure(enum dsp_clock_control control, uint32_t rmc) {
	struct dsp_clock_window result;
	uint16_t start = dsp_hw_shared_memory[DSP_CLOCK_COUNTER_OFFSET];
	uint16_t timer_start = dsp_hw_shared_memory[DSP_CLOCK_TIMER_OFFSET];

	DSP_COM_CLEAR = DSP_CLOCK_TEST_COM_FLAG;
	DSP_COM_SET = DSP_CLOCK_TEST_COM_FLAG;
	dsp_clock_control_apply(control, rmc);
	stopwatch_usleep_wd(DSP_CLOCK_WINDOW_US / 2);
	result.core_active = CGU_STAT & CGU_STAT_DSP_CLK;
	result.shared_marker = dsp_hw_shared_memory[DSP_CLOCK_READY_OFFSET];
	result.com_status = DSP_COM_STATUS & DSP_CLOCK_TEST_COM_FLAG;
	stopwatch_usleep_wd(DSP_CLOCK_WINDOW_US / 2);
	dsp_clock_control_restore();
	result.count = dsp_hw_shared_memory[DSP_CLOCK_COUNTER_OFFSET] - start;
	result.timer_count = dsp_hw_shared_memory[DSP_CLOCK_TIMER_OFFSET] - timer_start;
	result.restored_com_status = DSP_COM_STATUS & DSP_CLOCK_TEST_COM_FLAG;
	DSP_COM_CLEAR = DSP_CLOCK_TEST_COM_FLAG;
	return result;
}

static bool is_full_speed(uint16_t count, uint16_t reference) {
	return count * 10U >= reference * 9U && count * 10U <= reference * 11U;
}

static bool is_stopped(uint16_t count, uint16_t reference) {
	return count * 100U < reference;
}

static bool is_com_visible(struct dsp_clock_window result) {
	return result.com_status == DSP_CLOCK_TEST_COM_FLAG &&
		result.restored_com_status == DSP_CLOCK_TEST_COM_FLAG;
}

static void test_dsp_clock_controls(void) {
	struct dsp_clock_window running = dsp_clock_measure(DSP_CLOCK_CONTROL_NONE, 0);
	uint16_t reference = running.count;
	uint16_t timer_reference = running.timer_count;
	char name[96];

	test_category("DSP core clock controls");
	printf("# running: core=%u timer=%u active=%04lX shared=%04X COM=%04X/%04X\n",
		reference, timer_reference, running.core_active, running.shared_marker,
		running.com_status, running.restored_com_status);
	test_check("running core counter advances", reference != 0);
	test_check("running Timer 2 advances", timer_reference != 0);
	test_eq_u32("DSP_CLK reports the busy core as active", CGU_STAT_DSP_CLK, running.core_active);
	test_eq_u32("running Shared RAM is visible to the MCU", DSP_CLOCK_READY_MARKER, running.shared_marker);
	test_check("running COM interface exposes and preserves a pending flag", is_com_visible(running));

	struct dsp_clock_window disr = dsp_clock_measure(DSP_CLOCK_CONTROL_DISR, 0);
	printf("# DISR: core=%u timer=%u active=%04lX shared=%04X COM=%04X/%04X\n",
		disr.count, disr.timer_count, disr.core_active, disr.shared_marker,
		disr.com_status, disr.restored_com_status);
	test_check("DSP_CLC.DISR does not slow the DSP core", is_full_speed(disr.count, reference));
	test_check("DSP_CLC.DISR does not slow Timer 2", is_full_speed(disr.timer_count, timer_reference));
	test_eq_u32("DSP_CLK stays set while DISR leaves the core active", CGU_STAT_DSP_CLK, disr.core_active);
	test_check("DSP_CLC.DISR hides Shared RAM from the MCU", disr.shared_marker != DSP_CLOCK_READY_MARKER);
	test_check("DSP_CLC.DISR leaves the COM interface accessible", is_com_visible(disr));

	static const uint32_t RMC_VALUES[] = { 2, 16, 255 };
	for (size_t i = 0; i < ARRAY_SIZE(RMC_VALUES); i++) {
		struct dsp_clock_window divided = dsp_clock_measure(DSP_CLOCK_CONTROL_RMC, RMC_VALUES[i]);

		printf("# RMC=%lu: core=%u timer=%u active=%04lX shared=%04X COM=%04X/%04X\n",
			RMC_VALUES[i], divided.count, divided.timer_count, divided.core_active,
			divided.shared_marker, divided.com_status, divided.restored_com_status);
		sprintf(name, "DSP_CLC.RMC=%lu does not slow the DSP core", RMC_VALUES[i]);
		test_check(name, is_full_speed(divided.count, reference));
		sprintf(name, "DSP_CLC.RMC=%lu does not slow Timer 2", RMC_VALUES[i]);
		test_check(name, is_full_speed(divided.timer_count, timer_reference));
		sprintf(name, "DSP_CLC.RMC=%lu leaves the core active", RMC_VALUES[i]);
		test_eq_u32(name, CGU_STAT_DSP_CLK, divided.core_active);
		sprintf(name, "DSP_CLC.RMC=%lu leaves Shared RAM and COM accessible", RMC_VALUES[i]);
		test_check(name, divided.shared_marker == DSP_CLOCK_READY_MARKER && is_com_visible(divided));
	}

	struct dsp_clock_window rmc0 = dsp_clock_measure(DSP_CLOCK_CONTROL_RMC, 0);
	printf("# RMC=0: core=%u timer=%u active=%04lX shared=%04X COM=%04X/%04X\n",
		rmc0.count, rmc0.timer_count, rmc0.core_active, rmc0.shared_marker,
		rmc0.com_status, rmc0.restored_com_status);
	test_check("DSP_CLC.RMC=0 does not stop the DSP core", is_full_speed(rmc0.count, reference));
	test_check("DSP_CLC.RMC=0 does not stop Timer 2", is_full_speed(rmc0.timer_count, timer_reference));
	test_eq_u32("DSP_CLC.RMC=0 leaves the core active", CGU_STAT_DSP_CLK, rmc0.core_active);
	test_check("DSP_CLC.RMC=0 hides Shared RAM from the MCU", rmc0.shared_marker != DSP_CLOCK_READY_MARKER);
	test_check("DSP_CLC.RMC=0 leaves the COM interface accessible", is_com_visible(rmc0));

	DSP_CLC = 0;
	uint32_t rmc0_clc = DSP_CLC;
	dsp_clock_control_restore();
	test_eq_u32("DSP_CLC.RMC=0 reports the module as disabled", MOD_CLC_DISS, rmc0_clc & MOD_CLC_DISS);

	struct dsp_clock_window stopped = dsp_clock_measure(DSP_CLOCK_CONTROL_SOURCE_DISABLED, 0);
	printf("# source disabled: core=%u timer=%u active=%04lX shared=%04X COM=%04X/%04X\n",
		stopped.count, stopped.timer_count, stopped.core_active, stopped.shared_marker,
		stopped.com_status, stopped.restored_com_status);
	test_check("disabled CGU DSP source stops the DSP core", is_stopped(stopped.count, reference));
	test_check("disabled CGU DSP source stops Timer 2", is_stopped(stopped.timer_count, timer_reference));
	test_eq_u32("DSP_CLK reports the stopped non-idle core as active", CGU_STAT_DSP_CLK,
		stopped.core_active);
	test_eq_u32("disabled CGU DSP source leaves Shared RAM accessible", DSP_CLOCK_READY_MARKER,
		stopped.shared_marker);
	test_check("disabled CGU DSP source leaves the COM interface accessible", is_com_visible(stopped));
}

int main(void) {
	test_start("DSP clock domains test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	initial_cgu_con2 = CGU_CON2;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load the clock-domain runner",
		dsp_hw_load_image(DSP_CLOCK_DOMAINS_IMAGE, sizeof(DSP_CLOCK_DOMAINS_IMAGE)))) {
		return test_finish();
	}
	if (!test_check("BRANCH starts the clock-domain runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("clock-domain runner becomes ready",
		dsp_hw_wait_shared(DSP_CLOCK_READY_OFFSET, DSP_CLOCK_READY_MARKER, 100))) {
		return test_finish();
	}

	test_dsp_clock_controls();
	return test_finish();
}
