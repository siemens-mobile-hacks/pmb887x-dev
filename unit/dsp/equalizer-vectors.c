#include <pmb887x.h>
#include <gen/dsp.h>

#include <string.h>

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
#define INPUT_FIRST_OFFSET 0x0100
#define INPUT_LAST_OFFSET 0x02BF
#define W1_EMR_INPUT_OFFSET 0x0100
#define W1_EML_INPUT_OFFSET 0x0110
#define W1_EPR_INPUT_OFFSET 0x0120
#define W1_EPL_INPUT_OFFSET 0x0130
#define W2_EMR_INPUT_OFFSET 0x0180
#define W2_EML_INPUT_OFFSET 0x0190
#define W2_EPR_INPUT_OFFSET 0x01A0
#define W2_EPL_INPUT_OFFSET 0x01B0
#define BPAR_INPUT_OFFSET 0x0200
#define RX_INPUT_OFFSET 0x0280
#define CONTINUE_BPAR_INPUT_OFFSET 0x0600
#define CONTINUE_RX_INPUT_OFFSET 0x0680
#define CONTINUE_STATUS_OFFSET 0x0700
#define CONTINUE_COUNT_RESULT_OFFSET 0x0701
#define CONTINUE_IRQ_OFFSET 0x0702
#define CONTINUE_W1_EML_OFFSET 0x0710
#define CONTINUE_W1_EPL_OFFSET 0x0720
#define CONTINUE_W2_EML_OFFSET 0x0730
#define CONTINUE_W2_EPL_OFFSET 0x0740
#define CONTINUE_HOUT_OFFSET 0x0750
#define CONTINUE_SOUT_OFFSET 0x0770
#define CONTINUE_ELAT_OFFSET 0x0790
#define START_CONF2_OFFSET 0x0300
#define START_STATUS_OFFSET 0x0301
#define DONE_STATUS_OFFSET 0x0302
#define DONE_COUNT_OFFSET 0x0303
#define DONE_IRQ_OFFSET 0x0304
#define DONE_CONF2_OFFSET 0x0306
#define W1_EMR_OUTPUT_OFFSET 0x0310
#define W1_EML_OUTPUT_OFFSET 0x0320
#define W1_EPR_OUTPUT_OFFSET 0x0330
#define W1_EPL_OUTPUT_OFFSET 0x0340
#define W1_EB_OUTPUT_OFFSET 0x0350
#define W2_EMR_OUTPUT_OFFSET 0x0390
#define W2_EML_OUTPUT_OFFSET 0x03A0
#define W2_EPR_OUTPUT_OFFSET 0x03B0
#define W2_EPL_OUTPUT_OFFSET 0x03C0
#define W2_EB_OUTPUT_OFFSET 0x03D0
#define HOUT_OUTPUT_OFFSET 0x0410
#define SOUT_OUTPUT_OFFSET 0x0450
#define ELAT_OUTPUT_OFFSET 0x0510
#define SQUAL_OUTPUT_OFFSET 0x0550
#define SOUT_BEFORE_OFFSET 0x0580
#define MAX_PRINTED_DIFFERENCES 48

#ifdef PMB8875
#include "equalizer-vector-runner-8875.inc"
#define DSP_EQUALIZER_VECTOR_RUNNER DSP_EQUALIZER_VECTOR_RUNNER_8875
#else
#include "equalizer-vector-runner-8876.inc"
#define DSP_EQUALIZER_VECTOR_RUNNER DSP_EQUALIZER_VECTOR_RUNNER_8876
#endif

struct equalizer_vector {
	const char *name;
	uint16_t count;
	uint16_t flags;
	uint16_t scale;
	uint16_t first_offset;
	uint16_t first_value;
	uint16_t second_offset;
	uint16_t second_value;
};

struct output_region {
	const char *name;
	size_t offset;
	size_t words;
};

struct equalizer_model {
	int16_t metrics[8];
	uint16_t paths[8];
	uint16_t soft[8 * 4];
};

