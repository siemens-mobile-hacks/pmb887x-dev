#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

#define DSP_RUNTIME_RESULT_OFFSET 0x0200
#define DSP_RUNTIME_SIMULTANEOUS_RESULT_OFFSET 0x0300
#define DSP_RUNTIME_PENDING_OBSERVATION_US 100
#define DSP_RUNTIME_TIMING_SAMPLES 64
#define DSP_RUNTIME_PIPE_PARAMETERS 27
#define DSP_RUNTIME_PRIORITY_WAIT_ITERATIONS 1000000
#define DSP_RUNTIME_FIXTURE_SHARED_OFFSET 0x0400
#define DSP_RUNTIME_FIXTURE_PROGRAM_ADDRESS 0x1F80
#define DSP_RUNTIME_FIXTURE_WORDS (DSP_RUNTIME_PIPE_COUNT * 2)
#define DSP_RUNTIME_PRIORITY_PROGRAM_ADDRESS 0x2000
#define DSP_RUNTIME_FIXTURE_BASE 0xA500

static uint16_t fixture_word(size_t index) {
	return DSP_RUNTIME_FIXTURE_BASE ^ index;
}

static stopwatch_t microseconds_to_ticks(uint32_t microseconds) {
	return (stopwatch_t) stopwatch_ticks_per_s() * microseconds / 1000000;
}

static uint32_t ticks_to_nanoseconds(stopwatch_t ticks) {
	return (uint32_t) (ticks * 1000000000ULL / stopwatch_ticks_per_s());
}

static bool wait_for_command(
	uint16_t flags,
	stopwatch_t start,
	uint32_t timeout_us,
	stopwatch_t *elapsed_ticks
) {
	stopwatch_t timeout_ticks = microseconds_to_ticks(timeout_us);

	while ((DSP_COM_STATUS & flags) != 0) {
		if (stopwatch_elapsed(start) > timeout_ticks) {
			*elapsed_ticks = stopwatch_elapsed(start);
			return false;
		}
	}

	*elapsed_ticks = stopwatch_elapsed(start);
	return *elapsed_ticks <= timeout_ticks;
}

static bool install_program_fixture(void) {
	static const uint16_t PARAMETERS[] = {
		DSP_RUNTIME_FIXTURE_SHARED_OFFSET,
		DSP_RUNTIME_FIXTURE_PROGRAM_ADDRESS,
		DSP_RUNTIME_FIXTURE_WORDS,
	};

	for (size_t i = 0; i < DSP_RUNTIME_FIXTURE_WORDS; i++)
		dsp_hw_shared_memory[DSP_RUNTIME_FIXTURE_SHARED_OFFSET + i] = fixture_word(i);
	return dsp_runtime_command_execute(0, DSP_COMMAND_WRITE_PROG, PARAMETERS, ARRAY_SIZE(PARAMETERS), true);
}

static void test_interrupt_gating(void) {
	static const uint16_t PARAMETERS[] = {
		DSP_RUNTIME_FIXTURE_PROGRAM_ADDRESS, DSP_RUNTIME_RESULT_OFFSET, 2,
	};
	volatile uint16_t *pipe = dsp_runtime_pipe_get(0);

	dsp_runtime_command_prepare(0, DSP_COMMAND_READ_PROG, PARAMETERS, ARRAY_SIZE(PARAMETERS));
	DSP_COM_SET = BIT(0);
	stopwatch_usleep_wd(DSP_RUNTIME_PENDING_OBSERVATION_US);
	test_eq_u32("runtime command remains pending without an MCU interrupt", BIT(0), DSP_COM_STATUS & BIT(0));
	test_eq_u32("pending runtime command is not acknowledged", DSP_COMMAND_READ_PROG, pipe[0]);

	dsp_runtime_interrupt_pulse(BIT(1));
	stopwatch_usleep_wd(DSP_RUNTIME_PENDING_OBSERVATION_US);
	test_eq_u32("wrong MCU interrupt leaves runtime pipe zero pending", BIT(0), DSP_COM_STATUS & BIT(0));
	test_eq_u32("wrong MCU interrupt does not acknowledge runtime pipe zero", DSP_COMMAND_READ_PROG, pipe[0]);

	dsp_runtime_interrupt_pulse(BIT(0));
	if (!test_check("matching MCU interrupt completes runtime pipe zero", dsp_runtime_wait(BIT(0))))
		return;
	test_eq_u32("accepted runtime command is acknowledged with its negated ID",
		(uint16_t) -DSP_COMMAND_READ_PROG, pipe[0]);
	test_eq_u32("READ_PROG first result word is complete when the flag clears", fixture_word(0),
		dsp_hw_shared_memory[DSP_RUNTIME_RESULT_OFFSET]);
	test_eq_u32("READ_PROG second result word is complete when the flag clears", fixture_word(1),
		dsp_hw_shared_memory[DSP_RUNTIME_RESULT_OFFSET + 1]);
}

