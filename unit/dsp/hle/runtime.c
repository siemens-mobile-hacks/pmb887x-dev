#include <pmb887x.h>

#include "../dsp-hw.h"
#include "gsm-l1/el71-v45-dsp-container.inc"
#include "runtime-base-container.inc"
#include "runtime.h"
#include "test.h"

#define DSP_RUNTIME_SHARED_WORDS 0x0600
#define DSP_RUNTIME_COMMAND_READ_DSP 33

static const size_t DSP_RUNTIME_PIPE_OFFSETS[] = { 0x0005, 0x0021, 0x003D };

static const uint16_t BASE_ROUTE_TARGETS[] = {
	0x2BD9, 0x2BE7, 0x2BEF, 0x2CF1, 0x2D07, 0x2D19, DSP_RUNTIME_REJECT_HANDLER, 0x2D28,
	DSP_RUNTIME_REJECT_HANDLER, 0x2ED1, 0x2EF5, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER,
};

static const uint16_t EXTENDED_ROUTE_TARGETS[] = {
	0x2BD9, 0x44FC, 0x4525, 0x460C, 0x46E6, 0x46F5, 0x471C, 0x4728,
	0x4738, 0x4789, 0x4813, 0x00DD, 0x00E9, 0x46A6, 0x4829, 0x4845,
	0x13AE, 0x13C0, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
	DSP_RUNTIME_REJECT_HANDLER, DSP_RUNTIME_REJECT_HANDLER,
};

_Static_assert(ARRAY_SIZE(BASE_ROUTE_TARGETS) ==
	DSP_RUNTIME_ROUTE_LAST_COMMAND - DSP_RUNTIME_ROUTE_FIRST_COMMAND + 1, "base route table size");
_Static_assert(ARRAY_SIZE(EXTENDED_ROUTE_TARGETS) ==
	DSP_RUNTIME_ROUTE_LAST_COMMAND - DSP_RUNTIME_ROUTE_FIRST_COMMAND + 1, "extended route table size");

const struct dsp_runtime_profile DSP_RUNTIME_PROFILES[] = {
	{
		.name = "base startup runtime",
		.container = DSP_RUNTIME_BASE_CONTAINER,
		.container_size = sizeof(DSP_RUNTIME_BASE_CONTAINER),
		.route_targets = BASE_ROUTE_TARGETS,
	},
	{
		.name = "extended startup runtime",
		.container = DSP_EL71_V45_CONTAINER,
		.container_size = sizeof(DSP_EL71_V45_CONTAINER),
		.route_targets = EXTENDED_ROUTE_TARGETS,
		.has_extended_routes = true,
		.supports_a53_cch = true,
	},
};

const size_t DSP_RUNTIME_PROFILE_COUNT = ARRAY_SIZE(DSP_RUNTIME_PROFILES);

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
	SCU_DSP_INT |= flags;
	SCU_DSP_INT &= ~flags;
}

void dsp_runtime_firmware_delay(void) {
	for (size_t i = 0; i < DSP_RUNTIME_FIRMWARE_DELAY_ITERATIONS; i++)
		__asm__ volatile("" : : : "memory");
}

bool dsp_runtime_wait(uint16_t flags, size_t *polls) {
	for (size_t poll = 1; poll <= DSP_RUNTIME_FIRMWARE_POLL_LIMIT; poll++) {
		if ((DSP_COM_STATUS & flags) == 0) {
			if (polls != NULL)
				*polls = poll;
			return true;
		}
	}

	if (polls != NULL)
		*polls = DSP_RUNTIME_FIRMWARE_POLL_LIMIT;
	return false;
}

bool dsp_runtime_wait_stm(uint16_t flags, uint32_t timeout_ticks, uint32_t *elapsed_ticks) {
	uint32_t start = STM_TIM0;

	while ((DSP_COM_STATUS & flags) != 0) {
		uint32_t elapsed = STM_TIM0 - start;

		if (elapsed > timeout_ticks) {
			*elapsed_ticks = elapsed;
			return (DSP_COM_STATUS & flags) == 0;
		}
	}

	*elapsed_ticks = STM_TIM0 - start;
	return true;
}

bool dsp_runtime_startup_probe(uint16_t result_offset) {
	uint16_t parameters[] = { 0xE000, result_offset, 5 };
	volatile uint16_t *pipe = dsp_runtime_pipe_get(0);

	dsp_runtime_command_prepare(0, DSP_RUNTIME_COMMAND_READ_DSP, parameters, ARRAY_SIZE(parameters));
	dsp_runtime_firmware_delay();
	DSP_COM_SET = BIT(0);
	SCU_DSP_INT |= BIT(0);
	dsp_runtime_firmware_delay();
	SCU_DSP_INT &= ~BIT(0);

	return (DSP_COM_STATUS & BIT(0)) == 0 && pipe[0] == (uint16_t) -DSP_RUNTIME_COMMAND_READ_DSP;
}

bool dsp_runtime_command_execute(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count,
	bool accepted
) {
	uint16_t flag = BIT(pipe_index);

	dsp_runtime_command_prepare(pipe_index, command, parameters, parameter_count);
	DSP_COM_SET = flag;
	dsp_runtime_interrupt_pulse(flag);
	if (!dsp_runtime_wait(flag, NULL))
		return false;
	return dsp_runtime_pipe_get(pipe_index)[0] == (accepted ? (uint16_t) -command : 0);
}

bool dsp_runtime_profile_start(const struct dsp_runtime_profile *profile) {
	if (!dsp_hw_reset())
		return false;
	if (dsp_hw_shared_memory[0] != DSP_RUNTIME_MASK_ID)
		return false;

	for (size_t i = 0; i < DSP_RUNTIME_SHARED_WORDS; i++)
		dsp_hw_shared_memory[i] = 0;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!dsp_hw_load_container(profile->container, profile->container_size))
		return false;
	dsp_runtime_firmware_delay();
	return true;
}