static const struct equalizer_vector vectors[] = {
	{ "zero-edge-count-0", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, 0, 0, 0, 0 },
	{ "zero-edge-count-1", 1, TEAK_EQ_CONF2_EQ_EDGE, 0, 0, 0, 0, 0 },
	{ "zero-edge-count-2", 2, TEAK_EQ_CONF2_EQ_EDGE, 0, 0, 0, 0, 0 },
	{ "zero-edge-count-7", 7, TEAK_EQ_CONF2_EQ_EDGE, 0, 0, 0, 0, 0 },
	{ "zero-no-edge", 0, 0, 0, 0, 0, 0, 0 },
	{ "zero-right", 0, TEAK_EQ_CONF2_EQ_EDGE | TEAK_EQ_CONF2_EQ_RIGHT, 0, 0, 0, 0, 0 },
	{ "w1-eml-index0-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W1_EML_INPUT_OFFSET, 1, 0, 0 },
	{ "w1-eml-index1-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W1_EML_INPUT_OFFSET + 1, 1, 0, 0 },
	{ "w1-emr-index0-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W1_EMR_INPUT_OFFSET, 1, 0, 0 },
	{ "w2-eml-index0-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET, 1, 0, 0 },
	{ "w2-eml-index1-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET + 1, 1, 0, 0 },
	{ "w2-eml-index14-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET + 14, 1, 0, 0 },
	{ "w2-emr-index0-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EMR_INPUT_OFFSET, 1, 0, 0 },
	{ "w2-eml-index0-max", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET, 0x7FFF, 0, 0 },
	{ "w2-eml-index0-min", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET, 0x8000, 0, 0 },
	{ "w2-eml-index0-minus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET, 0xFFE0, 0, 0 },
	{ "w2-eml-index2-minus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET + 2, 0xFFE0, 0, 0 },
	{ "w2-eml-index14-minus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET + 14, 0xFFE0, 0, 0 },
	{ "w1-epl-word0-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W1_EPL_INPUT_OFFSET, 1, 0, 0 },
	{ "w1-epl-word1-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W1_EPL_INPUT_OFFSET + 1, 1, 0, 0 },
	{ "w1-epr-word0-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W1_EPR_INPUT_OFFSET, 1, 0, 0 },
	{ "w2-epl-word0-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EPL_INPUT_OFFSET, 1, 0, 0 },
	{ "bpar-branch0-real-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET, 1, 0, 0 },
	{ "bpar-branch0-imag-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 1, 1, 0, 0 },
	{ "bpar-branch1-real-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 2, 1, 0, 0 },
	{ "bpar-branch7-real-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 14, 1, 0, 0 },
	{ "bpar-branch8-real-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 16, 1, 0, 0 },
	{ "bpar-branch31-real-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 62, 1, 0, 0 },
	{ "bpar-branch63-real-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 126, 1, 0, 0 },
	{ "bpar-branch0-real-minus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET, 0xFFFF, 0, 0 },
	{ "bpar-branch0-real-plus31", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET, 0x001F, 0, 0 },
	{ "bpar-branch0-real-plus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET, 0x0020, 0, 0 },
	{ "bpar-branch0-real-plus33", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET, 0x0021, 0, 0 },
	{ "bpar-branch0-real-minus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET, 0xFFE0, 0, 0 },
	{ "bpar-branch0-imag-plus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 1, 0x0020, 0, 0 },
	{ "bpar-branch1-real-plus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 2, 0x0020, 0, 0 },
	{ "bpar-branch7-real-plus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 14, 0x0020, 0, 0 },
	{ "bpar-branch8-real-plus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 16, 0x0020, 0, 0 },
	{ "bpar-branch63-real-plus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, BPAR_INPUT_OFFSET + 126, 0x0020, 0, 0 },
	{ "rx-symbol0-real-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 1, 0, 0 },
	{ "rx-symbol0-imag-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET + 1, 1, 0, 0 },
	{ "rx-symbol1-real-plus1", 1, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET + 2, 1, 0, 0 },
	{ "rx-symbol0-real-max", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 0x7FFF, 0, 0 },
	{ "rx-symbol0-real-min", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 0x8000, 0, 0 },
	{ "rx-symbol0-real-plus31", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 0x001F, 0, 0 },
	{ "rx-symbol0-real-plus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 0x0020, 0, 0 },
	{ "rx-symbol0-real-plus33", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 0x0021, 0, 0 },
	{ "rx-symbol0-real-plus63", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 0x003F, 0, 0 },
	{ "rx-symbol0-real-plus64", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 0x0040, 0, 0 },
	{ "rx-symbol0-real-minus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 0xFFE0, 0, 0 },
	{ "rx-symbol0-imag-plus32", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET + 1, 0x0020, 0, 0 },
	{ "rx-and-bpar-real-plus1", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, RX_INPUT_OFFSET, 1, BPAR_INPUT_OFFSET, 1 },
	{ "rx-real-plus32-scale-right1", 0, TEAK_EQ_CONF2_EQ_EDGE, 2, RX_INPUT_OFFSET, 0x0020, 0, 0 },
	{ "rx-real-plus32-saturate15", 0, TEAK_EQ_CONF2_EQ_EDGE, 0x0200, RX_INPUT_OFFSET, 0x0020, 0, 0 },
};

static const struct output_region output_regions[] = {
	{ "w1-emr", W1_EMR_OUTPUT_OFFSET, 16 },
	{ "w1-eml", W1_EML_OUTPUT_OFFSET, 16 },
	{ "w1-epr", W1_EPR_OUTPUT_OFFSET, 16 },
	{ "w1-epl", W1_EPL_OUTPUT_OFFSET, 16 },
	{ "w1-eb", W1_EB_OUTPUT_OFFSET, 64 },
	{ "w2-emr", W2_EMR_OUTPUT_OFFSET, 16 },
	{ "w2-eml", W2_EML_OUTPUT_OFFSET, 16 },
	{ "w2-epr", W2_EPR_OUTPUT_OFFSET, 16 },
	{ "w2-epl", W2_EPL_OUTPUT_OFFSET, 16 },
	{ "w2-eb", W2_EB_OUTPUT_OFFSET, 64 },
	{ "hout", HOUT_OUTPUT_OFFSET, 64 },
	{ "sout", SOUT_OUTPUT_OFFSET, 192 },
	{ "elat", ELAT_OUTPUT_OFFSET, 64 },
	{ "squal", SQUAL_OUTPUT_OFFSET, 16 },
	{ "sout-before", SOUT_BEFORE_OFFSET, 32 },
};

static const uint16_t amplitude_values[] = {
	0x0000, 0x0001, 0x000F, 0x0010, 0x001F, 0x0020, 0x002F, 0x0030,
	0x0031, 0x003F, 0x0040, 0x0041, 0x007F, 0x0080, 0x00FF, 0x0100,
	0x01FF, 0x0200, 0x03FF, 0x0400, 0x07FF, 0x0800, 0x0FFF, 0x1000,
	0x1FFF, 0x2000, 0x3FFF, 0x4000, 0x7FDF, 0x7FE0, 0x7FFF,
};

static const uint16_t selector_amplitudes[] = {
	0x0200, 0x0400, 0x0600, 0x0800, 0x0A00, 0x0C00, 0x0E00, 0x1000,
};

static const uint16_t soft_scale_values[] = {
	0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040,
	0x0080, 0x00FF, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000,
	0x4000, 0x8000, 0xA000, 0xC000, 0xE000, 0xFFFF,
};

static uint16_t baseline[ARRAY_SIZE(output_regions)][192];

static uint32_t hash_region(size_t offset, size_t words) {
	uint32_t hash = UINT32_C(2166136261);

	for (size_t i = 0; i < words; i++) {
		hash ^= dsp_hw_shared_memory[offset + i] & 0xFF;
		hash *= UINT32_C(16777619);
		hash ^= dsp_hw_shared_memory[offset + i] >> 8;
		hash *= UINT32_C(16777619);
	}
	return hash;
}

static void clear_inputs(void) {
	dsp_hw_shared_memory[CONTINUE_OFFSET] = 0;
	for (size_t offset = INPUT_FIRST_OFFSET; offset <= INPUT_LAST_OFFSET; offset++)
		dsp_hw_shared_memory[offset] = 0;
}

static void apply_vector(const struct equalizer_vector *vector) {
	clear_inputs();
	dsp_hw_shared_memory[COUNT_OFFSET] = vector->count;
	dsp_hw_shared_memory[FLAGS_OFFSET] = vector->flags;
	dsp_hw_shared_memory[SCALE_OFFSET] = vector->scale;
	if (vector->first_offset != 0)
		dsp_hw_shared_memory[vector->first_offset] = vector->first_value;
	if (vector->second_offset != 0)
		dsp_hw_shared_memory[vector->second_offset] = vector->second_value;
}

static void save_baseline(void) {
	for (size_t region = 0; region < ARRAY_SIZE(output_regions); region++) {
		for (size_t i = 0; i < output_regions[region].words; i++)
			baseline[region][i] = dsp_hw_shared_memory[output_regions[region].offset + i];
	}
}

static void print_region(size_t region, bool have_baseline) {
	const struct output_region *description = &output_regions[region];
	size_t differences = 0;
	size_t printed = 0;

	printf("#   %s hash=%08X", description->name,
		(uint32_t) hash_region(description->offset, description->words));
	for (size_t i = 0; i < description->words; i++) {
		uint16_t value = dsp_hw_shared_memory[description->offset + i];
		uint16_t reference = have_baseline ? baseline[region][i] : 0;

		if (value == reference)
			continue;
		differences++;
		if (printed < MAX_PRINTED_DIFFERENCES) {
			printf(" %u:%04X/%04X", (uint32_t) i, (uint32_t) reference, (uint32_t) value);
			printed++;
		}
	}
	printf(" diff=%u\n", (uint32_t) differences);
}

static bool run_vector(const struct equalizer_vector *vector, uint16_t request, bool have_baseline) {
	apply_vector(vector);
	dsp_hw_shared_memory[RESPONSE_OFFSET] = 0;
	dsp_hw_shared_memory[REQUEST_OFFSET] = request;
	if (!test_check(vector->name, dsp_hw_wait_shared(RESPONSE_OFFSET, request, 1000)))
		return false;

	test_eq_u32("runner consumes the request", 0, dsp_hw_shared_memory[REQUEST_OFFSET]);
	test_eq_u32("equalizer is idle after completion", 0, dsp_hw_shared_memory[DONE_STATUS_OFFSET]);
	test_eq_u32("equalizer completion raises its interrupt source", TEAK_INT_FINTA0_EQ,
		dsp_hw_shared_memory[DONE_IRQ_OFFSET] & TEAK_INT_FINTA0_EQ);
	printf("# EQV %02u %s count=%04X flags=%04X scale=%04X start=%04X/%04X done=%04X/%04X/%04X\n",
		(uint32_t) request, vector->name, (uint32_t) vector->count, (uint32_t) vector->flags,
		(uint32_t) vector->scale, (uint32_t) dsp_hw_shared_memory[START_CONF2_OFFSET],
		(uint32_t) dsp_hw_shared_memory[START_STATUS_OFFSET], (uint32_t) dsp_hw_shared_memory[DONE_CONF2_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DONE_STATUS_OFFSET], (uint32_t) dsp_hw_shared_memory[DONE_COUNT_OFFSET]);
	for (size_t region = 0; region < ARRAY_SIZE(output_regions); region++)
		print_region(region, have_baseline);
	return true;
}

static void print_words(size_t offset, size_t words) {
	for (size_t i = 0; i < words; i++)
		printf("%s%04X", i == 0 ? "" : ",", (uint32_t) dsp_hw_shared_memory[offset + i]);
}

static bool capture_compact_probe(const char *kind, size_t index, uint16_t value, uint16_t flags, uint16_t request) {
	bool right = flags & TEAK_EQ_CONF2_EQ_RIGHT;
	size_t metric_offset = right ? W1_EMR_OUTPUT_OFFSET : W1_EML_OUTPUT_OFFSET;
	size_t path_offset = right ? W1_EPR_OUTPUT_OFFSET : W1_EPL_OUTPUT_OFFSET;

	dsp_hw_shared_memory[RESPONSE_OFFSET] = 0;
	dsp_hw_shared_memory[REQUEST_OFFSET] = request;
	if (!dsp_hw_wait_shared(RESPONSE_OFFSET, request, 1000))
		return false;
	if (dsp_hw_shared_memory[REQUEST_OFFSET] != 0)
		return false;
	if (dsp_hw_shared_memory[DONE_STATUS_OFFSET] != 0)
		return false;
	if ((dsp_hw_shared_memory[DONE_IRQ_OFFSET] & TEAK_INT_FINTA0_EQ) == 0)
		return false;

	printf("# %s %03u %04X m=", kind, (uint32_t) index, (uint32_t) value);
	print_words(metric_offset, 16);
	printf(" p=");
	print_words(path_offset, 16);
	printf(" h=");
	print_words(HOUT_OUTPUT_OFFSET, 16);
	printf(" s=");
	print_words(SOUT_OUTPUT_OFFSET, 4);
	printf(" q=");
	print_words(SQUAL_OUTPUT_OFFSET, 16);
	printf("\n");
	return true;
}

static bool run_compact_probe(const char *kind, size_t index, uint16_t value, const struct equalizer_vector *vector,
	uint16_t request)
{
	apply_vector(vector);
	return capture_compact_probe(kind, index, value, vector->flags, request);
}

static uint16_t model_squared_component(uint16_t value) {
	int32_t signed_value = (int16_t) value;
	uint32_t square = signed_value * signed_value;

	return square >> 11 & 0x7FFF;
}

static int16_t model_saturate_int16(int32_t value) {
	if (value > INT16_MAX)
		return INT16_MAX;
	if (value < INT16_MIN)
		return INT16_MIN;
	return value;
}

static uint16_t model_saturating_add(uint16_t first, uint16_t second) {
	int32_t sum = (int16_t) first + (int16_t) second;

	return (uint16_t) model_saturate_int16(sum);
}

static void load_selector_group(size_t group) {
	for (size_t symbol = 0; symbol < ARRAY_SIZE(selector_amplitudes); symbol++)
		dsp_hw_shared_memory[BPAR_INPUT_OFFSET + (group * 8 + symbol) * 2] = selector_amplitudes[symbol];
}

static bool check_forced_transition(size_t predecessor, uint16_t path, size_t group) {
	for (size_t next = 0; next < 8; next++) {
		size_t selector;

		if (group == 0) {
			selector = next;
		} else if (group < 6) {
			selector = path >> ((group - 1) * 3) & 7;
		} else {
			selector = path >> 15 & 7;
		}
		uint16_t expected_metric = 0xC000 + model_squared_component(selector_amplitudes[selector]);
		uint16_t expected_path = path << 3 | next;

		if (dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET + next] != expected_metric)
			return false;
		if (dsp_hw_shared_memory[W1_EPL_OUTPUT_OFFSET + next] != expected_path)
			return false;
	}
	for (size_t bit = 0; bit < 3; bit++) {
		bool negative = predecessor >> bit & 1;
		uint16_t expected_soft = negative ? 0xFF81 : 0x007F;

		if (dsp_hw_shared_memory[SOUT_OUTPUT_OFFSET + bit] != expected_soft)
			return false;
	}
	for (size_t i = 0; i < 16; i++) {
		uint16_t expected_hard = 0;

		if (i >= 3 && i <= 8) {
			size_t path_group = 9 - i;

			expected_hard = path >> ((path_group - 1) * 3) & 7;
		}
		if (dsp_hw_shared_memory[HOUT_OUTPUT_OFFSET + i] != expected_hard)
			return false;
	}
	for (size_t i = 0; i < 16; i++) {
		if (dsp_hw_shared_memory[SQUAL_OUTPUT_OFFSET + i] != (i == 0 ? 3 : 0))
			return false;
	}
	return true;
}

static bool run_forced_transition_probe(size_t predecessor, uint16_t path, size_t group, uint16_t request) {
	clear_inputs();
	dsp_hw_shared_memory[COUNT_OFFSET] = 0;
	dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
	dsp_hw_shared_memory[SCALE_OFFSET] = 0;
	dsp_hw_shared_memory[W2_EML_INPUT_OFFSET + predecessor] = 0xC000;
	dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET + predecessor] = path;
	load_selector_group(group);
	if (!capture_compact_probe("EQT", group * 8 + predecessor, path, TEAK_EQ_CONF2_EQ_EDGE, request))
		return false;
	return check_forced_transition(predecessor, path, group);
}

static bool run_transition_topology_probes(uint16_t *request) {
	for (size_t predecessor = 0; predecessor < 8; predecessor++) {
		if (!run_forced_transition_probe(predecessor, 0, 1, (*request)++))
			return false;
	}
	if (!run_forced_transition_probe(0, 0, 0, (*request)++))
		return false;

	uint16_t path = 1 | 2 << 3 | 3 << 6 | 4 << 9 | 5 << 12;

	for (size_t group = 1; group < 7; group++) {
		if (!run_forced_transition_probe(0, path, group, (*request)++))
			return false;
	}
	if (!run_forced_transition_probe(0, 0x8000, 6, (*request)++))
		return false;
	if (!run_forced_transition_probe(0, 0xFFFF, 6, (*request)++))
		return false;
	return true;
}

static bool run_last_tap_state_probes(uint16_t *request) {
	for (size_t predecessor = 0; predecessor < 8; predecessor++) {
		for (size_t high_bit = 0; high_bit < 2; high_bit++) {
			clear_inputs();
			dsp_hw_shared_memory[COUNT_OFFSET] = 0;
			dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
			dsp_hw_shared_memory[SCALE_OFFSET] = 0;
			dsp_hw_shared_memory[W2_EML_INPUT_OFFSET + predecessor] = 0xC000;
			dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET + predecessor] = high_bit << 15;
			load_selector_group(6);
			if (!capture_compact_probe("EQT6", predecessor * 2 + high_bit, high_bit << 15,
				TEAK_EQ_CONF2_EQ_EDGE, (*request)++))
				return false;
		}
	}
	return true;
}

static void load_constant_branch_sequence(uint16_t value) {
	for (size_t symbol = 0; symbol < 8; symbol++)
		dsp_hw_shared_memory[BPAR_INPUT_OFFSET + symbol * 2] = value;
	for (size_t group = 1; group < 7; group++)
		dsp_hw_shared_memory[BPAR_INPUT_OFFSET + group * 16] = value;
}

static bool run_metric_sum_probe(const char *kind, size_t index, uint16_t branch_value, uint16_t rx_real,
	uint16_t rx_imaginary, uint16_t expected_metric, uint16_t request)
{
	clear_inputs();
	dsp_hw_shared_memory[COUNT_OFFSET] = 0;
	dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
	dsp_hw_shared_memory[SCALE_OFFSET] = 0;
	dsp_hw_shared_memory[W2_EML_INPUT_OFFSET] = 0xC000;
	dsp_hw_shared_memory[RX_INPUT_OFFSET] = rx_real;
	dsp_hw_shared_memory[RX_INPUT_OFFSET + 1] = rx_imaginary;
	load_constant_branch_sequence(branch_value);
	if (!capture_compact_probe(kind, index, branch_value, TEAK_EQ_CONF2_EQ_EDGE, request))
		return false;
	for (size_t next = 0; next < 8; next++) {
		if (dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET + next] != expected_metric)
			return false;
	}
	return true;
}

static bool run_metric_sum_probes(uint16_t *request) {
	if (!run_metric_sum_probe("EQS", 0, 0x0200, 0x0200, 0, 0xE000, (*request)++))
		return false;
	if (!run_metric_sum_probe("EQS", 1, 0x2000, 0x2000, 0, 0x3FE0, (*request)++))
		return false;
	if (!run_metric_sum_probe("EQS", 2, 0xE000, 0xE000, 0, 0xC000, (*request)++))
		return false;
	return run_metric_sum_probe("EQS", 3, 0, 0x1000, 0x0800, 0xE800, (*request)++);
}

static int32_t model_arithmetic_shift_right(int32_t value, size_t shift) {
	if (value >= 0)
		return value >> shift;
	return -((-value + (1 << shift) - 1) >> shift);
}

static uint16_t model_scale_soft(int16_t value, uint16_t scale) {
	uint16_t first_stage = scale & 0x00FF;
	int32_t scaled = value;

	if (first_stage == 0x00FF) {
		scaled *= 2;
	} else if (first_stage != 0) {
		size_t shift = 0;

		for (size_t bit = 0; bit < 8; bit++) {
			if ((first_stage & 1 << bit) != 0)
				shift = bit + 1;
		}
		scaled = model_arithmetic_shift_right(scaled, shift);
	}
	if ((scale & 0x0100) != 0)
		scaled = model_arithmetic_shift_right(scaled, 3);

	int32_t shifted = scaled;

	if ((scale & 0x2000) != 0)
		scaled += model_arithmetic_shift_right(shifted, 3);
	if ((scale & 0x4000) != 0)
		scaled += model_arithmetic_shift_right(shifted, 2);
	if ((scale & 0x8000) != 0)
		scaled += model_arithmetic_shift_right(shifted, 1);

	uint16_t saturation = scale & 0x1E00;
	int32_t limit = 127;

	if (saturation == 0x0200) {
		limit = 15;
	} else if (saturation == 0x0400) {
		limit = 31;
	} else if (saturation == 0x0800) {
		limit = 63;
	}
	if (scaled > limit)
		scaled = limit;
	if (scaled < -limit)
		scaled = -limit;
	return (uint16_t) (int16_t) scaled;
}

static bool run_soft_scale_probes(uint16_t *request) {
	struct equalizer_vector vector = {
		"soft-scale", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, W2_EML_INPUT_OFFSET + 5, 0xFFE0, 0, 0,
	};

	for (size_t i = 0; i < ARRAY_SIZE(soft_scale_values); i++) {
		vector.scale = soft_scale_values[i];
		if (!run_compact_probe("EQSC", i, vector.scale, &vector, (*request)++))
			return false;
		if (dsp_hw_shared_memory[SOUT_OUTPUT_OFFSET] != model_scale_soft(-32, vector.scale))
			return false;
		if (dsp_hw_shared_memory[SOUT_OUTPUT_OFFSET + 1] != model_scale_soft(32, vector.scale))
			return false;
		if (dsp_hw_shared_memory[SOUT_OUTPUT_OFFSET + 2] != model_scale_soft(-32, vector.scale))
			return false;
	}
	return true;
}

static void load_multistep_inputs(void) {
	clear_inputs();
	for (size_t state = 0; state < 8; state++) {
		dsp_hw_shared_memory[W2_EML_INPUT_OFFSET + state] = 0xC000 + state * 0x0100;
		dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET + state] = state | (7 - state) << 3 | state << 6 |
			(7 - state) << 9 | state << 12;
	}
	for (size_t group = 0; group < 7; group++) {
		for (size_t selector = 0; selector < 8; selector++) {
			size_t offset = BPAR_INPUT_OFFSET + (group * 8 + selector) * 2;

			dsp_hw_shared_memory[offset] = 0x0100 + group * 0x40 + selector * 0x20;
			dsp_hw_shared_memory[offset + 1] = (uint16_t) -(0x0080 + group * 0x20 + selector * 0x10);
		}
	}
	for (size_t timestamp = 0; timestamp < 8; timestamp++) {
		dsp_hw_shared_memory[RX_INPUT_OFFSET + timestamp * 2] = (uint16_t) ((int32_t) timestamp * 0x60 - 0x140);
		dsp_hw_shared_memory[RX_INPUT_OFFSET + timestamp * 2 + 1] = 0x0180 - timestamp * 0x40;
	}
}

static uint16_t model_branch_metric(uint16_t path, size_t next, size_t timestamp) {
	int16_t real = dsp_hw_shared_memory[RX_INPUT_OFFSET + timestamp * 2];
	int16_t imaginary = dsp_hw_shared_memory[RX_INPUT_OFFSET + timestamp * 2 + 1];

	for (size_t group = 0; group < 7; group++) {
		size_t selector;

		if (group == 0) {
			selector = next;
		} else if (group < 6) {
			selector = path >> ((group - 1) * 3) & 7;
		} else {
			selector = path >> 15 ? 7 : 0;
		}
		size_t offset = BPAR_INPUT_OFFSET + (group * 8 + selector) * 2;

		real = model_saturate_int16(real + (int16_t) dsp_hw_shared_memory[offset]);
		imaginary = model_saturate_int16(imaginary + (int16_t) dsp_hw_shared_memory[offset + 1]);
	}
	uint32_t real_square = (int32_t) real * real;
	uint32_t imaginary_square = (int32_t) imaginary * imaginary;
	uint32_t distance = real_square + imaginary_square;

	return distance >> 11 & 0x7FFF;
}

static void model_equalizer_step(struct equalizer_model *model, size_t timestamp, uint16_t scale) {
	int16_t candidates[8][8];
	int16_t next_metrics[8];
	uint16_t next_paths[8];

	for (size_t next = 0; next < 8; next++) {
		size_t best_predecessor = 0;

		for (size_t predecessor = 0; predecessor < 8; predecessor++) {
			uint16_t branch = model_branch_metric(model->paths[predecessor], next, timestamp);

			candidates[next][predecessor] = model_saturate_int16(model->metrics[predecessor] + branch);
			if (candidates[next][predecessor] < candidates[next][best_predecessor])
				best_predecessor = predecessor;
		}
		next_metrics[next] = candidates[next][best_predecessor];
		next_paths[next] = model->paths[best_predecessor] << 3 | next;
	}
	for (size_t bit = 0; bit < 3; bit++) {
		int16_t minimum[2] = { INT16_MAX, INT16_MAX };

		for (size_t next = 0; next < 8; next++) {
			for (size_t predecessor = 0; predecessor < 8; predecessor++) {
				size_t hypothesis = predecessor >> bit & 1;

				if (candidates[next][predecessor] < minimum[hypothesis])
					minimum[hypothesis] = candidates[next][predecessor];
			}
		}
		model->soft[timestamp * 4 + bit] = model_scale_soft(minimum[1] - minimum[0], scale);
	}
	model->soft[timestamp * 4 + 3] = 0;
	memcpy(model->metrics, next_metrics, sizeof(next_metrics));
	memcpy(model->paths, next_paths, sizeof(next_paths));
}

static void check_multistep_model(size_t count, uint16_t scale) {
	struct equalizer_model model = { 0 };
	uint16_t expected_metrics[8];
	size_t metric_offset = count & 1 ? W1_EML_OUTPUT_OFFSET : W2_EML_OUTPUT_OFFSET;
	size_t path_offset = count & 1 ? W1_EPL_OUTPUT_OFFSET : W2_EPL_OUTPUT_OFFSET;

	for (size_t state = 0; state < 8; state++) {
		model.metrics[state] = dsp_hw_shared_memory[W2_EML_INPUT_OFFSET + state];
		model.paths[state] = dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET + state];
	}
	for (size_t timestamp = 0; timestamp < count; timestamp++)
		model_equalizer_step(&model, timestamp, scale);
	for (size_t state = 0; state < 8; state++)
		expected_metrics[state] = model.metrics[state];

	test_eq_memory("independent ACS model predicts all survivor metrics", expected_metrics,
		dsp_hw_shared_memory + metric_offset, sizeof(expected_metrics));
	test_eq_memory("independent ACS model predicts all survivor paths", model.paths,
		dsp_hw_shared_memory + path_offset, sizeof(model.paths));
	test_eq_memory("independent max-log model predicts every soft output", model.soft,
		dsp_hw_shared_memory + SOUT_OUTPUT_OFFSET, count * 4 * sizeof(model.soft[0]));
}

static bool run_multistep_characterization(uint16_t *request) {
	static const uint16_t counts[] = { 1, 2, 3, 4, 5, 6, 7 };

	test_category("Equalizer / independent multi-timestamp ACS model");

	for (size_t i = 0; i < ARRAY_SIZE(counts); i++) {
		load_multistep_inputs();
		dsp_hw_shared_memory[COUNT_OFFSET] = counts[i];
		dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
		dsp_hw_shared_memory[SCALE_OFFSET] = 0;
		if (!capture_compact_probe("EQTS", counts[i], counts[i], TEAK_EQ_CONF2_EQ_EDGE, (*request)++))
			return false;

		printf("# EQTS-DETAIL count=%u w1m=", (uint32_t) counts[i]);
		print_words(W1_EML_OUTPUT_OFFSET, 16);
		printf(" w2m=");
		print_words(W2_EML_OUTPUT_OFFSET, 16);
		printf(" w1p=");
		print_words(W1_EPL_OUTPUT_OFFSET, 16);
		printf(" w2p=");
		print_words(W2_EPL_OUTPUT_OFFSET, 16);
		printf(" h=");
		print_words(HOUT_OUTPUT_OFFSET, 24);
		printf(" s=");
		print_words(SOUT_OUTPUT_OFFSET, 24);
		printf(" e=");
		print_words(ELAT_OUTPUT_OFFSET, 16);
		printf(" q=");
		print_words(SQUAL_OUTPUT_OFFSET, 16);
		printf("\n");
		check_multistep_model(counts[i], 0);
	}

	load_multistep_inputs();
	dsp_hw_shared_memory[COUNT_OFFSET] = 7;
	dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
	dsp_hw_shared_memory[SCALE_OFFSET] = 0x0008;
	if (!capture_compact_probe("EQTS-SCALED", 7, 0x0008, TEAK_EQ_CONF2_EQ_EDGE, (*request)++))
		return false;
	check_multistep_model(7, 0x0008);
	return true;
}

static bool run_multistep_branch_probes(uint16_t *request) {
	static const uint16_t paths[] = { 0x0E38, 0x71C0, 0x8E00, 0x7000, 0x8000, 0x0000, 0x0000, 0x0000 };

	for (size_t timestamp = 0; timestamp < ARRAY_SIZE(paths); timestamp++) {
		load_multistep_inputs();
		for (size_t i = 0; i < 16; i++) {
			dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET + i] = 0;
			dsp_hw_shared_memory[W2_EML_INPUT_OFFSET + i] = 0;
		}
		dsp_hw_shared_memory[COUNT_OFFSET] = 0;
		dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
		dsp_hw_shared_memory[SCALE_OFFSET] = 0;
		dsp_hw_shared_memory[W1_EPL_INPUT_OFFSET] = paths[timestamp];
		dsp_hw_shared_memory[W2_EML_INPUT_OFFSET] = 0xC000;
		dsp_hw_shared_memory[RX_INPUT_OFFSET] = (uint16_t) ((int32_t) timestamp * 0x60 - 0x140);
		dsp_hw_shared_memory[RX_INPUT_OFFSET + 1] = 0x0180 - timestamp * 0x40;
		if (!capture_compact_probe("EQTB", timestamp, paths[timestamp], TEAK_EQ_CONF2_EQ_EDGE, (*request)++))
			return false;
	}
	return true;
}

static const uint16_t output_mode_hard_expected[32] = {
	0, 0, 0, 0, 0, 7, 0, 7,
};

static const uint16_t output_mode_raw_expected[24] = {
	0x007F, 0x007F, 0x007F, 0x0000, 0x007F, 0x007F, 0x007F, 0x0000,
	0x007F, 0x007F, 0x007F, 0x0000, 0x007F, 0x007F, 0x007F, 0x0000,
	0x007F, 0x007F, 0x007F, 0x0000, 0x007F, 0x007F, 0x007F, 0x0000,
};

static const uint16_t output_mode_combined_expected[24] = {
	0x0000, 0x0000, 0x0000, 0x0101, 0x0000, 0x0101, 0x0000, 0x01FF,
	0x0000, 0x7FFF, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F,
	0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F01, 0x0000, 0x0101,
};

static const uint16_t output_mode_segment_expected[24] = {
	0x0000, 0x0000, 0x0001, 0x0000, 0x0101, 0x0000, 0x0101, 0x0000,
	0xFF7F, 0x0000, 0xFF7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000,
	0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x7F7F, 0x0000, 0x0101, 0x0000,
};

static const uint16_t output_mode_latency_expected[32] = {
	0x1AEA, 0x1B6E, 0x1BF3, 0x1C7A, 0x1D01, 0x1D8A, 0x1E14, 0x1EA0,
};

static const uint16_t continuation_w1_path_expected[16] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x1AA2, 0x1BAA, 0x1CB6, 0x1DC8, 0x1B25, 0x1C2F, 0x1D3E, 0x1E52,
};

static const uint16_t continuation_w2_path_expected[16] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x1909, 0x1A08, 0x1B0D, 0x1C16, 0x1988, 0x1A8A, 0x1B91, 0x1C9D,
};

static const uint16_t continuation_hard_expected[32] = {
	0, 0, 7, 0, 7,
};

static const uint16_t continuation_soft_expected[32] = {
	0x007F, 0x007F, 0x007F, 0x0000, 0x007F, 0x007F, 0x007F, 0x0000,
	0x007F, 0x007F, 0x007F, 0x0000, 0x007F, 0x007F, 0x007F, 0x0000,
	0x007F, 0x007F, 0x007F, 0x0000,
};

static const uint16_t continuation_latency_expected[16] = {
	0x1E52, 0x1EDE, 0x1F6B, 0x1FFA, 0x2089, 0x211A, 0x21AC, 0x2240,
};

static bool run_output_mode_characterization(uint16_t *request) {
	static const uint16_t mode_flags[] = {
		0,
		TEAK_EQ_CONF2_S_SEG,
		TEAK_EQ_CONF2_S_COMB,
		TEAK_EQ_CONF2_S_COMB | TEAK_EQ_CONF2_S_SEG,
	};
	static const uint16_t *const soft_expected[] = {
		output_mode_raw_expected,
		output_mode_raw_expected,
		output_mode_combined_expected,
		output_mode_segment_expected,
	};
	static const char *const soft_names[] = {
		"raw SOUT matches the hardware vector",
		"S_SEG alone preserves raw SOUT",
		"S_COMB corrects signs and packs SOUT",
		"S_COMB with S_SEG uses adjacent-segment packing",
	};

	test_category("Equalizer / hard-soft output modes and latency");

	for (size_t mode = 0; mode < ARRAY_SIZE(mode_flags); mode++) {
		load_multistep_inputs();
		dsp_hw_shared_memory[COUNT_OFFSET] = 16;
		dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE | mode_flags[mode];
		dsp_hw_shared_memory[SCALE_OFFSET] = 0;
		if (!capture_compact_probe("EQOM", mode, mode_flags[mode],
			TEAK_EQ_CONF2_EQ_EDGE | mode_flags[mode], (*request)++))
			return false;
		test_eq_memory("HOUT is independent of the SOUT read mode", output_mode_hard_expected,
			dsp_hw_shared_memory + HOUT_OUTPUT_OFFSET, sizeof(output_mode_hard_expected));
		test_eq_memory(soft_names[mode], soft_expected[mode], dsp_hw_shared_memory + SOUT_OUTPUT_OFFSET,
			sizeof(output_mode_raw_expected));
		test_eq_memory("ELAT is independent of the SOUT read mode", output_mode_latency_expected,
			dsp_hw_shared_memory + ELAT_OUTPUT_OFFSET, sizeof(output_mode_latency_expected));
	}
	return true;
}

static bool run_continuation_test(uint16_t *request) {
	static uint16_t reference_w1_eml[16];
	static uint16_t reference_w1_epl[16];
	static uint16_t reference_w2_eml[16];
	static uint16_t reference_w2_epl[16];

	load_multistep_inputs();
	dsp_hw_shared_memory[COUNT_OFFSET] = 10;
	dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
	dsp_hw_shared_memory[SCALE_OFFSET] = 0;
	if (!capture_compact_probe("EQ1S", 10, 10, TEAK_EQ_CONF2_EQ_EDGE, (*request)++))
		return false;
	for (size_t i = 0; i < 16; i++) {
		reference_w1_eml[i] = dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET + i];
		reference_w1_epl[i] = dsp_hw_shared_memory[W1_EPL_OUTPUT_OFFSET + i];
		reference_w2_eml[i] = dsp_hw_shared_memory[W2_EML_OUTPUT_OFFSET + i];
		reference_w2_epl[i] = dsp_hw_shared_memory[W2_EPL_OUTPUT_OFFSET + i];
	}

	load_multistep_inputs();
	for (size_t i = 0; i < 128; i++)
		dsp_hw_shared_memory[CONTINUE_BPAR_INPUT_OFFSET + i] = dsp_hw_shared_memory[BPAR_INPUT_OFFSET + i];
	for (size_t i = 0; i < 64; i++)
		dsp_hw_shared_memory[CONTINUE_RX_INPUT_OFFSET + i] = dsp_hw_shared_memory[RX_INPUT_OFFSET + i];
	dsp_hw_shared_memory[COUNT_OFFSET] = 3;
	dsp_hw_shared_memory[FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
	dsp_hw_shared_memory[SCALE_OFFSET] = 0;
	dsp_hw_shared_memory[CONTINUE_OFFSET] = 1;
	dsp_hw_shared_memory[CONTINUE_COUNT_OFFSET] = 7;
	dsp_hw_shared_memory[CONTINUE_FLAGS_OFFSET] = TEAK_EQ_CONF2_EQ_EDGE;
	dsp_hw_shared_memory[CONTINUE_SCALE_OFFSET] = 0;

	if (!capture_compact_probe("EQ2S", 3, 7, TEAK_EQ_CONF2_EQ_EDGE, (*request)++))
		return false;
	test_eq_u32("continued segment finishes idle", 0, dsp_hw_shared_memory[CONTINUE_STATUS_OFFSET]);
	test_eq_u32("second segment reports its own timestamp count", 7,
		dsp_hw_shared_memory[CONTINUE_COUNT_RESULT_OFFSET]);
	test_eq_u32("continued segment raises a fresh completion source", TEAK_INT_FINTA0_EQ,
		dsp_hw_shared_memory[CONTINUE_IRQ_OFFSET] & TEAK_INT_FINTA0_EQ);
	test_eq_memory("3+7 W1 metrics equal one uninterrupted 10-timestamp run", reference_w1_eml,
		dsp_hw_shared_memory + CONTINUE_W1_EML_OFFSET, sizeof(reference_w1_eml));
	test_eq_memory("3+7 W1 survivor paths equal one uninterrupted 10-timestamp run", reference_w1_epl,
		dsp_hw_shared_memory + CONTINUE_W1_EPL_OFFSET, 8 * sizeof(reference_w1_epl[0]));
	test_eq_memory("3+7 W2 metrics equal one uninterrupted 10-timestamp run", reference_w2_eml,
		dsp_hw_shared_memory + CONTINUE_W2_EML_OFFSET, sizeof(reference_w2_eml));
	test_eq_memory("3+7 W2 survivor paths equal one uninterrupted 10-timestamp run", reference_w2_epl,
		dsp_hw_shared_memory + CONTINUE_W2_EPL_OFFSET, 8 * sizeof(reference_w2_epl[0]));
	test_eq_memory("segment boundary selects the hardware W1 scratch-path layout", continuation_w1_path_expected,
		dsp_hw_shared_memory + CONTINUE_W1_EPL_OFFSET, sizeof(continuation_w1_path_expected));
	test_eq_memory("segment boundary selects the hardware W2 scratch-path layout", continuation_w2_path_expected,
		dsp_hw_shared_memory + CONTINUE_W2_EPL_OFFSET, sizeof(continuation_w2_path_expected));
	test_eq_memory("second segment produces the hardware hard-output vector", continuation_hard_expected,
		dsp_hw_shared_memory + CONTINUE_HOUT_OFFSET, sizeof(continuation_hard_expected));
	test_eq_memory("second segment produces five delayed soft symbols", continuation_soft_expected,
		dsp_hw_shared_memory + CONTINUE_SOUT_OFFSET, sizeof(continuation_soft_expected));
	test_eq_memory("second segment produces the hardware latency vector", continuation_latency_expected,
		dsp_hw_shared_memory + CONTINUE_ELAT_OFFSET, sizeof(continuation_latency_expected));
	return true;
}

static bool run_one_hot_matrices(void) {
	struct equalizer_vector vector = { "matrix", 0, TEAK_EQ_CONF2_EQ_EDGE, 0, 0, 0, 0, 0 };
	uint16_t request = 100;

	for (size_t state = 0; state < 16; state++) {
		vector.first_offset = W2_EML_INPUT_OFFSET + state;
		vector.first_value = 0xFFE0;
		if (!run_compact_probe("EQM-L", state, vector.first_value, &vector, request++))
			return false;
	}
	vector.flags = TEAK_EQ_CONF2_EQ_EDGE | TEAK_EQ_CONF2_EQ_RIGHT;
	for (size_t state = 0; state < 16; state++) {
		vector.first_offset = W2_EMR_INPUT_OFFSET + state;
		if (!run_compact_probe("EQM-R", state, vector.first_value, &vector, request++))
			return false;
	}

	vector.flags = TEAK_EQ_CONF2_EQ_EDGE;
	vector.first_value = 1;
	for (size_t state = 0; state < 16; state++) {
		vector.first_offset = W1_EPL_INPUT_OFFSET + state;
		if (!run_compact_probe("EQP-L", state, vector.first_value, &vector, request++))
			return false;
	}
	vector.flags = TEAK_EQ_CONF2_EQ_EDGE | TEAK_EQ_CONF2_EQ_RIGHT;
	for (size_t state = 0; state < 16; state++) {
		vector.first_offset = W1_EPR_INPUT_OFFSET + state;
		if (!run_compact_probe("EQP-R", state, vector.first_value, &vector, request++))
			return false;
	}

	vector.flags = TEAK_EQ_CONF2_EQ_EDGE;
	for (size_t halfword = 0; halfword < 128; halfword++) {
		vector.first_offset = BPAR_INPUT_OFFSET + halfword;
		vector.first_value = 0x7FE0;
		if (!run_compact_probe("EQB+", halfword, vector.first_value, &vector, request++))
			return false;
	}
	for (size_t halfword = 0; halfword < 128; halfword++) {
		vector.first_offset = BPAR_INPUT_OFFSET + halfword;
		vector.first_value = 0x8020;
		if (!run_compact_probe("EQB-", halfword, vector.first_value, &vector, request++))
			return false;
	}

	for (size_t i = 0; i < ARRAY_SIZE(amplitude_values); i++) {
		vector.first_offset = BPAR_INPUT_OFFSET;
		vector.first_value = amplitude_values[i];
		vector.second_offset = 0;
		if (!run_compact_probe("EQA-B+", i, vector.first_value, &vector, request++))
			return false;
		if (dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET] != model_squared_component(vector.first_value))
			return false;
	}
	for (size_t i = 1; i < ARRAY_SIZE(amplitude_values); i++) {
		vector.first_value = (uint16_t) -amplitude_values[i];
		if (!run_compact_probe("EQA-B-", i, vector.first_value, &vector, request++))
			return false;
		if (dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET] != model_squared_component(vector.first_value))
			return false;
	}
	for (size_t i = 0; i < ARRAY_SIZE(amplitude_values); i++) {
		vector.first_offset = RX_INPUT_OFFSET;
		vector.first_value = amplitude_values[i];
		if (!run_compact_probe("EQA-R+", i, vector.first_value, &vector, request++))
			return false;
		if (dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET] != model_squared_component(vector.first_value))
			return false;
	}
	for (size_t i = 1; i < ARRAY_SIZE(amplitude_values); i++) {
		vector.first_value = (uint16_t) -amplitude_values[i];
		if (!run_compact_probe("EQA-R-", i, vector.first_value, &vector, request++))
			return false;
		if (dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET] != model_squared_component(vector.first_value))
			return false;
	}
	for (size_t i = 0; i < ARRAY_SIZE(amplitude_values); i++) {
		vector.first_offset = RX_INPUT_OFFSET;
		vector.first_value = amplitude_values[i];
		vector.second_offset = BPAR_INPUT_OFFSET;
		vector.second_value = amplitude_values[i];
		if (!run_compact_probe("EQA-EQ", i, vector.first_value, &vector, request++))
			return false;
		uint16_t sum = model_saturating_add(vector.first_value, vector.second_value);

		if (dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET] != model_squared_component(sum))
			return false;
	}
	for (size_t i = 1; i < ARRAY_SIZE(amplitude_values); i++) {
		vector.second_value = (uint16_t) -amplitude_values[i];
		if (!run_compact_probe("EQA-OP", i, vector.first_value, &vector, request++))
			return false;
		uint16_t sum = model_saturating_add(vector.first_value, vector.second_value);

		if (dsp_hw_shared_memory[W1_EML_OUTPUT_OFFSET] != model_squared_component(sum))
			return false;
	}
	if (!run_transition_topology_probes(&request))
		return false;
	if (!run_last_tap_state_probes(&request))
		return false;
	if (!run_metric_sum_probes(&request))
		return false;
	if (!run_soft_scale_probes(&request))
		return false;
	request = 1000;
	if (!run_multistep_characterization(&request))
		return false;
	if (!run_multistep_branch_probes(&request))
		return false;
	if (!run_output_mode_characterization(&request))
		return false;
	test_category("Equalizer / successive segments without reset");
	return run_continuation_test(&request);
}

int main(void) {
	test_start("DSP equalizer vector characterization");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Equalizer / differential hardware vectors");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load equalizer vector runner",
		dsp_hw_load_image(DSP_EQUALIZER_VECTOR_RUNNER, sizeof(DSP_EQUALIZER_VECTOR_RUNNER))))
		return test_finish();
	if (!test_check("BRANCH starts equalizer vector runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("equalizer vector runner becomes ready", dsp_hw_wait_shared(0, READY_MARKER, 100)))
		return test_finish();

	for (size_t i = 0; i < ARRAY_SIZE(vectors); i++) {
		if (!run_vector(&vectors[i], i + 1, i != 0))
			break;
		if (i == 0)
			save_baseline();
	}
	test_category("Equalizer / complete one-hot transition matrices");
	test_check("all metric, path, and branch-partial-sum probes complete", run_one_hot_matrices());

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
