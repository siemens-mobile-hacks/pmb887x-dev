#include <pmb887x.h>

#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

#define MCU_INITIAL_SEMAPHORE_MASK 0xA55A
#define MCU_REMAINING_SEMAPHORE_MASK ((uint16_t) ~MCU_INITIAL_SEMAPHORE_MASK)
#define DSP_SEMAPHORE_RESULT_OFFSET 0x0400
#define DSP_SEMAPHORE_SOURCE_ADDRESS 0xE500

static void test_runtime_semaphores(void) {
	test_category("Selected original User ROM semaphore integration");
	if (!test_check("platform User ROM starts for semaphore integration",
		dsp_runtime_start(NULL))) {
		return;
	}
	test_eq_u32("stock runtime startup preserves the free semaphore state", UINT16_MAX, DSP_SEM_STATUS);

	DSP_SEM_SET = MCU_INITIAL_SEMAPHORE_MASK;
	test_eq_u32("MCU acquires a non-contiguous semaphore set",
		MCU_REMAINING_SEMAPHORE_MASK, DSP_SEM_STATUS);
	DSP_SEM_SET = MCU_INITIAL_SEMAPHORE_MASK;
	test_eq_u32("re-requesting MCU-owned semaphores preserves ownership",
		MCU_REMAINING_SEMAPHORE_MASK, DSP_SEM_STATUS);

	for (size_t pipe_index = 0; pipe_index < DSP_RUNTIME_PIPE_COUNT; pipe_index++) {
		uint16_t parameters[] = {
			DSP_SEMAPHORE_SOURCE_ADDRESS + (uint16_t) (pipe_index * 2),
			DSP_SEMAPHORE_RESULT_OFFSET + (uint16_t) (pipe_index * 2),
			2,
		};
		char name[72];

		sprintf(name, "runtime pipe %lu operates while MCU owns semaphores", (uint32_t) pipe_index);
		test_check(name, dsp_runtime_command_execute(pipe_index, DSP_COMMAND_READ_DSP,
			parameters, ARRAY_SIZE(parameters), true));
	}
	test_eq_u32("stock runtime traffic preserves MCU semaphore ownership",
		MCU_REMAINING_SEMAPHORE_MASK, DSP_SEM_STATUS);

	DSP_SEM_SET = MCU_REMAINING_SEMAPHORE_MASK;
	test_eq_u32("MCU can acquire the remaining semaphore set", 0, DSP_SEM_STATUS);
	DSP_SEM_CLEAR = MCU_INITIAL_SEMAPHORE_MASK;
	test_eq_u32("MCU releases only the selected semaphore set",
		MCU_INITIAL_SEMAPHORE_MASK, DSP_SEM_STATUS);
	DSP_SEM_CLEAR = MCU_INITIAL_SEMAPHORE_MASK;
	test_eq_u32("releasing already-free semaphores preserves remaining ownership",
		MCU_INITIAL_SEMAPHORE_MASK, DSP_SEM_STATUS);
	DSP_SEM_CLEAR = MCU_REMAINING_SEMAPHORE_MASK;
	test_eq_u32("MCU release returns all semaphores to free state", UINT16_MAX, DSP_SEM_STATUS);

	DSP_SEM_SET = UINT16_MAX;
	test_eq_u32("MCU owns all semaphores before runtime reset", 0, DSP_SEM_STATUS);
	if (!test_check("runtime reset succeeds while MCU owns semaphores", dsp_hw_reset()))
		return;
	test_eq_u32("runtime reset preserves MCS semaphore ownership", 0, DSP_SEM_STATUS);
	DSP_SEM_CLEAR = UINT16_MAX;
	test_eq_u32("explicit MCU release frees semaphores after runtime reset", UINT16_MAX, DSP_SEM_STATUS);
}

int main(void) {
	test_start("DSP runtime semaphore integration test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_runtime_semaphores();
	return test_finish();
}
