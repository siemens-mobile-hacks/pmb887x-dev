#include <pmb887x.h>

#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

#define DSP_RUNTIME_RESULT_OFFSET 0x0200
static void test_runtime_probe(void) {
	static const uint16_t PARAMETERS[] = { 0xE000, DSP_RUNTIME_RESULT_OFFSET, 5 };

	for (size_t i = 0; i < PARAMETERS[2]; i++)
		dsp_hw_shared_memory[DSP_RUNTIME_RESULT_OFFSET + i] = 0xA500 + i;
	if (!test_check("READ_DSP startup probe completes during firmware's 2000-iteration IRQ window",
		dsp_runtime_startup_probe(DSP_RUNTIME_RESULT_OFFSET))) {
		return;
	}

	bool destination_replaced = true;
	for (size_t i = 0; i < PARAMETERS[2]; i++)
		destination_replaced &= dsp_hw_shared_memory[DSP_RUNTIME_RESULT_OFFSET + i] != 0xA500 + i;
	test_check("READ_DSP startup probe replaces the complete destination", destination_replaced);
}

static void test_profile_specific_dispatch(const struct dsp_runtime_profile *profile) {
	static const uint16_t PARAMETERS[] = { 0 };

	test_check("runtime command 48 has the platform-specific acceptance behavior",
		dsp_runtime_command_execute(0, DSP_COMMAND_I2S_SWAP, PARAMETERS, ARRAY_SIZE(PARAMETERS),
			profile->accepts_i2s_swap));
}

static void test_runtime_profile(void) {
	const struct dsp_runtime_profile *profile;

	test_category("Selected original User ROM firmware behavior");
	if (!test_check("boot protocol selects and starts the platform User ROM",
		dsp_runtime_profile_start(&profile))) {
		return;
	}
	printf("# DSP runtime profile: %s\n", profile->name);

	test_runtime_probe();
	test_profile_specific_dispatch(profile);
}

int main(void) {
	test_start("DSP startup runtime HLE test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_runtime_profile();
	return test_finish();
}
