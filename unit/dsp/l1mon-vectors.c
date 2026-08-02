#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "l1mon-vectors-8876.inc"
#include "test.h"

#define READY_MARKER 0xA55A
#define READY_OFFSET 0x0000
#define REQUEST_OFFSET 0x0001
#define RESPONSE_OFFSET 0x0002
#define SAMPLE_WORDS_OFFSET 0x0003
#define MOD_STATUS_BEFORE_OFFSET 0x0010
#define MOD_STATUS_AFTER_OFFSET 0x0011
#define MOD_CONTROL_AFTER_OFFSET 0x0012
#define MON_INDEX_OFFSET 191
#define MON_VALUES_OFFSET 192
#define MON_VALUE_COUNT 8
#define MON_ZERO_RESULT 0xFF80
#define MON_MIN_SAMPLE_WORDS (7 * 4)
#define MON_USEFUL_SAMPLE_WORDS (60 * 4)
#define MON_MAX_SAMPLE_WORDS (190 * 4)
#define MON_SENTINEL_BASE 0x5100

static uint16_t request_id;

static void initialize_ring(uint16_t index) {
	dsp_hw_shared_memory[MON_INDEX_OFFSET] = index;
	for (size_t i = 0; i < MON_VALUE_COUNT; i++)
		dsp_hw_shared_memory[MON_VALUES_OFFSET + i] = MON_SENTINEL_BASE + i;
}

static bool run_vector(uint16_t sample_words) {
	request_id++;
	dsp_hw_shared_memory[RESPONSE_OFFSET] = 0;
	dsp_hw_shared_memory[SAMPLE_WORDS_OFFSET] = sample_words;
	dsp_hw_shared_memory[REQUEST_OFFSET] = request_id;
	return dsp_hw_wait_shared(RESPONSE_OFFSET, request_id, 100);
}

