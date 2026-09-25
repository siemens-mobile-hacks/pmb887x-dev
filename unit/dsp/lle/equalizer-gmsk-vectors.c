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
#define CONTINUE_OFFSET 0x0006
#define CONTINUE_COUNT_OFFSET 0x0007
#define CONTINUE_FLAGS_OFFSET 0x0008
#define CONTINUE_SCALE_OFFSET 0x0009
#define CONTINUE2_OFFSET 0x000A
#define CONTINUE2_COUNT_OFFSET 0x000B
#define CONTINUE2_FLAGS_OFFSET 0x000C
#define CONTINUE2_SCALE_OFFSET 0x000D
#define INPUT_FIRST_OFFSET 0x0100
#define INPUT_LAST_OFFSET 0x02BF
#define W1_EMR_INPUT_OFFSET 0x0100
#define W1_EML_INPUT_OFFSET 0x0110
#define W1_EPR_INPUT_OFFSET 0x0120
#define W1_EPL_INPUT_OFFSET 0x0130
#define BPAR_INPUT_OFFSET 0x0200
#define RX_INPUT_OFFSET 0x0280
#define CONTINUE_BPAR_INPUT_OFFSET 0x0600
#define CONTINUE_RX_INPUT_OFFSET 0x0680
#define CONTINUE_STATUS_OFFSET 0x0700
#define CONTINUE_COUNT_RESULT_OFFSET 0x0701
#define CONTINUE_SOUT_OFFSET 0x0770
#define CONTINUE2_BPAR_INPUT_OFFSET 0x0800
#define CONTINUE2_RX_INPUT_OFFSET 0x0880
#define CONTINUE2_STATUS_OFFSET 0x0900
#define CONTINUE2_COUNT_RESULT_OFFSET 0x0901
#define CONTINUE2_SOUT_OFFSET 0x0930
#define DONE_STATUS_OFFSET 0x0302
#define DONE_IRQ_OFFSET 0x0304
#define W2_EMR_OUTPUT_OFFSET 0x0390
#define W2_EML_OUTPUT_OFFSET 0x03A0
#define W2_EPR_OUTPUT_OFFSET 0x03B0
#define W2_EPL_OUTPUT_OFFSET 0x03C0
#define W2_EB_OUTPUT_OFFSET 0x03D0
#define SOUT_OUTPUT_OFFSET 0x0450

#ifdef PMB8875
#include "equalizer-vector-runner-8875.inc"
#define DSP_EQUALIZER_GMSK_VECTOR_RUNNER DSP_EQUALIZER_VECTOR_RUNNER_8875
#else
#include "equalizer-vector-runner-8876.inc"
#define DSP_EQUALIZER_GMSK_VECTOR_RUNNER DSP_EQUALIZER_VECTOR_RUNNER_8876
#endif

static const uint16_t BRANCH_SELECTOR_METRICS[][2] = {
	{ 0xC020, 0xC028 }, { 0xC032, 0xC03C }, { 0xC048, 0xC054 }, { 0xC062, 0xC070 },
	{ 0xC080, 0xC090 }, { 0xC0A2, 0xC0B4 }, { 0xC0C8, 0xC0DC }, { 0xC0F2, 0xC108 },
	{ 0xC120, 0xC138 }, { 0xC152, 0xC16C }, { 0xC188, 0xC1A4 }, { 0xC1C2, 0xC1E0 },
	{ 0xC200, 0xC220 }, { 0xC242, 0xC264 }, { 0xC288, 0xC2AC }, { 0xC2D2, 0xC2F8 },
	{ 0xC320, 0xC348 }, { 0xC372, 0xC39C }, { 0xC3C8, 0xC3F4 }, { 0xC422, 0xC450 },
	{ 0xC480, 0xC4B0 }, { 0xC4E2, 0xC514 }, { 0xC548, 0xC57C }, { 0xC5B2, 0xC5E8 },
	{ 0xC620, 0xC658 }, { 0xC692, 0xC6CC }, { 0xC708, 0xC744 }, { 0xC782, 0xC7C0 },
	{ 0xC800, 0xC840 }, { 0xC882, 0xC8C4 }, { 0xC908, 0xC94C }, { 0xC992, 0xC9D8 },
};

