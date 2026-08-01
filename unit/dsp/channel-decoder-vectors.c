#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A
#define METRIC_COUNT_16 16
#define PHYSICAL_METRIC_COUNT 64
#define MAX_DECODE_TIMESTAMPS 32
#define MAX_TRACE_WORDS (MAX_DECODE_TIMESTAMPS * 4)
#define SIN0_MASK 0x01
#define SIN1_MASK 0x02
#define SIN2_MASK 0x04
#define ALL_SIN_MASK (SIN0_MASK | SIN1_MASK | SIN2_MASK)

static const uint16_t firmware_metrics_16[METRIC_COUNT_16] = {
	0x0065, 0x0055, 0x0045, 0x0033, 0x0023, 0x0013, 0x0003, 0x0067,
	0x0057, 0x0047, 0x0035, 0x0025, 0x0015, 0x0005, 0x0069, 0x0059,
};

static const uint16_t firmware_references_16[8] = {
	0x0037, 0x0037, 0x0037, 0x0037, 0x0037, 0x0037, 0x0033, 0x0033,
};

static const uint16_t polynomial_references_16[8] = {
	0x6CA0, 0x0AC6, 0, 0, 0, 0, 0, 0,
};

static const uint16_t repeated_references_64[8] = {
	0x6CA0, 0x0AC6, 0x6CA0, 0x0AC6, 0x6CA0, 0x0AC6, 0x6CA0, 0x0AC6,
};

static const uint16_t polynomial_references_64[8] = {
	0x6240, 0xAE8C, 0x8CAE, 0x4062, 0xC8EA, 0x0426, 0x2604, 0xEAC8,
};

static const uint8_t polynomial_generators_16[3] = { 0x17, 0x1D, 0x1B };
static const uint8_t polynomial_generators_64[3] = { 0x79, 0x5B, 0x75 };

static const uint16_t metric_amplitudes[] = { 1, 16, 256 };
static const uint16_t trace_extent_counts[] = { 1, 2, 4, 6, 8, 12, 16, 24, 32 };

struct soft_vector {
	const char *name;
	uint16_t sin0;
	uint16_t sin1;
	uint16_t sin2;
};

struct decoder_input {
	int8_t sin0;
	int8_t sin1;
	int8_t sin2;
};

struct decoder_result {
	uint16_t metrics[METRIC_COUNT_16];
	uint16_t trace[2];
};

struct input_mutation {
	size_t timestamp;
	uint8_t invert_mask;
	uint8_t erase_mask;
};

struct decoding_vector {
	const char *name;
	size_t metric_count;
	const uint8_t *generators;
	uint32_t payload;
	uint32_t expected_payload;
	size_t payload_bits;
	int8_t amplitude;
	const struct input_mutation *mutations;
	size_t mutation_count;
	size_t repeat_count;
	bool expect_correction;
};

static const struct input_mutation full_error_7[] = {
	{ 7, ALL_SIN_MASK, 0 },
};

static const struct input_mutation full_errors_4_8[] = {
	{ 4, ALL_SIN_MASK, 0 }, { 8, ALL_SIN_MASK, 0 },
};

static const struct input_mutation full_errors_4_8_12[] = {
	{ 4, ALL_SIN_MASK, 0 }, { 8, ALL_SIN_MASK, 0 }, { 12, ALL_SIN_MASK, 0 },
};

static const struct input_mutation full_errors_7_8_9[] = {
	{ 7, ALL_SIN_MASK, 0 }, { 8, ALL_SIN_MASK, 0 }, { 9, ALL_SIN_MASK, 0 },
};

static const struct input_mutation component_errors_5_11_17[] = {
	{ 5, SIN0_MASK, 0 }, { 11, SIN1_MASK, 0 }, { 17, SIN2_MASK, 0 },
};

static const struct input_mutation full_erasures_7_8_9[] = {
	{ 7, 0, ALL_SIN_MASK }, { 8, 0, ALL_SIN_MASK }, { 9, 0, ALL_SIN_MASK },
};

static const struct input_mutation full_error_9[] = {
	{ 9, ALL_SIN_MASK, 0 },
};

static const struct input_mutation full_errors_5_10[] = {
	{ 5, ALL_SIN_MASK, 0 }, { 10, ALL_SIN_MASK, 0 },
};

static const struct input_mutation full_errors_8_9_10[] = {
	{ 8, ALL_SIN_MASK, 0 }, { 9, ALL_SIN_MASK, 0 }, { 10, ALL_SIN_MASK, 0 },
};

static const struct input_mutation full_errors_4_8_12_16[] = {
	{ 4, ALL_SIN_MASK, 0 }, { 8, ALL_SIN_MASK, 0 },
	{ 12, ALL_SIN_MASK, 0 }, { 16, ALL_SIN_MASK, 0 },
};

static const struct input_mutation full_erasures_4_8_12[] = {
	{ 4, 0, ALL_SIN_MASK }, { 8, 0, ALL_SIN_MASK }, { 12, 0, ALL_SIN_MASK },
};

static const struct decoding_vector decoding_vectors[] = {
	{ "K5-noiseless", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D39A, 24, 48, NULL, 0, 0, true },
	{ "K5-full-symbol-1", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D39A, 24, 48, full_error_7, ARRAY_SIZE(full_error_7), 0, true },
	{ "K5-full-symbol-2", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D38A, 24, 48, full_errors_4_8, ARRAY_SIZE(full_errors_4_8), 0, false },
	{ "K5-full-symbol-3-spaced", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D24A, 24, 48, full_errors_4_8_12, ARRAY_SIZE(full_errors_4_8_12), 0, false },
	{ "K5-full-symbol-3-burst", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D31A, 24, 48, full_errors_7_8_9, ARRAY_SIZE(full_errors_7_8_9), 0, false },
	{ "K5-component-errors", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D39A, 24, 48, component_errors_5_11_17, ARRAY_SIZE(component_errors_5_11_17), 0, true },
	{ "K5-full-erasure-burst", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D39A, 24, 48, full_erasures_7_8_9, ARRAY_SIZE(full_erasures_7_8_9), 0, true },
	{ "K5-soft-amplitude-1", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D39A, 24, 1, NULL, 0, 0, true },
	{ "K5-soft-amplitude-127", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D39A, 24, 127, NULL, 0, 0, true },
	{ "K5-repeat-without-DSP-reset", METRIC_COUNT_16, polynomial_generators_16,
		0xB4D39A, 0xB4D39A, 24, 48, NULL, 0, 1, true },
	{ "K7-noiseless", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6D35, 20, 48, NULL, 0, 0, true },
	{ "K7-full-symbol-1", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6D35, 20, 48, full_error_9, ARRAY_SIZE(full_error_9), 0, true },
	{ "K7-full-symbol-2", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6D35, 20, 48, full_errors_5_10, ARRAY_SIZE(full_errors_5_10), 0, true },
	{ "K7-full-symbol-3-spaced", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6D35, 20, 48, full_errors_4_8_12, ARRAY_SIZE(full_errors_4_8_12), 0, true },
	{ "K7-full-symbol-3-burst", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6C35, 20, 48, full_errors_8_9_10, ARRAY_SIZE(full_errors_8_9_10), 0, false },
	{ "K7-full-symbol-4-spaced", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6905, 20, 48, full_errors_4_8_12_16, ARRAY_SIZE(full_errors_4_8_12_16), 0, false },
	{ "K7-component-errors", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6D35, 20, 48, component_errors_5_11_17, ARRAY_SIZE(component_errors_5_11_17), 0, true },
	{ "K7-full-erasures", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6D35, 20, 48, full_erasures_4_8_12, ARRAY_SIZE(full_erasures_4_8_12), 0, true },
	{ "K7-soft-amplitude-1", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6D35, 20, 1, NULL, 0, 0, true },
	{ "K7-soft-amplitude-127", PHYSICAL_METRIC_COUNT, polynomial_generators_64,
		0xA6D35, 0xA6D35, 20, 127, NULL, 0, 0, true },
};

#define INPUT(sin0, sin1, sin2) { sin0, sin1, sin2 }

