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

#define DSP_INTERRUPT_REPEAT_NESTED_READY_MARKER 0xA551
#define DSP_INTERRUPT_REPEAT_ABANDON_READY_MARKER 0xA552
#define DSP_INTERRUPT_REPEAT_AFTER_ABANDON_READY_MARKER 0xA553
#define DSP_INTERRUPT_REPEAT_RETURN_TO_BODY_READY_MARKER 0xA554
#define DSP_INTERRUPT_REPEAT_LAST_READY_MARKER 0xA555
#define DSP_INTERRUPT_REPEAT_COMPLETE_MARKER 0x600D
#define DSP_INTERRUPT_REPEAT_READY_OFFSET 0x0400
#define DSP_INTERRUPT_REPEAT_ENTRIES_OFFSET 0x0402
#define DSP_INTERRUPT_REPEAT_NESTED_INTERRUPTED_OFFSET 0x0403
#define DSP_INTERRUPT_REPEAT_NESTED_REPC_BEFORE_OFFSET 0x0404
#define DSP_INTERRUPT_REPEAT_NESTED_COUNT_OFFSET 0x0405
#define DSP_INTERRUPT_REPEAT_NESTED_REPC_AFTER_OFFSET 0x0406
#define DSP_INTERRUPT_REPEAT_NESTED_RESULT_LOW_OFFSET 0x0407
#define DSP_INTERRUPT_REPEAT_NESTED_RESULT_HIGH_OFFSET 0x0408
#define DSP_INTERRUPT_REPEAT_ABANDON_INTERRUPTED_OFFSET 0x0409
#define DSP_INTERRUPT_REPEAT_ABANDON_STACKED_PC_OFFSET 0x040A
#define DSP_INTERRUPT_REPEAT_ABANDON_RETURN_COUNT_OFFSET 0x040B
#define DSP_INTERRUPT_REPEAT_AFTER_ABANDON_INTERRUPTED_OFFSET 0x040C
#define DSP_INTERRUPT_REPEAT_AFTER_ABANDON_RESULT_LOW_OFFSET 0x040D
#define DSP_INTERRUPT_REPEAT_AFTER_ABANDON_RESULT_HIGH_OFFSET 0x040E
#define DSP_INTERRUPT_REPEAT_DIRECT_BODY_COUNT_OFFSET 0x040F
#define DSP_INTERRUPT_REPEAT_RETURN_TO_BODY_COUNT_OFFSET 0x0410
#define DSP_INTERRUPT_REPEAT_LAST_INTERRUPTED_OFFSET 0x0411
#define DSP_INTERRUPT_REPEAT_LAST_RESULT_LOW_OFFSET 0x0412
#define DSP_INTERRUPT_REPEAT_LAST_RESULT_HIGH_OFFSET 0x0413
#define DSP_INTERRUPT_REPEAT_COMPLETE_OFFSET 0x0415
// REP R0 with R0 = 0xFFFE runs 0xFFFF iterations.
#define DSP_INTERRUPT_REPEAT_ITERATIONS 0xFFFF
#define DSP_INTERRUPT_REPEAT_NESTED_ITERATIONS 4
#define DSP_INTERRUPT_REPEAT_ABANDONED_BODY_PC 0x0201

static uint32_t shared_u32(size_t high_offset, size_t low_offset) {
	return ((uint32_t) dsp_hw_shared_memory[high_offset] << 16) | dsp_hw_shared_memory[low_offset];
}

