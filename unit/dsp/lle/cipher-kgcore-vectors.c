#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define KGCORE_COMMAND 0x0904
#define KGCORE_PARAMETER_BASE 0x0B00
#define KGCORE_RESULT_BASE 0x0B20
#define KGCORE_KEY_WORDS 8
#define KGCORE_KDATA_WORDS 4
#define KGCORE_RESULT_WORDS 16
#define KGCORE_WORDS_PER_STREAM 8
#define KGCORE_TAIL_MASK 0x0003

#ifdef PMB8875
#include "cipher-kgcore-vectors-8875.inc"
#define DSP_CIPHER_KGCORE_IMAGE DSP_CIPHER_KGCORE_IMAGE_8875
#else
#include "cipher-kgcore-vectors-8876.inc"
#define DSP_CIPHER_KGCORE_IMAGE DSP_CIPHER_KGCORE_IMAGE_8876
#endif

struct kgcore_vector {
	const char *name;
	uint16_t key[KGCORE_KEY_WORDS];
	uint16_t kdata[KGCORE_KDATA_WORDS];
	uint16_t expected[KGCORE_RESULT_WORDS];
};

// A5/4 vectors from 3GPP TS 55.217 and TS 55.218, encoded in the physical
// A5/3 register layout. KEY0..7 select logical CK byte pairs 12/13, 15/14,
// 8/9, 11/10, 4/5, 7/6, 0/1, and 3/2 respectively; each first byte occupies
// the low half of its word. KDATA1=CA<<8, KDATA3=COUNT[23:16]<<8, and
// KDATA4=COUNT[15:0]. Unlike the MCU A5/3 command, these vectors exercise all
// 128 independent key bits accepted by the KGCORE engine.
static const struct kgcore_vector vectors[] = {
	{
		"3GPP A5/4 set 1",
		{ 0xC1B5, 0xF85E, 0xF17F, 0xF97E, 0x58C9, 0x1E33, 0x433D, 0xC388 },
		{ 0x0F00, 0x0000, 0x3500, 0xD2CF },
		{
			0xFB0D, 0xC242, 0x8CF1, 0xB313, 0xDAC8, 0xC0D2, 0x8BF8, 0x0002,
			0x2EA2, 0xA2BB, 0xE46F, 0xE25E, 0x0CBF, 0xD25D, 0x5C35, 0x0000,
		},
	},
	{
		"3GPP A5/4 set 2",
		{ 0x3E4A, 0x07A1, 0x453B, 0x0681, 0x4FDF, 0x399F, 0x49A4, 0x6A64 },
		{ 0x0F00, 0x0000, 0x2100, 0x2777 },
		{
			0x5681, 0xD5DD, 0x06F3, 0x440A, 0x37E4, 0xB8D8, 0x2737, 0x0002,
			0xEBDF, 0x2460, 0x2FF1, 0x6327, 0xDE56, 0xB031, 0xCE08, 0x0000,
		},
	},
};

static bool result_matches(const uint16_t expected[KGCORE_RESULT_WORDS]) {
	for (size_t stream = 0; stream < 2; stream++) {
		for (size_t word = 0; word < KGCORE_WORDS_PER_STREAM; word++) {
			size_t index = stream * KGCORE_WORDS_PER_STREAM + word;
			uint16_t mask = word + 1 == KGCORE_WORDS_PER_STREAM ? KGCORE_TAIL_MASK : UINT16_MAX;

			if ((dsp_hw_shared_memory[KGCORE_RESULT_BASE + index] & mask) != expected[index])
				return false;
		}
	}
	return true;
}

static bool run_request(const struct kgcore_vector *vector, uint16_t request) {
	for (size_t i = 0; i < KGCORE_KEY_WORDS; i++)
		dsp_hw_shared_memory[KGCORE_PARAMETER_BASE + i] = vector->key[i];
	for (size_t i = 0; i < KGCORE_KDATA_WORDS; i++)
		dsp_hw_shared_memory[KGCORE_PARAMETER_BASE + KGCORE_KEY_WORDS + i] = vector->kdata[i];
	dsp_hw_shared_memory[KGCORE_COMMAND] = request;
	return dsp_hw_wait_shared(KGCORE_COMMAND, 0, 100);
}

int main(void) {
	test_start("DSP cipher KGCORE 128-bit vectors");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load KGCORE vector server",
		dsp_hw_load_image(DSP_CIPHER_KGCORE_IMAGE, sizeof(DSP_CIPHER_KGCORE_IMAGE))))
		return test_finish();
	if (!test_check("BRANCH starts KGCORE vector server", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("KGCORE vector server becomes ready", dsp_hw_wait_shared(0x0900, READY_MARKER, 100)))
		return test_finish();

	for (size_t i = 0; i < ARRAY_SIZE(vectors); i++) {
		test_category(vectors[i].name);
		if (!test_check("hardware vector completes", run_request(&vectors[i], (uint16_t) (i + 1))))
			continue;
		test_check("both 114-bit streams match the 3GPP vector", result_matches(vectors[i].expected));
	}

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
