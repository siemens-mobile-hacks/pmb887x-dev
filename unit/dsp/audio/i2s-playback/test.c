#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/audio/board.h"
#include "dsp/audio/pcm.h"
#include "dsp/runtime.h"
#include "test.h"

#define I2S_PLAYBACK_DURATION_SECONDS 3
#define I2S_PLAYBACK_DURATION_TOLERANCE_MS 250
#define I2S_PLAYBACK_MAX_TONE_PERIOD_FRAMES 2205
#define I2S_PLAYBACK_FIRST_TONE_HZ 440
#define I2S_PLAYBACK_SECOND_TONE_HZ 880

static int16_t tone_samples[I2S_PLAYBACK_MAX_TONE_PERIOD_FRAMES];

static bool play_tone(uint32_t tone_hz, uint32_t duration_seconds) {
	uint32_t sample_rate_hz = audio_output_get_rate_hz(AUDIO_OUTPUT_I2S);
	int32_t amplitude = test_is_qemu() ? 30000 : 16384;
	size_t tone_sample_count = pcm_triangle_period_frame_count(sample_rate_hz, tone_hz);
	if (tone_sample_count == 0)
		return false;
	if (tone_sample_count > ARRAY_SIZE(tone_samples))
		return false;
	pcm_triangle_generate(tone_samples, tone_sample_count, sample_rate_hz, tone_hz, amplitude);
	const struct dsp_audio_pcm PCM = {
		.samples = tone_samples,
		.sample_count = tone_sample_count,
		.sample_rate_hz = sample_rate_hz,
	};
	size_t total_frames = sample_rate_hz * duration_seconds;
	size_t frames_played = 0;
	stopwatch_t playback_start = stopwatch_get();
	bool played = audio_output_play(AUDIO_OUTPUT_I2S, &PCM, total_frames, &frames_played);
	uint32_t playback_ms = stopwatch_elapsed_ms(playback_start);

	printf("# PCM playback: tone=%lu Hz, amplitude=%ld, input=%lu frames/s, frames=%lu/%lu, elapsed=%lu ms\n",
		tone_hz, (long) amplitude, sample_rate_hz,
		(uint32_t) frames_played, (uint32_t) total_frames, playback_ms);
	return played && frames_played == total_frames &&
		playback_ms >= duration_seconds * 1000 - I2S_PLAYBACK_DURATION_TOLERANCE_MS &&
		playback_ms <= duration_seconds * 1000 + I2S_PLAYBACK_DURATION_TOLERANCE_MS;
}

int main(void) {
	test_start("DSP runtime I2S playback test");
	printf("# CGU before DSP start: OSC=%08lX CON0=%08lX CON1=%08lX CON2=%08lX STAT=%08lX\n",
		CGU_OSC, CGU_CON0, CGU_CON1, CGU_CON2, CGU_STAT);
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Original User ROM PCM playback through the board I2S route");
	const struct dsp_user_rom *user_rom;
	bool runtime_started = dsp_runtime_start(&user_rom);
	bool clocks_ready = runtime_started && audio_board_configure_clocks();
	if (!test_check("platform User ROM and board clocks start for PCM playback", clocks_ready))
		return test_finish();
	if (!test_check("board audio driver supports the DSP Mask ROM",
		audio_board_has_mask_rom(user_rom->mask_id))) {
		return test_finish();
	}

	printf("# Board: %s\n", audio_board_get_name());
	if (!test_check("board I2S output configures through the original firmware route",
		audio_output_configure(AUDIO_OUTPUT_I2S))) {
		return test_finish();
	}
	test_check("first three-second PCM tone traverses the original firmware I2S route",
		play_tone(I2S_PLAYBACK_FIRST_TONE_HZ, I2S_PLAYBACK_DURATION_SECONDS));
	test_check("second three-second PCM tone restarts the original firmware I2S route",
		play_tone(I2S_PLAYBACK_SECOND_TONE_HZ, I2S_PLAYBACK_DURATION_SECONDS));
	test_check("board audio driver completes PCM playback",
		audio_output_stop(AUDIO_OUTPUT_I2S));

	return test_finish();
}