static const uint16_t CONTINUATION_METRICS[] = {
	0xCC71, 0xCD2A, 0xCCA3, 0xCD87, 0xCC63, 0xCD23, 0xCC9C, 0xCD88,
	0xCC16, 0xCC40, 0xCC00, 0xCC56, 0xCBEA, 0xCC1B, 0xCBDB, 0xCC38,
};

static const uint16_t CONTINUATION_PATHS[] = {
	0x0000, 0x0004, 0x0002, 0x0006, 0x0001, 0x0005, 0x0003, 0x0007,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

static const uint16_t CONTINUATION_COMBINED[][3][24] = {
	{
		{
			0x0000, 0x0101, 0x0000, 0xFF01, 0x0000, 0x0101, 0x0000, 0x017F,
			0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
			0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
		},
		{
			0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x013E,
			0x0000, 0x477F, 0x0000, 0x3366, 0x0000, 0x0101, 0x0000, 0x017F,
			0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
		},
		{
			0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x0101,
			0x0000, 0xE37F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
			0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
		},
	},
	{
		{
			0x0000, 0x0101, 0x0000, 0x01FF, 0x0000, 0x0101, 0x0000, 0x7F01,
			0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
			0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
		},
		{
			0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x3E01,
			0x0000, 0x7F47, 0x0000, 0x6633, 0x0000, 0x0101, 0x0000, 0x7F01,
			0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
		},
		{
			0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x0101,
			0x0000, 0x7FE3, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
			0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
		},
	},
};

static uint16_t request_id;

static void clear_inputs(void) {
	dsp_hw_shared_memory[CONTINUE_OFFSET] = 0;
	dsp_hw_shared_memory[CONTINUE2_OFFSET] = 0;
	for (size_t offset = INPUT_FIRST_OFFSET; offset <= INPUT_LAST_OFFSET; offset++)
		dsp_hw_shared_memory[offset] = 0;
}

static bool run_vector(void) {
	request_id++;
	dsp_hw_shared_memory[RESPONSE_OFFSET] = 0;
	dsp_hw_shared_memory[REQUEST_OFFSET] = request_id;
	if (!dsp_hw_wait_shared(RESPONSE_OFFSET, request_id, 1000))
		return false;
	if (dsp_hw_shared_memory[REQUEST_OFFSET] != 0)
		return false;
	if (dsp_hw_shared_memory[DONE_STATUS_OFFSET] != 0)
		return false;
	return (dsp_hw_shared_memory[DONE_IRQ_OFFSET] & TEAK_INT_FINTA0_EQ) != 0;
}

static bool run_topology_probe(size_t predecessor) {
	uint16_t expected_metrics[16];
	uint16_t expected_paths[16] = { 0, 0, 0, 0, 1, 1, 1, 1 };
	uint16_t expected_zero[16] = { 0 };
	uint16_t expected_soft = (predecessor & 1) != 0 ? 0xFF81 : 0x007F;

	clear_inputs();
	for (size_t state = 0; state < 16; state++) {
		dsp_hw_shared_memory[W1_EML_INPUT_OFFSET + state] = 0x2000;
		dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET + state] = 0;
		expected_metrics[state] = 0x2000;
	}
	dsp_hw_shared_memory[COUNT_OFFSET] = 0;
	dsp_hw_shared_memory[FLAGS_OFFSET] = 0;
	dsp_hw_shared_memory[SCALE_OFFSET] = 0;
	dsp_hw_shared_memory[W1_EML_INPUT_OFFSET + predecessor] = 0xC000;
	dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET + (predecessor & 7)] = 0x0035;
	if (!run_vector())
		return false;

	expected_metrics[predecessor >> 1] = 0xC000;
	expected_metrics[(predecessor >> 1) + 8] = 0xC000;
	if ((predecessor & 1) == 0) {
		size_t path = (predecessor >> 1) & 3;

		expected_paths[path] = 0x006A;
		expected_paths[path + 4] = 0x006B;
	}

	test_eq_memory("GMSK ACS topology matches the hardware vector", expected_metrics,
		dsp_hw_shared_memory + W2_EML_OUTPUT_OFFSET, sizeof(expected_metrics));
	test_eq_memory("GMSK survivor-path topology matches the hardware vector", expected_paths,
		dsp_hw_shared_memory + W2_EPL_OUTPUT_OFFSET, sizeof(expected_paths));
	test_eq_memory("GMSK path updates do not overlap EB RAM", expected_zero,
		dsp_hw_shared_memory + W2_EB_OUTPUT_OFFSET, sizeof(expected_zero));
	test_eq_u32("GMSK soft output selects the metric hypothesis", expected_soft,
		dsp_hw_shared_memory[SOUT_OUTPUT_OFFSET]);
	return true;
}

