#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

#define FIRMWARE_DELAY_ITERATIONS 2000
#define DSP_RUNTIME_WAIT_TIMEOUT_US 2000
#define DSP_RUNTIME_START_TIMEOUT_US 2000

static const size_t DSP_RUNTIME_PIPE_OFFSETS[] = { 0x0005, 0x0021, 0x003D };

volatile uint16_t *dsp_runtime_pipe_get(size_t index) {
	return dsp_hw_shared_memory + DSP_RUNTIME_PIPE_OFFSETS[index];
}

void dsp_runtime_command_prepare(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count
) {
	volatile uint16_t *pipe = dsp_runtime_pipe_get(pipe_index);

	pipe[0] = command;
	for (size_t i = 0; i < parameter_count; i++)
		pipe[i + 1] = parameters[i];
}

void dsp_runtime_interrupt_pulse(uint16_t flags) {
	SCU_DSP_INT = flags;
	SCU_DSP_INT = 0;
}

void dsp_runtime_command_start(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count
) {
	uint16_t flag = BIT(pipe_index);

	dsp_runtime_command_prepare(pipe_index, command, parameters, parameter_count);
	DSP_COM_SET = flag;
	dsp_runtime_interrupt_pulse(flag);
}

void dsp_runtime_firmware_delay(void) {
	for (size_t i = 0; i < FIRMWARE_DELAY_ITERATIONS; i++)
		__asm__ volatile("" : : : "memory");
}

bool dsp_runtime_wait(uint16_t flags) {
	stopwatch_t start = stopwatch_get();
	while ((DSP_COM_STATUS & flags) != 0) {
		if (stopwatch_elapsed_us(start) >= DSP_RUNTIME_WAIT_TIMEOUT_US)
			return false;
	}
	return true;
}

bool dsp_runtime_startup_probe(uint16_t result_offset) {
	uint16_t parameters[] = { 0xE000, result_offset, 5 };
	volatile uint16_t *pipe = dsp_runtime_pipe_get(0);

	dsp_runtime_command_prepare(0, DSP_COMMAND_READ_DSP, parameters, ARRAY_SIZE(parameters));
	dsp_runtime_firmware_delay();
	DSP_COM_SET = BIT(0);
	SCU_DSP_INT |= BIT(0);
	dsp_runtime_firmware_delay();
	SCU_DSP_INT &= ~BIT(0);

	return (DSP_COM_STATUS & BIT(0)) == 0 && pipe[0] == (uint16_t) -DSP_COMMAND_READ_DSP;
}

bool dsp_runtime_command_execute(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count,
	bool accepted
) {
	uint16_t flag = BIT(pipe_index);

	dsp_runtime_command_start(pipe_index, command, parameters, parameter_count);
	if (!dsp_runtime_wait(flag))
		return false;
	return dsp_runtime_pipe_get(pipe_index)[0] == (accepted ? (uint16_t) -command : 0);
}

bool dsp_runtime_data_read(
	uint16_t address,
	uint16_t result_offset,
	uint16_t *values,
	size_t word_count
) {
	if (word_count > UINT16_MAX)
		return false;
	uint16_t parameters[] = { address, result_offset, (uint16_t) word_count };

	if (!dsp_runtime_command_execute(0, DSP_COMMAND_READ_DSP,
		parameters, ARRAY_SIZE(parameters), true)) {
		return false;
	}
	for (size_t i = 0; i < word_count; i++)
		values[i] = dsp_hw_shared_memory[result_offset + i];
	return true;
}

bool dsp_runtime_data_write(uint16_t source_offset, uint16_t address, size_t word_count) {
	if (word_count > UINT16_MAX)
		return false;
	uint16_t parameters[] = { source_offset, address, (uint16_t) word_count };

	return dsp_runtime_command_execute(0, DSP_COMMAND_WRITE_DSP,
		parameters, ARRAY_SIZE(parameters), true);
}

bool dsp_runtime_start(const struct dsp_user_rom **selected_rom) {
	if (!dsp_user_rom_start(selected_rom))
		return false;

	stopwatch_t start = stopwatch_get();
	while ((CGU_STAT & CGU_STAT_DSP_CLK) != 0) {
		if (stopwatch_elapsed_us(start) >= DSP_RUNTIME_START_TIMEOUT_US)
			return false;
		test_watchdog_serve();
	}
	return true;
}
