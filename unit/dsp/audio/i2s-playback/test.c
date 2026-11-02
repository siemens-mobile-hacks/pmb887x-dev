#include <pmb887x.h>

#include "dsp/audio/board.h"
#include "dsp/audio/tone.h"
#include "dsp/runtime.h"
#include "test.h"

static const struct audio_tone TONES[] = {
	{ PCM_WAVEFORM_TRIANGLE, 440, 3000 },
	{ PCM_WAVEFORM_SAWTOOTH, 220, 3000 },
	{ PCM_WAVEFORM_SQUARE, 1000, 3000 },
	{ PCM_WAVEFORM_TRIANGLE, 3000, 3000 },
};

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
	for (size_t i = 0; i < ARRAY_SIZE(TONES); i++) {
		test_check("PCM tone traverses the original firmware I2S route",
			audio_tone_play(AUDIO_OUTPUT_I2S, &TONES[i]));
	}
	test_check("board audio driver completes PCM playback",
		audio_output_stop(AUDIO_OUTPUT_I2S));

	return test_finish();
}
