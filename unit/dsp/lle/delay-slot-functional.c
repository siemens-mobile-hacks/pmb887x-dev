#include <pmb887x.h>
#include <gen/dsp.h>

#ifdef PMB8875
#include "delay-slot-functional-8875.inc"
#define DSP_DELAY_SLOT_IMAGE DSP_DELAY_SLOT_IMAGE_8875
#endif
#ifdef PMB8876
#include "delay-slot-functional-8876.inc"
#define DSP_DELAY_SLOT_IMAGE DSP_DELAY_SLOT_IMAGE_8876
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x0300
#define RESULT_LOW_OFFSET 0x0301
#define RESULT_HIGH_OFFSET 0x0302
#define RESULT_STATUS_OFFSET 0x0303
#define SLOTS_COMPLETE_OFFSET 0x0400
#define SLOTS_RESULT_OFFSET 0x0410
#define SLOTS_RESULT_STRIDE 4
#define SLOTS_POINTER (TEAK_SHARED_RAM_BASE + 0x0480)

struct slot_scenario {
	const char *name;
	uint16_t a1;
	uint16_t a0;
	uint16_t r1;
};

static const struct slot_scenario SLOT_SCENARIOS[] = {
	{ "retd with two one-cycle slots", 0x0003, 0x0000, SLOTS_POINTER },
	{ "retd with a long immediate add slot", 0x0006, 0x0000, SLOTS_POINTER },
	{ "retd with a long immediate mov slot", 0x0001, 0x0000, 0x1234 },
	{ "retd with a norm slot", 0x0001, 0x0000, SLOTS_POINTER },
	{ "retid with two one-cycle slots", 0x0003, 0x0000, SLOTS_POINTER },
	{ "retid with a long immediate add slot", 0x0006, 0x0000, SLOTS_POINTER },
};

int main(void) {
	test_start("DSP delayed return functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = COMPLETE_OFFSET; offset <= RESULT_STATUS_OFFSET; offset++)
		dsp_hw_shared_memory[offset] = 0;
	size_t slots_end = SLOTS_RESULT_OFFSET + SLOTS_RESULT_STRIDE * ARRAY_SIZE(SLOT_SCENARIOS);
	for (size_t offset = SLOTS_COMPLETE_OFFSET; offset < slots_end; offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_DELAY_SLOT_IMAGE, sizeof(DSP_DELAY_SLOT_IMAGE));
	if (!test_check("boot commands load delayed-return test", loaded))
		return test_finish();
	if (!test_check("BRANCH starts delayed-return test", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();

	bool completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 100);
	printf("# DSP_DELAY_SLOT,low=%04lX,high=%04lX,status=%04lX\n",
		(uint32_t) dsp_hw_shared_memory[RESULT_LOW_OFFSET],
		(uint32_t) dsp_hw_shared_memory[RESULT_HIGH_OFFSET],
		(uint32_t) dsp_hw_shared_memory[RESULT_STATUS_OFFSET]);
	if (test_check("RETD executes SQR and PACR delay slots before returning", completed)) {
		test_eq_u32("SQR and PACR produce the hardware accumulator low word", 0xDA90,
			dsp_hw_shared_memory[RESULT_LOW_OFFSET]);
		test_eq_u32("SQR and PACR produce the hardware accumulator high word", 0x014B,
			dsp_hw_shared_memory[RESULT_HIGH_OFFSET]);
		test_eq_u32("SQR and PACR produce the hardware status flags", 0x0001,
			dsp_hw_shared_memory[RESULT_STATUS_OFFSET]);
	}

	bool slots_completed = dsp_hw_wait_shared(SLOTS_COMPLETE_OFFSET, COMPLETE_MARKER, 100);
	for (size_t i = 0; i < ARRAY_SIZE(SLOT_SCENARIOS); i++) {
		const struct slot_scenario *scenario = &SLOT_SCENARIOS[i];
		volatile uint16_t *result = &dsp_hw_shared_memory[SLOTS_RESULT_OFFSET + SLOTS_RESULT_STRIDE * i];
		printf("# DSP_DELAY_SLOT_CONTENT,%lu,a1=%04lX,a0=%04lX,r1=%04lX,sp=%04lX\n", (uint32_t) i,
			(uint32_t) result[0], (uint32_t) result[1], (uint32_t) result[2], (uint32_t) result[3]);
		test_category(scenario->name);
		test_eq_u32("a1 after the delay slots", scenario->a1, result[0]);
		test_eq_u32("words after the delay slots do not execute", scenario->a0, result[1]);
		test_eq_u32("r1 after the delay slots", scenario->r1, result[2]);
	}
	printf("# DSP_DELAY_SLOT_CONTENT,last_started=%lu\n", (uint32_t) dsp_hw_shared_memory[SLOTS_COMPLETE_OFFSET + 1]);
	test_check("delay-slot content runner completes every scenario", slots_completed);

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
