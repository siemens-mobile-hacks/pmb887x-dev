#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "user-rom.h"

#define DSP_RUNTIME_PIPE_COUNT 3
// SL98 uses this fixed delay after BRANCH and while asserting the startup-probe interrupt.
#define DSP_RUNTIME_FIRMWARE_DELAY_ITERATIONS 2000
// SL98 dsp_submit_command() permits this many DSP_COM_STATUS reads before asserting.
#define DSP_RUNTIME_FIRMWARE_POLL_LIMIT 1000
// EL71 runtime command wrappers pass this raw STM tick budget to their wait functions.
#define DSP_RUNTIME_FIRMWARE_STM_TIMEOUT_TICKS 100
// Firmware configures the STM for 1 MHz before using that runtime-command timeout.
#define DSP_RUNTIME_FIRMWARE_STM_CLC 0x001A1A00
#define DSP_RUNTIME_FIRMWARE_STM_HZ 1000000
// PMB7870 DSP firmware specification: a runtime command is accepted within 70 us.
#define DSP_RUNTIME_DATASHEET_ACCEPT_US 70
struct dsp_runtime_profile {
	const char *name;
	uint16_t mask_id;
	enum dsp_user_rom_platform platform;
	bool accepts_i2s_swap;
	bool supports_fc_notifications;
	bool uses_stm_command_timeout;
	bool supports_a53_cch;
};

volatile uint16_t *dsp_runtime_pipe_get(size_t index);
void dsp_runtime_command_prepare(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count
);
void dsp_runtime_interrupt_pulse(uint16_t flags);
void dsp_runtime_firmware_delay(void);
bool dsp_runtime_wait(uint16_t flags, size_t *polls);
bool dsp_runtime_wait_stm(uint16_t flags, uint32_t timeout_ticks, uint32_t *elapsed_ticks);
bool dsp_runtime_startup_probe(uint16_t result_offset);
bool dsp_runtime_command_execute(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count,
	bool accepted
);
bool dsp_runtime_profile_start(const struct dsp_runtime_profile **selected_profile);
