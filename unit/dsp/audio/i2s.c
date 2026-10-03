#include <pmb887x.h>
#include <cpu.h>
#include <gen/dsp.h>
#include <stopwatch.h>

#include "dsp/runtime.h"
#include "dsp/audio/i2s.h"
#include "test.h"

static bool runtime_word_read(uint16_t address, uint16_t result_offset, uint16_t *value) {
	return dsp_runtime_data_read(address, result_offset, value, 1);
}

static bool i2s2_tx_sample_rate_get(const uint16_t *registers, uint32_t *sample_rate_hz) {
	uint16_t clock_select = registers[1] & TEAK_I2S2_CSEL_TXCLKSEL;
	uint16_t numerator;
	uint16_t denominator;
	uint32_t reference_hz;

	if (clock_select == 0) {
		numerator = registers[3] & TEAK_I2S2_NUM0_NUMERATOR;
		denominator = registers[4] & TEAK_I2S2_DEN0_DENOMINATOR;
		uint16_t reference_select = registers[3] & TEAK_I2S2_NUM0_FREF;
		if (reference_select == TEAK_I2S2_NUM0_FREF_MODULE_CLOCK) {
			reference_hz = cpu_get_dsp_freq();
		} else if (reference_select == TEAK_I2S2_NUM0_FREF_CLOCK_104MHZ) {
			reference_hz = cpu_get_ms_freq();
		} else {
			printf("# I2S2 TX divider 0 uses reserved reference clock %04X\n", reference_select);
			return false;
		}
	} else if (clock_select == 3) {
		numerator = registers[5] & TEAK_I2S2_NUM1_NUMERATOR;
		denominator = registers[6] & TEAK_I2S2_DEN1_DENOMINATOR;
		uint16_t reference_select = registers[5] & TEAK_I2S2_NUM1_FREF;
		if (reference_select == TEAK_I2S2_NUM1_FREF_MODULE_CLOCK) {
			reference_hz = cpu_get_dsp_freq();
		} else if (reference_select == TEAK_I2S2_NUM1_FREF_CLOCK_104MHZ) {
			reference_hz = cpu_get_ms_freq();
		} else {
			printf("# I2S2 TX divider 1 uses reserved reference clock %04X\n", reference_select);
			return false;
		}
	} else {
		printf("# I2S2 TX uses external clock selector %u\n", clock_select);
		return false;
	}

	if (numerator == 0 || denominator == 0)
		return false;

	uint32_t frame_clocks;
	switch (registers[9] & TEAK_I2S2_TXCONF_PERIOD) {
		case TEAK_I2S2_TXCONF_PERIOD_CLOCKS_64:
			frame_clocks = 64;
			break;
		case TEAK_I2S2_TXCONF_PERIOD_CLOCKS_48:
			frame_clocks = 48;
			break;
		case TEAK_I2S2_TXCONF_PERIOD_CLOCKS_32:
			frame_clocks = 32;
			break;
		default:
			return false;
	}

	*sample_rate_hz = (uint64_t) reference_hz * numerator / (4 * denominator * frame_clocks);
	return *sample_rate_hz != 0;
}

static bool samples_are_tone(
	const uint16_t *samples,
	size_t sample_count,
	size_t sample_stride,
	uint32_t sample_rate_hz,
	uint32_t tone_hz,
	size_t max_large_steps
) {
	int16_t minimum = INT16_MAX;
	int16_t maximum = INT16_MIN;
	size_t zero_crossings = 0;
	size_t large_steps = 0;

	for (size_t i = 0; i < sample_count; i++) {
		int16_t sample = samples[i * sample_stride];

		if (sample < minimum)
			minimum = sample;
		if (sample > maximum)
			maximum = sample;
	}

	int32_t range = (int32_t) maximum - minimum;
	int32_t midpoint = ((int32_t) minimum + maximum) / 2;
	for (size_t i = 1; i < sample_count; i++) {
		int32_t previous = (int16_t) samples[(i - 1) * sample_stride];
		int32_t current = (int16_t) samples[i * sample_stride];
		if ((previous < midpoint && current >= midpoint) ||
			(previous >= midpoint && current < midpoint)) {
			zero_crossings++;
		}

		int32_t step = current - previous;
		if (step < 0)
			step = -step;
		if (step * 2 > range)
			large_steps++;
	}

	size_t expected_crossings = (2 * tone_hz * sample_count + sample_rate_hz / 2) / sample_rate_hz;
	if (range == 0)
		return false;
	if (zero_crossings + 1 < expected_crossings || zero_crossings > expected_crossings + 1)
		return false;
	return large_steps <= max_large_steps;
}

