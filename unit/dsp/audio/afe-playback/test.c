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
	for (size_t i = 0; i < ARRAY_SIZE(TONES); i++)
		test_check("PCM tone is consumed at the AFE rate", audio_tone_play(AUDIO_OUTPUT_AFE, &TONES[i]));
	test_check("board AFE output stops after playback",
		audio_output_stop(AUDIO_OUTPUT_AFE));

	return test_finish();
}
