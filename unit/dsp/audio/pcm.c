#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/audio/pcm.h"
#include "test.h"

static uint32_t pcm_common_divisor(uint32_t first, uint32_t second) {
	while (second != 0) {
		uint32_t remainder = first % second;
		first = second;
		second = remainder;
	}
	return first;
}

size_t pcm_period_frame_count(uint32_t sample_rate_hz, uint32_t tone_hz) {
	if (sample_rate_hz == 0)
		return 0;
	if (tone_hz == 0)
		return 0;
	return sample_rate_hz / pcm_common_divisor(sample_rate_hz, tone_hz);
}

static int32_t pcm_triangle_sample(uint32_t phase, uint32_t sample_rate_hz, int32_t amplitude) {
	if (phase < sample_rate_hz)
		return (int64_t) amplitude * phase / sample_rate_hz;
	if (phase < sample_rate_hz * 3)
		return amplitude - (int64_t) amplitude * (phase - sample_rate_hz) / sample_rate_hz;
	return -amplitude + (int64_t) amplitude * (phase - sample_rate_hz * 3) / sample_rate_hz;
}

void pcm_waveform_generate(
	int16_t *samples,
	size_t sample_count,
	uint32_t sample_rate_hz,
	uint32_t tone_hz,
	int32_t amplitude,
	enum pcm_waveform waveform
) {
	uint32_t phase_period = sample_rate_hz * 4;

	for (size_t i = 0; i < sample_count; i++) {
		uint32_t phase = (uint64_t) i * tone_hz * 4 % phase_period;
		int32_t sample = 0;

		switch (waveform) {
			case PCM_WAVEFORM_TRIANGLE:
				sample = pcm_triangle_sample(phase, sample_rate_hz, amplitude);
				break;
			case PCM_WAVEFORM_SAWTOOTH:
				sample = -amplitude + (int64_t) 2 * amplitude * phase / phase_period;
				break;
			case PCM_WAVEFORM_SQUARE:
				sample = phase < sample_rate_hz * 2 ? amplitude : -amplitude;
				break;
		}
		samples[i] = (int16_t) sample;
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

static bool pcm_shared_ring_wait_consumed(const struct pcm_shared_ring *ring, size_t frames_submitted) {
	stopwatch_t start = stopwatch_get();

	while ((DSP_COM_STATUS & ring->response_flag) != 0) {
		if (stopwatch_elapsed_ms(start) >= ring->request_timeout_ms) {
			printf("# PCM buffer %lu was not consumed: communication=%04lX\n",
				(uint32_t) frames_submitted, (uint32_t) DSP_COM_STATUS);
			return false;
		}
		test_watchdog_serve();
	}
	return true;
}

bool pcm_shared_ring_play(
	const struct pcm_shared_ring *ring,
	const struct dsp_audio_pcm *pcm,
	size_t total_frames
) {
	if (ring->buffer_word_count == 0)
		return false;
	if (ring->channel_count == 0)
		return false;

	size_t block_frames = ring->buffer_word_count / ring->channel_count;
	size_t frame_count = total_frames < block_frames ? total_frames : block_frames;
	pcm_buffer_fill(ring->buffer, ring->buffer_word_count,
		pcm, 0, frame_count, ring->source_frame_step, ring->channel_count);
	DSP_COM_SET = ring->response_flag;
	if (!ring->start())
		return false;

	size_t frames_submitted = 0;
	test_watchdog_reset();

	while (frames_submitted < total_frames) {
		if (!pcm_shared_ring_wait_consumed(ring, frames_submitted))
			return false;

		size_t remaining = total_frames - frames_submitted;
		frame_count = remaining < block_frames ? remaining : block_frames;
		frames_submitted += frame_count;
		if (frames_submitted < total_frames) {
			remaining = total_frames - frames_submitted;
			frame_count = remaining < block_frames ? remaining : block_frames;
			pcm_buffer_fill(ring->buffer, ring->buffer_word_count,
				pcm, frames_submitted, frame_count, ring->source_frame_step, ring->channel_count);
			DSP_COM_SET = ring->response_flag;
		}
		test_watchdog_reset();
	}

	/* The DSP replays the last buffer until a new one arrives, so silence flushes its output pipeline. */
	size_t silence_frames = (size_t) pcm->sample_rate_hz * PCM_SILENCE_MS / 1000;
	size_t silence_blocks = (silence_frames + block_frames - 1) / block_frames;
	for (size_t i = 0; i < silence_blocks; i++) {
		if (!pcm_shared_ring_wait_consumed(ring, total_frames))
			return false;
		pcm_buffer_fill(ring->buffer, ring->buffer_word_count, pcm, 0, 0, ring->source_frame_step, ring->channel_count);
		DSP_COM_SET = ring->response_flag;
		test_watchdog_reset();
	}
	return pcm_shared_ring_wait_consumed(ring, total_frames);
}