static void load_segment_inputs(size_t bpar_offset, size_t rx_offset, size_t segment) {
	for (size_t branch = 0; branch < 64; branch++) {
		int32_t real = 0x0080 + segment * 0x0031 + branch * 0x001D;
		int32_t imaginary = 0x0040 + segment * 0x0025 + branch * 0x0013;

		if (((branch + segment) & 1) != 0)
			real = -real;
		if (((branch + segment) & 2) != 0)
			imaginary = -imaginary;
		dsp_hw_shared_memory[bpar_offset + branch * 2] = (uint16_t) real;
		dsp_hw_shared_memory[bpar_offset + branch * 2 + 1] = (uint16_t) imaginary;
	}
	for (size_t timestamp = 0; timestamp < 32; timestamp++) {
		int32_t real = -0x0380 + segment * 0x0043 + timestamp * 0x004D;
		int32_t imaginary = 0x0300 - segment * 0x0037 - timestamp * 0x0039;

		dsp_hw_shared_memory[rx_offset + timestamp * 2] = (uint16_t) real;
		dsp_hw_shared_memory[rx_offset + timestamp * 2 + 1] = (uint16_t) imaginary;
	}
}

static bool run_continuation_probe(bool right) {
	size_t metric_offset = right ? W1_EMR_INPUT_OFFSET : W1_EML_INPUT_OFFSET;
	size_t path_offset = right ? W1_EPR_INPUT_OFFSET : W1_EPL_INPUT_OFFSET;
	size_t metric_output_offset = right ? W2_EMR_OUTPUT_OFFSET : W2_EML_OUTPUT_OFFSET;
	size_t path_output_offset = right ? W2_EPR_OUTPUT_OFFSET : W2_EPL_OUTPUT_OFFSET;
	uint16_t flags = TEAK_EQ_CONF2_S_COMB | (right ? TEAK_EQ_CONF2_EQ_RIGHT : 0);

	clear_inputs();
	for (size_t state = 0; state < 16; state++)
		dsp_hw_shared_memory[metric_offset + state] = 0xC000 + state * 0x0100;
	for (size_t path = 0; path < 8; path++)
		dsp_hw_shared_memory[path_offset + path] = 0x0100 + path * 0x0017;
	load_segment_inputs(BPAR_INPUT_OFFSET, RX_INPUT_OFFSET, 0);
	load_segment_inputs(CONTINUE_BPAR_INPUT_OFFSET, CONTINUE_RX_INPUT_OFFSET, 1);
	load_segment_inputs(CONTINUE2_BPAR_INPUT_OFFSET, CONTINUE2_RX_INPUT_OFFSET, 2);
	dsp_hw_shared_memory[COUNT_OFFSET] = 18;
	dsp_hw_shared_memory[FLAGS_OFFSET] = flags;
	dsp_hw_shared_memory[SCALE_OFFSET] = 0;
	dsp_hw_shared_memory[CONTINUE_OFFSET] = 1;
	dsp_hw_shared_memory[CONTINUE_COUNT_OFFSET] = 14;
	dsp_hw_shared_memory[CONTINUE_FLAGS_OFFSET] = flags;
	dsp_hw_shared_memory[CONTINUE_SCALE_OFFSET] = 0;
	dsp_hw_shared_memory[CONTINUE2_OFFSET] = 1;
	dsp_hw_shared_memory[CONTINUE2_COUNT_OFFSET] = 11;
	dsp_hw_shared_memory[CONTINUE2_FLAGS_OFFSET] = flags;
	dsp_hw_shared_memory[CONTINUE2_SCALE_OFFSET] = 0;
	if (!run_vector())
		return false;

	test_eq_u32("continued GMSK segment finishes idle", 0, dsp_hw_shared_memory[CONTINUE_STATUS_OFFSET]);
	test_eq_u32("GMSK continuation keeps the hardware count result", 0,
		dsp_hw_shared_memory[CONTINUE_COUNT_RESULT_OFFSET]);
	test_eq_u32("third GMSK segment finishes idle", 0, dsp_hw_shared_memory[CONTINUE2_STATUS_OFFSET]);
	test_eq_u32("third GMSK segment keeps the hardware count result", 0,
		dsp_hw_shared_memory[CONTINUE2_COUNT_RESULT_OFFSET]);
	test_eq_memory("first GMSK segment produces the hardware metrics", CONTINUATION_METRICS,
		dsp_hw_shared_memory + metric_output_offset, sizeof(CONTINUATION_METRICS));
	test_eq_memory("first GMSK segment produces the hardware paths", CONTINUATION_PATHS,
		dsp_hw_shared_memory + path_output_offset, sizeof(CONTINUATION_PATHS));
	test_eq_memory("first GMSK segment produces the hardware combined output", CONTINUATION_COMBINED[right][0],
		dsp_hw_shared_memory + SOUT_OUTPUT_OFFSET, sizeof(CONTINUATION_COMBINED[right][0]));
	test_eq_memory("second GMSK segment preserves and extends the combined output", CONTINUATION_COMBINED[right][1],
		dsp_hw_shared_memory + CONTINUE_SOUT_OFFSET, sizeof(CONTINUATION_COMBINED[right][1]));
	test_eq_memory("third GMSK segment preserves and extends the combined output", CONTINUATION_COMBINED[right][2],
		dsp_hw_shared_memory + CONTINUE2_SOUT_OFFSET, sizeof(CONTINUATION_COMBINED[right][2]));
	return true;
}

