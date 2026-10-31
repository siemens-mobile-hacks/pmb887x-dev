#include <pmb887x.h>

#ifdef PMB8875
#include "block-repeat-limits-8875.inc"
#define DSP_BLOCK_REPEAT_LIMITS_IMAGE DSP_BLOCK_REPEAT_LIMITS_8875
#endif
#ifdef PMB8876
#include "block-repeat-limits-8876.inc"
#define DSP_BLOCK_REPEAT_LIMITS_IMAGE DSP_BLOCK_REPEAT_LIMITS_8876
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x0400
#define PROGRESS_OFFSET 0x0401
#define RESULT_OFFSET 0x0410
#define RESULT_STRIDE 8

struct block_repeat_limit {
	const char *name;
	uint16_t inner_count;
	uint16_t outer_count;
	uint16_t lc;
	uint16_t icr;
};

static const struct block_repeat_limit SCENARIOS[] = {
	{ "a fifth nested block repeat level", 0x0014, 0x0002, 0x0000, 0xFF00 },
	{ "two block repeat levels share the last address", 0x0003, 0x0003, 0x0001, 0xFF30 },
	{ "break as the last instruction of the body", 0x0002, 0x0001, 0x0002, 0xFF00 },
	{ "a one-word block repeat body", 0x0003, 0x0001, 0x0000, 0xFF00 },
	{ "a block repeat whose last address precedes it", 0x0001, 0x0001, 0x0003, 0xFF30 },
};

int main(void) {
	test_start("DSP block repeat limits test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = COMPLETE_OFFSET; offset < RESULT_OFFSET + RESULT_STRIDE * ARRAY_SIZE(SCENARIOS); offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_BLOCK_REPEAT_LIMITS_IMAGE, sizeof(DSP_BLOCK_REPEAT_LIMITS_IMAGE));
	if (!test_check("boot commands load the block repeat limits runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the block repeat limits runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	bool completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 100);

	for (size_t i = 0; i < ARRAY_SIZE(SCENARIOS); i++) {
		const struct block_repeat_limit *scenario = &SCENARIOS[i];
		volatile uint16_t *result = &dsp_hw_shared_memory[RESULT_OFFSET + RESULT_STRIDE * i];
		printf("# DSP_BLOCK_REPEAT_LIMITS,%lu,a1=%04lX,a0=%04lX,lc=%04lX,icr=%04lX\n", (uint32_t) i,
			(uint32_t) result[0], (uint32_t) result[1], (uint32_t) result[2], (uint32_t) result[3]);
		test_category(scenario->name);
		test_eq_u32("inner counter", scenario->inner_count, result[0]);
		test_eq_u32("outer counter", scenario->outer_count, result[1]);
		test_eq_u32("lc after the scenario", scenario->lc, result[2]);
		test_eq_u32("icr after the scenario", scenario->icr, result[3]);
	}
	printf("# DSP_BLOCK_REPEAT_LIMITS,last_started=%lu\n", (uint32_t) dsp_hw_shared_memory[PROGRESS_OFFSET]);
	test_check("runner completes every scenario", completed);
	return test_finish();
}
