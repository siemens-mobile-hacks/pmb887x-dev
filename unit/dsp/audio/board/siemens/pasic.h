#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "test.h"

/* Real phones play quietly; QEMU output is checked at full scale. */
#ifndef PASIC_OUTPUT_GAIN_PERCENT
#define PASIC_OUTPUT_GAIN_PERCENT (test_is_qemu() ? 100 : 1)
#endif

enum pasic_amplifier_path {
	PASIC_AMPLIFIER_PATH_0,
	PASIC_AMPLIFIER_PATH_1,
	PASIC_AMPLIFIER_PATH_2,
	PASIC_AMPLIFIER_PATH_3,
	PASIC_AMPLIFIER_PATH_4,
	PASIC_AMPLIFIER_PATH_5,
	PASIC_AMPLIFIER_PATH_6,
	PASIC_AMPLIFIER_PATH_7,
	PASIC_AMPLIFIER_PATH_8,
	PASIC_AMPLIFIER_PATH_9,
};

enum pasic_push_pull_path {
	PASIC_PUSH_PULL_PATH_A,
	PASIC_PUSH_PULL_PATH_B,
	PASIC_PUSH_PULL_PATH_C,
	PASIC_PUSH_PULL_PATH_D,
};

enum pasic_push_pull_route {
	PASIC_PUSH_PULL_ROUTE_1,
	PASIC_PUSH_PULL_ROUTE_2,
};

enum pasic_switch_mux_path {
	PASIC_SWITCH_MUX_PATH_1 = 1,
	PASIC_SWITCH_MUX_PATH_2,
	PASIC_SWITCH_MUX_PATH_3,
	PASIC_SWITCH_MUX_PATH_4,
	PASIC_SWITCH_MUX_PATH_5,
};

enum pasic_line_route {
	PASIC_LINE_ROUTE_DISABLED,
	PASIC_LINE_ROUTE_MONO,
	PASIC_LINE_ROUTE_DIFFERENTIAL_STEREO,
};

enum pasic_mono_route {
	PASIC_MONO_ROUTE_DISABLED,
	PASIC_MONO_ROUTE_DAC,
};

enum pasic_stereo_route {
	PASIC_STEREO_ROUTE_DISABLED,
	PASIC_STEREO_ROUTE_DAC,
};

struct pasic_gain_table {
	const uint8_t *codes;
	size_t code_count;
};

bool pasic_detect(void);
void pasic_register_write(uint8_t reg, uint8_t value);
void pasic_output_enable(
	uint8_t supply_mode_mask,
	uint8_t supply_1_mask,
	uint8_t supply_2_mask,
	uint32_t supply_settle_ms
);
void pasic_audio_controls_reset(void);
bool pasic_amplifier_gain_set(enum pasic_amplifier_path path, uint8_t gain);
bool pasic_output_gain_percent_set(
	enum pasic_amplifier_path path,
	const struct pasic_gain_table *table,
	uint8_t percent
);
bool pasic_push_pull_configure(
	enum pasic_push_pull_path path,
	bool enabled,
	uint8_t source
);
bool pasic_push_pull_route_set(enum pasic_push_pull_route route, uint8_t source);
bool pasic_mono_route_set(enum pasic_mono_route route);
bool pasic_stereo_route_set(enum pasic_stereo_route route);
bool pasic_line_route_set(enum pasic_line_route route);
bool pasic_switch_mux_raw_set(enum pasic_switch_mux_path path, uint8_t value);
bool pasic_adc_sample_rate_set(uint8_t code);
bool pasic_dac_sample_rate_set(uint8_t code);
