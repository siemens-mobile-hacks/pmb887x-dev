#include <pmb887x.h>
#include <gen/dsp.h>

#ifdef PMB8875
#include "data-ram-8875.inc"
#define DATA_RAM_IMAGE DSP_DATA_RAM_8875
#endif
#ifdef PMB8876
#include "data-ram-8876.inc"
#define DATA_RAM_IMAGE DSP_DATA_RAM_8876
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define RUNNER_ADDRESS 0x0100
#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x01FF
#define RESULT_OFFSET 0x0200

#ifdef PMB8875
#define MODULATOR_RAM_BITS 0x0001
#endif
#ifdef PMB8876
#define MODULATOR_RAM_BITS 0x000F
#endif

static const uint16_t WRITTEN_VALUES[] = { 0x1234, 0xA5C3, 0xFFFF };

static const struct {
	const char *name;
	uint16_t stored_bits;
} REGIONS[] = {
	{ "X RAM", 0xFFFF },
	{ "Y RAM", 0xFFFF },
	{ "demodulator RAM", 0xFFFF },
	{ "modulator RAM", MODULATOR_RAM_BITS },
};

int main(void) {
	size_t result_words = ARRAY_SIZE(REGIONS) * ARRAY_SIZE(WRITTEN_VALUES);

	test_start("DSP data RAM write test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();

	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t i = COMPLETE_OFFSET; i < RESULT_OFFSET + result_words; i++)
		dsp_hw_shared_memory[i] = 0;
	if (!test_check("boot commands load the data RAM runner",
		dsp_hw_load_image(DATA_RAM_IMAGE, sizeof(DATA_RAM_IMAGE))))
		return test_finish();
	if (!test_check("BRANCH starts the data RAM runner", dsp_hw_branch(RUNNER_ADDRESS)))
		return test_finish();
	if (!test_check("data RAM runner completes", dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 500)))
		return test_finish();

	test_category("DSP writes read back from data RAM");
	for (size_t region = 0; region < ARRAY_SIZE(REGIONS); region++) {
		for (size_t value = 0; value < ARRAY_SIZE(WRITTEN_VALUES); value++) {
			uint16_t read_back = dsp_hw_shared_memory[RESULT_OFFSET + region * ARRAY_SIZE(WRITTEN_VALUES) + value];
			char name[64];

			sprintf(name, "%s keeps stored bits of %04X", REGIONS[region].name, WRITTEN_VALUES[value]);
			test_eq_u32(name, WRITTEN_VALUES[value] & REGIONS[region].stored_bits, read_back);
		}
	}

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
