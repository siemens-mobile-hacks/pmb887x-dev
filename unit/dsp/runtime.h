#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dsp/hle/user-rom.h"

#define DSP_RUNTIME_PIPE_COUNT 3

enum dsp_runtime_command_id {
	DSP_COMMAND_FC_INIT = 1,
	DSP_COMMAND_MODU_INIT = 2,
	DSP_COMMAND_IQ_SWAP_1 = 3,
	DSP_COMMAND_IQ_SWAP_2 = 4,
	DSP_COMMAND_DEC_INIT = 5,
	DSP_COMMAND_BB_OFF = 13,
	DSP_COMMAND_IDLE = 14,
	DSP_COMMAND_VM_CMD = 25,
	DSP_COMMAND_WRITE_DSP = 32,
	DSP_COMMAND_READ_DSP = 33,
	DSP_COMMAND_WRITE_PROG = 34,
	DSP_COMMAND_READ_PROG = 35,
	DSP_COMMAND_MCU_INT = 36,
	DSP_COMMAND_VB_I2SY = 37,
	DSP_COMMAND_UMTS_ON = 40,
	DSP_COMMAND_MP3 = 41,
	DSP_COMMAND_SYNTH = 42,
	DSP_COMMAND_RF_ADAPT = 43,
	DSP_COMMAND_AUDIOPOSTPROC = 44,
	DSP_COMMAND_PCMPLAY = 45,
	DSP_COMMAND_I2S_SWAP = 48,
};

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
void dsp_runtime_command_start(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count
);
void dsp_runtime_interrupt_pulse(uint16_t flags);
void dsp_runtime_firmware_delay(void);
bool dsp_runtime_wait(uint16_t flags, size_t *polls);
bool dsp_runtime_startup_probe(uint16_t result_offset);
bool dsp_runtime_command_execute(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count,
	bool accepted
);
bool dsp_runtime_data_read(
	uint16_t address,
	uint16_t result_offset,
	uint16_t *values,
	size_t word_count
);
bool dsp_runtime_data_write(uint16_t source_offset, uint16_t address, size_t word_count);
bool dsp_runtime_profile_start(const struct dsp_runtime_profile **selected_profile);
