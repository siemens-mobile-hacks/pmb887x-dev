#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define REQUEST_OFFSET 0x0001
#define RESPONSE_OFFSET 0x0002
#define COUNT_OFFSET 0x0003
#define FLAGS_OFFSET 0x0004
#define SCALE_OFFSET 0x0005
#define INITIALIZE_OFFSET 0x0006
#define W1_EPR_INPUT_OFFSET 0x0120
#define W1_EPL_INPUT_OFFSET 0x0130
#define W2_EMR_INPUT_OFFSET 0x0180
#define W2_EML_INPUT_OFFSET 0x0190
#define BPAR_INPUT_OFFSET 0x0200
#define RX_INPUT_OFFSET 0x0280
#define DONE_STATUS_OFFSET 0x0300
#define DONE_COUNT_OFFSET 0x0301
#define DONE_IRQ_OFFSET 0x0302
#define W1_EPR_OUTPUT_OFFSET 0x0330
#define W1_EPL_OUTPUT_OFFSET 0x0340
#define W2_EMR_OUTPUT_OFFSET 0x0350
#define W2_EML_OUTPUT_OFFSET 0x0360
#define W2_EPR_OUTPUT_OFFSET 0x0370
#define W2_EPL_OUTPUT_OFFSET 0x0380
#define HOUT_OUTPUT_OFFSET 0x0410
#define SOUT_OUTPUT_OFFSET 0x0450
#define ELAT_OUTPUT_OFFSET 0x0560

#ifdef PMB8875
#include "equalizer-burst-runner-8875.inc"
#define DSP_EQUALIZER_BURST_RUNNER DSP_EQUALIZER_BURST_RUNNER_8875
#else
#include "equalizer-burst-runner-8876.inc"
#define DSP_EQUALIZER_BURST_RUNNER DSP_EQUALIZER_BURST_RUNNER_8876
#endif

struct burst_segment {
	bool right;
	uint16_t count;
	uint16_t scale;
	uint32_t metric_hash;
	uint32_t path_hash;
	uint32_t hard_hash;
	uint32_t soft_hash;
	uint32_t latency_hash;
};

static const struct burst_segment segments[] = {
	{ false, 20, 0x0008, UINT32_C(0x1806A79C), UINT32_C(0xB9EC788D), UINT32_C(0xB8AB0F37),
		UINT32_C(0xC98B46C8), UINT32_C(0x66029B7D) },
	{ true, 20, 0x0010, UINT32_C(0x354A565D), UINT32_C(0xB2FE3A95), UINT32_C(0x3DE95A27),
		UINT32_C(0xF150F5C3), UINT32_C(0x3A523A11) },
	{ false, 20, 0x0020, UINT32_C(0x66313D66), UINT32_C(0x9425F825), UINT32_C(0xDEDDF34A),
		UINT32_C(0xB5F0FEB1), UINT32_C(0x7A9B5579) },
	{ true, 20, 0x0004, UINT32_C(0x7390725D), UINT32_C(0x2B8BB005), UINT32_C(0x9EE8735E),
		UINT32_C(0x8D5BD1A0), UINT32_C(0x65264961) },
	{ false, 18, 0x0020, UINT32_C(0x6F68B389), UINT32_C(0x30191075), UINT32_C(0x74FA266A),
		UINT32_C(0x28A0C507), UINT32_C(0x43DB6088) },
	{ true, 18, 0x0010, UINT32_C(0xC3CE28CD), UINT32_C(0xCFE1D1ED), UINT32_C(0xE03B4A74),
		UINT32_C(0xE767A5AB), UINT32_C(0x8713CBC4) },
};

static void initialize_half(size_t metric_offset, size_t path_offset, size_t preferred_state,
	uint16_t training_history)
{
	for (size_t state = 0; state < 8; state++) {
		dsp_hw_shared_memory[metric_offset + state] = 0xD8F0;
		dsp_hw_shared_memory[path_offset + state] = state;
	}
	dsp_hw_shared_memory[metric_offset + preferred_state] = 0x8000;
	dsp_hw_shared_memory[path_offset + preferred_state] = training_history;
	for (size_t state = 8; state < 16; state++) {
		dsp_hw_shared_memory[metric_offset + state] = 0;
		dsp_hw_shared_memory[path_offset + state] = 0;
	}
}

static void initialize_burst(void) {
	initialize_half(W2_EML_INPUT_OFFSET, W1_EPL_INPUT_OFFSET, 3, 0x4E39);
	initialize_half(W2_EMR_INPUT_OFFSET, W1_EPR_INPUT_OFFSET, 6, 0x72C5);
}

static void load_segment_inputs(size_t segment) {
	for (size_t group = 0; group < 7; group++) {
		for (size_t selector = 0; selector < 8; selector++) {
			size_t offset = BPAR_INPUT_OFFSET + (group * 8 + selector) * 2;
			int32_t real = 0x0090 + segment * 0x0030 + group * 0x0029 + selector * 0x0017;
			int32_t imaginary = 0x0060 + segment * 0x0020 + group * 0x001D + selector * 0x0013;

			if (((segment + group + selector) & 1) != 0)
				real = -real;
			if (((segment + selector) & 2) != 0)
				imaginary = -imaginary;
			dsp_hw_shared_memory[offset] = (uint16_t) real;
			dsp_hw_shared_memory[offset + 1] = (uint16_t) imaginary;
		}
	}
	for (size_t timestamp = 0; timestamp < 32; timestamp++) {
		int32_t real = -0x0500 + segment * 0x0041 + timestamp * 0x0053;
		int32_t imaginary = 0x0480 - segment * 0x0037 - timestamp * 0x0041;

		dsp_hw_shared_memory[RX_INPUT_OFFSET + timestamp * 2] = (uint16_t) real;
		dsp_hw_shared_memory[RX_INPUT_OFFSET + timestamp * 2 + 1] = (uint16_t) imaginary;
	}
}

