#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A
#define A53_COMMAND 0x0904
#define A53_PARAMETER_BASE 0x0A00
#define A53_RESULT_BASE 0x0A20
#define A53_KEY_WORDS 8
#define A53_COUNTER_WORDS 3
#define A53_RESULT_WORDS 16
#define A53_WORDS_PER_STREAM 8
#define A53_TAIL_MASK 0x0003

#ifdef PMB8875
#include "cipher-a53-functional-8875.inc"
#define DSP_CIPHER_A53_IMAGE DSP_CIPHER_A53_IMAGE_8875
#else
#include "cipher-a53-functional-8876.inc"
#define DSP_CIPHER_A53_IMAGE DSP_CIPHER_A53_IMAGE_8876
#endif

struct a53_vector {
	const char *name;
	uint16_t key[A53_KEY_WORDS];
	uint16_t counters[A53_COUNTER_WORDS];
	uint16_t expected[A53_RESULT_WORDS];
};

// A5/3 vectors from 3GPP TS 55.217 and TS 55.218. The key and counter words use
// the MCU CIPH_KEY command layout; each expected 114-bit stream is reversed by
// the TEAK A5/3 wrapper and packed least-significant bit first in Cipher RAM:
// physical[p] = logical[113 - p] independently for DL and UL.
static const struct a53_vector vectors[] = {
	{
		"3GPP A5/3 set 1",
		{ 0xBC00, 0x82C5, 0x459F, 0x2BD6, 0xBC00, 0x82C5, 0x459F, 0x2BD6 },
		{ 0x000F, 0x0010, 0x049E },
		{
			0xCB91, 0x0D88, 0xEF61, 0xE86A, 0x7B46, 0xAABE, 0x227B, 0x0002,
			0x8B7D, 0xEAB6, 0x3C11, 0x3DA7, 0x8913, 0x01AA, 0x728D, 0x0001,
		},
	},
	{
		"3GPP A5/3 set 2",
		{ 0xFF48, 0x4881, 0x4910, 0x952C, 0xFF48, 0x4881, 0x4910, 0x952C },
		{ 0x0012, 0x0013, 0x00C2 },
		{
			0xA697, 0x5A1B, 0x24A1, 0x8CCE, 0xB84E, 0x7EF3, 0xED35, 0x0003,
			0x8B91, 0xD9D9, 0x15F8, 0x115F, 0x8150, 0x0DE3, 0x9424, 0x0000,
		},
	},
	{
		"3GPP A5/3 set 3",
		{ 0x0C2A, 0x9E72, 0xB222, 0xEFA8, 0x0C2A, 0x9E72, 0xB222, 0xEFA8 },
		{ 0x001F, 0x0029, 0x067F },
		{
			0x8C0D, 0x1A03, 0x0F76, 0x91A7, 0x68CD, 0x55D5, 0x3900, 0x0000,
			0x91FE, 0x68A3, 0x090C, 0x62C1, 0xAD38, 0x9A78, 0xBC41, 0x0001,
		},
	},
	{
		"3GPP A5/3 set 4",
		{ 0xFF48, 0x4881, 0x4910, 0x952C, 0xFF48, 0x4881, 0x4910, 0x952C },
		{ 0x0007, 0x0029, 0x00C2 },
		{
			0x0029, 0x932E, 0xA9DB, 0xC976, 0x5CE8, 0xCE29, 0xADF6, 0x0002,
			0xF042, 0xDEDE, 0x25E2, 0x03FA, 0xAA74, 0x653F, 0x312D, 0x0001,
		},
	},
	{
		"3GPP A5/3 set 5",
		{ 0x2C87, 0x43BD, 0xF23A, 0x3451, 0x2C87, 0x43BD, 0xF23A, 0x3451 },
		{ 0x000C, 0x000C, 0x01C8 },
		{
			0xED53, 0x791B, 0xEE81, 0x4177, 0x5582, 0x1314, 0xD7DF, 0x0001,
			0x56FC, 0x1B7C, 0x1506, 0x5E78, 0xF37E, 0x54D4, 0x64B2, 0x0000,
		},
	},
	{
		"3GPP A5/3 set 6",
		{ 0x35CF, 0xE824, 0x639B, 0xCAA2, 0x35CF, 0xE824, 0x639B, 0xCAA2 },
		{ 0x0009, 0x0011, 0x05FE },
		{
			0x81BB, 0x18C5, 0x2413, 0x5975, 0x5359, 0xDF93, 0xC050, 0x0000,
			0xD4F7, 0xDA10, 0x6A0B, 0x34F8, 0x7899, 0xE1E5, 0xC28E, 0x0003,
		},
	},
	{
		"3GPP A5/3 set 7",
		{ 0x9FA6, 0x400B, 0x7E87, 0x7AE6, 0x9FA6, 0x400B, 0x7E87, 0x7AE6 },
		{ 0x0005, 0x0027, 0x05E4 },
		{
			0xA884, 0x9DE5, 0x8D22, 0xF4BA, 0xBD90, 0xA43F, 0xDE50, 0x0003,
			0x0614, 0xA5D6, 0x7FBF, 0x16BE, 0xE29C, 0x9B1A, 0x2DBE, 0x0003,
		},
	},
	{
		"3GPP A5/3 set 8",
		{ 0x698B, 0x5540, 0x6993, 0x58AF, 0x698B, 0x5540, 0x6993, 0x58AF },
		{ 0x000B, 0x0023, 0x00A8 },
		{
			0xF91D, 0x8757, 0x6311, 0x9696, 0xDA47, 0x939A, 0xD272, 0x0001,
			0xB6F1, 0x820A, 0x6AA2, 0x9DEB, 0x3411, 0x9113, 0xC727, 0x0000,
		},
	},
	{
		"3GPP A5/3 set 9",
		{ 0xFE62, 0xF236, 0x81E5, 0x017F, 0xFE62, 0xF236, 0x81E5, 0x017F },
		{ 0x0006, 0x0019, 0x02AD },
		{
			0x09DB, 0x4858, 0x3E7D, 0x313A, 0x7983, 0xD9D8, 0xA9A5, 0x0000,
			0x45E7, 0x4A3C, 0x5850, 0xE30D, 0x7CB1, 0x611D, 0x9513, 0x0002,
		},
	},
	{
		"3GPP A5/3 set 10",
		{ 0x7B39, 0x8B76, 0x8B44, 0x1ACA, 0x7B39, 0x8B76, 0x8B44, 0x1ACA },
		{ 0x0015, 0x001D, 0x0178 },
		{
			0xC419, 0x183A, 0x7E87, 0xC1FD, 0x8B25, 0x3716, 0x93DC, 0x0002,
			0xFA14, 0xD208, 0x2D71, 0x0500, 0xCCA3, 0xD65E, 0xDE02, 0x0001,
		},
	},
};

