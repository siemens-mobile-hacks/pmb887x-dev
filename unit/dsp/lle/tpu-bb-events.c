#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp/dsp-hw.h"
#include "test.h"
#ifdef PMB8875
#include "tpu-bb-events-8875.inc"
#define DSP_TPU_BB_EVENTS DSP_TPU_BB_EVENTS_8875
#endif
#ifdef PMB8876
#include "tpu-bb-events-8876.inc"
#define DSP_TPU_BB_EVENTS DSP_TPU_BB_EVENTS_8876
#endif

#define TPU_TIMER_RAM_BASE 512
#define TPU_DECODER_SHIFT 6
#define TPU_DECODER_MONON_SET 2
#define TPU_DECODER_RECEIVE_CLEAR 5
#define TPU_CLOCK_HZ 135416
#define TPU_FRAME_TICKS 6000
#define TPU_MONON_HIGH_TICK 400
#define TPU_MONON_FALL_TICK 500
#define TPU_TIMEOUT_MS 2500
#define COUNT_INTERVALS 50
#define TIMING_TOLERANCE_PERCENT 2
#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0x5AA5
#define COMPLETE_OFFSET 0x0000
#define READY_OFFSET 0x000F
#define LOW_TARGET_OFFSET 0x0010
#define HIGH_COUNT_OFFSET 0x0020
#define LOW_COUNT_OFFSET 0x0021
#define LAST_FLAGS_OFFSET 0x0022
#define FINAL_STATUS_OFFSET 0x0023
#define HIGH_STATUS_OFFSET 0x0024
#define LOW_STATUS_OFFSET 0x0025
#define HIGH_FLAGS_OFFSET 0x0026
#define LOW_FLAGS_OFFSET 0x0027

struct tpu_event {
	uint16_t time;
	uint16_t decoder;
};

static const struct tpu_event COUNT_EVENTS[] = {
	{ TPU_MONON_HIGH_TICK, TPU_DECODER_MONON_SET },
	{ TPU_MONON_FALL_TICK, TPU_DECODER_RECEIVE_CLEAR },
};

static const struct tpu_event CLEANUP_EVENTS[] = {
	{ 100, TPU_DECODER_RECEIVE_CLEAR },
};

static const uint16_t PHASE_WIDTHS[] = { 100, 200, 400, 800 };

static void configure_tpu(const struct tpu_event *events, size_t count) {
	TPU_PARAM = 0;
	TPU_TGER = 0;
	TPU_EAPT = 0;
	TPU_EAPB = 0;
	TPU_OFFSET = 0;
	TPU_PARAM = TPU_PARAM_TINI;
	TPU_PARAM = 0;

	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = 32 << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = TPU_FRAME_TICKS - 1;
	TPU_OFFSET = 0;
	TPU_TGER = BIT(0);
	TPU_EAPB = 0;
	TPU_EAPT = count * 3;
	for (size_t i = 0; i < count; i++) {
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3) = 0;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 1) = events[i].time;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 2) = events[i].decoder << TPU_DECODER_SHIFT;
	}
}

static bool prepare_runner(uint16_t low_target) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	dsp_hw_shared_memory[COMPLETE_OFFSET] = 0;
	dsp_hw_shared_memory[READY_OFFSET] = 0;
	if (!test_check("boot commands load the TPU baseband event runner",
		dsp_hw_load_image(DSP_TPU_BB_EVENTS, sizeof(DSP_TPU_BB_EVENTS)))) {
		return false;
	}
	if (!test_check("BRANCH starts the TPU baseband event runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("TPU baseband event runner becomes ready",
		dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, 100))) {
		return false;
	}

	dsp_hw_shared_memory[LOW_TARGET_OFFSET] = low_target;
	return true;
}

static void cleanup_tpu_signals(void) {
	configure_tpu(CLEANUP_EVENTS, ARRAY_SIZE(CLEANUP_EVENTS));
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	stopwatch_usleep_wd(3000);
	TPU_PARAM = 0;
}

static uint32_t expected_count_elapsed_us(void) {
	uint64_t ticks = TPU_MONON_FALL_TICK + (uint64_t) (COUNT_INTERVALS - 1) * TPU_FRAME_TICKS;

	return (uint32_t) (ticks * 1000000 / TPU_CLOCK_HZ);
}

