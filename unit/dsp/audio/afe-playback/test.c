#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/audio/board.h"
#include "dsp/audio/pcm.h"
#include "dsp/runtime.h"
#include "test.h"

#define AFE_PLAYBACK_DURATION_SECONDS 3
#define AFE_PLAYBACK_DURATION_TOLERANCE_MS 250
#define AFE_PLAYBACK_MAX_TONE_PERIOD_FRAMES 400
#define AFE_PLAYBACK_TONE_HZ 440

static int16_t tone_samples[AFE_PLAYBACK_MAX_TONE_PERIOD_FRAMES];

static bool play_tone(uint32_t tone_hz, uint32_t duration_seconds) {
	uint32_t sample_rate_hz = audio_output_get_rate_hz(AUDIO_OUTPUT_AFE);
	size_t tone_sample_count = pcm_triangle_period_frame_count(sample_rate_hz, tone_hz);
	if (tone_sample_count == 0)
		return false;
	if (tone_sample_count > ARRAY_SIZE(tone_samples))
		return false;
	int32_t amplitude = test_is_qemu() ? 30000 : 16384;
	pcm_triangle_generate(tone_samples, tone_sample_count, sample_rate_hz, tone_hz, amplitude);
	const struct dsp_audio_pcm PCM = {
		.samples = tone_samples,
		.sample_count = tone_sample_count,
		.sample_rate_hz = sample_rate_hz,
	};
	size_t total_frames = sample_rate_hz * duration_seconds;
	size_t frames_played = 0;
	stopwatch_t playback_start = stopwatch_get();
	bool played = audio_output_play(AUDIO_OUTPUT_AFE, &PCM, total_frames, &frames_played);
	uint32_t playback_ms = stopwatch_elapsed_ms(playback_start);

	printf("# AFE PCM playback: tone=%lu Hz, amplitude=%ld, input=%lu frames/s, frames=%lu/%lu, elapsed=%lu ms\n",
		tone_hz, (long) amplitude, sample_rate_hz,
		(uint32_t) frames_played, (uint32_t) total_frames, playback_ms);
	return played && frames_played == total_frames &&
		playback_ms >= duration_seconds * 1000 - AFE_PLAYBACK_DURATION_TOLERANCE_MS &&
		playback_ms <= duration_seconds * 1000 + AFE_PLAYBACK_DURATION_TOLERANCE_MS;
}

int main(void) {
	test_start("DSP runtime AFE playback test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Original User ROM PCM playback through the board AFE route");
	const struct dsp_user_rom *user_rom;
	bool runtime_started = dsp_runtime_start(&user_rom);
	bool clocks_ready = runtime_started && audio_board_configure_clocks();
	if (!test_check("platform User ROM and board clocks start for AFE playback", clocks_ready))
		return test_finish();
	if (!test_check("board audio driver supports the DSP Mask ROM",
		audio_board_has_mask_rom(user_rom->mask_id))) {
		return test_finish();
	}

	printf("# Board: %s\n", audio_board_get_name());
	if (!test_check("original firmware configures the board AFE route",
		audio_output_configure(AUDIO_OUTPUT_AFE))) {
		return test_finish();
	}
	test_check("three-second PCM tone is consumed at the AFE rate",
		play_tone(AFE_PLAYBACK_TONE_HZ, AFE_PLAYBACK_DURATION_SECONDS));
	test_check("board AFE output stops after playback",
		audio_output_stop(AUDIO_OUTPUT_AFE));

	return test_finish();
}
