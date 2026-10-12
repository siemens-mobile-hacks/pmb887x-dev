#include <pmb887x.h>

#include "dsp/dsp-hw.h"
#ifdef PMB8875
#include "sch-decoder-vector-runner-8875.inc"
#define DSP_SCH_DECODER_VECTOR_RUNNER DSP_SCH_DECODER_VECTOR_RUNNER_8875
#else
#include "sch-decoder-vector-runner-8876.inc"
#define DSP_SCH_DECODER_VECTOR_RUNNER DSP_SCH_DECODER_VECTOR_RUNNER_8876
#endif
#include "test.h"

#define INPUT_WORD_COUNT 39
#define STATE_WORD_COUNT 190
#define RESULT_OFFSET 0x0100
#define COMPLETE_OFFSET 0x0108
#define REGISTER_OFFSET 0x0109
#define STATE_COPY_OFFSET 0x0180
#define COMPLETE_MARKER 0xA55A
#define CANARY_BEFORE 0xC33C
#define CANARY_AFTER 0x5AA5
#define SOFT_ONE ((int8_t) -127)
#define SOFT_ZERO ((int8_t) 127)

enum sch_vector_kind {
	SCH_VECTOR_VALID,
	SCH_VECTOR_DISTURBED,
	SCH_VECTOR_INVALID,
};

struct sch_vector {
	const char *name;
	enum sch_vector_kind kind;
};

struct sch_golden {
	uint16_t a0l;
	uint16_t a0h;
	uint16_t a1l;
	uint16_t a1h;
	uint16_t data0;
	uint16_t data1;
};

struct sch_mask_profile {
	uint16_t mask_id;
	uint16_t decoder_address;
	uint16_t callback_address;
	uint16_t input_address;
	uint16_t state_first_address;
	uint16_t state_result_address;
	uint16_t state_work_address;
	uint16_t state_last_address;
	uint16_t st1;
	uint16_t stack_pointer;
};

static const struct sch_vector SCH_VECTORS[] = {
	{ "valid", SCH_VECTOR_VALID },
	{ "correctable", SCH_VECTOR_DISTURBED },
	{ "invalid", SCH_VECTOR_INVALID },
};

static const struct sch_golden SCH_GOLDENS[] = {
#include "sch-decoder-golden.inc"
};

static const struct sch_mask_profile SCH_MASK_PROFILES[] = {
	{
		.mask_id = 0x0602,
		.decoder_address = 0xB115,
		.callback_address = 0x587D,
		.input_address = 0x02C5,
		.state_first_address = 0x025A,
		.state_result_address = 0x025B,
		.state_work_address = 0x025D,
		.state_last_address = 0x0317,
		.st1 = 0x005D,
		.stack_pointer = 0x5C28,
	},
	{
		.mask_id = 0x0604,
		.decoder_address = 0xB2FE,
		.callback_address = 0x58CE,
		.input_address = 0x02CA,
		.state_first_address = 0x025F,
		.state_result_address = 0x0260,
		.state_work_address = 0x0262,
		.state_last_address = 0x031C,
		.st1 = 0x005D,
		.stack_pointer = 0x5C28,
	},
	{
		.mask_id = 0x0801,
		.decoder_address = 0xA115,
		.callback_address = 0x779E,
		.input_address = 0x03E2,
		.state_first_address = 0x0373,
		.state_result_address = 0x0374,
		.state_work_address = 0x0376,
		.state_last_address = 0x0430,
		.st1 = 0x007D,
		.stack_pointer = 0x7C14,
	},
};

static const struct sch_mask_profile *find_sch_mask_profile(uint16_t mask_id) {
	for (size_t i = 0; i < ARRAY_SIZE(SCH_MASK_PROFILES); i++) {
		if (SCH_MASK_PROFILES[i].mask_id == mask_id)
			return &SCH_MASK_PROFILES[i];
	}

	return NULL;
}

static uint8_t parity(uint8_t value) {
	value ^= value >> 4;
	value ^= value >> 2;
	value ^= value >> 1;
	return value & 1;
}

static void sch_information_bits(uint8_t bits[25]) {
	const uint16_t t1 = 0x321;
	const uint8_t bsic = 0x2A;
	const uint8_t t2 = 17;
	const uint8_t t3p = 3;

	bits[0] = (t1 >> 9) & 1;
	bits[1] = (t1 >> 10) & 1;
	for (size_t i = 0; i < 6; i++)
		bits[2 + i] = (bsic >> i) & 1;
	for (size_t i = 0; i < 8; i++)
		bits[8 + i] = (t1 >> (i + 1)) & 1;
	bits[16] = (t3p >> 1) & 1;
	bits[17] = (t3p >> 2) & 1;
	for (size_t i = 0; i < 5; i++)
		bits[18 + i] = (t2 >> i) & 1;
	bits[23] = t1 & 1;
	bits[24] = t3p & 1;
}

