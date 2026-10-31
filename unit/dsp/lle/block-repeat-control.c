#include <pmb887x.h>

#ifdef PMB8875
#include "block-repeat-control-8875.inc"
#define DSP_BLOCK_REPEAT_CONTROL_IMAGE DSP_BLOCK_REPEAT_CONTROL_8875
#endif
#ifdef PMB8876
#include "block-repeat-control-8876.inc"
#define DSP_BLOCK_REPEAT_CONTROL_IMAGE DSP_BLOCK_REPEAT_CONTROL_8876
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x0400
#define RESULT_OFFSET 0x0410
#define RESULT_STRIDE 8

struct block_repeat_scenario {
	const char *name;
	uint16_t inner_count;
	uint16_t outer_count;
	bool reads_lc;
	uint16_t lc_read;
	uint16_t lc;
	uint16_t icr;
};

static const struct block_repeat_scenario SCENARIOS[] = {
	{ "ICR LP write at the first body word aborts a single loop", 0x0001, 0x0001, false, 0x0000, 0x0004, 0xFF00 },
	{ "ICR LP write three words before the end aborts a single loop", 0x0001, 0x0001, false, 0x0000, 0x0004, 0xFF00 },
	{ "ICR LP write in the inner loop aborts both levels", 0x0001, 0x0001, false, 0x0000, 0x0002, 0xFF00 },
	{ "ICR LP write in the innermost of four levels aborts all levels", 0x0001, 0x0001, false, 0x0000, 0x0001, 0xFF00 },
	{ "ICR write without LP leaves the loop running", 0x0005, 0x0005, false, 0x0000, 0x0000, 0xFF0E },
	{ "branch out and back to the last instruction keeps looping", 0x0004, 0x0004, false, 0x0000, 0x0000, 0xFF00 },
	{ "branch to the last instruction continues the loop", 0x0004, 0x0000, false, 0x0000, 0x0000, 0xFF00 },
	{ "branch out of the body without break leaves the loop active", 0x0001, 0x0000, false, 0x0000, 0x0003, 0xFF30 },
	{ "lc reads the loop counter inside the body and after completion", 0x0004, 0x0000, true, 0x0000, 0x0000, 0xFF00 },
	{ "break at the outer level keeps lc", 0x0002, 0x0002, true, 0x0004, 0x0004, 0xFF00 },
	{ "lc reads the outer counter after the inner loop completes", 0x000C, 0x0003, true, 0x0000, 0x0000, 0xFF00 },
	{ "rep inside a block repeat body runs every repetition", 0x000C, 0x0004, false, 0x0000, 0x0000, 0xFF00 },
};

int main(void) {
	test_start("DSP block repeat control test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = COMPLETE_OFFSET; offset < RESULT_OFFSET + RESULT_STRIDE * ARRAY_SIZE(SCENARIOS); offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_BLOCK_REPEAT_CONTROL_IMAGE, sizeof(DSP_BLOCK_REPEAT_CONTROL_IMAGE));
	if (!test_check("boot commands load the block repeat control runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the block repeat control runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	bool completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 100);

	for (size_t i = 0; i < ARRAY_SIZE(SCENARIOS); i++) {
		const struct block_repeat_scenario *scenario = &SCENARIOS[i];
		volatile uint16_t *result = &dsp_hw_shared_memory[RESULT_OFFSET + RESULT_STRIDE * i];
		printf("# DSP_BLOCK_REPEAT_CONTROL,%lu,a1=%04lX,a0=%04lX,r1=%04lX,lc=%04lX,icr=%04lX\n", (uint32_t) i,
			(uint32_t) result[0], (uint32_t) result[1], (uint32_t) result[2], (uint32_t) result[3],
			(uint32_t) result[4]);
		test_category(scenario->name);
		test_eq_u32("inner counter", scenario->inner_count, result[0]);
		test_eq_u32("outer counter", scenario->outer_count, result[1]);
		if (scenario->reads_lc)
			test_eq_u32("lc read inside the scenario", scenario->lc_read, result[2]);
		test_eq_u32("lc after the scenario", scenario->lc, result[3]);
		test_eq_u32("icr after the scenario", scenario->icr, result[4]);
	}
	test_check("runner completes every scenario", completed);
	return test_finish();
}
