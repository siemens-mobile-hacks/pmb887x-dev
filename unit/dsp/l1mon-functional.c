#include <pmb887x.h>
#include <gen/dsp.h>
#include <wdt.h>

#include "dsp-hw.h"
#include "l1mon-functional-8876.inc"
#include "test.h"

#define TPU_TIMER_RAM_BASE 512
#define TPU_DECODER_SHIFT 6
#define TPU_DECODER_MONON_SET 2
#define TPU_DECODER_RECEIVE_CLEAR 5
#define TPU_DECODER_RXON_SET 6
#define TPU_DECODER_RXON_CLEAR 7
#define TPU_FRAME_TICKS 6000
#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0x5AA5
#define COMPLETE_OFFSET 0x0000
#define READY_OFFSET 0x000F
#define WINDOW_TARGET_OFFSET 0x0010
#define HIGH_COUNT_OFFSET 0x0020
#define LOW_COUNT_OFFSET 0x0021
#define LAST_FLAGS_OFFSET 0x0022
#define LAST_STATUS_OFFSET 0x0023
#define HIGH_FLAGS_OFFSET 0x0024
#define HIGH_STATUS_OFFSET 0x0025
#define LOW_FLAGS_OFFSET 0x0026
#define LOW_STATUS_OFFSET 0x0027
#define MOD_STATUS_BEFORE_OFFSET 0x0028
#define MOD_STATUS_AFTER_OFFSET 0x0029
#define SAMPLE_WORDS_OFFSET 0x002A
#define MON_INDEX_OFFSET 191
#define MON_VALUES_OFFSET 192
#define MON_VALUE_COUNT 8
#define MON_SENTINEL 0x5AA5
#define MON_MIN_SAMPLE_WORDS (7 * 4)
#define MON_MAX_SAMPLE_WORDS (190 * 4)
#define MONITOR_WINDOWS 10
#define TEST_TIMEOUT_MS 1000

struct tpu_event {
	uint16_t time;
	uint16_t decoder;
};

static const struct tpu_event MONITOR_EVENTS[] = {
	{ 100, TPU_DECODER_RXON_SET },
	{ 400, TPU_DECODER_MONON_SET },
	{ 440, TPU_DECODER_RECEIVE_CLEAR },
	{ 600, TPU_DECODER_RXON_CLEAR },
};

static const struct tpu_event CLEANUP_EVENTS[] = {
	{ 100, TPU_DECODER_RECEIVE_CLEAR },
	{ 200, TPU_DECODER_RXON_CLEAR },
};

static void configure_tpu(const struct tpu_event *events, size_t count) {
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

static void cleanup_tpu(void) {
	configure_tpu(CLEANUP_EVENTS, ARRAY_SIZE(CLEANUP_EVENTS));
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	stopwatch_usleep_wd(3000);
	TPU_PARAM = 0;
}

static bool prepare_runner(void) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load the RX-only monitoring runner",
		dsp_hw_load_image(DSP_L1MON_FUNCTIONAL_8876, sizeof(DSP_L1MON_FUNCTIONAL_8876))))
	{
		return false;
	}
	if (!test_check("BRANCH starts the RX-only monitoring runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	return test_check("RX-only monitoring runner becomes ready",
		dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, 100));
}

static void validate_result(void) {
	uint16_t sample_words = dsp_hw_shared_memory[SAMPLE_WORDS_OFFSET];

	test_eq_u32("every MONON rising edge enters DSP INT0", MONITOR_WINDOWS,
		dsp_hw_shared_memory[HIGH_COUNT_OFFSET]);
	test_eq_u32("every MONON falling edge enters DSP INT0", MONITOR_WINDOWS,
		dsp_hw_shared_memory[LOW_COUNT_OFFSET]);
	test_eq_u32("MONON rising edge exposes BBHI", TEAK_INT_FINTA0_BBHI,
		dsp_hw_shared_memory[HIGH_FLAGS_OFFSET] & (TEAK_INT_FINTA0_BBHI | TEAK_INT_FINTA0_BBLO));
	test_eq_u32("MONON rising edge exposes active monitor status", TEAK_BB_STATUS_MONON,
		dsp_hw_shared_memory[HIGH_STATUS_OFFSET] & TEAK_BB_STATUS_MONON);
	test_eq_u32("MONON falling edge exposes BBLO", TEAK_INT_FINTA0_BBLO,
		dsp_hw_shared_memory[LOW_FLAGS_OFFSET] & (TEAK_INT_FINTA0_BBHI | TEAK_INT_FINTA0_BBLO));
	test_eq_u32("MONON falling edge clears monitor status", 0,
		dsp_hw_shared_memory[LOW_STATUS_OFFSET] & TEAK_BB_STATUS_MONON);
	test_check("receive window produces a valid Mask ROM sample count",
		sample_words >= MON_MIN_SAMPLE_WORDS && sample_words <= MON_MAX_SAMPLE_WORDS);
	for (size_t i = 0; i < MON_VALUE_COUNT; i++) {
		char name[64];

		tfp_sprintf(name, "MONON series fills result slot %u", (uint32_t) i);
		test_check(name, dsp_hw_shared_memory[MON_VALUES_OFFSET + i] != MON_SENTINEL);
	}
	test_eq_u32("ten monitoring results leave the ring index at two", 2,
		dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	test_eq_u32("modulator is inactive before the receive window", 0,
		dsp_hw_shared_memory[MOD_STATUS_BEFORE_OFFSET] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("RX-only monitoring never activates the modulator", 0,
		dsp_hw_shared_memory[MOD_STATUS_AFTER_OFFSET] & TEAK_MOD_STAT_MSTAT);
}

static void run_pass(size_t pass) {
	char category[56];
	bool completed;

	tfp_sprintf(category, "RX-only MONON integration / pass %u", (uint32_t) pass);
	test_category(category);
	if (!prepare_runner())
		return;

	dsp_hw_shared_memory[MON_INDEX_OFFSET] = 0;
	dsp_hw_shared_memory[WINDOW_TARGET_OFFSET] = MONITOR_WINDOWS;
	for (size_t i = 0; i < MON_VALUE_COUNT; i++)
		dsp_hw_shared_memory[MON_VALUES_OFFSET + i] = MON_SENTINEL;
	configure_tpu(MONITOR_EVENTS, ARRAY_SIZE(MONITOR_EVENTS));

	wdt_set_max_execution_time(UINT32_MAX);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, TEST_TIMEOUT_MS);
	TPU_PARAM = 0;
	cleanup_tpu();
	printf("# L1MON-STATE,complete=%04X,high=%u,low=%u,flags=%04X,status=%04X,samples=%u,result=%04X,index=%04X\n",
		(uint32_t) dsp_hw_shared_memory[COMPLETE_OFFSET],
		(uint32_t) dsp_hw_shared_memory[HIGH_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[LOW_COUNT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[LAST_FLAGS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[LAST_STATUS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[SAMPLE_WORDS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[MON_VALUES_OFFSET],
		(uint32_t) dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	if (!test_check("MONON series reaches the Mask ROM result publisher", completed))
		return;

	validate_result();
}

int main(void) {
	test_start("DSP RX-only L1 monitoring functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	wdt_set_max_execution_time(UINT32_MAX);

	for (size_t pass = 1; pass <= 2; pass++)
		run_pass(pass);

	TPU_PARAM = 0;
	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