static const struct decoder_input polynomial_constant_inputs[16] = {
	INPUT(7, 29, 91), INPUT(7, 29, 91), INPUT(7, 29, 91), INPUT(7, 29, 91),
	INPUT(7, 29, 91), INPUT(7, 29, 91), INPUT(7, 29, 91), INPUT(7, 29, 91),
	INPUT(7, 29, 91), INPUT(7, 29, 91), INPUT(7, 29, 91), INPUT(7, 29, 91),
	INPUT(7, 29, 91), INPUT(7, 29, 91), INPUT(7, 29, 91), INPUT(7, 29, 91),
};

static const struct decoder_input polynomial_basis_inputs[16] = {
	INPUT(64, 0, 0), INPUT(0, 64, 0), INPUT(0, 0, 64), INPUT(-64, 0, 0),
	INPUT(0, -64, 0), INPUT(0, 0, -64), INPUT(17, 31, 47), INPUT(-17, -31, -47),
	INPUT(64, 64, 0), INPUT(64, 0, 64), INPUT(0, 64, 64), INPUT(-64, -64, 0),
	INPUT(-64, 0, -64), INPUT(0, -64, -64), INPUT(32, -48, 80), INPUT(-32, 48, -80),
};

static const struct decoder_input polynomial_boundary_inputs[16] = {
	INPUT(127, 127, 127), INPUT(-128, -128, -128), INPUT(127, -128, 0), INPUT(-128, 127, 0),
	INPUT(127, 0, -128), INPUT(-128, 0, 127), INPUT(0, 127, -128), INPUT(0, -128, 127),
	INPUT(127, 1, -1), INPUT(-128, -1, 1), INPUT(1, 127, -1), INPUT(-1, -128, 1),
	INPUT(1, -1, 127), INPUT(-1, 1, -128), INPUT(127, -128, 127), INPUT(-128, 127, -128),
};

static const struct decoder_input polynomial_scrambled_inputs[16] = {
	INPUT(13, -57, 92), INPUT(-101, 44, 7), INPUT(63, 18, -77), INPUT(-9, 111, -36),
	INPUT(87, -24, -115), INPUT(-68, -3, 54), INPUT(29, 73, -42), INPUT(-127, 6, 119),
	INPUT(5, -88, 33), INPUT(104, 51, -12), INPUT(-46, 97, 26), INPUT(71, -109, -1),
	INPUT(-84, 39, 125), INPUT(22, -66, -93), INPUT(118, -14, 61), INPUT(-35, -122, 48),
};

static const uint16_t polynomial_constant_trace[32] = {
	0x005A, 0x00A5, 0x005A, 0x00A5, 0x005A, 0x00A5, 0x005A, 0x00A5,
	0x005A, 0x00A5, 0x005A, 0x00A5, 0x005A, 0x00A5, 0x005A, 0x00A5,
	0x005A, 0x00A5, 0x005A, 0x00A5, 0x005A, 0x00A5, 0x005A, 0x00A5,
	0x005A, 0x00A5, 0x005A, 0x00A5, 0x005A, 0x00A5, 0x005A, 0x00A5,
};

static const uint16_t polynomial_basis_trace[32] = {
	0x0066, 0x0099, 0x003C, 0x00C3, 0x005A, 0x00A5, 0x0099, 0x0066,
	0x00C3, 0x003C, 0x00A5, 0x005A, 0x007E, 0x0081, 0x00A1, 0x007A,
	0x002E, 0x008B, 0x004E, 0x008D, 0x003A, 0x00A3, 0x008B, 0x002E,
	0x008D, 0x004E, 0x00C5, 0x005C, 0x00CA, 0x00AC, 0x00AD, 0x004A,
};

static const uint16_t polynomial_boundary_trace[32] = {
	0x007E, 0x0081, 0x0081, 0x007E, 0x00C2, 0x00BC, 0x00B8, 0x00A2,
	0x0027, 0x001B, 0x00D9, 0x0064, 0x0035, 0x0053, 0x0043, 0x0035,
	0x0065, 0x0059, 0x0098, 0x00E6, 0x00EC, 0x00C8, 0x00C1, 0x002C,
	0x005A, 0x00A5, 0x00B5, 0x0052, 0x00C2, 0x00BC, 0x00B5, 0x0042,
};

static const uint16_t polynomial_scrambled_trace[32] = {
	0x005A, 0x00A5, 0x0019, 0x0067, 0x0027, 0x001B, 0x003D, 0x0043,
	0x00E5, 0x0058, 0x001B, 0x0027, 0x002C, 0x00CB, 0x0059, 0x0065,
	0x00C3, 0x003C, 0x0075, 0x0051, 0x00A8, 0x00EA, 0x00EA, 0x00A8,
	0x001A, 0x00A7, 0x0065, 0x0059, 0x006A, 0x00A9, 0x00D3, 0x0034,
};

static const uint16_t polynomial_64_probe_trace[4] = {
	0xEDFE, 0xFEF7, 0xEDFE, 0xFEF7,
};

static const uint16_t polynomial_64_scrambled_trace[32] = {
	0x5A5A, 0x5A5A, 0xA5A5, 0xA5A5, 0x1919, 0x1919, 0x6767, 0x6767,
	0x2727, 0x2727, 0x1B1B, 0x1B1B, 0x343C, 0x3C3D, 0x43C3, 0xC3D3,
	0xE565, 0x6565, 0x5959, 0x5958, 0x98F2, 0x999A, 0xA666, 0xB0E6,
	0x3534, 0x3534, 0xD353, 0xD353, 0x5919, 0x5F19, 0x6705, 0x6765,
};

static int16_t channel_decoder_branch_metric(uint16_t reference, const struct decoder_input *input) {
	int16_t metric = reference & 8 ? -input->sin0 : input->sin0;

	metric += reference & 4 ? -input->sin1 : input->sin1;
	metric += reference & 2 ? -input->sin2 : input->sin2;
	return metric;
}

static uint8_t channel_decoder_parity(uint16_t value) {
	uint8_t parity = 0;

	while (value != 0) {
		parity ^= value & 1;
		value >>= 1;
	}
	return parity;
}

static void channel_decoder_generate_references(size_t metric_count, const uint8_t generators[3],
	uint16_t references[8])
{
	for (size_t i = 0; i < 8; i++)
		references[i] = 0;
	for (size_t butterfly = 0; butterfly < metric_count / 2; butterfly++) {
		uint16_t encoder_state = metric_count | butterfly * 2;
		uint16_t reference = 0;

		for (size_t component = 0; component < 3; component++) {
			uint16_t encoded_bit = channel_decoder_parity(encoder_state & generators[component]);

			reference |= (encoded_bit ^ 1) << (3 - component);
		}
		references[butterfly / 4] |= reference << (butterfly % 4 * 4);
	}
}

static void channel_decoder_encode(const uint8_t *bits, size_t timestamp_count, size_t metric_count,
	const uint8_t generators[3], int8_t amplitude, struct decoder_input *inputs)
{
	uint16_t state = 0;

	for (size_t timestamp = 0; timestamp < timestamp_count; timestamp++) {
		uint16_t encoder_state = state | (bits[timestamp] ? metric_count : 0);

		inputs[timestamp].sin0 = channel_decoder_parity(encoder_state & generators[0]) ? amplitude : -amplitude;
		inputs[timestamp].sin1 = channel_decoder_parity(encoder_state & generators[1]) ? amplitude : -amplitude;
		inputs[timestamp].sin2 = channel_decoder_parity(encoder_state & generators[2]) ? amplitude : -amplitude;
		state = state / 2 + (bits[timestamp] ? metric_count / 2 : 0);
	}
}

static uint64_t channel_decoder_unpack_trace(const uint16_t *trace, size_t metric_count) {
	uint64_t decisions = 0;

	if (metric_count == METRIC_COUNT_16) {
		uint16_t packed = trace[0] | trace[1] << 8;

		return (uint16_t) ~(packed << 8 | packed >> 8);
	}
	for (size_t group = 0; group < 4; group++) {
		uint16_t decision_group = (uint16_t) ~trace[group];

		decisions |= (uint64_t) decision_group << ((3 - group) * 16);
	}
	return decisions;
}

