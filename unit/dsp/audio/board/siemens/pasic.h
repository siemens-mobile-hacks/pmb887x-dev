#pragma once

#include <stdbool.h>
#include <stdint.h>

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
	PASIC_PUSH_PULL_PATH_A = 10,
	PASIC_PUSH_PULL_PATH_B,
	PASIC_PUSH_PULL_PATH_C,
	PASIC_PUSH_PULL_PATH_D,
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

bool pasic_initialize(void);
bool pasic_output_enable(
	uint8_t supply_mode_mask,
	uint8_t supply_1_mask,
	uint8_t supply_2_mask,
	uint32_t supply_settle_ms
);
bool pasic_supply_mode_set(uint8_t value);
bool pasic_supply_enable_1_set(uint8_t value);
bool pasic_supply_enable_2_set(uint8_t value);
bool pasic_light_control_set(uint8_t value);
bool pasic_audio_controls_reset(void);
bool pasic_amplifier_gain_set(enum pasic_amplifier_path path, uint8_t gain);
bool pasic_push_pull_configure(
	enum pasic_push_pull_path path,
	bool enabled,
	uint8_t source
);
bool pasic_switch_mux_set(enum pasic_switch_mux_path path, uint8_t value);
bool pasic_mux_input_set(uint8_t value);
bool pasic_adc_sample_rate_set(uint8_t code);
bool pasic_dac_sample_rate_set(uint8_t code);
bool pasic_stereo_mode_set(uint8_t value);
bool pasic_mono_control_set(uint8_t value);
bool pasic_stereo_control_set(uint8_t value);
bool pasic_adc_control_set(uint8_t value);
bool pasic_dac_control_set(uint8_t value);
bool pasic_i2s_rx_control_set(uint8_t value);
bool pasic_i2s_tx_control_set(uint8_t value);
bool pasic_output_is_muted(void);
