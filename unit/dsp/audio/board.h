#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dsp/audio/pcm.h"

bool dsp_audio_mask_is_supported(const uint16_t *mask_ids, size_t mask_id_count, uint16_t mask_id);
const char *dsp_audio_board_get_name(void);
uint32_t dsp_audio_board_get_sample_rate_hz(void);
bool dsp_audio_board_clocks_configure(void);
bool dsp_audio_board_supports_mask_rom(uint16_t mask_id);
bool dsp_audio_board_play(const struct dsp_audio_pcm *pcm, size_t total_frames, size_t *frames_played);
bool dsp_audio_board_stop(void);
bool dsp_audio_board_output_is_muted(void);