static uint16_t channel_decoder_traceback(const uint64_t *decisions, size_t timestamp_count, size_t metric_count,
	uint8_t *decoded)
{
	uint16_t state = 0;

	for (size_t timestamp = timestamp_count; timestamp-- > 0;) {
		bool first_predecessor = decisions[timestamp] >> state & 1;

		decoded[timestamp] = state >= metric_count / 2;
		state = state % (metric_count / 2) * 2 + (first_predecessor ? 0 : 1);
	}
	return state;
}

static bool channel_decoder_bits_equal(const uint8_t *first, const uint8_t *second, size_t count) {
	for (size_t i = 0; i < count; i++) {
		if (first[i] != second[i])
			return false;
	}
	return true;
}

static size_t channel_decoder_best_state(const int16_t *metrics, size_t metric_count) {
	size_t best_state = 0;

	for (size_t state = 1; state < metric_count; state++) {
		if (metrics[state] > metrics[best_state])
			best_state = state;
	}
	return best_state;
}

static uint64_t channel_decoder_model_decisions(int16_t *metrics, size_t metric_count, const uint16_t references[8],
	const struct decoder_input *input)
{
	int16_t next_metrics[PHYSICAL_METRIC_COUNT];
	uint64_t decisions = 0;

	for (size_t butterfly = 0; butterfly < metric_count / 2; butterfly++) {
		uint16_t reference_register = references[butterfly / 4];
		uint16_t reference = reference_register >> (butterfly % 4 * 4) & 0xF;
		int16_t branch_metric = channel_decoder_branch_metric(reference, input);
		int16_t even_metric = metrics[butterfly * 2];
		int16_t odd_metric = metrics[butterfly * 2 + 1];
		int16_t lower_first = even_metric - branch_metric;
		int16_t lower_second = odd_metric + branch_metric;
		int16_t upper_first = even_metric + branch_metric;
		int16_t upper_second = odd_metric - branch_metric;

		next_metrics[butterfly] = lower_first >= lower_second ? lower_first : lower_second;
		next_metrics[butterfly + metric_count / 2] =
			upper_first >= upper_second ? upper_first : upper_second;
		if (lower_first >= lower_second)
			decisions |= UINT64_C(1) << butterfly;
		if (upper_first >= upper_second)
			decisions |= UINT64_C(1) << (butterfly + metric_count / 2);
	}
	for (size_t state = 0; state < metric_count; state++)
		metrics[state] = next_metrics[state];
	return decisions;
}

static uint16_t channel_decoder_model_step(int16_t metrics[METRIC_COUNT_16], const uint16_t references[8],
	const struct decoder_input *input)
{
	uint16_t decisions = channel_decoder_model_decisions(metrics, METRIC_COUNT_16, references, input);

	return ~(decisions << 8 | decisions >> 8);
}

static void channel_decoder_model_series(const struct decoder_input inputs[16], uint16_t trace[32]) {
	int16_t metrics[METRIC_COUNT_16] = { 0 };

	for (size_t timestamp = 0; timestamp < 16; timestamp++) {
		uint16_t decisions = channel_decoder_model_step(metrics, polynomial_references_16, &inputs[timestamp]);

		trace[timestamp * 2] = decisions & 0xFF;
		trace[timestamp * 2 + 1] = decisions >> 8;
	}
}

static void channel_decoder_model_trace_64(uint64_t decisions, uint16_t trace[4]) {
	for (size_t group = 0; group < 4; group++) {
		uint16_t decision_group = decisions >> ((3 - group) * 16);

		trace[group] = ~decision_group;
	}
}

static size_t channel_decoder_model_trace(uint64_t decisions, size_t metric_count, uint16_t *trace) {
	if (metric_count == METRIC_COUNT_16) {
		uint16_t decision_word = (uint16_t) decisions;
		uint16_t packed = (uint16_t) ~(decision_word << 8 | decision_word >> 8);

		trace[0] = packed & 0xFF;
		trace[1] = packed >> 8;
		return 2;
	}
	channel_decoder_model_trace_64(decisions, trace);
	return 4;
}

#undef INPUT

static const struct soft_vector soft_basis_vectors[] = {
	{ "zero", 0x00, 0x00, 0x00 },
	{ "sin0-1", 0x01, 0x00, 0x00 },
	{ "sin1-1", 0x00, 0x01, 0x00 },
	{ "sin2-1", 0x00, 0x00, 0x01 },
	{ "sin0-16", 0x10, 0x00, 0x00 },
	{ "sin1-16", 0x00, 0x10, 0x00 },
	{ "sin2-16", 0x00, 0x00, 0x10 },
	{ "all-16", 0x10, 0x10, 0x10 },
	{ "sin0-127", 0x7F, 0x00, 0x00 },
	{ "sin0-128", 0x80, 0x00, 0x00 },
	{ "sin0-255", 0xFF, 0x00, 0x00 },
};

#ifdef PMB8875
#include "channel-decoder-vector-runner-8875.inc"
#define DSP_CHANNEL_DECODER_VECTOR_RUNNER DSP_CHANNEL_DECODER_VECTOR_RUNNER_8875
#else
#include "channel-decoder-vector-runner-8876.inc"
#define DSP_CHANNEL_DECODER_VECTOR_RUNNER DSP_CHANNEL_DECODER_VECTOR_RUNNER_8876
#endif

static void prepare_common_vector(uint16_t configured_timestamp_count, uint16_t input_timestamp_count,
	uint16_t metric_count)
{
	dsp_hw_shared_memory[0x0800] = 0;
	dsp_hw_shared_memory[0x0801] = 0;
	dsp_hw_shared_memory[0x0802] = configured_timestamp_count;
	dsp_hw_shared_memory[0x0803] = 0;
	dsp_hw_shared_memory[0x0804] = metric_count;
	dsp_hw_shared_memory[0x0805] = 1;
	dsp_hw_shared_memory[0x0806] = input_timestamp_count;
	dsp_hw_shared_memory[0x0807] = 0xDEAD;
	dsp_hw_shared_memory[0x0808] = 0;
	dsp_hw_shared_memory[0x0809] = 0;
	dsp_hw_shared_memory[0x080A] = 0xDEAD;
	dsp_hw_shared_memory[0x080B] = 0;
	for (size_t i = 0; i < 8; i++)
		dsp_hw_shared_memory[0x0810 + i] = 0;
	for (size_t i = 0; i < metric_count; i++) {
		dsp_hw_shared_memory[0x0820 + i] = 0;
		dsp_hw_shared_memory[0x0860 + i] = 0;
		dsp_hw_shared_memory[0x0940 + i] = 0xDEAD;
		dsp_hw_shared_memory[0x0980 + i] = 0xDEAD;
	}
	for (size_t step = 0; step < input_timestamp_count; step++) {
		dsp_hw_shared_memory[0x08A0 + step * 2] = 7;
		dsp_hw_shared_memory[0x08A1 + step * 2] = 29;
		dsp_hw_shared_memory[0x08E0 + step * 2] = 91;
		dsp_hw_shared_memory[0x08E1 + step * 2] = 0;
	}
	dsp_hw_shared_memory[0x09C0] = 0xDEAD;
}

static bool run_vector(void) {
	bool complete;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load channel-decoder vector runner",
		dsp_hw_load_image(DSP_CHANNEL_DECODER_VECTOR_RUNNER, sizeof(DSP_CHANNEL_DECODER_VECTOR_RUNNER))))
		return false;
	if (!test_check("BRANCH starts channel-decoder vector runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("channel-decoder vector runner becomes ready", dsp_hw_wait_shared(0x0800, READY_MARKER, 100)))
		return false;
	complete = dsp_hw_wait_shared(0x0801, COMPLETE_MARKER, 1000);
	if (!complete) {
		printf("# runner-timeout repeat=%04X runs=%04X trace=%04X status=%04X\n",
			(uint32_t) dsp_hw_shared_memory[0x0808], (uint32_t) dsp_hw_shared_memory[0x0809],
			(uint32_t) dsp_hw_shared_memory[0x09C0], (uint32_t) dsp_hw_shared_memory[0x0A41]);
	}
	return test_check("channel-decoder vector completes", complete);
}

static void print_metrics(const char *name) {
	printf("# %s W1:", name);
	for (size_t i = 0; i < METRIC_COUNT_16; i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x0940 + i]);
	printf("\n# %s W2:", name);
	for (size_t i = 0; i < METRIC_COUNT_16; i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x0980 + i]);
	printf("\n# %s trace=%04X start=%04X busy=%04X count=%04X irq=%04X\n", name,
		(uint32_t) dsp_hw_shared_memory[0x09C0], (uint32_t) dsp_hw_shared_memory[0x0A40],
		(uint32_t) dsp_hw_shared_memory[0x0A41], (uint32_t) dsp_hw_shared_memory[0x0A42],
		(uint32_t) dsp_hw_shared_memory[0x0A43]);
}