static bool channels_are_dual_mono(const uint16_t *samples, size_t frame_count) {
	size_t mismatched_frames = 0;

	for (size_t i = 0; i < frame_count; i++) {
		if (samples[i * 2] != samples[i * 2 + 1])
			mismatched_frames++;
	}
	return mismatched_frames <= 1;
}

static bool samples_are_silent(const uint16_t *samples, size_t sample_count, size_t sample_stride) {
	size_t nonzero_samples = 0;

	for (size_t i = 0; i < sample_count; i++) {
		if (samples[i * sample_stride] != 0)
			nonzero_samples++;
	}
	return nonzero_samples == 0;
}

static bool output_format_validate(
	const struct dsp_audio_pcm *pcm,
	const uint16_t *samples,
	size_t word_count,
	uint16_t tx_configuration,
	uint32_t sample_rate_hz,
	size_t max_large_steps
) {
	size_t channel_samples = word_count / 2;
	uint16_t mono_mode = tx_configuration & TEAK_I2S2_TXCONF_MONO;

	switch (mono_mode) {
		case TEAK_I2S2_TXCONF_MONO_STEREO: {
			bool left_is_tone = samples_are_tone(samples, channel_samples, 2,
				sample_rate_hz, pcm->expected_tone_hz, max_large_steps);
			bool right_is_tone = samples_are_tone(samples + 1, channel_samples, 2,
				sample_rate_hz, pcm->expected_tone_hz, max_large_steps);
			return left_is_tone && right_is_tone && channels_are_dual_mono(samples, channel_samples);
		}
		case TEAK_I2S2_TXCONF_MONO_LEFT:
			return samples_are_tone(samples, channel_samples, 2,
				sample_rate_hz, pcm->expected_tone_hz, max_large_steps) &&
				samples_are_silent(samples + 1, channel_samples, 2);
		case TEAK_I2S2_TXCONF_MONO_RIGHT:
			return samples_are_tone(samples + 1, channel_samples, 2,
				sample_rate_hz, pcm->expected_tone_hz, max_large_steps) &&
				samples_are_silent(samples, channel_samples, 2);
	}
	return false;
}

bool i2s2_output_validate(const struct dsp_audio_pcm *pcm, const struct i2s_output_probe *probe) {
	uint16_t registers[11];
	if (!dsp_runtime_data_read(TEAK_I2S2_BASE, probe->result_offset, registers, ARRAY_SIZE(registers)))
		return false;
	printf("# I2S2: CTRL=%04X CSEL=%04X RWADDR=%04X NUM0=%04X DEN0=%04X NUM1=%04X DEN1=%04X "
		"RXCONF=%04X RXINTADDR=%04X TXCONF=%04X TXINTADDR=%04X\n",
		registers[0], registers[1], registers[2], registers[3], registers[4], registers[5],
		registers[6], registers[7], registers[8], registers[9], registers[10]);
	uint32_t sample_rate_hz;
	if (!i2s2_tx_sample_rate_get(registers, &sample_rate_hz))
		return false;
	printf("# I2S2 TX sample rate: %lu Hz\n", sample_rate_hz);

	uint16_t control;
	if (!runtime_word_read(TEAK_I2S2_CTRL, probe->result_offset, &control))
		return false;
	uint16_t required_control = TEAK_I2S2_CTRL_I2SON | TEAK_I2S2_CTRL_I2STXSTART;
	if ((control & required_control) != required_control) {
		printf("# I2S2 output is not active: control=%04X\n", control);
		return false;
	}

	uint16_t previous;
	if (!runtime_word_read(TEAK_I2S2_RWADDR, probe->result_offset, &previous))
		return false;
	uint16_t address_mask = TEAK_I2S2_RWADDR_RDADDR | TEAK_I2S2_RWADDR_WRADDR;
	previous &= address_mask;
	bool advanced = false;
	for (size_t i = 0; i < 8; i++) {
		stopwatch_usleep_wd(137);
		uint16_t current;
		if (!runtime_word_read(TEAK_I2S2_RWADDR, probe->result_offset, &current))
			return false;
		current &= address_mask;
		advanced |= current != previous;

		uint16_t samples[TEAK_I2S2_RAM_SIZE / 2];
		if (!dsp_runtime_data_read(TEAK_I2S2_RAM_BASE, probe->result_offset,
			samples, ARRAY_SIZE(samples))) {
			return false;
		}
		if (advanced && output_format_validate(pcm, samples, ARRAY_SIZE(samples), registers[9],
			sample_rate_hz, probe->max_large_steps)) {
			return true;
		}
		previous = current;
	}

	printf("# I2S2 output is invalid: control=%04X, address=%04X, advanced=%s\n",
		control, previous, advanced ? "yes" : "no");
	return false;
}
