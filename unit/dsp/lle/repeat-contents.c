#include <pmb887x.h>

#ifdef PMB8875
#include "repeat-contents-8875.inc"
#define DSP_REPEAT_CONTENTS_IMAGE DSP_REPEAT_CONTENTS_8875
#endif
#ifdef PMB8876
#include "repeat-contents-8876.inc"
#define DSP_REPEAT_CONTENTS_IMAGE DSP_REPEAT_CONTENTS_8876
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x0400
#define RESULT_OFFSET 0x0410
#define RESULT_STRIDE 8

struct repeat_scenario {
	const char *name;
	uint16_t observed;
};

static const struct repeat_scenario SCENARIOS[] = {
	{ "rep push stores four words that pop returns", 0x1234 },
	{ "rep pop loads the first pushed word last", 0x1111 },
	{ "rep movp rereads the same program word", 0x1111 },
	{ "rep movd copies four data words to program memory", 0x5A04 },
	{ "rep mov register to icr", 0xFF0E },
	{ "rep dint keeps interrupts disabled", 0x0A01 },
	{ "rep break leaves the block repeat after one iteration", 0x0001 },
};

int main(void) {
	test_start("DSP repeat contents test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = COMPLETE_OFFSET; offset < RESULT_OFFSET + RESULT_STRIDE * ARRAY_SIZE(SCENARIOS); offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_REPEAT_CONTENTS_IMAGE, sizeof(DSP_REPEAT_CONTENTS_IMAGE));
	if (!test_check("boot commands load the repeat contents runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the repeat contents runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	bool completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 100);

	for (size_t i = 0; i < ARRAY_SIZE(SCENARIOS); i++) {
		const struct repeat_scenario *scenario = &SCENARIOS[i];
		volatile uint16_t *result = &dsp_hw_shared_memory[RESULT_OFFSET + RESULT_STRIDE * i];
		printf("# DSP_REPEAT_CONTENTS,%lu,a1=%04lX,a0=%04lX,r1=%04lX,sp=%04lX,sp_before=%04lX\n", (uint32_t) i,
			(uint32_t) result[0], (uint32_t) result[1], (uint32_t) result[2], (uint32_t) result[3],
			(uint32_t) result[4]);
		test_category(scenario->name);
		test_eq_u32("observed register value", scenario->observed, result[2]);
		test_eq_u32("stack pointer returns to its starting value", result[4], result[3]);
	}
	test_check("runner completes every scenario", completed);
	return test_finish();
}