static bool run_predecessor_vector(void) {
	prepare_common_vector(1, 1, METRIC_COUNT_16);
	for (size_t i = 0; i < METRIC_COUNT_16; i++) {
		dsp_hw_shared_memory[0x0820 + i] = i * 0x0100;
		dsp_hw_shared_memory[0x0860 + i] = 0x7000 + i;
	}
	if (!run_vector())
		return false;
	for (size_t i = 0; i < METRIC_COUNT_16; i++) {
		test_eq_u32("RAMW1 metric survives pre-decode transport", i * 0x0100,
			dsp_hw_shared_memory[0x0B00 + i]);
		test_eq_u32("RAMW2 metric survives pre-decode transport", 0x7000 + i,
			dsp_hw_shared_memory[0x0B40 + i]);
	}
	print_metrics("predecessor-map");
	return true;
}

static bool run_trellis_probe(void) {
	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	for (size_t i = 0; i < METRIC_COUNT_16; i++)
		dsp_hw_shared_memory[0x0820 + i] = i * 0x0010;
	if (!run_vector())
		return false;
	print_metrics("trellis-probe");
	return true;
}

static bool run_tie_vector(void) {
	const struct decoder_input input = { 0, 0, 0 };
	int16_t metrics[METRIC_COUNT_16] = { 0 };

	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	dsp_hw_shared_memory[0x08A0] = 0;
	dsp_hw_shared_memory[0x08A1] = 0;
	dsp_hw_shared_memory[0x08E0] = 0;
	dsp_hw_shared_memory[0x08E1] = 0;
	if (!run_vector())
		return false;
	test_eq_u32("equal ACS candidates select the first lower predecessor", 0, dsp_hw_shared_memory[0x09C0]);
	test_eq_u32("equal ACS candidates select the first upper predecessor", 0, dsp_hw_shared_memory[0x09C1]);
	test_eq_u32("ACS model uses the hardware tie rule", 0,
		channel_decoder_model_step(metrics, polynomial_references_16, &input));
	return true;
}

static bool run_preferred_state_vector(uint16_t state) {
	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	dsp_hw_shared_memory[0x08A0] = 0;
	dsp_hw_shared_memory[0x08A1] = 0;
	dsp_hw_shared_memory[0x08E0] = 0;
	dsp_hw_shared_memory[0x08E1] = 0;
	dsp_hw_shared_memory[0x0820 + state] = 0x0100;
	if (!run_vector())
		return false;
	printf("# preferred-state-%u trace=%04X,%04X\n", (uint32_t) state,
		(uint32_t) dsp_hw_shared_memory[0x09C0], (uint32_t) dsp_hw_shared_memory[0x09C1]);
	return true;
}

static bool run_rejected_state_vector(uint16_t state) {
	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	dsp_hw_shared_memory[0x08A0] = 0;
	dsp_hw_shared_memory[0x08A1] = 0;
	dsp_hw_shared_memory[0x08E0] = 0;
	dsp_hw_shared_memory[0x08E1] = 0;
	for (size_t i = 0; i < METRIC_COUNT_16; i++)
		dsp_hw_shared_memory[0x0820 + i] = 0x0100;
	dsp_hw_shared_memory[0x0820 + state] = 0;
	if (!run_vector())
		return false;
	printf("# rejected-state-%u trace=%04X,%04X\n", (uint32_t) state,
		(uint32_t) dsp_hw_shared_memory[0x09C0], (uint32_t) dsp_hw_shared_memory[0x09C1]);
	return true;
}

static bool run_metric_impulse_vector(uint16_t state, uint16_t amplitude) {
	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	dsp_hw_shared_memory[0x08A0] = 0;
	dsp_hw_shared_memory[0x08A1] = 0;
	dsp_hw_shared_memory[0x08E0] = 0;
	dsp_hw_shared_memory[0x08E1] = 0;
	dsp_hw_shared_memory[0x0820 + state] = amplitude;
	if (!run_vector())
		return false;
	printf("# metric-impulse-%u-%u trace=%04X,%04X\n", (uint32_t) amplitude, (uint32_t) state,
		(uint32_t) dsp_hw_shared_memory[0x09C0], (uint32_t) dsp_hw_shared_memory[0x09C1]);
	return true;
}

static bool run_branch_index_vector(uint16_t index) {
	char name[32];
	uint16_t references = index * 0x1111;

	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	for (size_t i = 0; i < 8; i++)
		dsp_hw_shared_memory[0x0810 + i] = references;
	if (!run_vector())
		return false;
	tfp_sprintf(name, "branch-index-%u", (uint32_t) index);
	print_metrics(name);
	return true;
}

static bool capture_reference_index_result(uint16_t index, struct decoder_result *result) {
	uint16_t references = index * 0x1111;

	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = ARRAY_SIZE(result->trace);
	for (size_t state = 0; state < METRIC_COUNT_16; state++)
		dsp_hw_shared_memory[0x0820 + state] = state * 0x0100 + state * state;
	for (size_t register_index = 0; register_index < 8; register_index++)
		dsp_hw_shared_memory[0x0810 + register_index] = references;
	if (!run_vector())
		return false;
	for (size_t state = 0; state < METRIC_COUNT_16; state++)
		result->metrics[state] = dsp_hw_shared_memory[0x0980 + state];
	for (size_t i = 0; i < ARRAY_SIZE(result->trace); i++)
		result->trace[i] = dsp_hw_shared_memory[0x09C0 + i];
	return true;
}

static bool run_reference_bit_zero_vector(uint16_t even_index) {
	struct decoder_result even_result;
	struct decoder_result odd_result;
	char assertion[64];

	if (!capture_reference_index_result(even_index, &even_result))
		return false;
	if (!capture_reference_index_result(even_index + 1, &odd_result))
		return false;
	tfp_sprintf(assertion, "reference indices %u/%u produce identical metrics", (uint32_t) even_index,
		(uint32_t) even_index + 1);
	test_eq_memory(assertion, even_result.metrics, odd_result.metrics, sizeof(even_result.metrics));
	tfp_sprintf(assertion, "reference indices %u/%u produce identical traceback", (uint32_t) even_index,
		(uint32_t) even_index + 1);
	test_eq_memory(assertion, even_result.trace, odd_result.trace, sizeof(even_result.trace));
	return true;
}

static bool run_metric_wrap_vector(void) {
	static const uint16_t initial_metrics[METRIC_COUNT_16] = {
		0x7E80, 0x7FF0, 0x8000, 0x8110, 0xFF00, 0x0000, 0x0100, 0x7F80,
		0x8080, 0xF000, 0x1000, 0x7000, 0x8FFF, 0x0001, 0xFFFF, 0x7FFF,
	};
	const struct decoder_input input = { 127, 127, 127 };
	const uint16_t references[8] = { 0 };
	int16_t model_metrics[METRIC_COUNT_16];
	uint16_t model_trace;
	uint16_t hardware_trace[2];

	for (size_t state = 0; state < METRIC_COUNT_16; state++)
		model_metrics[state] = (int16_t) initial_metrics[state];
	model_trace = channel_decoder_model_step(model_metrics, references, &input);
	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = ARRAY_SIZE(hardware_trace);
	for (size_t state = 0; state < METRIC_COUNT_16; state++)
		dsp_hw_shared_memory[0x0820 + state] = initial_metrics[state];
	dsp_hw_shared_memory[0x08A0] = input.sin0;
	dsp_hw_shared_memory[0x08A1] = input.sin1;
	dsp_hw_shared_memory[0x08E0] = input.sin2;
	if (!run_vector())
		return false;
	hardware_trace[0] = dsp_hw_shared_memory[0x09C0];
	hardware_trace[1] = dsp_hw_shared_memory[0x09C1];
	test_eq_memory("signed 16-bit metric wrap matches the ACS model", model_metrics,
		dsp_hw_shared_memory + 0x0980, sizeof(model_metrics));
	test_eq_u32("metric wrap traceback low byte matches the ACS model", model_trace & 0xFF, hardware_trace[0]);
	test_eq_u32("metric wrap traceback high byte matches the ACS model", model_trace >> 8, hardware_trace[1]);
	return true;
}

