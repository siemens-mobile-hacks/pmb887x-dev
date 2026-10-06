#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dsp/audio/pcm.h"

enum audio_output {
	AUDIO_OUTPUT_I2S,
	AUDIO_OUTPUT_AFE,
};

const char *audio_board_get_name(void);
bool audio_board_configure_clocks(void);
bool audio_board_has_mask_rom(uint16_t mask_id);
uint32_t audio_output_get_rate_hz(enum audio_output output);
bool audio_output_configure(enum audio_output output);
bool audio_output_play(
	enum audio_output output,
	const struct dsp_audio_pcm *pcm,
	size_t total_frames,
	size_t *frames_played
);
bool audio_output_stop(enum audio_output output);