static void sch_encode(uint8_t coded[78]) {
	uint8_t uncoded[39] = { 0 };
	uint16_t crc = 0;
	uint8_t state = 0;

	sch_information_bits(uncoded);
	// 3GPP TS 05.03 SCH: CRC generator x^10+x^8+x^6+x^5+x^4+x^2+1,
	// followed by four zero tail bits.
	for (size_t i = 0; i < 25; i++) {
		crc ^= (uint16_t) uncoded[i] << 9;
		crc = (crc << 1) ^ ((crc & BIT(10)) != 0 ? 0x175 : 0);
		crc &= 0x3FF;
	}
	crc ^= 0x3FF;
	for (size_t i = 0; i < 10; i++)
		uncoded[25 + i] = (crc >> (9 - i)) & 1;

	// Rate-1/2, K=5 convolutional code: G0=1+D^3+D^4, G1=1+D+D^3+D^4.
	for (size_t i = 0; i < ARRAY_SIZE(uncoded); i++) {
		uint8_t history = (state << 1) | uncoded[i];

		coded[i * 2] = parity(history & 0x19);
		coded[i * 2 + 1] = parity(history & 0x1B);
		state = history & 0x0F;
	}
}

static void prepare_input(enum sch_vector_kind kind, uint16_t words[INPUT_WORD_COUNT]) {
	uint8_t coded[78];

	sch_encode(coded);
	for (size_t i = 0; i < ARRAY_SIZE(coded); i++) {
		uint8_t bit = coded[i];
		// Twenty-four inverted coded bits produce a stable CRC failure, while
		// eight zero-confidence soft bits remain correctable.
		if (kind == SCH_VECTOR_INVALID && i < 24)
			bit ^= 1;
		int8_t soft = bit ? SOFT_ONE : SOFT_ZERO;

		if (kind == SCH_VECTOR_DISTURBED && i >= 24 && i < 32)
			soft = 0;
		// A115 consumes consecutive signed bytes, low byte first.
		if ((i & 1) == 0) {
			words[i / 2] = (uint8_t) soft;
		} else {
			words[i / 2] |= (uint16_t) (uint8_t) soft << 8;
		}
	}
}

static bool load_vector(const struct sch_mask_profile *profile, enum sch_vector_kind kind) {
	uint16_t state[STATE_WORD_COUNT] = { 0 };
	uint16_t input[INPUT_WORD_COUNT] = { 0 };
	uint16_t callback = 0;

	state[0] = CANARY_BEFORE;
	state[STATE_WORD_COUNT - 1] = CANARY_AFTER;
	prepare_input(kind, input);
	return dsp_hw_write_data(profile->state_first_address, state, ARRAY_SIZE(state)) &&
		dsp_hw_write_data(profile->input_address, input, ARRAY_SIZE(input)) &&
		dsp_hw_write_data(profile->callback_address, &callback, 1);
}

static void print_capture(const struct sch_mask_profile *profile, const char *name) {
	printf("# SCH_GOLDEN_BEGIN name=%s mask=%04X\n", name, profile->mask_id);
	printf("# RESULT A0=%04lX:%04lX A1=%04lX:%04lX DATA0=%04lX DATA1=%04lX\n",
		(uint32_t) dsp_hw_shared_memory[RESULT_OFFSET + 1],
		(uint32_t) dsp_hw_shared_memory[RESULT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[RESULT_OFFSET + 3],
		(uint32_t) dsp_hw_shared_memory[RESULT_OFFSET + 2],
		(uint32_t) dsp_hw_shared_memory[RESULT_OFFSET + 4],
		(uint32_t) dsp_hw_shared_memory[RESULT_OFFSET + 5]);
	printf("# REG R0=%04lX R1=%04lX R2=%04lX R3=%04lX R4=%04lX R5=%04lX R7=%04lX Y0=%04lX SV=%04lX "
		"SP=%04lX ST0=%04lX ST1=%04lX ST2=%04lX CFGI=%04lX CFGJ=%04lX LC=%04lX MIXP=%04lX\n",
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 1],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 2],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 3],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 4],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 5],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 6],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 7],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 8],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 9],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 10],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 11],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 12],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 13],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 14],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 15],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 16]);
	printf("# REG X0=%04lX:%04lX P=%04lX:%04lX B0=%04lX:%04lX B1=%04lX:%04lX\n",
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 18],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 17],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 20],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 19],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 22],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 21],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 24],
		(uint32_t) dsp_hw_shared_memory[REGISTER_OFFSET + 23]);
	printf("# STATE");
	for (size_t i = 0; i < STATE_WORD_COUNT - 2; i++)
		printf(" %04lX", (uint32_t) dsp_hw_shared_memory[STATE_COPY_OFFSET + i]);
	printf("\n# CANARY BEFORE=%04lX AFTER=%04lX COMPLETE=%04lX\n",
		(uint32_t) dsp_hw_shared_memory[RESULT_OFFSET + 6],
		(uint32_t) dsp_hw_shared_memory[RESULT_OFFSET + 7],
		(uint32_t) dsp_hw_shared_memory[COMPLETE_OFFSET]);
	printf("# SCH_GOLDEN_END\n");
}