static bool run_64_state_probe(void) {
	const struct decoder_input input = { 13, -57, 92 };
	int16_t model_metrics[PHYSICAL_METRIC_COUNT];
	uint16_t model_trace[4];
	uint64_t model_decisions;

	prepare_common_vector(1, 1, PHYSICAL_METRIC_COUNT);
	dsp_hw_shared_memory[0x0803] = 0x8000;
	dsp_hw_shared_memory[0x0805] = ARRAY_SIZE(model_trace);
	for (size_t register_index = 0; register_index < 8; register_index++)
		dsp_hw_shared_memory[0x0810 + register_index] = repeated_references_64[register_index];
	for (size_t state = 0; state < PHYSICAL_METRIC_COUNT; state++) {
		uint16_t initial_metric = state * 0x1234 + state * state * 73;

		model_metrics[state] = (int16_t) initial_metric;
		dsp_hw_shared_memory[0x0820 + state] = initial_metric;
	}
	dsp_hw_shared_memory[0x08A0] = (uint8_t) input.sin0;
	dsp_hw_shared_memory[0x08A1] = (uint8_t) input.sin1;
	dsp_hw_shared_memory[0x08E0] = (uint8_t) input.sin2;
	model_decisions = channel_decoder_model_decisions(model_metrics, PHYSICAL_METRIC_COUNT,
		repeated_references_64, &input);
	channel_decoder_model_trace_64(model_decisions, model_trace);
	if (!run_vector())
		return false;
	test_eq_memory("64-state metrics match the generalized ACS model", model_metrics,
		dsp_hw_shared_memory + 0x0980, sizeof(model_metrics));
	test_eq_memory("64-state traceback matches the hardware golden", polynomial_64_probe_trace,
		dsp_hw_shared_memory + 0x09C0, sizeof(polynomial_64_probe_trace));
	test_eq_memory("64-state traceback matches the generalized ACS model", model_trace,
		dsp_hw_shared_memory + 0x09C0, sizeof(model_trace));
	printf("# 64-state traceback:");
	for (size_t i = 0; i < ARRAY_SIZE(model_trace); i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x09C0 + i]);
	printf("\n");
	return true;
}

static bool run_64_state_series(void) {
	int16_t model_metrics[PHYSICAL_METRIC_COUNT] = { 0 };
	uint16_t model_trace[32];

	prepare_common_vector(8, 8, PHYSICAL_METRIC_COUNT);
	dsp_hw_shared_memory[0x0803] = 0x8000;
	dsp_hw_shared_memory[0x0805] = ARRAY_SIZE(model_trace);
	for (size_t register_index = 0; register_index < 8; register_index++)
		dsp_hw_shared_memory[0x0810 + register_index] = repeated_references_64[register_index];
	for (size_t timestamp = 0; timestamp < 8; timestamp++) {
		const struct decoder_input *input = &polynomial_scrambled_inputs[timestamp];
		uint64_t decisions;

		dsp_hw_shared_memory[0x08A0 + timestamp * 2] = (uint8_t) input->sin0;
		dsp_hw_shared_memory[0x08A1 + timestamp * 2] = (uint8_t) input->sin1;
		dsp_hw_shared_memory[0x08E0 + timestamp * 2] = (uint8_t) input->sin2;
		decisions = channel_decoder_model_decisions(model_metrics, PHYSICAL_METRIC_COUNT,
			repeated_references_64, input);
		channel_decoder_model_trace_64(decisions, model_trace + timestamp * 4);
	}
	if (!run_vector())
		return false;
	test_eq_memory("64-state series metrics match the generalized ACS model", model_metrics,
		dsp_hw_shared_memory + 0x0940, sizeof(model_metrics));
	test_eq_memory("64-state series traceback matches the hardware golden", polynomial_64_scrambled_trace,
		dsp_hw_shared_memory + 0x09C0, sizeof(polynomial_64_scrambled_trace));
	test_eq_memory("64-state series traceback matches the generalized ACS model", model_trace,
		dsp_hw_shared_memory + 0x09C0, sizeof(model_trace));
	printf("# 64-state traceback series:");
	for (size_t i = 0; i < ARRAY_SIZE(model_trace); i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x09C0 + i]);
	printf("\n");
	return true;
}

static bool run_64_reference_field_vector(size_t field) {
	const struct decoder_input input = { 13, 29, 71 };
	uint16_t references[8] = { 0 };
	int16_t model_metrics[PHYSICAL_METRIC_COUNT] = { 0 };
	uint16_t model_trace[4];
	uint64_t decisions;
	char assertion[80];

	references[field / 4] = 2 << (field % 4 * 4);
	decisions = channel_decoder_model_decisions(model_metrics, PHYSICAL_METRIC_COUNT, references, &input);
	channel_decoder_model_trace_64(decisions, model_trace);
	prepare_common_vector(1, 1, PHYSICAL_METRIC_COUNT);
	dsp_hw_shared_memory[0x0803] = 0x8000;
	dsp_hw_shared_memory[0x0805] = ARRAY_SIZE(model_trace);
	for (size_t register_index = 0; register_index < 8; register_index++)
		dsp_hw_shared_memory[0x0810 + register_index] = references[register_index];
	dsp_hw_shared_memory[0x08A0] = (uint8_t) input.sin0;
	dsp_hw_shared_memory[0x08A1] = (uint8_t) input.sin1;
	dsp_hw_shared_memory[0x08E0] = (uint8_t) input.sin2;
	if (!run_vector())
		return false;
	tfp_sprintf(assertion, "64-state reference field %u routes to its butterfly metrics", (uint32_t) field);
	test_eq_memory(assertion, model_metrics, dsp_hw_shared_memory + 0x0980, sizeof(model_metrics));
	tfp_sprintf(assertion, "64-state reference field %u routes to its decisions", (uint32_t) field);
	test_eq_memory(assertion, model_trace, dsp_hw_shared_memory + 0x09C0, sizeof(model_trace));
	return true;
}

static bool run_64_reference_bit_zero_vector(void) {
	const struct decoder_input input = { 13, 29, 71 };
	uint16_t references[8];
	int16_t model_metrics[PHYSICAL_METRIC_COUNT] = { 0 };
	uint16_t model_trace[4];
	uint64_t decisions;

	for (size_t register_index = 0; register_index < 8; register_index++)
		references[register_index] = 0x1111;
	decisions = channel_decoder_model_decisions(model_metrics, PHYSICAL_METRIC_COUNT, references, &input);
	channel_decoder_model_trace_64(decisions, model_trace);
	prepare_common_vector(1, 1, PHYSICAL_METRIC_COUNT);
	dsp_hw_shared_memory[0x0803] = 0x8000;
	dsp_hw_shared_memory[0x0805] = ARRAY_SIZE(model_trace);
	for (size_t register_index = 0; register_index < 8; register_index++)
		dsp_hw_shared_memory[0x0810 + register_index] = references[register_index];
	dsp_hw_shared_memory[0x08A0] = (uint8_t) input.sin0;
	dsp_hw_shared_memory[0x08A1] = (uint8_t) input.sin1;
	dsp_hw_shared_memory[0x08E0] = (uint8_t) input.sin2;
	if (!run_vector())
		return false;
	test_eq_memory("reference bit zero is ignored by all 64-state butterflies", model_metrics,
		dsp_hw_shared_memory + 0x0980, sizeof(model_metrics));
	test_eq_memory("reference bit zero does not alter 64-state decisions", model_trace,
		dsp_hw_shared_memory + 0x09C0, sizeof(model_trace));
	return true;
}