static uint32_t run_count_pass(size_t pass) {
	char category[64];

	sprintf(category, "Repeated MONON timing / pass %lu", (uint32_t) pass);
	test_category(category);
	configure_tpu(COUNT_EVENTS, ARRAY_SIZE(COUNT_EVENTS));
	if (!prepare_runner(COUNT_INTERVALS))
		return 0;

	stopwatch_t start = stopwatch_get();
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, TPU_TIMEOUT_MS);
	uint32_t elapsed_us = stopwatch_elapsed_us(start);
	uint16_t high_count = dsp_hw_shared_memory[HIGH_COUNT_OFFSET];
	uint16_t low_count = dsp_hw_shared_memory[LOW_COUNT_OFFSET];
	uint16_t last_flags = dsp_hw_shared_memory[LAST_FLAGS_OFFSET];
	uint16_t final_status = dsp_hw_shared_memory[FINAL_STATUS_OFFSET];
	uint16_t high_status = dsp_hw_shared_memory[HIGH_STATUS_OFFSET];
	uint16_t low_status = dsp_hw_shared_memory[LOW_STATUS_OFFSET];
	uint32_t timeout_param = TPU_PARAM;
	uint32_t timeout_counter = TPU_COUNTER;
	uint32_t timeout_ceap = TPU_CEAP;
	uint32_t timeout_eapb = TPU_EAPB;
	uint32_t timeout_eapt = TPU_EAPT;
	uint32_t timeout_tger = TPU_TGER;
	TPU_PARAM = 0;
	cleanup_tpu_signals();

	printf("# TPU_BB_COUNT,pass=%lu,elapsed_us=%lu,high=%lu,low=%lu,flags=%04lX,status=%04lX\n",
		(uint32_t) pass, elapsed_us, (uint32_t) high_count, (uint32_t) low_count,
		(uint32_t) last_flags, (uint32_t) final_status);
	if (!completed) {
		printf("# TPU_BB_TIMEOUT,param=%08lX,counter=%08lX,ceap=%08lX,eapb=%08lX,eapt=%08lX,"
			"tger=%08lX,high_status=%04lX,low_status=%04lX\n",
			timeout_param, timeout_counter, timeout_ceap, timeout_eapb, timeout_eapt, timeout_tger,
			(uint32_t) high_status, (uint32_t) low_status);
	}
	if (!test_check("DSP reaches the requested frame boundary", completed))
		return 0;
	test_eq_u32("MONON rising edge is delivered once per frame", COUNT_INTERVALS, high_count);
	test_eq_u32("MONON falling edge is delivered once per frame", COUNT_INTERVALS, low_count);
	test_eq_u32("MONON is inactive at the terminal frame boundary", 0, final_status & TEAK_BB_STATUS_MONON);
	test_eq_u32("unreachable interrupt pointer prevents buffer-full events", 0,
		last_flags & TEAK_INT_FINTA0_BB_FULL);

	uint32_t expected_us = expected_count_elapsed_us();
	uint32_t tolerance_us = expected_us * TIMING_TOLERANCE_PERCENT / 100;
	test_check("50 TPU frame periods match the configured TPU clock",
		test_u32_in_interval(elapsed_us, expected_us - tolerance_us, expected_us + tolerance_us));
	return elapsed_us;
}

static void test_count_timing(void) {
	uint32_t first = run_count_pass(1);
	uint32_t second = run_count_pass(2);

	if (first == 0 || second == 0)
		return;
	uint32_t difference = first > second ? first - second : second - first;
	uint32_t tolerance = expected_count_elapsed_us() / 200;
	test_check("independent 50-frame runs repeat within 0.5 percent", difference <= tolerance);
}

static void run_phase_width(size_t index) {
	uint16_t width = PHASE_WIDTHS[index];
	struct tpu_event events[] = {
		{ TPU_MONON_HIGH_TICK, TPU_DECODER_MONON_SET },
		{ TPU_MONON_HIGH_TICK + width, TPU_DECODER_RECEIVE_CLEAR },
	};
	char category[64];

	sprintf(category, "MONON edge phase / %lu TPU ticks", (uint32_t) width);
	test_category(category);
	configure_tpu(events, ARRAY_SIZE(events));
	if (!prepare_runner(1))
		return;

	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool high_seen = dsp_hw_wait_shared(HIGH_COUNT_OFFSET, 1, 100);
	bool completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 100);
	uint16_t high_count = dsp_hw_shared_memory[HIGH_COUNT_OFFSET];
	uint16_t low_count = dsp_hw_shared_memory[LOW_COUNT_OFFSET];
	uint16_t high_status = dsp_hw_shared_memory[HIGH_STATUS_OFFSET];
	uint16_t low_status = dsp_hw_shared_memory[LOW_STATUS_OFFSET];
	uint16_t high_flags = dsp_hw_shared_memory[HIGH_FLAGS_OFFSET];
	uint16_t low_flags = dsp_hw_shared_memory[LOW_FLAGS_OFFSET];
	TPU_PARAM = 0;
	cleanup_tpu_signals();
	if (!test_check("programmed MONON rising edge completes", high_seen))
		return;
	if (!test_check("programmed MONON window completes", completed))
		return;
	test_eq_u32("MONON rising edge enters DSP INT0 once", 1, high_count);
	test_eq_u32("MONON falling edge enters DSP INT0 once", 1, low_count);
	test_eq_u32("MONON rising edge exposes MONON status", TEAK_BB_STATUS_MONON,
		high_status & TEAK_BB_STATUS_MONON);
	test_eq_u32("MONON falling edge clears MONON status", 0, low_status & TEAK_BB_STATUS_MONON);
	test_eq_u32("MONON rising edge reports only BBHI", TEAK_INT_FINTA0_BBHI,
		high_flags & (TEAK_INT_FINTA0_BBHI | TEAK_INT_FINTA0_BBLO));
	test_eq_u32("MONON falling edge reports only BBLO", TEAK_INT_FINTA0_BBLO,
		low_flags & (TEAK_INT_FINTA0_BBHI | TEAK_INT_FINTA0_BBLO));
}

static void test_phase_widths(void) {
	for (size_t i = 0; i < ARRAY_SIZE(PHASE_WIDTHS); i++)
		run_phase_width(i);
}

int main(void) {
	test_start("DSP TPU baseband event timing test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_count_timing();
	test_phase_widths();

	TPU_PARAM = 0;
	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
