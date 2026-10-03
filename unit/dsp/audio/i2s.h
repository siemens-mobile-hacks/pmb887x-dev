#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dsp/audio/pcm.h"

struct i2s_output_probe {
	uint16_t result_offset;
	size_t max_large_steps;
};

bool i2s2_output_validate(const struct dsp_audio_pcm *pcm, const struct i2s_output_probe *probe);