static bool run_decoding_vector(const struct decoding_vector *vector) {
	struct decoder_input inputs[MAX_DECODE_TIMESTAMPS];
	int16_t model_metrics[PHYSICAL_METRIC_COUNT];
	uint64_t model_decisions[MAX_DECODE_TIMESTAMPS];
	uint64_t hardware_decisions[MAX_DECODE_TIMESTAMPS];
	uint16_t model_trace[MAX_TRACE_WORDS];
	uint16_t hardware_trace[MAX_TRACE_WORDS];
	uint16_t references[8];
	uint8_t transmitted[MAX_DECODE_TIMESTAMPS] = { 0 };
	uint8_t expected[MAX_DECODE_TIMESTAMPS] = { 0 };
	uint8_t model_decoded[MAX_DECODE_TIMESTAMPS];
	uint8_t hardware_decoded[MAX_DECODE_TIMESTAMPS];
	size_t tail_bits = vector->metric_count == METRIC_COUNT_16 ? 4 : 6;
	size_t timestamp_count = vector->payload_bits + tail_bits;
	size_t trace_words_per_timestamp = vector->metric_count == METRIC_COUNT_16 ? 2 : 4;
	size_t trace_word_count = timestamp_count * trace_words_per_timestamp;
	volatile uint16_t *active_metrics;
	uint16_t model_start_state;
	uint16_t hardware_start_state;
	uint32_t decoded_payload = 0;
	char assertion[96];

	for (size_t bit = 0; bit < vector->payload_bits; bit++) {
		transmitted[bit] = vector->payload >> bit & 1;
		expected[bit] = vector->expected_payload >> bit & 1;
	}
	channel_decoder_generate_references(vector->metric_count, vector->generators, references);
	channel_decoder_encode(transmitted, timestamp_count, vector->metric_count, vector->generators,
		vector->amplitude, inputs);
	for (size_t i = 0; i < vector->mutation_count; i++) {
		const struct input_mutation *mutation = &vector->mutations[i];
		struct decoder_input *input = &inputs[mutation->timestamp];

		if ((mutation->invert_mask & SIN0_MASK))
			input->sin0 = -input->sin0;
		if ((mutation->invert_mask & SIN1_MASK))
			input->sin1 = -input->sin1;
		if ((mutation->invert_mask & SIN2_MASK))
			input->sin2 = -input->sin2;
		if ((mutation->erase_mask & SIN0_MASK))
			input->sin0 = 0;
		if ((mutation->erase_mask & SIN1_MASK))
			input->sin1 = 0;
		if ((mutation->erase_mask & SIN2_MASK))
			input->sin2 = 0;
	}
	for (size_t state = 0; state < vector->metric_count; state++)
		model_metrics[state] = state == 0 ? 0 : -12000;
	for (size_t timestamp = 0; timestamp < timestamp_count; timestamp++) {
		model_decisions[timestamp] = channel_decoder_model_decisions(model_metrics, vector->metric_count,
			references, &inputs[timestamp]);
		channel_decoder_model_trace(model_decisions[timestamp], vector->metric_count,
			model_trace + timestamp * trace_words_per_timestamp);
	}

	prepare_common_vector(timestamp_count, timestamp_count, vector->metric_count);
	dsp_hw_shared_memory[0x0803] = vector->metric_count == PHYSICAL_METRIC_COUNT ? 0x8000 : 0;
	dsp_hw_shared_memory[0x0805] = trace_word_count;
	dsp_hw_shared_memory[0x0808] = vector->repeat_count;
	for (size_t register_index = 0; register_index < 8; register_index++)
		dsp_hw_shared_memory[0x0810 + register_index] = references[register_index];
	for (size_t state = 0; state < vector->metric_count; state++)
		dsp_hw_shared_memory[0x0820 + state] = state == 0 ? 0 : (uint16_t) -12000;
	for (size_t timestamp = 0; timestamp < timestamp_count; timestamp++) {
		dsp_hw_shared_memory[0x08A0 + timestamp * 2] = (uint8_t) inputs[timestamp].sin0;
		dsp_hw_shared_memory[0x08A1 + timestamp * 2] = (uint8_t) inputs[timestamp].sin1;
		dsp_hw_shared_memory[0x08E0 + timestamp * 2] = (uint8_t) inputs[timestamp].sin2;
	}
	if (!run_vector())
		return false;
	active_metrics = dsp_hw_shared_memory + (timestamp_count & 1 ? 0x0980 : 0x0940);
	for (size_t word = 0; word < trace_word_count; word++)
		hardware_trace[word] = dsp_hw_shared_memory[0x09C0 + word];
	for (size_t timestamp = 0; timestamp < timestamp_count; timestamp++) {
		hardware_decisions[timestamp] = channel_decoder_unpack_trace(
			hardware_trace + timestamp * trace_words_per_timestamp, vector->metric_count);
	}

	tfp_sprintf(assertion, "%s final metrics match the independent ACS model", vector->name);
	test_eq_memory(assertion, model_metrics, active_metrics, vector->metric_count * sizeof(model_metrics[0]));
	tfp_sprintf(assertion, "%s decision series matches the independent ACS model", vector->name);
	test_eq_memory(assertion, model_trace, hardware_trace, trace_word_count * sizeof(model_trace[0]));
	tfp_sprintf(assertion, "%s executes every requested decode without a DSP reset", vector->name);
	test_eq_u32(assertion, vector->repeat_count + 1, dsp_hw_shared_memory[0x0809]);
	tfp_sprintf(assertion, "%s acknowledges the previous completion before starting", vector->name);
	test_eq_u32(assertion, 0, dsp_hw_shared_memory[0x080A] & 0x0100);
	tfp_sprintf(assertion, "%s observes every distinct completion source", vector->name);
	test_eq_u32(assertion, vector->repeat_count + 1, dsp_hw_shared_memory[0x080B]);
	tfp_sprintf(assertion, "%s final completion source is pending", vector->name);
	test_eq_u32(assertion, 0x0100, dsp_hw_shared_memory[0x0A43] & 0x0100);
	model_start_state = channel_decoder_traceback(model_decisions, timestamp_count, vector->metric_count,
		model_decoded);
	hardware_start_state = channel_decoder_traceback(hardware_decisions, timestamp_count, vector->metric_count,
		hardware_decoded);
	tfp_sprintf(assertion, "%s model traceback reaches the configured start state", vector->name);
	test_eq_u32(assertion, 0, model_start_state);
	tfp_sprintf(assertion, "%s hardware traceback reaches the configured start state", vector->name);
	test_eq_u32(assertion, 0, hardware_start_state);
	tfp_sprintf(assertion, "%s terminated path has state zero as the best final metric", vector->name);
	test_eq_u32(assertion, 0, channel_decoder_best_state(model_metrics, vector->metric_count));
	tfp_sprintf(assertion, "%s model produces the fixed decoded-bit golden", vector->name);
	test_eq_memory(assertion, expected, model_decoded, timestamp_count);
	tfp_sprintf(assertion, "%s hardware produces the fixed decoded-bit golden", vector->name);
	test_eq_memory(assertion, expected, hardware_decoded, timestamp_count);
	tfp_sprintf(assertion, "%s correction classification matches the injected errors", vector->name);
	test_check(assertion, channel_decoder_bits_equal(transmitted, hardware_decoded, vector->payload_bits) ==
		vector->expect_correction);
	for (size_t bit = 0; bit < vector->payload_bits; bit++)
		decoded_payload |= (uint32_t) hardware_decoded[bit] << bit;
	printf("# CHDEC-DECODE,%s,TX=%08X,RX=%08X,MUTATIONS=%u,AMPLITUDE=%d\n", vector->name, vector->payload,
		decoded_payload, (uint32_t) vector->mutation_count, vector->amplitude);
	return true;
}

static void validate_generated_reference_tables(void) {
	uint16_t references[8];

	channel_decoder_generate_references(METRIC_COUNT_16, polynomial_generators_16, references);
	test_eq_memory("K=5 generators produce the Mask ROM reference table", polynomial_references_16,
		references, sizeof(references));
	channel_decoder_generate_references(PHYSICAL_METRIC_COUNT, polynomial_generators_64, references);
	test_eq_memory("K=7 generators produce the full 64-state reference table", polynomial_references_64,
		references, sizeof(references));
}