static void check_transmitter_inactive(void) {
	test_eq_u32("modulator is inactive before monitoring", 0,
		dsp_hw_shared_memory[MOD_STATUS_BEFORE_OFFSET] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("monitoring does not activate the modulator", 0,
		dsp_hw_shared_memory[MOD_STATUS_AFTER_OFFSET] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("monitoring leaves the software modulator clock disabled", 0,
		dsp_hw_shared_memory[MOD_CONTROL_AFTER_OFFSET] & TEAK_MOD_CTRL_MSWACT);
}

static void check_valid_vector(const char *category, uint16_t sample_words, uint16_t initial_index,
	uint16_t expected_index)
{
	test_category(category);
	initialize_ring(initial_index);
	if (!test_check("Mask ROM monitoring vector completes", run_vector(sample_words)))
		return;

	test_eq_u32("zero I/Q produces the documented RMS result", MON_ZERO_RESULT,
		dsp_hw_shared_memory[MON_VALUES_OFFSET + initial_index]);
	test_eq_u32("monitoring advances the result index modulo eight", expected_index,
		dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	for (size_t i = 0; i < MON_VALUE_COUNT; i++) {
		char name[72];

		if (i == initial_index)
			continue;
		tfp_sprintf(name, "unselected result slot %u remains intact", (uint32_t) i);
		test_eq_u32(name, MON_SENTINEL_BASE + i, dsp_hw_shared_memory[MON_VALUES_OFFSET + i]);
	}
	check_transmitter_inactive();
}

static void check_invalid_length(const char *category, uint16_t sample_words) {
	test_category(category);
	initialize_ring(3);
	if (!test_check("out-of-range monitoring request returns", run_vector(sample_words)))
		return;

	test_eq_u32("out-of-range window does not advance the result index", 3,
		dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	for (size_t i = 0; i < MON_VALUE_COUNT; i++) {
		char name[72];

		tfp_sprintf(name, "out-of-range window preserves result slot %u", (uint32_t) i);
		test_eq_u32(name, MON_SENTINEL_BASE + i, dsp_hw_shared_memory[MON_VALUES_OFFSET + i]);
	}
	check_transmitter_inactive();
}

static void check_invalid_index(uint16_t index) {
	char category[64];

	tfp_sprintf(category, "Invalid result index 0x%04X", (uint32_t) index);
	test_category(category);
	initialize_ring(index);
	if (!test_check("monitoring with an invalid index completes", run_vector(MON_USEFUL_SAMPLE_WORDS)))
		return;

	test_eq_u32("invalid index directs the result to slot zero", MON_ZERO_RESULT,
		dsp_hw_shared_memory[MON_VALUES_OFFSET]);
	test_eq_u32("invalid result index is replaced with minus one", UINT16_MAX,
		dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	for (size_t i = 1; i < MON_VALUE_COUNT; i++) {
		char name[72];

		tfp_sprintf(name, "invalid index preserves result slot %u", (uint32_t) i);
		test_eq_u32(name, MON_SENTINEL_BASE + i, dsp_hw_shared_memory[MON_VALUES_OFFSET + i]);
	}
	check_transmitter_inactive();
}

static void check_ring_wrap(void) {
	test_category("Eight-entry monitoring result ring");
	initialize_ring(0);
	for (size_t i = 0; i < MON_VALUE_COUNT; i++) {
		char name[64];

		tfp_sprintf(name, "monitoring request %u completes", (uint32_t) i + 1);
		if (!test_check(name, run_vector(MON_USEFUL_SAMPLE_WORDS)))
			return;
	}

	test_eq_u32("eight monitoring results wrap the index to zero", 0,
		dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	for (size_t i = 0; i < MON_VALUE_COUNT; i++) {
		char name[64];

		tfp_sprintf(name, "monitoring fills result slot %u", (uint32_t) i);
		test_eq_u32(name, MON_ZERO_RESULT, dsp_hw_shared_memory[MON_VALUES_OFFSET + i]);
	}

	dsp_hw_shared_memory[MON_VALUES_OFFSET] = MON_SENTINEL_BASE;
	dsp_hw_shared_memory[MON_VALUES_OFFSET + 1] = MON_SENTINEL_BASE + 1;
	if (!test_check("ninth monitoring request completes", run_vector(MON_USEFUL_SAMPLE_WORDS)))
		return;
	if (!test_check("tenth monitoring request completes", run_vector(MON_USEFUL_SAMPLE_WORDS)))
		return;

	test_eq_u32("ten monitoring results leave the index at two", 2,
		dsp_hw_shared_memory[MON_INDEX_OFFSET]);
	test_eq_u32("ninth result overwrites wrapped slot zero", MON_ZERO_RESULT,
		dsp_hw_shared_memory[MON_VALUES_OFFSET]);
	test_eq_u32("tenth result overwrites wrapped slot one", MON_ZERO_RESULT,
		dsp_hw_shared_memory[MON_VALUES_OFFSET + 1]);
	check_transmitter_inactive();
}

static bool prepare_runner(void) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load the monitoring vector runner",
		dsp_hw_load_image(DSP_L1MON_VECTORS_8876, sizeof(DSP_L1MON_VECTORS_8876))))
	{
		return false;
	}
	if (!test_check("BRANCH starts the monitoring vector runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	return test_check("monitoring vector runner becomes ready",
		dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, 100));
}

static void run_pass(size_t pass) {
	char category[48];

	tfp_sprintf(category, "Independent DSP reset pass %u", (uint32_t) pass);
	test_category(category);
	if (!prepare_runner())
		return;

	check_valid_vector("Minimum seven-symbol window", MON_MIN_SAMPLE_WORDS, 0, 1);
	check_valid_vector("Useful sixty-symbol window", MON_USEFUL_SAMPLE_WORDS, 3, 4);
	check_valid_vector("Maximum 190-symbol window", MON_MAX_SAMPLE_WORDS, 7, 0);
	check_invalid_length("Window below the documented minimum", MON_MIN_SAMPLE_WORDS - 4);
	check_invalid_length("Window above the documented maximum", MON_MAX_SAMPLE_WORDS + 4);
	check_invalid_index(UINT16_MAX);
	check_invalid_index(MON_VALUE_COUNT);
	check_ring_wrap();
}

int main(void) {
	test_start("DSP Mask ROM monitoring vectors");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t pass = 1; pass <= 2; pass++)
		run_pass(pass);

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
