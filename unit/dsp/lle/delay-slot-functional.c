#include <pmb887x.h>

#include "delay-slot-functional-8876.inc"
#include "dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x0300
#define RESULT_LOW_OFFSET 0x0301
#define RESULT_HIGH_OFFSET 0x0302
#define RESULT_STATUS_OFFSET 0x0303

int main(void) {
	test_start("DSP delayed return functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = COMPLETE_OFFSET; offset <= RESULT_STATUS_OFFSET; offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_DELAY_SLOT_IMAGE_8876, sizeof(DSP_DELAY_SLOT_IMAGE_8876));
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

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