static void test_individual_pipes(void) {
	for (size_t i = 0; i < DSP_RUNTIME_PIPE_COUNT; i++) {
		uint16_t parameters[DSP_RUNTIME_PIPE_PARAMETERS];
		char name[72];

		for (size_t parameter = 0; parameter < ARRAY_SIZE(parameters); parameter++)
			parameters[parameter] = 0xA500 + parameter;
		parameters[0] = DSP_RUNTIME_FIXTURE_PROGRAM_ADDRESS + i * 2;
		parameters[1] = DSP_RUNTIME_RESULT_OFFSET + (uint16_t) (i * 2);
		parameters[2] = 2;

		sprintf(name, "runtime pipe %lu accepts an independently signalled command", (uint32_t) i);
		if (!test_check(name, dsp_runtime_command_execute(i, DSP_COMMAND_READ_PROG, parameters,
			ARRAY_SIZE(parameters), true))) {
			continue;
		}

		size_t result = DSP_RUNTIME_RESULT_OFFSET + i * 2;
		sprintf(name, "runtime pipe %lu publishes its complete result at ACK", (uint32_t) i);
		test_check(name, dsp_hw_shared_memory[result] == fixture_word(i * 2) &&
			dsp_hw_shared_memory[result + 1] == fixture_word(i * 2 + 1));

		bool unused_parameters_unchanged = true;
		volatile uint16_t *pipe = dsp_runtime_pipe_get(i);
		for (size_t parameter = 3; parameter < ARRAY_SIZE(parameters); parameter++)
			unused_parameters_unchanged &= pipe[parameter + 1] == parameters[parameter];
		sprintf(name, "runtime pipe %lu preserves unused parameter words", (uint32_t) i);
		test_check(name, unused_parameters_unchanged);
	}
}

static void test_selective_interrupts(void) {
	uint16_t flags = 0;

	for (size_t i = 0; i < DSP_RUNTIME_PIPE_COUNT; i++) {
		uint16_t parameters[] = {
			DSP_RUNTIME_FIXTURE_PROGRAM_ADDRESS + i * 2,
			DSP_RUNTIME_SIMULTANEOUS_RESULT_OFFSET + (uint16_t) (i * 2),
			2,
		};

		dsp_runtime_command_prepare(i, DSP_COMMAND_READ_PROG, parameters, ARRAY_SIZE(parameters));
		flags |= BIT(i);
	}

	DSP_COM_SET = flags;
	dsp_runtime_interrupt_pulse(BIT(1));
	if (!test_check("pipe one interrupt acknowledges pipe one", dsp_runtime_wait(BIT(1))))
		return;
	stopwatch_usleep_wd(DSP_RUNTIME_PENDING_OBSERVATION_US);
	test_eq_u32("pipe zero and two remain pending after pipe one interrupt",
		BIT(0) | BIT(2), DSP_COM_STATUS & flags);
	test_eq_u32("pipe zero command remains unacknowledged", DSP_COMMAND_READ_PROG, dsp_runtime_pipe_get(0)[0]);
	test_eq_u32("pipe two command remains unacknowledged", DSP_COMMAND_READ_PROG, dsp_runtime_pipe_get(2)[0]);

	dsp_runtime_interrupt_pulse(BIT(0) | BIT(2));
	if (!test_check("remaining interrupts acknowledge pipe zero and two",
		dsp_runtime_wait(BIT(0) | BIT(2)))) {
		return;
	}

	bool results_complete = true;
	for (size_t i = 0; i < DSP_RUNTIME_PIPE_COUNT; i++) {
		size_t result = DSP_RUNTIME_SIMULTANEOUS_RESULT_OFFSET + i * 2;

		results_complete &= dsp_hw_shared_memory[result] == fixture_word(i * 2);
		results_complete &= dsp_hw_shared_memory[result + 1] == fixture_word(i * 2 + 1);
	}
	test_check("selectively acknowledged pipes publish complete results", results_complete);
}

