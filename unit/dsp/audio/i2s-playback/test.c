#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "dsp/audio/board.h"
#include "dsp/audio/pcm.h"
#include "test.h"

#define I2S_PLAYBACK_DURATION_SECONDS 3
#define I2S_PLAYBACK_DURATION_TOLERANCE_MS 250
#define I2S_PLAYBACK_TONE_HZ 440
#define I2S_RESTART_TONE_HZ 660

static bool play_tone(uint32_t tone_hz, uint32_t duration_seconds) {
	uint32_t sample_rate_hz = dsp_audio_board_get_sample_rate_hz();
	int32_t amplitude = test_is_qemu() ? 30000 : 328;
	int16_t tone_samples[640];
	pcm_triangle_generate(tone_samples, ARRAY_SIZE(tone_samples), sample_rate_hz, tone_hz, amplitude);
	const struct dsp_audio_pcm pcm = {
		.samples = tone_samples,
		.sample_count = ARRAY_SIZE(tone_samples),
		.sample_rate_hz = sample_rate_hz,
		.expected_tone_hz = tone_hz,
	};
	size_t total_frames = sample_rate_hz * duration_seconds;
	size_t frames_played = 0;
	stopwatch_t playback_start = stopwatch_get();
	bool played = dsp_audio_board_play(&pcm, total_frames, &frames_played);
	uint32_t playback_ms = stopwatch_elapsed_ms(playback_start);

	printf("# PCM playback: source=%lu Hz, output=%lu Hz, amplitude=%ld, input=%lu frames/s, "
		"frames=%lu/%lu, elapsed=%lu ms\n",
		tone_hz, (uint32_t) pcm.expected_tone_hz, (long) amplitude, sample_rate_hz,
		(uint32_t) frames_played, (uint32_t) total_frames, playback_ms);
	return played && frames_played == total_frames &&
		playback_ms >= duration_seconds * 1000 - I2S_PLAYBACK_DURATION_TOLERANCE_MS &&
		playback_ms <= duration_seconds * 1000 + I2S_PLAYBACK_DURATION_TOLERANCE_MS;
}

int main(void) {
	test_start("DSP runtime I2S playback test");
	printf("# CGU before DSP start: OSC=%08lX CON0=%08lX CON1=%08lX CON2=%08lX STAT=%08lX\n",
		CGU_OSC, CGU_CON0, CGU_CON1, CGU_CON2, CGU_STAT);
	bool clocks_ready = dsp_audio_board_clocks_configure();
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Original User ROM PCM playback through the board I2S route");
	const struct dsp_runtime_profile *runtime;
	bool runtime_started = clocks_ready && dsp_runtime_profile_start(&runtime);
	if (!test_check("board clocks and platform User ROM start for PCM playback", runtime_started))
		return test_finish();
	if (!test_check("board audio driver supports the DSP Mask ROM",
		dsp_audio_board_supports_mask_rom(runtime->mask_id))) {
		return test_finish();
	}

	printf("# Board: %s\n", dsp_audio_board_get_name());
	test_check("three-second PCM tone traverses the original firmware I2S route",
		play_tone(I2S_PLAYBACK_TONE_HZ, I2S_PLAYBACK_DURATION_SECONDS));
	bool stopped = dsp_audio_board_stop();
	bool muted = stopped && dsp_audio_board_output_is_muted();
	test_check("firmware stop command mutes the board audio output", muted);
	test_check("PCM playback restarts with new content without restarting the DSP",
		muted && play_tone(I2S_RESTART_TONE_HZ, 1));
	test_check("restarted audio output mutes through the original firmware route",
		dsp_audio_board_stop() && dsp_audio_board_output_is_muted());

	return test_finish();
}
