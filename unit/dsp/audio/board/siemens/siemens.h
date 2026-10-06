#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dsp/audio/board/siemens/pasic.h"

struct siemens_audio_output {
	uint32_t sample_rate_hz;
	bool (*configure)(void);
	bool (*start)(void);
	bool (*stop)(void);
};

struct siemens_audio_board {
	const char *name;
	const uint16_t *mask_ids;
	size_t mask_id_count;
	uint16_t buffer_word_offset;
	uint16_t block_words;
	uint8_t buffer_channels;
	uint16_t response_flag;
	struct pasic_gain_table speaker_gain;
	struct siemens_audio_output i2s;
	struct siemens_audio_output afe;
};

extern const struct siemens_audio_board SIEMENS_AUDIO_BOARD;