static bool run_branch_selector_probe(size_t path) {
	clear_inputs();
	for (size_t state = 0; state < 16; state++)
		dsp_hw_shared_memory[W1_EML_INPUT_OFFSET + state] = 0x2000;
	dsp_hw_shared_memory[W1_EML_INPUT_OFFSET] = 0xC000;
	dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET] = path;
	for (size_t selector = 0; selector < 64; selector++)
		dsp_hw_shared_memory[BPAR_INPUT_OFFSET + selector * 2] = 0x0100 + selector * 0x0020;
	dsp_hw_shared_memory[COUNT_OFFSET] = 0;
	dsp_hw_shared_memory[FLAGS_OFFSET] = 0;
	dsp_hw_shared_memory[SCALE_OFFSET] = 0;
	if (!run_vector())
		return false;

	test_eq_u32("branch selector maps to the first hardware metric", BRANCH_SELECTOR_METRICS[path][0],
		dsp_hw_shared_memory[W2_EML_OUTPUT_OFFSET]);
	test_eq_u32("branch selector maps to the second hardware metric", BRANCH_SELECTOR_METRICS[path][1],
		dsp_hw_shared_memory[W2_EML_OUTPUT_OFFSET + 8]);
	return true;
}

int main(void) {
	test_start("DSP equalizer GMSK vectors");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Equalizer / GMSK forced-predecessor topology");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	bool loaded = dsp_hw_load_image(DSP_EQUALIZER_GMSK_VECTOR_RUNNER, sizeof(DSP_EQUALIZER_GMSK_VECTOR_RUNNER));
	if (!test_check("boot commands load equalizer vector runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts equalizer vector runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("equalizer vector runner becomes ready", dsp_hw_wait_shared(0, READY_MARKER, 100)))
		return test_finish();

	for (size_t predecessor = 0; predecessor < 16; predecessor++) {
		if (!test_check("forced predecessor vector completes", run_topology_probe(predecessor)))
			break;
	}
	test_category("Equalizer / GMSK branch selector mapping");
	for (size_t path = 0; path < ARRAY_SIZE(BRANCH_SELECTOR_METRICS); path++) {
		if (!test_check("branch selector vector completes", run_branch_selector_probe(path)))
			break;
	}
	test_category("Equalizer / GMSK 18+14+11 continuation");
	test_check("left GMSK continuation vector completes", run_continuation_probe(false));
	test_check("right GMSK continuation vector completes", run_continuation_probe(true));

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
