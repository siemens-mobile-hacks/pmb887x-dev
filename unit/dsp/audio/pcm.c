#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/audio/pcm.h"
#include "test.h"

void pcm_triangle_generate(
	int16_t *samples,
	size_t sample_count,
	uint32_t sample_rate_hz,
	uint32_t tone_hz,
	int32_t amplitude
) {
	uint32_t phase_period = sample_rate_hz * 4;

	for (size_t i = 0; i < sample_count; i++) {
		uint32_t phase = (uint64_t) i * tone_hz * 4 % phase_period;
		int32_t sample;

		if (phase < sample_rate_hz) {
			sample = (int64_t) amplitude * phase / sample_rate_hz;
		} else if (phase < sample_rate_hz * 3) {
			sample = amplitude - (int64_t) amplitude * (phase - sample_rate_hz) / sample_rate_hz;
		} else {
			sample = -amplitude +
				(int64_t) amplitude * (phase - sample_rate_hz * 3) / sample_rate_hz;
		}
		samples[i] = sample;
	}
}

void pcm_buffer_fill(
	volatile uint16_t *buffer,
	size_t buffer_word_count,
	const struct dsp_audio_pcm *pcm,
	size_t first_frame,
	size_t frame_count,
	size_t source_frame_step,
	size_t channel_count
) {
	size_t source_index = first_frame * source_frame_step % pcm->sample_count;
	size_t word_index = 0;

	for (size_t i = 0; i < frame_count; i++) {
		uint16_t sample = (uint16_t) pcm->samples[source_index];

		source_index = (source_index + source_frame_step) % pcm->sample_count;
		for (size_t channel = 0; channel < channel_count; channel++)
			buffer[word_index++] = sample;
	}
	for (; word_index < buffer_word_count; word_index++)
		buffer[word_index] = 0;
}

bool pcm_shared_ring_play(
	const struct pcm_shared_ring *ring,
	const struct dsp_audio_pcm *pcm,
	size_t total_frames
) {
	if (ring->buffer_word_count == 0 || ring->channel_count == 0)
		return false;
	if (ring->output_probe_block_count == 0)
		return false;

	size_t block_frames = ring->buffer_word_count / ring->channel_count;
	size_t frame_count = total_frames < block_frames ? total_frames : block_frames;
	pcm_buffer_fill(ring->buffer, ring->buffer_word_count,
		pcm, 0, frame_count, ring->source_frame_step, ring->channel_count);
	DSP_COM_SET = ring->response_flag;
	if (!ring->start())
		return false;

	size_t frames_submitted = 0;
	size_t blocks_submitted = 0;
	bool output_valid = false;
	test_watchdog_reset();

	while (frames_submitted < total_frames) {
		stopwatch_t start = stopwatch_get();
		while ((DSP_COM_STATUS & ring->response_flag) != 0) {
			if (stopwatch_elapsed_ms(start) >= ring->request_timeout_ms) {
				printf("# PCM buffer %lu was not consumed: communication=%04lX\n",
					(uint32_t) frames_submitted, (uint32_t) DSP_COM_STATUS);
				return false;
			}
			test_watchdog_serve();
		}

		size_t remaining = total_frames - frames_submitted;
		frame_count = remaining < block_frames ? remaining : block_frames;
		frames_submitted += frame_count;
		blocks_submitted++;
		if (frames_submitted < total_frames) {
			remaining = total_frames - frames_submitted;
			frame_count = remaining < block_frames ? remaining : block_frames;
			pcm_buffer_fill(ring->buffer, ring->buffer_word_count,
				pcm, frames_submitted, frame_count, ring->source_frame_step, ring->channel_count);
			DSP_COM_SET = ring->response_flag;
		}
		if (!output_valid && blocks_submitted % ring->output_probe_block_count == 0)
			output_valid = ring->output_validate(pcm);
		test_watchdog_reset();
	}
	return output_valid;
}
