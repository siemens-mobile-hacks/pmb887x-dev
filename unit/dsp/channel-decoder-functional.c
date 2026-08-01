#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A

#ifdef PMB8875
#include "channel-decoder-functional-8875.inc"
#define DSP_CHANNEL_DECODER_IMAGE DSP_CHANNEL_DECODER_IMAGE_8875
#else
#include "channel-decoder-functional-8876.inc"
#define DSP_CHANNEL_DECODER_IMAGE DSP_CHANNEL_DECODER_IMAGE_8876
#endif

static const uint16_t SIN01_UNPACKED[] = { 0x0000, 0x0001, 0x7FFF, 0x8000, 0xA55A, 0x5AA5 };
static const uint16_t SIN2_UNPACKED[] = { 0x1357, 0x2468, 0x369C, 0x47AD };
static const uint16_t SIN01_PACKED[] = { 0x0000, 0x007F, 0xFF80, 0xFFFF, 0x0055, 0xFFAA };
static const uint16_t RAMW1_PACKED[] = { 0x1122, 0x3344, 0x5566, 0x7788 };
static const uint16_t RAMW2_PACKED[] = { 0x89AB, 0xCDEF, 0x0F1E, 0x2D3C };
static const uint16_t RAMW1_UNPACKED[] = { 0x1357, 0x8ACE, 0x2468, 0x9BDF };
static const uint16_t BUTTERFLY_REFERENCES[] = {
	0x6CA0, 0x0AC6, 0, 0, 0, 0, 0, 0,
};
static const uint16_t TRACEBACK_EXPECTED[] = {
	0x005A, 0x00A5, 0x0019, 0x0067, 0x0027, 0x001B, 0x003D, 0x0043,
	0x00E5, 0x0058, 0x001B, 0x0027, 0x002C, 0x00CB, 0x0059, 0x0065,
	0x00C3, 0x003C, 0x0075, 0x0051, 0x00A8, 0x00EA, 0x00EA, 0x00A8,
	0x001A, 0x00A7, 0x0065, 0x0059, 0x006A, 0x00A9,
};

static void validate_ram2(void) {
	test_category("Channel decoder / RAM2 transport");
	test_eq_memory("SIN01 preserves unpacked 16-bit words", SIN01_UNPACKED,
		dsp_hw_shared_memory + 0x0610, sizeof(SIN01_UNPACKED));
	test_eq_memory("SIN2 has an independent pointer and storage area", SIN2_UNPACKED,
		dsp_hw_shared_memory + 0x0620, sizeof(SIN2_UNPACKED));
	test_eq_memory("packed 8-bit reads are ordered and sign-extended", SIN01_PACKED,
		dsp_hw_shared_memory + 0x0630, sizeof(SIN01_PACKED));
}

static void validate_working_ram(void) {
	test_category("Channel decoder / Working RAM transport");
	test_eq_memory("RAMW1 packs low and high halfwords in order", RAMW1_PACKED,
		dsp_hw_shared_memory + 0x0640, sizeof(RAMW1_PACKED));
	test_eq_memory("RAMW2 packs low and high halfwords in order", RAMW2_PACKED,
		dsp_hw_shared_memory + 0x0650, sizeof(RAMW2_PACKED));
	test_eq_memory("RAMW1 and RAMW2 remain isolated", RAMW1_PACKED,
		dsp_hw_shared_memory + 0x0660, sizeof(RAMW1_PACKED));
	test_eq_memory("RAMW1 preserves unpacked 16-bit words", RAMW1_UNPACKED,
		dsp_hw_shared_memory + 0x0672, sizeof(RAMW1_UNPACKED));
}

static void validate_reset(void) {
	test_category("Channel decoder / Internal reset");
	test_eq_u32("RES_ALL clears before the first two-wait-state read completes", 0,
		dsp_hw_shared_memory[0x0670]);
	test_eq_u32("RES_ALL clears CONF2 and disables the accelerator", 0, dsp_hw_shared_memory[0x0671]);
}

static void validate_decode(void) {
	test_category("Channel decoder / 16-state processing and interrupt");
	test_check("DEC_ON is accepted or DEC_BUSY is already visible",
		(dsp_hw_shared_memory[0x0680] & (0x0004 | 0x4000)) != 0);
	test_check("decoder exposes busy state after start", dsp_hw_shared_memory[0x0681] & 0x4000);
	test_eq_u32("decoder processes every configured timestamp", 15, dsp_hw_shared_memory[0x0682]);
	test_eq_u32("CHADEC completion enters INT0 exactly once", 1, dsp_hw_shared_memory[0x0684]);
	test_eq_u32("INT0 handler observes the CHADEC source", 0x0100, dsp_hw_shared_memory[0x0685] & 0x0100);
	test_eq_u32("handler acknowledges the CHADEC source", 0, dsp_hw_shared_memory[0x0683] & 0x0100);
	test_eq_memory("all eight butterfly-reference registers retain their values", BUTTERFLY_REFERENCES,
		dsp_hw_shared_memory + 0x0686, sizeof(BUTTERFLY_REFERENCES));
	test_eq_u32("traceback RAM accepts ext0 writes", 0xA55A, dsp_hw_shared_memory[0x068F]);
	test_eq_memory("16-state decode matches the hardware traceback vector", TRACEBACK_EXPECTED,
		dsp_hw_shared_memory + 0x0690, sizeof(TRACEBACK_EXPECTED));
	printf("# Channel decoder traceback:");
	for (size_t i = 0; i < ARRAY_SIZE(TRACEBACK_EXPECTED); i++)
		printf(" %04X", (uint32_t) dsp_hw_shared_memory[0x0690 + i]);
	printf("\n");
}

static bool run_pass(size_t pass) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = 0x0600; offset <= 0x0675; offset++)
		dsp_hw_shared_memory[offset] = 0xDEAD;
	if (!test_check("boot commands load channel-decoder test",
		dsp_hw_load_image(DSP_CHANNEL_DECODER_IMAGE, sizeof(DSP_CHANNEL_DECODER_IMAGE))))
		return false;
	if (!test_check("BRANCH starts channel-decoder test", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("channel-decoder test becomes ready", dsp_hw_wait_shared(0x0600, READY_MARKER, 100)))
		return false;
	if (!test_check("channel-decoder memory scenarios complete",
		dsp_hw_wait_shared(0x0601, COMPLETE_MARKER, 1000)))
		return false;

	validate_ram2();
	validate_working_ram();
	validate_decode();
	validate_reset();
	test_eq_u32("test reaches final memory phase", 1, dsp_hw_shared_memory[0x0602]);
	printf("# Channel decoder ext0 pass %u complete\n", (uint32_t) pass);
	return true;
}

int main(void) {
	test_start("DSP channel decoder functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t pass = 1; pass <= 2; pass++) {
		char category[32];

		tfp_sprintf(category, "Independent reset pass %u", (uint32_t) pass);
		test_category(category);
		if (!run_pass(pass))
			break;
	}

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