static void test_simultaneous_pipes(void) {
	uint16_t flags = 0;

	for (size_t i = 0; i < DSP_RUNTIME_PIPE_COUNT; i++) {
		uint16_t parameters[] = {
			DSP_RUNTIME_FIXTURE_PROGRAM_ADDRESS + i * 2,
			DSP_RUNTIME_SIMULTANEOUS_RESULT_OFFSET + (uint16_t) (i * 2),
			2,
		};

		dsp_runtime_command_prepare(i, DSP_COMMAND_READ_PROG, parameters, ARRAY_SIZE(parameters));
		flags |= BIT(i);
	}

	DSP_COM_SET = flags;
	dsp_runtime_interrupt_pulse(flags);
	if (!test_check("one MCU pulse services all three pending runtime pipes", dsp_runtime_wait(flags)))
		return;

	for (size_t i = 0; i < DSP_RUNTIME_PIPE_COUNT; i++) {
		char name[72];
		size_t result = DSP_RUNTIME_SIMULTANEOUS_RESULT_OFFSET + i * 2;

		sprintf(name, "runtime pipe %lu acknowledges simultaneous READ_PROG", (uint32_t) i);
		test_eq_u32(name, (uint16_t) -DSP_COMMAND_READ_PROG, dsp_runtime_pipe_get(i)[0]);
		sprintf(name, "runtime pipe %lu publishes the simultaneous result", (uint32_t) i);
		test_check(name, dsp_hw_shared_memory[result] == fixture_word(i * 2) &&
			dsp_hw_shared_memory[result + 1] == fixture_word(i * 2 + 1));
	}
}

static void test_simultaneous_pipe_priority(void) {
	static const uint16_t EXPECTED_STATUS[] = {
		BIT(0) | BIT(1) | BIT(2),
		BIT(1) | BIT(2),
		BIT(2),
		0,
	};
	static const uint16_t PARAMETERS[] = {
		DSP_RUNTIME_PRIORITY_PROGRAM_ADDRESS, DSP_RUNTIME_RESULT_OFFSET, DSP_HW_BOOT_TRANSFER_WORDS,
	};
	bool ordered = true;

	for (size_t sample = 0; sample < DSP_RUNTIME_TIMING_SAMPLES; sample++) {
		for (size_t pipe_index = 0; pipe_index < DSP_RUNTIME_PIPE_COUNT; pipe_index++)
			dsp_runtime_command_prepare(pipe_index, DSP_COMMAND_READ_PROG, PARAMETERS, ARRAY_SIZE(PARAMETERS));

		DSP_COM_SET = EXPECTED_STATUS[0];
		dsp_runtime_interrupt_pulse(EXPECTED_STATUS[0]);
		size_t status_index = 0;
		for (size_t poll = 0; poll < DSP_RUNTIME_PRIORITY_WAIT_ITERATIONS; poll++) {
			if (status_index + 1 == ARRAY_SIZE(EXPECTED_STATUS))
				break;
			uint16_t status = DSP_COM_STATUS & EXPECTED_STATUS[0];

			if (status == EXPECTED_STATUS[status_index])
				continue;
			if (status != EXPECTED_STATUS[status_index + 1]) {
				ordered = false;
				break;
			}
			status_index++;
		}

		ordered &= status_index + 1 == ARRAY_SIZE(EXPECTED_STATUS);
		for (size_t pipe_index = 0; pipe_index < DSP_RUNTIME_PIPE_COUNT; pipe_index++)
			ordered &= dsp_runtime_pipe_get(pipe_index)[0] == (uint16_t) -DSP_COMMAND_READ_PROG;
		if (!ordered)
			break;
	}

	test_check("simultaneous runtime commands are accepted in pipe zero-to-two priority order", ordered);
}

static void test_rejected_commands(void) {
	static const uint16_t REJECTED_COMMANDS[] = { 0, 68 };
	uint16_t parameters[DSP_RUNTIME_PIPE_PARAMETERS];

	for (size_t parameter = 0; parameter < ARRAY_SIZE(parameters); parameter++)
		parameters[parameter] = 0x5A00 + parameter;

	for (size_t pipe_index = 0; pipe_index < DSP_RUNTIME_PIPE_COUNT; pipe_index++) {
		for (size_t command_index = 0; command_index < ARRAY_SIZE(REJECTED_COMMANDS); command_index++) {
			char name[72];
			uint16_t command = REJECTED_COMMANDS[command_index];

			sprintf(name, "runtime pipe %lu rejects command %lu", (uint32_t) pipe_index, (uint32_t) command);
			test_check(name, dsp_runtime_command_execute(pipe_index, command, parameters,
				ARRAY_SIZE(parameters), false));

			bool parameters_unchanged = true;
			volatile uint16_t *pipe = dsp_runtime_pipe_get(pipe_index);
			for (size_t parameter = 0; parameter < ARRAY_SIZE(parameters); parameter++)
				parameters_unchanged &= pipe[parameter + 1] == parameters[parameter];
			sprintf(name, "rejected command %lu preserves pipe %lu parameters",
				(uint32_t) command, (uint32_t) pipe_index);
			test_check(name, parameters_unchanged);
		}
	}
}

