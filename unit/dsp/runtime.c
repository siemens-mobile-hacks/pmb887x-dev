#include <pmb887x.h>

#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

#define FIRMWARE_DELAY_ITERATIONS 2000
#define FIRMWARE_POLL_LIMIT 1000

static const size_t DSP_RUNTIME_PIPE_OFFSETS[] = { 0x0005, 0x0021, 0x003D };

static const struct dsp_runtime_profile DSP_RUNTIME_PROFILES[] = {
	{
		.name = "APOXI 0603 runtime",
		.mask_id = 0x0603,
		.platform = DSP_USER_ROM_PLATFORM_APOXI,
	},
	{
		.name = "APOXI 0604 runtime",
		.mask_id = 0x0604,
		.platform = DSP_USER_ROM_PLATFORM_APOXI,
	},
	{
		.name = "APOXI 0800 runtime",
		.mask_id = 0x0800,
		.platform = DSP_USER_ROM_PLATFORM_APOXI,
	},
	{
		.name = "APOXI 0801 runtime",
		.mask_id = 0x0801,
		.platform = DSP_USER_ROM_PLATFORM_APOXI,
	},
	{
		.name = "Siemens 0602 runtime",
		.mask_id = 0x0602,
		.platform = DSP_USER_ROM_PLATFORM_SIEMENS,
	},
	{
		.name = "Siemens 0605 runtime",
		.mask_id = 0x0605,
		.platform = DSP_USER_ROM_PLATFORM_SIEMENS,
	},
	{
		.name = "Siemens 0800 runtime",
		.mask_id = 0x0800,
		.platform = DSP_USER_ROM_PLATFORM_SIEMENS,
		.accepts_i2s_swap = true,
	},
	{
		.name = "Siemens 0801 runtime",
		.mask_id = 0x0801,
		.platform = DSP_USER_ROM_PLATFORM_SIEMENS,
		.accepts_i2s_swap = true,
		.supports_fc_notifications = true,
		.uses_stm_command_timeout = true,
		.supports_a53_cch = true,
	},
};

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

bool dsp_runtime_wait(uint16_t flags, size_t *polls) {
	for (size_t poll = 1; poll <= FIRMWARE_POLL_LIMIT; poll++) {
		if ((DSP_COM_STATUS & flags) == 0) {
			if (polls != NULL)
				*polls = poll;
			return true;
		}
	}

	if (polls != NULL)
		*polls = FIRMWARE_POLL_LIMIT;
	return false;
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
	if (!dsp_runtime_wait(flag, NULL))
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

static const struct dsp_runtime_profile *dsp_runtime_profile_find(const struct dsp_user_rom *user_rom) {
	for (size_t i = 0; i < ARRAY_SIZE(DSP_RUNTIME_PROFILES); i++) {
		if (DSP_RUNTIME_PROFILES[i].mask_id == user_rom->mask_id &&
			DSP_RUNTIME_PROFILES[i].platform == user_rom->platform) {
			return &DSP_RUNTIME_PROFILES[i];
		}
	}

	return NULL;
}

bool dsp_runtime_profile_start(const struct dsp_runtime_profile **selected_profile) {
	const struct dsp_user_rom *user_rom;

	if (!dsp_user_rom_start(&user_rom))
		return false;
	const struct dsp_runtime_profile *profile = dsp_runtime_profile_find(user_rom);
	if (profile == NULL) {
		printf("# No runtime ABI profile for %s\n", user_rom->name);
		return false;
	}

	dsp_runtime_firmware_delay();
	*selected_profile = profile;
	return true;
}
