#include <pmb887x.h>

#include "../dsp-hw.h"
#include "runtime.h"
#include "test.h"

#define DSP_RUNTIME_RESULT_OFFSET 0x0200
#define DSP_RUNTIME_ROUTE_WORDS \
	((DSP_RUNTIME_ROUTE_LAST_COMMAND - DSP_RUNTIME_ROUTE_FIRST_COMMAND + 1) * 2)
#define DSP_COMMAND_READ_PROG 35
#define DSP_COMMAND_I2S_SWAP 48

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

static void test_runtime_routes(const struct dsp_runtime_profile *profile) {
	static const uint16_t PARAMETERS[] = {
		DSP_RUNTIME_ROUTE_PROGRAM_ADDRESS, DSP_RUNTIME_RESULT_OFFSET, DSP_RUNTIME_ROUTE_WORDS,
	};

	if (!test_check("READ_PROG exposes the installed runtime route table",
		dsp_runtime_command_execute(0, DSP_COMMAND_READ_PROG, PARAMETERS, ARRAY_SIZE(PARAMETERS), true))) {
		return;
	}

	for (size_t i = 0; i < DSP_RUNTIME_ROUTE_WORDS / 2; i++) {
		char opcode_name[64];
		char target_name[64];
		uint32_t command = DSP_RUNTIME_ROUTE_FIRST_COMMAND + i;

		sprintf(opcode_name, "runtime command %lu trampoline is an unconditional branch", command);
		test_eq_u32(opcode_name, 0x4180, dsp_hw_shared_memory[DSP_RUNTIME_RESULT_OFFSET + i * 2]);
		sprintf(target_name, "runtime command %lu has the expected route target", command);
		test_eq_u32(target_name, profile->route_targets[i],
			dsp_hw_shared_memory[DSP_RUNTIME_RESULT_OFFSET + i * 2 + 1]);
	}
}

static void test_profile_specific_dispatch(const struct dsp_runtime_profile *profile) {
	static const uint16_t PARAMETERS[] = { 0 };

	test_check("runtime command 48 follows the installed accept/reject route",
		dsp_runtime_command_execute(0, DSP_COMMAND_I2S_SWAP, PARAMETERS, ARRAY_SIZE(PARAMETERS),
			profile->has_extended_routes));
}

static void test_runtime_profile(const struct dsp_runtime_profile *profile) {
	char category[72];

	sprintf(category, "%s firmware behavior", profile->name);
	test_category(category);
	if (!test_check("boot protocol loads and starts the complete runtime container",
		dsp_runtime_profile_start(profile))) {
		return;
	}

	test_runtime_probe();
	test_runtime_routes(profile);
	test_profile_specific_dispatch(profile);
}

int main(void) {
	test_start("DSP startup runtime HLE test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t i = 0; i < DSP_RUNTIME_PROFILE_COUNT; i++)
		test_runtime_profile(&DSP_RUNTIME_PROFILES[i]);

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