static uint32_t hash_words(size_t offset, size_t words) {
	uint32_t hash = UINT32_C(2166136261);

	for (size_t i = 0; i < words; i++) {
		hash ^= dsp_hw_shared_memory[offset + i] & 0xFF;
		hash *= UINT32_C(16777619);
		hash ^= dsp_hw_shared_memory[offset + i] >> 8;
		hash *= UINT32_C(16777619);
	}
	return hash;
}

static bool run_segment(size_t index) {
	const struct burst_segment *segment = &segments[index];
	size_t w1_path_offset = segment->right ? W1_EPR_OUTPUT_OFFSET : W1_EPL_OUTPUT_OFFSET;
	size_t w2_metric_offset = segment->right ? W2_EMR_OUTPUT_OFFSET : W2_EML_OUTPUT_OFFSET;
	size_t w2_path_offset = segment->right ? W2_EPR_OUTPUT_OFFSET : W2_EPL_OUTPUT_OFFSET;
	uint16_t request = index + 1;

	load_segment_inputs(index);
	dsp_hw_shared_memory[COUNT_OFFSET] = segment->count;
	dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE |
		(segment->right ? TEAK_EQ_CONF2_EQ_RIGHT : 0);
	dsp_hw_shared_memory[SCALE_OFFSET] = segment->scale;
	dsp_hw_shared_memory[INITIALIZE_OFFSET] = index == 0;
	dsp_hw_shared_memory[RESPONSE_OFFSET] = 0;
	dsp_hw_shared_memory[REQUEST_OFFSET] = request;
	if (!test_check("burst runner completes the segment", dsp_hw_wait_shared(RESPONSE_OFFSET, request, 1000))) {
		printf("# runner request=%04X response=%04X status=%04X count=%04X irq=%04X\n",
			(uint32_t) dsp_hw_shared_memory[REQUEST_OFFSET], (uint32_t) dsp_hw_shared_memory[RESPONSE_OFFSET],
			(uint32_t) dsp_hw_shared_memory[DONE_STATUS_OFFSET],
			(uint32_t) dsp_hw_shared_memory[DONE_COUNT_OFFSET],
			(uint32_t) dsp_hw_shared_memory[DONE_IRQ_OFFSET]);
		return false;
	}

	test_eq_u32("segment finishes idle", 0, dsp_hw_shared_memory[DONE_STATUS_OFFSET]);
	test_eq_u32("segment reports its own symbol count", segment->count,
		dsp_hw_shared_memory[DONE_COUNT_OFFSET]);
	test_eq_u32("segment raises a fresh equalizer interrupt", TEAK_INT_FINTA0_EQ,
		dsp_hw_shared_memory[DONE_IRQ_OFFSET] & TEAK_INT_FINTA0_EQ);
	test_eq_u32("active working-bank metrics match the hardware burst vector", segment->metric_hash,
		hash_words(w2_metric_offset, 16));
	test_eq_u32("first working-bank paths match the hardware burst vector", segment->path_hash,
		hash_words(w1_path_offset, 16));
	test_eq_u32("second working-bank paths match the hardware burst vector", segment->path_hash,
		hash_words(w2_path_offset, 16));
	test_eq_u32("hard outputs match the hardware burst vector", segment->hard_hash,
		hash_words(HOUT_OUTPUT_OFFSET, 64));
	test_eq_u32("soft outputs match the hardware burst vector", segment->soft_hash,
		hash_words(SOUT_OUTPUT_OFFSET, 128));
	test_eq_u32("latency RAM matches the hardware burst vector", segment->latency_hash,
		hash_words(ELAT_OUTPUT_OFFSET, 64));
	printf("# EQ-BURST segment=%u side=%s count=%u scale=%04X metrics=%08X paths=%08X "
		"hard=%08X soft=%08X latency=%08X\n",
		(uint32_t) index, segment->right ? "right" : "left", (uint32_t) segment->count,
		(uint32_t) segment->scale, (uint32_t) hash_words(w2_metric_offset, 16),
		(uint32_t) hash_words(w2_path_offset, 16), (uint32_t) hash_words(HOUT_OUTPUT_OFFSET, 64),
		(uint32_t) hash_words(SOUT_OUTPUT_OFFSET, 128), (uint32_t) hash_words(ELAT_OUTPUT_OFFSET, 64));
	return true;
}

int main(void) {
	uint16_t completed[2] = { 0, 0 };

	test_start("DSP equalizer complete EDGE burst");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Equalizer / firmware-style six-segment EDGE burst");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load equalizer burst runner",
		dsp_hw_load_image(DSP_EQUALIZER_BURST_RUNNER, sizeof(DSP_EQUALIZER_BURST_RUNNER))))
		return test_finish();
	if (!test_check("BRANCH starts equalizer burst runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("equalizer burst runner becomes ready", dsp_hw_wait_shared(0, READY_MARKER, 100)))
		return test_finish();

	initialize_burst();
	for (size_t segment = 0; segment < ARRAY_SIZE(segments); segment++) {
		if (!run_segment(segment))
			break;
		completed[segments[segment].right] += segments[segment].count;
	}
	test_eq_u32("left half completes all 58 equalized symbols", 58, completed[false]);
	test_eq_u32("right half completes all 58 equalized symbols", 58, completed[true]);

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