static bool result_matches(const uint16_t expected[A53_RESULT_WORDS]) {
	for (size_t stream = 0; stream < 2; stream++) {
		for (size_t word = 0; word < A53_WORDS_PER_STREAM; word++) {
			size_t index = stream * A53_WORDS_PER_STREAM + word;
			uint16_t mask = word + 1 == A53_WORDS_PER_STREAM ? A53_TAIL_MASK : UINT16_MAX;

			if ((dsp_hw_shared_memory[A53_RESULT_BASE + index] & mask) != expected[index])
				return false;
		}
	}
	return true;
}

static bool run_request(const struct a53_vector *vector, uint16_t request) {
	for (size_t i = 0; i < A53_KEY_WORDS; i++)
		dsp_hw_shared_memory[A53_PARAMETER_BASE + i] = vector->key[i];
	for (size_t i = 0; i < A53_COUNTER_WORDS; i++)
		dsp_hw_shared_memory[A53_PARAMETER_BASE + A53_KEY_WORDS + i] = vector->counters[i];
	dsp_hw_shared_memory[A53_COMMAND] = request;
	return dsp_hw_wait_shared(A53_COMMAND, 0, 100);
}

int main(void) {
	uint16_t request = 1;

	test_start("DSP cipher A5/3 standard vectors");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load A5/3 vector server",
		dsp_hw_load_image(DSP_CIPHER_A53_IMAGE, sizeof(DSP_CIPHER_A53_IMAGE))))
		return test_finish();
	if (!test_check("BRANCH starts A5/3 vector server", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("A5/3 vector server becomes ready", dsp_hw_wait_shared(0x0900, READY_MARKER, 100)))
		return test_finish();
	if (!test_check("A5/3 vector scenarios become available", dsp_hw_wait_shared(0x0901, COMPLETE_MARKER, 3000)))
		return test_finish();

	for (size_t i = 0; i < ARRAY_SIZE(vectors); i++) {
		test_category(vectors[i].name);
		if (!test_check("hardware vector completes", run_request(&vectors[i], request++)))
			continue;
		test_check("both 114-bit streams match the 3GPP vector", result_matches(vectors[i].expected));
	}
	test_eq_u32("each vector delivers one cipher interrupt", ARRAY_SIZE(vectors), dsp_hw_shared_memory[0x0902]);
	test_eq_u32("INT1 handler observes the CIPH source", BIT(0), dsp_hw_shared_memory[0x0903] & BIT(0));

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
