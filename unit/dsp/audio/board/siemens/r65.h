#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct siemens_r65_audio_board {
	const char *name;
	const uint16_t *mask_ids;
	size_t mask_id_count;
	uint32_t sample_rate_hz;
	uint16_t buffer_address;
	uint16_t block_samples;
	uint16_t progress_address;
	bool (*prepare)(void);
	bool (*start)(void);
	bool (*stop)(void);
	bool (*output_configure)(void);
	bool (*mute)(void);
	uint8_t supply_mode_mask;
	uint8_t supply_1_mask;
	uint8_t supply_2_mask;
	uint32_t supply_settle_ms;
};

extern const struct siemens_r65_audio_board siemens_r65_audio_board;
