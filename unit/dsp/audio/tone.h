#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dsp/audio/board.h"
#include "dsp/audio/pcm.h"

struct audio_tone {
	enum pcm_waveform waveform;
	uint32_t frequency_hz;
	uint32_t duration_ms;
};

bool audio_tone_play(enum audio_output output, const struct audio_tone *tone);
