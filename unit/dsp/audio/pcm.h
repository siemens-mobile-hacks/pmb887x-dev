#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct dsp_audio_pcm {
	const int16_t *samples;
	size_t sample_count;
	uint32_t sample_rate_hz;
	uint32_t expected_tone_hz;
};

struct pcm_shared_ring {
	volatile uint16_t *buffer;
	size_t buffer_word_count;
	size_t channel_count;
	size_t source_frame_step;
	uint16_t response_flag;
	uint32_t request_timeout_ms;
	size_t output_probe_block_count;
	bool (*start)(void);
	bool (*output_validate)(const struct dsp_audio_pcm *pcm);
};

void pcm_triangle_generate(
	int16_t *samples,
	size_t sample_count,
	uint32_t sample_rate_hz,
	uint32_t tone_hz,
	int32_t amplitude
);
void pcm_buffer_fill(
	volatile uint16_t *buffer,
	size_t buffer_word_count,
	const struct dsp_audio_pcm *pcm,
	size_t first_frame,
	size_t frame_count,
	size_t source_frame_step,
	size_t channel_count
);
bool pcm_shared_ring_play(
	const struct pcm_shared_ring *ring,
	const struct dsp_audio_pcm *pcm,
	size_t total_frames
);
