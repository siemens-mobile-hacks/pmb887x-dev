#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define OUTPUT_WORDS 64
#define SOFT_BYTES 192

#ifdef PMB8875
#include "equalizer-output-runner-8875.inc"
#define DSP_EQUALIZER_OUTPUT_IMAGE DSP_EQUALIZER_OUTPUT_RUNNER_8875
#else
#include "equalizer-output-runner-8876.inc"
#define DSP_EQUALIZER_OUTPUT_IMAGE DSP_EQUALIZER_OUTPUT_RUNNER_8876
#endif

static uint16_t request_id;

static void clear_hard_outputs(void) {
	for (size_t i = 0; i < OUTPUT_WORDS; i++)
		dsp_hw_shared_memory[0x0100 + i] = 0;
}

static void load_soft_pattern(void) {
	static const uint8_t pattern[] = { 0xE0, 0x20, 0xFE, 0x00 };

	for (size_t i = 0; i < SOFT_BYTES; i++)
		dsp_hw_shared_memory[0x0140 + i] = pattern[i % ARRAY_SIZE(pattern)];
}

static bool run_vector(void) {
	request_id++;
	dsp_hw_shared_memory[0x0002] = 0;
	dsp_hw_shared_memory[0x0001] = request_id;
	return dsp_hw_wait_shared(0x0002, request_id, 100);
}

static bool raw_output_matches_input(void) {
	for (size_t i = 0; i < SOFT_BYTES; i++) {
		uint16_t input = dsp_hw_shared_memory[0x0140 + i] & 0x00FF;
		uint16_t expected = input & 0x0080 ? input | 0xFF00 : input;

		if (dsp_hw_shared_memory[0x0300 + i] != expected)
			return false;
	}
	return true;
}

static bool combined_output_matches_reset_state(size_t offset, bool segment) {
	for (size_t i = 0; i < 96; i++) {
		bool even_tail = i >= 4 && i % 2 == 0;
		bool odd_tail = i >= 3 && i % 2 != 0;
		bool one_word = segment ? i == 1 || i == 2 || even_tail : odd_tail;
		uint16_t expected = one_word ? 0x0101 : 0;

		if (dsp_hw_shared_memory[offset + i] != expected)
			return false;
	}
	return true;
}

int main(void) {
	test_start("DSP equalizer output combiner test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load equalizer output runner",
		dsp_hw_load_image(DSP_EQUALIZER_OUTPUT_IMAGE, sizeof(DSP_EQUALIZER_OUTPUT_IMAGE))))
		return test_finish();
	if (!test_check("BRANCH starts equalizer output runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("equalizer output runner becomes ready", dsp_hw_wait_shared(0x0000, READY_MARKER, 100)))
		return test_finish();

	test_category("Equalizer / hard-soft output combining and packing");
	clear_hard_outputs();
	load_soft_pattern();
	if (!test_check("baseline output vector completes", run_vector()))
		return test_finish();
	test_check("raw soft-output read preserves and sign-extends every byte", raw_output_matches_input());
	test_check("S_COMB packs the reset-state output into 16-bit words",
		combined_output_matches_reset_state(0x0400, false));
	test_check("S_SEG selects the training-sequence-adjacent packing layout",
		combined_output_matches_reset_state(0x0500, true));

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