static bool run_overflow_schedule_vector(uint16_t count, bool protection) {
	uint16_t expected_metrics[METRIC_COUNT_16];
	volatile uint16_t *active_metrics;
	char name[80];

	prepare_common_vector(count, count, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0803] = protection ? 0x4000 : 0;
	for (size_t state = 0; state < METRIC_COUNT_16; state++)
		dsp_hw_shared_memory[0x0820 + state] = 0x5000;
	for (size_t timestamp = 0; timestamp < count; timestamp++) {
		dsp_hw_shared_memory[0x08A0 + timestamp * 2] = 0;
		dsp_hw_shared_memory[0x08A1 + timestamp * 2] = 0;
		dsp_hw_shared_memory[0x08E0 + timestamp * 2] = 0;
	}
	if (!run_vector())
		return false;
	for (size_t state = 0; state < METRIC_COUNT_16; state++)
		expected_metrics[state] = protection && count >= 3 ? 0x4000 : 0x5000;
	active_metrics = dsp_hw_shared_memory + (count & 1 ? 0x0980 : 0x0940);
	tfp_sprintf(name, "overflow %s count %u follows the documented schedule",
		protection ? "protection" : "disabled", (uint32_t) count);
	test_eq_memory(name, expected_metrics, active_metrics, sizeof(expected_metrics));
	return true;
}

static bool run_count_vector(uint16_t count) {
	char name[64];

	prepare_common_vector(count, count, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 16;
	if (!run_vector())
		return false;
	tfp_sprintf(name, "timestamp-count-%u reaches the requested final count", (uint32_t) count);
	test_eq_u32(name, count, dsp_hw_shared_memory[0x0A42]);
	tfp_sprintf(name, "timestamp-count-%u", (uint32_t) count);
	print_metrics(name);
	printf("# %s trace-series:", name);
	for (size_t i = 0; i < 16; i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x09C0 + i]);
	printf("\n");
	return true;
}

