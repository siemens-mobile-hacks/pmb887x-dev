#include <pmb887x.h>

#ifdef PMB8875
#include "self-modifying-8875.inc"
#define DSP_SELF_MODIFYING_IMAGE DSP_SELF_MODIFYING_8875
#endif
#ifdef PMB8876
#include "self-modifying-8876.inc"
#define DSP_SELF_MODIFYING_IMAGE DSP_SELF_MODIFYING_8876
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x0400
#define RESULT_OFFSET 0x0410
#define RESULT_STRIDE 4

struct self_modifying_scenario {
	const char *name;
	uint16_t original_code_count;
	uint16_t program_word;
};

static const struct self_modifying_scenario SCENARIOS[] = {
	{ "movd rewrites the word 1 after itself", 0x0004, 0x67D0 },
	{ "movd rewrites the word 2 after itself", 0x0003, 0x67D0 },
	{ "movd rewrites the word 3 after itself", 0x0003, 0x67D0 },
	{ "movd rewrites the word 4 after itself", 0x0003, 0x67D0 },
	{ "movd rewrites an executed subroutine twice", 0x0000, 0x77E0 },
	{ "movd rewrites the active block repeat body", 0x0001, 0x67D0 },
	{ "movd rewrites a calla target between calls", 0x0001, 0x67D0 },
};

int main(void) {
	test_start("DSP self-modifying program test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = COMPLETE_OFFSET; offset < RESULT_OFFSET + RESULT_STRIDE * ARRAY_SIZE(SCENARIOS); offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_SELF_MODIFYING_IMAGE, sizeof(DSP_SELF_MODIFYING_IMAGE));
	if (!test_check("boot commands load the self-modifying runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the self-modifying runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	bool completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 100);

	for (size_t i = 0; i < ARRAY_SIZE(SCENARIOS); i++) {
		const struct self_modifying_scenario *scenario = &SCENARIOS[i];
		volatile uint16_t *result = &dsp_hw_shared_memory[RESULT_OFFSET + RESULT_STRIDE * i];
		printf("# DSP_SELF_MODIFYING,%lu,a1=%04lX,a0=%04lX,word=%04lX\n", (uint32_t) i, (uint32_t) result[0],
			(uint32_t) result[1], (uint32_t) result[2]);
		test_category(scenario->name);
		test_eq_u32("a1 counts executions of the original code", scenario->original_code_count, result[0]);
		test_eq_u32("program memory holds the last movd word", scenario->program_word, result[2]);
	}
	test_check("runner completes every scenario", completed);
	return test_finish();
}
