#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A

#ifdef PMB8875
#include "equalizer-functional-8875.inc"
#define DSP_EQUALIZER_IMAGE DSP_EQUALIZER_IMAGE_8875
#else
#include "equalizer-functional-8876.inc"
#define DSP_EQUALIZER_IMAGE DSP_EQUALIZER_IMAGE_8876
#endif

static const uint16_t RX_UNPACKED[] = { 0x0000, 0x0001, 0x7FFF, 0x8000, 0xA55A, 0x5AA5 };
static const uint16_t BPAR_UNPACKED[] = { 0x1357, 0x2468, 0x369C, 0x47AD };
static const uint16_t SOUT_PACKED[] = { 0x0000, 0x007F, 0xFF80, 0xFFFF, 0x0055, 0xFFAA };
static const uint16_t HOUT_UNPACKED[] = { 0x1122, 0x3344, 0x5566, 0x7788 };
static const uint16_t EML_PACKED[] = { 0x1122, 0x3344, 0x5566, 0x7788 };
static const uint16_t EMR_PACKED[] = { 0x89AB, 0xCDEF, 0x0F1E, 0x2D3C };
static const uint16_t HARD_OUTPUT_EXPECTED[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0001,
};
static const uint16_t SOFT_OUTPUT_EXPECTED[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
};

static void validate_transport(void) {
	test_category("Equalizer / RAM transport");
	test_eq_memory("RX preserves unpacked 16-bit words", RX_UNPACKED,
		dsp_hw_shared_memory + 0x0710, sizeof(RX_UNPACKED));
	test_eq_memory("BPAR has an independent pointer and storage area", BPAR_UNPACKED,
		dsp_hw_shared_memory + 0x0720, sizeof(BPAR_UNPACKED));
	test_eq_memory("SOUT packed reads are ordered and sign-extended", SOUT_PACKED,
		dsp_hw_shared_memory + 0x0730, sizeof(SOUT_PACKED));
	test_eq_memory("HOUT preserves unpacked words", HOUT_UNPACKED,
		dsp_hw_shared_memory + 0x0740, sizeof(HOUT_UNPACKED));
	test_eq_memory("RAMW1 EML packs low and high halfwords", EML_PACKED,
		dsp_hw_shared_memory + 0x0750, sizeof(EML_PACKED));
	test_eq_memory("RAMW2 EMR packs low and high halfwords", EMR_PACKED,
		dsp_hw_shared_memory + 0x0760, sizeof(EMR_PACKED));
	test_eq_memory("RAMW1 EML remains isolated from RAMW2 EMR", EML_PACKED,
		dsp_hw_shared_memory + 0x0770, sizeof(EML_PACKED));
}

static void print_outputs(void) {
	printf("# Equalizer hard output:");
	for (size_t i = 0; i < 16; i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x0790 + i]);
	printf("\n# Equalizer soft output:");
	for (size_t i = 0; i < 16; i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x07A0 + i]);
	printf("\n");
}

static void validate_process(void) {
	uint16_t start_mask = 0x0004 | 0x8000;

	test_category("Equalizer / 32-symbol processing and interrupt");
	test_check("EQ_ON is accepted or EQ_BUSY is already visible", dsp_hw_shared_memory[0x0780] & start_mask);
	test_check("equalizer exposes busy state after start", dsp_hw_shared_memory[0x0781] & 0x8000);
	test_eq_u32("equalizer completion resets the symbol counter", 0, dsp_hw_shared_memory[0x0782]);
	test_eq_u32("EQ completion enters INT0 exactly once", 1, dsp_hw_shared_memory[0x0784]);
	test_eq_u32("INT0 handler observes the EQ source", 0x0200, dsp_hw_shared_memory[0x0785] & 0x0200);
	test_eq_u32("handler acknowledges the EQ source", 0, dsp_hw_shared_memory[0x0783] & 0x0200);
	test_eq_memory("hard outputs match the hardware vector", HARD_OUTPUT_EXPECTED,
		dsp_hw_shared_memory + 0x0790, sizeof(HARD_OUTPUT_EXPECTED));
	test_eq_memory("soft outputs match the hardware vector", SOFT_OUTPUT_EXPECTED,
		dsp_hw_shared_memory + 0x07A0, sizeof(SOFT_OUTPUT_EXPECTED));
	print_outputs();
}

static bool run_pass(void) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load equalizer test",
		dsp_hw_load_image(DSP_EQUALIZER_IMAGE, sizeof(DSP_EQUALIZER_IMAGE))))
		return false;
	if (!test_check("BRANCH starts equalizer test", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("equalizer test becomes ready", dsp_hw_wait_shared(0x0700, READY_MARKER, 100)))
		return false;
	if (!test_check("equalizer scenarios complete", dsp_hw_wait_shared(0x0701, COMPLETE_MARKER, 1000)))
		return false;

	validate_transport();
	validate_process();
	test_eq_u32("RES_ALL clears before the first two-wait-state read completes", 0,
		dsp_hw_shared_memory[0x0786]);
	test_eq_u32("RES_ALL clears CONF2 and disables the accelerator", 0, dsp_hw_shared_memory[0x0787]);
	return true;
}

int main(void) {
	test_start("DSP equalizer functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t pass = 1; pass <= 2; pass++) {
		char category[32];

		tfp_sprintf(category, "Independent reset pass %u", (uint32_t) pass);
		test_category(category);
		if (!run_pass())
			break;
	}

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
