#include <pmb887x.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#ifdef PMB8875
#include "interrupt-block-repeat-8875.inc"
#define DSP_INTERRUPT_BLOCK_REPEAT_IMAGE DSP_INTERRUPT_BLOCK_REPEAT_8875
#else
#include "interrupt-block-repeat-8876.inc"
#define DSP_INTERRUPT_BLOCK_REPEAT_IMAGE DSP_INTERRUPT_BLOCK_REPEAT_8876
#endif

#define DSP_INTERRUPT_BLOCK_REPEAT_READY_MARKER 0xA55A
#define DSP_INTERRUPT_BLOCK_REPEAT_COMPLETE_MARKER 0x600D
#define DSP_INTERRUPT_BLOCK_REPEAT_READY_OFFSET 0x0400
#define DSP_INTERRUPT_BLOCK_REPEAT_MAINLINE_ITERATIONS_OFFSET 0x0401
#define DSP_INTERRUPT_BLOCK_REPEAT_HANDLER_ITERATIONS_OFFSET 0x0402
#define DSP_INTERRUPT_BLOCK_REPEAT_HANDLER_ENTRIES_OFFSET 0x0403
#define DSP_INTERRUPT_BLOCK_REPEAT_COMPLETE_OFFSET 0x0404
// BKREP Rn runs Rn + 1 iterations: the runner loads R5 = 15 and R7 = 4.
#define DSP_INTERRUPT_BLOCK_REPEAT_MAINLINE_ITERATIONS 16
#define DSP_INTERRUPT_BLOCK_REPEAT_HANDLER_ITERATIONS 5

int main(void) {
	test_start("DSP interrupt block repeat test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = DSP_INTERRUPT_BLOCK_REPEAT_READY_OFFSET;
		offset <= DSP_INTERRUPT_BLOCK_REPEAT_COMPLETE_OFFSET; offset++) {
		dsp_hw_shared_memory[offset] = 0;
	}
	bool loaded = dsp_hw_load_image(DSP_INTERRUPT_BLOCK_REPEAT_IMAGE, sizeof(DSP_INTERRUPT_BLOCK_REPEAT_IMAGE));
	if (!test_check("boot commands load the interrupt block repeat runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the interrupt block repeat runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();

	test_category("Block repeat in an interrupt that preempts a block repeat");
	if (!test_check("runner becomes ready", dsp_hw_wait_shared(DSP_INTERRUPT_BLOCK_REPEAT_READY_OFFSET,
		DSP_INTERRUPT_BLOCK_REPEAT_READY_MARKER, 100))) {
		return test_finish();
	}
	bool completed = dsp_hw_wait_shared(DSP_INTERRUPT_BLOCK_REPEAT_COMPLETE_OFFSET,
		DSP_INTERRUPT_BLOCK_REPEAT_COMPLETE_MARKER, 100);
	printf("# DSP_INT_BKREP,mainline=%lu,handler=%lu,entries=%lu\n",
		(uint32_t) dsp_hw_shared_memory[DSP_INTERRUPT_BLOCK_REPEAT_MAINLINE_ITERATIONS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_INTERRUPT_BLOCK_REPEAT_HANDLER_ITERATIONS_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_INTERRUPT_BLOCK_REPEAT_HANDLER_ENTRIES_OFFSET]);
	if (test_check("mainline block repeat completes after handler block repeats", completed)) {
		test_eq_u32("each mainline iteration raises and completes one handler",
			DSP_INTERRUPT_BLOCK_REPEAT_MAINLINE_ITERATIONS,
			dsp_hw_shared_memory[DSP_INTERRUPT_BLOCK_REPEAT_HANDLER_ENTRIES_OFFSET]);
		test_eq_u32("handler block repeat ending after the interrupted loop runs every iteration",
			DSP_INTERRUPT_BLOCK_REPEAT_MAINLINE_ITERATIONS * DSP_INTERRUPT_BLOCK_REPEAT_HANDLER_ITERATIONS,
			dsp_hw_shared_memory[DSP_INTERRUPT_BLOCK_REPEAT_HANDLER_ITERATIONS_OFFSET]);
		test_eq_u32("interrupted block repeat resumes with its own count",
			DSP_INTERRUPT_BLOCK_REPEAT_MAINLINE_ITERATIONS,
			dsp_hw_shared_memory[DSP_INTERRUPT_BLOCK_REPEAT_MAINLINE_ITERATIONS_OFFSET]);
	}

	return test_finish();
}
