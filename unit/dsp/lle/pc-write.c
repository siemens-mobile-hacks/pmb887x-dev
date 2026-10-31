#include <pmb887x.h>

#ifdef PMB8875
#include "pc-write-8875.inc"
#define DSP_PC_WRITE_IMAGE DSP_PC_WRITE_8875
#endif
#ifdef PMB8876
#include "pc-write-8876.inc"
#define DSP_PC_WRITE_IMAGE DSP_PC_WRITE_8876
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x0400
#define RESULT_OFFSET 0x0410
#define RESULT_WORDS 4

struct pc_write {
	const char *name;
	uint16_t executed_words;
	int16_t sp_change;
};

static const struct pc_write WRITES[] = {
	{ "ret", 0, 0 },
	{ "mov r0 pc", 2, 0 },
	{ "mov a0l pc", 2, 0 },
	{ "mov long immediate pc", 1, 0 },
	{ "pop pc", 2, 0 },
	{ "mov mixp pc", 2, 0 },
	{ "mov indirect r0 pc", 2, 0 },
	{ "movp indirect a0l pc", 2, 0 },
	{ "mov stack top pc", 2, -1 },
};

int main(void) {
	test_start("DSP program counter write test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = COMPLETE_OFFSET; offset < RESULT_OFFSET + RESULT_WORDS * ARRAY_SIZE(WRITES); offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_PC_WRITE_IMAGE, sizeof(DSP_PC_WRITE_IMAGE));
	if (!test_check("boot commands load the program counter write runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the program counter write runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	bool completed = dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 100);

	for (size_t i = 0; i < ARRAY_SIZE(WRITES); i++) {
		const struct pc_write *write = &WRITES[i];
		volatile uint16_t *result = &dsp_hw_shared_memory[RESULT_OFFSET + RESULT_WORDS * i];
		printf("# DSP_PC_WRITE,%lu,%s,executed=%04lX,sp=%04lX,fallthrough=%04lX,sp_before=%04lX\n", (uint32_t) i,
			write->name, (uint32_t) result[0], (uint32_t) result[1], (uint32_t) result[2], (uint32_t) result[3]);
		test_category(write->name);
		test_eq_u32("following words executed before the jump", write->executed_words, result[0]);
		test_eq_u32("execution does not fall through past the following words", 0, result[2]);
		test_eq_u32("stack pointer after the jump", (uint16_t) (result[3] + write->sp_change), result[1]);
	}
	test_check("runner completes every program counter write", completed);
	return test_finish();
}