static void check_decoded_fields(void) {
	uint32_t packed = dsp_hw_shared_memory[RESULT_OFFSET + 4] |
		((uint32_t) dsp_hw_shared_memory[RESULT_OFFSET + 5] << 16);
	uint16_t t1 = ((packed >> 23) & 1) |
		(((packed >> 8) & 0xFF) << 1) |
		(((packed >> 0) & 0x03) << 9);
	uint8_t bsic = (packed >> 2) & 0x3F;
	uint8_t t2 = (packed >> 18) & 0x1F;
	uint8_t t3p = ((packed >> 24) & 1) | (((packed >> 16) & 0x03) << 1);
	uint8_t t3 = t3p * 10 + 1;
	uint8_t tt = t3 < t2 ? t3 + 26 - t2 : (t3 - t2) % 26;
	uint32_t frame_number = t1 * 51 * 26 + tt * 51 + t3;

	test_eq_u32("decoded T1 matches the independently constructed SCH", 0x321, t1);
	test_eq_u32("decoded BSIC matches the independently constructed SCH", 0x2A, bsic);
	test_eq_u32("decoded T2 matches the independently constructed SCH", 17, t2);
	test_eq_u32("decoded T3' matches the independently constructed SCH", 3, t3p);
	test_eq_u32("decoded GSM frame number matches the independently constructed SCH", 1062871, frame_number);
}

static void check_golden(const struct sch_golden *golden) {
	test_eq_u32("A0 low matches the hardware golden", golden->a0l,
		dsp_hw_shared_memory[RESULT_OFFSET]);
	test_eq_u32("A0 high matches the hardware golden", golden->a0h,
		dsp_hw_shared_memory[RESULT_OFFSET + 1]);
	test_eq_u32("A1 low matches the hardware golden", golden->a1l,
		dsp_hw_shared_memory[RESULT_OFFSET + 2]);
	test_eq_u32("A1 high matches the hardware golden", golden->a1h,
		dsp_hw_shared_memory[RESULT_OFFSET + 3]);
	test_eq_u32("first decoded result word matches the hardware golden", golden->data0,
		dsp_hw_shared_memory[RESULT_OFFSET + 4]);
	test_eq_u32("second decoded result word matches the hardware golden", golden->data1,
		dsp_hw_shared_memory[RESULT_OFFSET + 5]);
}

static void configure_runner(const struct sch_mask_profile *profile) {
	dsp_hw_shared_memory[0x0000] = profile->decoder_address;
	dsp_hw_shared_memory[0x0001] = profile->input_address;
	dsp_hw_shared_memory[0x0002] = profile->state_result_address;
	dsp_hw_shared_memory[0x0003] = profile->state_work_address;
	dsp_hw_shared_memory[0x0004] = profile->state_first_address;
	dsp_hw_shared_memory[0x0005] = profile->st1;
	dsp_hw_shared_memory[0x0006] = profile->state_last_address;
	dsp_hw_shared_memory[0x0007] = profile->stack_pointer;
}

static void run_vector(const struct sch_mask_profile *profile, const struct sch_vector *vector,
	const struct sch_golden *golden) {
	test_category(vector->name);
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return;
	if (!test_eq_u32("Mask ROM revision remains selected", profile->mask_id, dsp_hw_shared_memory[0]))
		return;
	for (size_t i = RESULT_OFFSET; i < STATE_COPY_OFFSET + STATE_WORD_COUNT; i++)
		dsp_hw_shared_memory[i] = 0;
	if (!test_check("DLOAD initializes every decoder input", load_vector(profile, vector->kind)))
		return;
	if (!test_check("PLOAD installs the direct SCH decoder wrapper",
		dsp_hw_load_image(DSP_SCH_DECODER_VECTOR_RUNNER, sizeof(DSP_SCH_DECODER_VECTOR_RUNNER)))) {
		return;
	}
	configure_runner(profile);
	if (!test_check("BRANCH starts the direct SCH decoder wrapper", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return;
	if (!test_check("Mask ROM SCH decoder returns",
		dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 1000))) {
		return;
	}

	print_capture(profile, vector->name);
	check_golden(golden);
	if (vector->kind != SCH_VECTOR_INVALID)
		check_decoded_fields();
	test_eq_u32("lower workspace canary survives", CANARY_BEFORE,
		dsp_hw_shared_memory[RESULT_OFFSET + 6]);
	test_eq_u32("upper workspace canary survives", CANARY_AFTER,
		dsp_hw_shared_memory[RESULT_OFFSET + 7]);
}

int main(void) {
	test_start("DSP Mask ROM SCH decoder vectors");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	uint16_t mask_id = dsp_hw_shared_memory[0];
	printf("# DSP mask ID: %04lX\n", (uint32_t) mask_id);
	const struct sch_mask_profile *profile = find_sch_mask_profile(mask_id);
	if (!test_check("DSP Mask ROM SCH decoder profile is supported", profile != NULL))
		return test_finish();

	_Static_assert(ARRAY_SIZE(SCH_VECTORS) == ARRAY_SIZE(SCH_GOLDENS), "SCH golden count");
	for (size_t i = 0; i < ARRAY_SIZE(SCH_VECTORS); i++)
		run_vector(profile, &SCH_VECTORS[i], &SCH_GOLDENS[i]);

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