static bool run_trace_extent_vector(uint16_t count) {
	uint16_t first_capture[16];
	size_t expected_changed = count * 2 < ARRAY_SIZE(first_capture) ? count * 2 : ARRAY_SIZE(first_capture);
	size_t changed = 0;

	prepare_common_vector(count, count, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = ARRAY_SIZE(first_capture);
	dsp_hw_shared_memory[0x0807] = 0xBEEF;
	if (!run_vector())
		return false;
	for (size_t i = 0; i < ARRAY_SIZE(first_capture); i++)
		first_capture[i] = dsp_hw_shared_memory[0x09C0 + i];
	prepare_common_vector(count, count, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = ARRAY_SIZE(first_capture);
	if (!run_vector())
		return false;
	printf("# trace-layout-%u changed:", (uint32_t) count);
	for (size_t i = 0; i < ARRAY_SIZE(first_capture); i++) {
		uint16_t value = dsp_hw_shared_memory[0x09C0 + i];

		if (value != first_capture[i])
			continue;
		printf(" %u=%04X", (uint32_t) i, (uint32_t) value);
		changed++;
	}
	printf(" count=%u", (uint32_t) changed);
	printf("\n");
	test_eq_u32("captured traceback contains two bytes per configured timestamp", expected_changed, changed);
	return true;
}

static void print_changed_metrics(const char *name, uint16_t count) {
	printf("# %s W1 changed:", name);
	for (size_t i = 0; i < PHYSICAL_METRIC_COUNT; i++) {
		uint16_t value = dsp_hw_shared_memory[0x0940 + i];

		if (value != 0)
			printf(" %u=%04X", (uint32_t) i, (uint32_t) value);
	}
	printf("\n# %s W2 changed:", name);
	for (size_t i = 0; i < PHYSICAL_METRIC_COUNT; i++) {
		uint16_t value = dsp_hw_shared_memory[0x0980 + i];

		if (value != 0x7000 + i)
			printf(" %u=%04X", (uint32_t) i, (uint32_t) value);
	}
	printf("\n# %s count=%u trace=%04X\n", name, (uint32_t) count,
		(uint32_t) dsp_hw_shared_memory[0x09C0]);
}

static bool run_physical_layout_vector(uint16_t count) {
	char name[32];

	prepare_common_vector(count, count, PHYSICAL_METRIC_COUNT);
	for (size_t i = 0; i < PHYSICAL_METRIC_COUNT; i++)
		dsp_hw_shared_memory[0x0860 + i] = 0x7000 + i;
	if (!run_vector())
		return false;
	tfp_sprintf(name, "physical-layout-%u", (uint32_t) count);
	print_changed_metrics(name, count);
	return true;
}

static bool run_firmware_configuration_vector(void) {
	prepare_common_vector(16, 16, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 32;
	for (size_t i = 0; i < METRIC_COUNT_16; i++)
		dsp_hw_shared_memory[0x0820 + i] = firmware_metrics_16[i];
	for (size_t i = 0; i < 8; i++)
		dsp_hw_shared_memory[0x0810 + i] = firmware_references_16[i];
	if (!run_vector())
		return false;
	print_metrics("firmware-configuration");
	printf("# firmware-configuration trace-series:");
	for (size_t i = 0; i < 32; i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x09C0 + i]);
	printf("\n");
	return true;
}

static bool run_polynomial_configuration_vector(const char *name, const struct decoder_input inputs[16],
	const uint16_t expected_trace[32])
{
	char assertion[64];
	uint16_t model_trace[32];

	prepare_common_vector(16, 16, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 32;
	for (size_t i = 0; i < 8; i++)
		dsp_hw_shared_memory[0x0810 + i] = polynomial_references_16[i];
	for (size_t i = 0; i < 16; i++) {
		dsp_hw_shared_memory[0x08A0 + i * 2] = (uint8_t) inputs[i].sin0;
		dsp_hw_shared_memory[0x08A1 + i * 2] = (uint8_t) inputs[i].sin1;
		dsp_hw_shared_memory[0x08E0 + i * 2] = (uint8_t) inputs[i].sin2;
		dsp_hw_shared_memory[0x08E1 + i * 2] = 0;
	}
	if (!run_vector())
		return false;
	channel_decoder_model_series(inputs, model_trace);
	tfp_sprintf(assertion, "%s ACS model matches hardware golden", name);
	test_eq_memory(assertion, expected_trace, model_trace, sizeof(model_trace));
	tfp_sprintf(assertion, "%s polynomial trace matches hardware", name);
	test_eq_memory(assertion, expected_trace, dsp_hw_shared_memory + 0x09C0, sizeof(polynomial_constant_trace));
	printf("# polynomial-%s trace-series:", name);
	for (size_t i = 0; i < 32; i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x09C0 + i]);
	printf("\n");
	return true;
}

static bool run_soft_basis_vector(const struct soft_vector *vector) {
	uint16_t expected_sin0 = (uint16_t) (int16_t) (int8_t) vector->sin0;
	uint16_t expected_sin1 = (uint16_t) (int16_t) (int8_t) vector->sin1;
	uint16_t expected_sin2 = (uint16_t) (int16_t) (int8_t) vector->sin2;

	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x08A0] = vector->sin0;
	dsp_hw_shared_memory[0x08A1] = vector->sin1;
	dsp_hw_shared_memory[0x08E0] = vector->sin2;
	if (!run_vector())
		return false;
	test_eq_u32("SIN0 survives runner packing", expected_sin0, dsp_hw_shared_memory[0x0B80]);
	test_eq_u32("SIN1 survives runner packing", expected_sin1, dsp_hw_shared_memory[0x0B81]);
	test_eq_u32("SIN2 survives runner packing", expected_sin2, dsp_hw_shared_memory[0x0BC0]);
	printf("# soft-%s input=%02X,%02X,%02X metric=%04X,%04X,%04X,%04X,%04X\n", vector->name,
		(uint32_t) vector->sin0, (uint32_t) vector->sin1, (uint32_t) vector->sin2,
		(uint32_t) dsp_hw_shared_memory[0x0980], (uint32_t) dsp_hw_shared_memory[0x0981],
		(uint32_t) dsp_hw_shared_memory[0x0982], (uint32_t) dsp_hw_shared_memory[0x0983],
		(uint32_t) dsp_hw_shared_memory[0x0984]);
	return true;
}

static bool capture_branch_trace(uint16_t index, uint16_t sin0, uint16_t sin1, uint16_t sin2, uint16_t trace[2]) {
	uint16_t references = index * 0x1111;

	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	for (size_t i = 0; i < 8; i++)
		dsp_hw_shared_memory[0x0810 + i] = references;
	dsp_hw_shared_memory[0x08A0] = sin0;
	dsp_hw_shared_memory[0x08A1] = sin1;
	dsp_hw_shared_memory[0x08E0] = sin2;
	if (!run_vector())
		return false;
	trace[0] = dsp_hw_shared_memory[0x09C0];
	trace[1] = dsp_hw_shared_memory[0x09C1];
	return true;
}

static bool run_branch_coefficient_vector(uint16_t index) {
	uint16_t positive_sin0[2];
	uint16_t negative_sin0[2];
	uint16_t positive_sin1[2];
	uint16_t negative_sin1[2];
	uint16_t positive_sin2[2];
	uint16_t negative_sin2[2];

	if (!capture_branch_trace(index, 0x40, 0, 0, positive_sin0))
		return false;
	if (!capture_branch_trace(index, 0xC0, 0, 0, negative_sin0))
		return false;
	if (!capture_branch_trace(index, 0, 0x40, 0, positive_sin1))
		return false;
	if (!capture_branch_trace(index, 0, 0xC0, 0, negative_sin1))
		return false;
	if (!capture_branch_trace(index, 0, 0, 0x40, positive_sin2))
		return false;
	if (!capture_branch_trace(index, 0, 0, 0xC0, negative_sin2))
		return false;
	printf("# branch-coeff-%u trace-basis+64=%04X/%04X,%04X/%04X,%04X/%04X"
		" trace-basis-64=%04X/%04X,%04X/%04X,%04X/%04X\n", (uint32_t) index,
		(uint32_t) positive_sin0[0], (uint32_t) positive_sin0[1], (uint32_t) positive_sin1[0],
		(uint32_t) positive_sin1[1], (uint32_t) positive_sin2[0], (uint32_t) positive_sin2[1],
		(uint32_t) negative_sin0[0], (uint32_t) negative_sin0[1], (uint32_t) negative_sin1[0],
		(uint32_t) negative_sin1[1], (uint32_t) negative_sin2[0], (uint32_t) negative_sin2[1]);
	return true;
}

static bool run_reference_field_vector(uint16_t field) {
	uint16_t register_index = field / 4;
	uint16_t shift = field % 4 * 4;

	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	dsp_hw_shared_memory[0x0810 + register_index] = 2 << shift;
	if (!run_vector())
		return false;
	printf("# reference-field-%u trace=%04X,%04X\n", (uint32_t) field,
		(uint32_t) dsp_hw_shared_memory[0x09C0], (uint32_t) dsp_hw_shared_memory[0x09C1]);
	return true;
}

static bool run_reference_polarity_vector(uint16_t field) {
	uint16_t register_index = field / 4;
	uint16_t shift = field % 4 * 4;

	prepare_common_vector(1, 1, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 2;
	dsp_hw_shared_memory[0x0810 + register_index] = 1 << shift;
	if (!run_vector())
		return false;
	printf("# reference-polarity-field-%u trace=%04X,%04X\n", (uint32_t) field,
		(uint32_t) dsp_hw_shared_memory[0x09C0], (uint32_t) dsp_hw_shared_memory[0x09C1]);
	return true;
}

static bool run_pipeline_alignment_vector(uint16_t impulse_timestamp) {
	prepare_common_vector(13, 13, METRIC_COUNT_16);
	dsp_hw_shared_memory[0x0805] = 26;
	dsp_hw_shared_memory[0x0810] = 2;
	for (size_t timestamp = 0; timestamp < 13; timestamp++) {
		dsp_hw_shared_memory[0x08A0 + timestamp * 2] = 0;
		dsp_hw_shared_memory[0x08A1 + timestamp * 2] = 0;
		dsp_hw_shared_memory[0x08E0 + timestamp * 2] = 0;
		dsp_hw_shared_memory[0x08E1 + timestamp * 2] = 0;
	}
	if (impulse_timestamp < 13)
		dsp_hw_shared_memory[0x08E0 + impulse_timestamp * 2] = 64;
	if (!run_vector())
		return false;
	printf("# pipeline-impulse-%u trace:", (uint32_t) impulse_timestamp);
	for (size_t i = 0; i < 26; i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x09C0 + i]);
	printf("\n");
	return true;
}

int main(void) {
	test_start("DSP channel decoder algorithm vectors");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Channel decoder / predecessor mapping");
	if (run_predecessor_vector()) {
		test_category("Channel decoder / trellis mapping");
		if (run_trellis_probe()) {
			run_tie_vector();
			for (uint16_t state = 0; state < METRIC_COUNT_16; state++) {
				if (!run_preferred_state_vector(state))
					break;
			}
			for (uint16_t state = 0; state < METRIC_COUNT_16; state++) {
				if (!run_rejected_state_vector(state))
					break;
			}

			for (size_t i = 0; i < ARRAY_SIZE(metric_amplitudes); i++) {
				for (uint16_t state = 0; state < METRIC_COUNT_16; state++) {
					if (!run_metric_impulse_vector(state, metric_amplitudes[i]))
						break;
				}
			}
		}

		test_category("Channel decoder / branch metric index mapping");
		for (uint16_t index = 0; index < 16; index++) {
			if (!run_branch_index_vector(index))
				break;
		}
		for (uint16_t index = 0; index < 16; index += 2) {
			if (!run_reference_bit_zero_vector(index))
				break;
		}

		test_category("Channel decoder / timestamp schedule");
		for (uint16_t count = 1; count <= MAX_DECODE_TIMESTAMPS; count++) {
			if (!run_count_vector(count))
				break;
		}
		for (size_t i = 0; i < ARRAY_SIZE(trace_extent_counts); i++) {
			if (!run_trace_extent_vector(trace_extent_counts[i]))
				break;
		}

		test_category("Channel decoder / physical metric layout");
		if (run_physical_layout_vector(1))
			run_physical_layout_vector(16);

		test_category("Channel decoder / metric arithmetic");
		run_metric_wrap_vector();
		for (uint16_t count = 1; count <= 4; count++) {
			if (!run_overflow_schedule_vector(count, false))
				break;
			if (!run_overflow_schedule_vector(count, true))
				break;
		}

		test_category("Channel decoder / 64-state ACS");
		if (run_64_state_probe())
			run_64_state_series();

		test_category("Channel decoder / 64-state reference routing");
		for (size_t field = 0; field < 32; field++) {
			if (!run_64_reference_field_vector(field))
				break;
		}
		run_64_reference_bit_zero_vector();

		test_category("Channel decoder / end-to-end traceback");
		validate_generated_reference_tables();
		for (size_t i = 0; i < ARRAY_SIZE(decoding_vectors); i++) {
			if (!run_decoding_vector(&decoding_vectors[i]))
				break;
		}

		test_category("Channel decoder / Mask ROM configuration");
		if (run_firmware_configuration_vector()) {
			test_category("Channel decoder / K=5 rate-1/3 polynomial");
			run_polynomial_configuration_vector("constant", polynomial_constant_inputs, polynomial_constant_trace);
			run_polynomial_configuration_vector("basis", polynomial_basis_inputs, polynomial_basis_trace);
			run_polynomial_configuration_vector("boundary", polynomial_boundary_inputs, polynomial_boundary_trace);
			run_polynomial_configuration_vector("scrambled", polynomial_scrambled_inputs, polynomial_scrambled_trace);

			test_category("Channel decoder / soft-input basis");
			for (size_t i = 0; i < ARRAY_SIZE(soft_basis_vectors); i++) {
				if (!run_soft_basis_vector(&soft_basis_vectors[i]))
					break;
			}

			test_category("Channel decoder / branch metric coefficients");
			for (uint16_t index = 0; index < 16; index++) {
				if (!run_branch_coefficient_vector(index))
					break;
			}

			test_category("Channel decoder / butterfly reference routing");
			for (uint16_t field = 0; field < 32; field++) {
				if (!run_reference_field_vector(field))
					break;
			}
			for (uint16_t field = 0; field < 8; field++) {
				if (!run_reference_polarity_vector(field))
					break;
			}

			test_category("Channel decoder / pipeline alignment");
			for (uint16_t timestamp = 0; timestamp <= 13; timestamp++) {
				if (!run_pipeline_alignment_vector(timestamp))
					break;
			}
		}
	}

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
