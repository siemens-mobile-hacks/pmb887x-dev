#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum siemens_audio_command_id {
	SIEMENS_AUDIO_COMMAND_DCONF_AUX = 25,
	SIEMENS_AUDIO_COMMAND_DCONF_2S = 37,
	SIEMENS_AUDIO_COMMAND_DSET_VOL = 40,
	SIEMENS_AUDIO_COMMAND_DSET_SCAL = 41,
	SIEMENS_AUDIO_COMMAND_DSET_MIC_MODE = 42,
	SIEMENS_AUDIO_COMMAND_DCONF_JTAB = 43,
	SIEMENS_AUDIO_COMMAND_DCONF_LIB = 44,
	SIEMENS_AUDIO_COMMAND_DCONF_PCMA = 45,
	SIEMENS_AUDIO_COMMAND_USER_15 = 49,
	SIEMENS_AUDIO_COMMAND_USER_14 = 50,
};

struct dconf_jtab_record {
	const uint16_t *words;
	size_t word_count;
};

#define DCONF_JTAB_RECORD(...) { \
	.words = (const uint16_t[]) { __VA_ARGS__ }, \
	.word_count = sizeof((const uint16_t[]) { __VA_ARGS__ }) / sizeof(uint16_t), \
}

bool dsp_siemens_cmd_exec(
	size_t pipe_index,
	uint16_t command,
	const uint16_t *parameters,
	size_t parameter_count
);
bool dsp_siemens_jtab_exec(
	size_t pipe_index,
	const struct dconf_jtab_record *records,
	size_t record_count
);
