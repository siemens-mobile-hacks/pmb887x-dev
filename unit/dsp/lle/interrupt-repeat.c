#include <pmb887x.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#ifdef PMB8875
#include "interrupt-repeat-8875.inc"
#define DSP_INTERRUPT_REPEAT_IMAGE DSP_INTERRUPT_REPEAT_8875
#else
#include "interrupt-repeat-8876.inc"
#define DSP_INTERRUPT_REPEAT_IMAGE DSP_INTERRUPT_REPEAT_8876
#endif

#define DSP_INTERRUPT_REPEAT_READY_MARKER 0xA55A
#define DSP_INTERRUPT_REPEAT_COMPLETE_MARKER 0x600D
#define DSP_INTERRUPT_REPEAT_READY_OFFSET 0x0400
#define DSP_INTERRUPT_REPEAT_INTERRUPTED_OFFSET 0x0401
#define DSP_INTERRUPT_REPEAT_ENTRIES_OFFSET 0x0402
#define DSP_INTERRUPT_REPEAT_RESULT_LOW_OFFSET 0x0403
#define DSP_INTERRUPT_REPEAT_RESULT_HIGH_OFFSET 0x0404
#define DSP_INTERRUPT_REPEAT_COMPLETE_OFFSET 0x0405
// REP R0 with R0 = 0xFFFE runs 0xFFFF iterations.
#define DSP_INTERRUPT_REPEAT_ITERATIONS 0xFFFF

int main(void) {
	test_start("DSP interrupt repeat test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	SCU_DSP_INT = 0;
	for (size_t offset = DSP_INTERRUPT_REPEAT_READY_OFFSET; offset <= DSP_INTERRUPT_REPEAT_COMPLETE_OFFSET; offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_INTERRUPT_REPEAT_IMAGE, sizeof(DSP_INTERRUPT_REPEAT_IMAGE));
	if (!test_check("boot commands load the interrupt repeat runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the interrupt repeat runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();

	test_category("MCU0 interrupt during a single-instruction repeat");
	if (!test_check("runner enters the repeat", dsp_hw_wait_shared(DSP_INTERRUPT_REPEAT_READY_OFFSET,
		DSP_INTERRUPT_REPEAT_READY_MARKER, 100))) {
		return test_finish();
	}
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;

	bool completed = dsp_hw_wait_shared(DSP_INTERRUPT_REPEAT_COMPLETE_OFFSET,
		DSP_INTERRUPT_REPEAT_COMPLETE_MARKER, 100);
	uint32_t interrupted = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_INTERRUPTED_OFFSET];
	uint32_t result = ((uint32_t) dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_RESULT_HIGH_OFFSET] << 16) |
		dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_RESULT_LOW_OFFSET];
	printf("# DSP_INT_REP,interrupted=%lu,entries=%lu,result=%lu\n", interrupted,
		(uint32_t) dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_ENTRIES_OFFSET], result);
	if (!test_check("repeat completes after the handler returns", completed))
		return test_finish();
	test_eq_u32("MCU0 enters the handler once", 1, dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_ENTRIES_OFFSET]);
	test_check("the handler preempts the repeat before its last iteration",
		interrupted != 0 && interrupted < DSP_INTERRUPT_REPEAT_ITERATIONS);
	test_eq_u32("the interrupted repeat resumes with its remaining count", DSP_INTERRUPT_REPEAT_ITERATIONS, result);

	return test_finish();
}
