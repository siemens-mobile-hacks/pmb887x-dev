#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/audio/tone.h"
#include "test.h"

#define AUDIO_TONE_MAX_PERIOD_FRAMES 2205
#define AUDIO_TONE_DURATION_TOLERANCE_MS 250

static const char *const WAVEFORM_NAMES[] = {
	[PCM_WAVEFORM_TRIANGLE] = "triangle",
	[PCM_WAVEFORM_SAWTOOTH] = "sawtooth",
	[PCM_WAVEFORM_SQUARE] = "square",
};

static int16_t tone_samples[AUDIO_TONE_MAX_PERIOD_FRAMES];

bool audio_tone_play(enum audio_output output, const struct audio_tone *tone) {
	uint32_t sample_rate_hz = audio_output_get_rate_hz(output);
	size_t period_frames = pcm_period_frame_count(sample_rate_hz, tone->frequency_hz);
	if (period_frames == 0)
		return false;
	if (period_frames > ARRAY_SIZE(tone_samples))
		return false;

	int32_t amplitude = test_is_qemu() ? 30000 : 16384;
	pcm_waveform_generate(tone_samples, period_frames, sample_rate_hz, tone->frequency_hz, amplitude, tone->waveform);
	const struct dsp_audio_pcm PCM = {
		.samples = tone_samples,
		.sample_count = period_frames,
		.sample_rate_hz = sample_rate_hz,
	};
	size_t total_frames = (uint64_t) sample_rate_hz * tone->duration_ms / 1000;
	size_t frames_played = 0;

	test_audio_expect_tone(tone->frequency_hz, sample_rate_hz, tone->duration_ms);
	stopwatch_t playback_start = stopwatch_get();
	bool played = audio_output_play(output, &PCM, total_frames, &frames_played);
	uint32_t playback_ms = stopwatch_elapsed_ms(playback_start);

	printf("# PCM playback: %s %lu Hz, amplitude=%ld, input=%lu frames/s, frames=%lu/%lu, elapsed=%lu ms\n",
		WAVEFORM_NAMES[tone->waveform], tone->frequency_hz, (long) amplitude, sample_rate_hz,
		(uint32_t) frames_played, (uint32_t) total_frames, playback_ms);
	return played && frames_played == total_frames &&
		test_u32_in_interval(playback_ms,
			tone->duration_ms + PCM_SILENCE_MS - AUDIO_TONE_DURATION_TOLERANCE_MS,
			tone->duration_ms + PCM_SILENCE_MS + AUDIO_TONE_DURATION_TOLERANCE_MS);
}
