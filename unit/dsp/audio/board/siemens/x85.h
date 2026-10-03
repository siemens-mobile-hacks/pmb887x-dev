#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct siemens_x85_audio_route {
	const uint16_t *mask_ids;
	size_t mask_id_count;
	uint32_t source_sample_rate_hz;
	uint8_t i2s_max_large_steps;
	uint16_t buffer_offset;
	uint16_t block_words;
	uint8_t buffer_channels;
	uint16_t response_flag;
	bool (*configure)(void);
	bool (*start)(void);
	bool (*stop)(void);
};

struct siemens_x85_audio_board {
	const char *name;
	const struct siemens_x85_audio_route *route;
};

extern const struct siemens_x85_audio_board siemens_x85_audio_board;