static bool interrupt_when_ready(const char *name, uint16_t ready_marker) {
	if (!test_check(name, dsp_hw_wait_shared(DSP_INTERRUPT_REPEAT_READY_OFFSET, ready_marker, 100)))
		return false;
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;
	return true;
}

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

	if (!interrupt_when_ready("runner enters the first repeat", DSP_INTERRUPT_REPEAT_NESTED_READY_MARKER))
		return test_finish();
	if (!interrupt_when_ready("runner enters the abandoned repeat", DSP_INTERRUPT_REPEAT_ABANDON_READY_MARKER))
		return test_finish();
	if (!interrupt_when_ready("runner enters the repeat after the abandoned one",
		DSP_INTERRUPT_REPEAT_AFTER_ABANDON_READY_MARKER)) {
		return test_finish();
	}
	if (!interrupt_when_ready("runner waits for the return to the abandoned instruction",
		DSP_INTERRUPT_REPEAT_RETURN_TO_BODY_READY_MARKER)) {
		return test_finish();
	}
	if (!interrupt_when_ready("runner enters the last repeat", DSP_INTERRUPT_REPEAT_LAST_READY_MARKER))
		return test_finish();
	bool completed = dsp_hw_wait_shared(DSP_INTERRUPT_REPEAT_COMPLETE_OFFSET,
		DSP_INTERRUPT_REPEAT_COMPLETE_MARKER, 100);

	uint32_t nested_interrupted = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_NESTED_INTERRUPTED_OFFSET];
	uint32_t repc_before = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_NESTED_REPC_BEFORE_OFFSET];
	uint32_t nested_count = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_NESTED_COUNT_OFFSET];
	uint32_t repc_after = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_NESTED_REPC_AFTER_OFFSET];
	uint32_t nested_result = shared_u32(DSP_INTERRUPT_REPEAT_NESTED_RESULT_HIGH_OFFSET,
		DSP_INTERRUPT_REPEAT_NESTED_RESULT_LOW_OFFSET);
	uint32_t abandon_interrupted = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_ABANDON_INTERRUPTED_OFFSET];
	uint32_t stacked_pc = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_ABANDON_STACKED_PC_OFFSET];
	uint32_t return_count = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_ABANDON_RETURN_COUNT_OFFSET];
	uint32_t after_interrupted = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_AFTER_ABANDON_INTERRUPTED_OFFSET];
	uint32_t after_result = shared_u32(DSP_INTERRUPT_REPEAT_AFTER_ABANDON_RESULT_HIGH_OFFSET,
		DSP_INTERRUPT_REPEAT_AFTER_ABANDON_RESULT_LOW_OFFSET);
	uint32_t direct_count = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_DIRECT_BODY_COUNT_OFFSET];
	uint32_t return_to_body_count = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_RETURN_TO_BODY_COUNT_OFFSET];
	uint32_t last_interrupted = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_LAST_INTERRUPTED_OFFSET];
	uint32_t last_result = shared_u32(DSP_INTERRUPT_REPEAT_LAST_RESULT_HIGH_OFFSET,
		DSP_INTERRUPT_REPEAT_LAST_RESULT_LOW_OFFSET);
	uint32_t entries = dsp_hw_shared_memory[DSP_INTERRUPT_REPEAT_ENTRIES_OFFSET];
	printf("# Nested: interrupted after %lu iterations, handler REPC %lu, nested repeat %lu iterations, "
		"REPC after it %lu, outer total %lu\n", nested_interrupted, repc_before, nested_count, repc_after,
		nested_result);
	printf("# Abandoned: interrupted after %lu iterations, stacked PC %04lX, replaced return ran %lu time(s), "
		"next repeat interrupted after %lu, direct branch ran %lu time(s), RETI resumed %lu iterations, "
		"last repeat interrupted after %lu\n", abandon_interrupted, stacked_pc, return_count, after_interrupted,
		direct_count, return_to_body_count, last_interrupted);
	printf("# DSP_INT_REP,nested_interrupted=%lu,repc_before=%lu,nested_count=%lu,repc_after=%lu,"
		"nested_result=%lu\n", nested_interrupted, repc_before, nested_count, repc_after, nested_result);
	printf("# DSP_INT_REP,abandon_interrupted=%lu,stacked_pc=%04lX,return_count=%lu,after_interrupted=%lu,"
		"after_result=%lu,direct_count=%lu,entries=%lu\n", abandon_interrupted, stacked_pc, return_count,
		after_interrupted, after_result, direct_count, entries);
	printf("# DSP_INT_REP,return_to_body_count=%lu,last_interrupted=%lu,last_result=%lu\n", return_to_body_count,
		last_interrupted, last_result);
	if (!test_check("the runner completes all repeats", completed))
		return test_finish();
	test_eq_u32("MCU0 enters the handler once per interrupt window", 5, entries);

	test_category("Nested repeat in the interrupt handler");
	test_check("the handler preempts the repeat before its last iteration",
		nested_interrupted != 0 && nested_interrupted < DSP_INTERRUPT_REPEAT_ITERATIONS);
	test_eq_u32("REPC in the handler holds the remaining count",
		DSP_INTERRUPT_REPEAT_ITERATIONS - 1 - nested_interrupted, repc_before);
	test_eq_u32("the nested repeat runs its own count", DSP_INTERRUPT_REPEAT_NESTED_ITERATIONS, nested_count);
	test_eq_u32("the nested repeat leaves REPC at zero", 0, repc_after);
	test_eq_u32("the interrupted repeat resumes with its remaining count", DSP_INTERRUPT_REPEAT_ITERATIONS,
		nested_result);

	test_category("Interrupt handler that abandons the repeat");
	test_check("the handler preempts the abandoned repeat",
		abandon_interrupted != 0 && abandon_interrupted < DSP_INTERRUPT_REPEAT_ITERATIONS);
	test_eq_u32("the stacked return address is the repeated instruction", DSP_INTERRUPT_REPEAT_ABANDONED_BODY_PC,
		stacked_pc);
	test_eq_u32("the instruction at the replaced return address runs once", 1, return_count);
	test_eq_u32("a later repeat is not preempted while the abandoned one is pending", DSP_INTERRUPT_REPEAT_ITERATIONS,
		after_interrupted);
	test_eq_u32("the later repeat completes its count", DSP_INTERRUPT_REPEAT_ITERATIONS, after_result);
	test_eq_u32("a branch to the abandoned repeated instruction runs it once", 1, direct_count);
	test_eq_u32("RETI to the abandoned repeated instruction resumes its remaining count",
		DSP_INTERRUPT_REPEAT_ITERATIONS - abandon_interrupted, return_to_body_count);
	test_check("a repeat after the resumed one is preempted again",
		last_interrupted != 0 && last_interrupted < DSP_INTERRUPT_REPEAT_ITERATIONS);
	test_eq_u32("the last repeat completes its count", DSP_INTERRUPT_REPEAT_ITERATIONS, last_result);

	return test_finish();
}