static void characterize_accept_latency(void) {
	for (size_t pipe_index = 0; pipe_index < DSP_RUNTIME_PIPE_COUNT; pipe_index++) {
		uint32_t minimum = UINT32_MAX;
		uint32_t maximum = 0;
		uint64_t total = 0;
		uint32_t samples = 0;
		bool success = true;

		for (size_t sample = 0; sample < DSP_RUNTIME_TIMING_SAMPLES; sample++) {
			uint16_t parameters[] = {
				DSP_RUNTIME_FIXTURE_PROGRAM_ADDRESS,
				DSP_RUNTIME_RESULT_OFFSET,
				2,
			};
			uint16_t flag = BIT(pipe_index);

			dsp_runtime_command_prepare(pipe_index, DSP_COMMAND_READ_PROG, parameters, ARRAY_SIZE(parameters));
			DSP_COM_SET = flag;
			stopwatch_t start = stopwatch_get();
			dsp_runtime_interrupt_pulse(flag);
			bool command_complete = dsp_runtime_wait(flag);
			uint32_t elapsed = (uint32_t) stopwatch_elapsed(start);
			command_complete &= dsp_runtime_pipe_get(pipe_index)[0] == (uint16_t) -DSP_COMMAND_READ_PROG;
			success &= command_complete;

			if (elapsed < minimum)
				minimum = elapsed;
			if (elapsed > maximum)
				maximum = elapsed;
			total += elapsed;
			samples++;
			if (!command_complete)
				break;
		}

		char name[72];
		sprintf(name, "runtime pipe %lu completes every timing sample", (uint32_t) pipe_index);
		test_check(name, success);
		sprintf(name, "runtime pipe %lu completes within firmware's 100 us timeout", (uint32_t) pipe_index);
		test_check(name, success && ticks_to_nanoseconds(maximum) <= 100000);
		printf("# runtime pipe %lu IRQ-to-ACK: samples=%lu min=%lu ns mean=%lu ns max=%lu ns\n",
			(uint32_t) pipe_index, samples, ticks_to_nanoseconds(minimum),
			ticks_to_nanoseconds(total / samples), ticks_to_nanoseconds(maximum));
	}
}

static void test_timed_command_acceptance(void) {
	static const struct {
		uint16_t command;
		uint16_t parameters[11];
		size_t parameter_count;
		const char *name;
	} CASES[] = {
		{ 6, { 1, 0 }, 2, "cipher-mode command on runtime pipe zero" },
		{ 36, { 0, 0, 0 }, 3, "PDCH interrupt command on runtime pipe one" },
		{ 6, { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 11,
			"cipher-key command on runtime pipe two" },
	};

	for (size_t pipe_index = 0; pipe_index < ARRAY_SIZE(CASES); pipe_index++) {
		stopwatch_t maximum_ticks = 0;
		uint32_t samples = 0;
		bool success = true;

		for (size_t sample = 0; sample < DSP_RUNTIME_TIMING_SAMPLES; sample++) {
			uint16_t flag = BIT(pipe_index);

			dsp_runtime_command_prepare(pipe_index, CASES[pipe_index].command, CASES[pipe_index].parameters,
				CASES[pipe_index].parameter_count);
			DSP_COM_SET = flag;
			stopwatch_t start = stopwatch_get();
			dsp_runtime_interrupt_pulse(flag);
			stopwatch_t elapsed_ticks;
			bool command_complete = wait_for_command(flag, start, 100, &elapsed_ticks);
			command_complete &= dsp_runtime_pipe_get(pipe_index)[0] == (uint16_t) -CASES[pipe_index].command;
			success &= command_complete;
			samples++;
			if (elapsed_ticks > maximum_ticks)
				maximum_ticks = elapsed_ticks;
			if (!command_complete)
				break;
		}

		char name[104];
		sprintf(name, "%s meets the 70-us acceptance deadline", CASES[pipe_index].name);
		test_check(name, success && maximum_ticks <= microseconds_to_ticks(70));
		printf("# %s: samples=%lu max=%lu ns\n", CASES[pipe_index].name, samples,
			ticks_to_nanoseconds(maximum_ticks));
	}
}

static void test_runtime(void) {
	const struct dsp_user_rom *user_rom;

	test_category("Selected original User ROM transport");
	if (!test_check("boot protocol selects and starts the platform User ROM",
		dsp_runtime_start(&user_rom))) {
		return;
	}
	if (!test_check("runtime installs a controlled Program RAM transport fixture", install_program_fixture()))
		return;

	test_interrupt_gating();
	test_individual_pipes();
	test_selective_interrupts();
	test_simultaneous_pipes();
	test_simultaneous_pipe_priority();
	test_rejected_commands();
	characterize_accept_latency();
	if (user_rom->uses_stm_command_timeout)
		test_timed_command_acceptance();
}

int main(void) {
	test_start("DSP startup runtime transport test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_runtime();
	return test_finish();
}
