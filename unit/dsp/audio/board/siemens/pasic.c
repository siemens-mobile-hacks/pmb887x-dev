#include <pmb887x.h>
#include <gen/peripheral/pmic/PASIC.h>
#include <stopwatch.h>

#include "dsp/audio/board/siemens/pasic.h"

enum pasic_family {
	PASIC_FAMILY_DIALOG_MODEL_4,
	PASIC_FAMILY_ST_MODEL_4,
	PASIC_FAMILY_DIALOG_MODEL_5,
	PASIC_FAMILY_ST_MODEL_5,
	PASIC_FAMILY_DIALOG_MODEL_6,
	PASIC_FAMILY_ST_MODEL_6,
};

static enum pasic_family pasic_family;
static uint8_t pasic_revision;
static bool pasic_initialized;

static bool pasic_register_write(uint8_t reg, uint8_t value) {
	if (!pasic_initialize())
		return false;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, reg, value);
	uint8_t readback = i2c_smbus_read_byte(PASIC_I2C_ADDR, reg);

	if (readback == value)
		return true;
	printf("# PASIC register %02X readback=%02X, expected=%02X\n", reg, readback, value);
	return false;
}

static bool pasic_register_update(uint8_t reg, uint8_t value, uint8_t mask) {
	if (!pasic_initialize())
		return false;
	uint8_t old_value = i2c_smbus_read_byte(PASIC_I2C_ADDR, reg);
	return pasic_register_write(reg, (old_value & ~mask) | (value & mask));
}

bool pasic_initialize(void) {
	if (pasic_initialized)
		return true;

	i2c_init();
	uint8_t identification = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_IDENTIFICATION);
	switch (identification & (PASIC_IDENTIFICATION_VENDOR | PASIC_IDENTIFICATION_MODEL)) {
		case (PASIC_IDENTIFICATION_VENDOR_DIALOG | 4):
			pasic_family = PASIC_FAMILY_DIALOG_MODEL_4;
			break;
		case (PASIC_IDENTIFICATION_VENDOR_ST | 4):
			pasic_family = PASIC_FAMILY_ST_MODEL_4;
			break;
		case (PASIC_IDENTIFICATION_VENDOR_DIALOG | 5):
			pasic_family = PASIC_FAMILY_DIALOG_MODEL_5;
			break;
		case (PASIC_IDENTIFICATION_VENDOR_ST | 5):
			pasic_family = PASIC_FAMILY_ST_MODEL_5;
			break;
		case (PASIC_IDENTIFICATION_VENDOR_DIALOG | 6):
			pasic_family = PASIC_FAMILY_DIALOG_MODEL_6;
			break;
		case (PASIC_IDENTIFICATION_VENDOR_ST | 6):
			pasic_family = PASIC_FAMILY_ST_MODEL_6;
			break;
		default:
			printf("# Unsupported PASIC identification: %02X\n", identification);
			return false;
	}

	pasic_revision = (identification & PASIC_IDENTIFICATION_REVISION) >>
		PASIC_IDENTIFICATION_REVISION_SHIFT;
	printf("# PASIC identification: %02X, family=%u, revision=%u\n",
		identification, (unsigned int) pasic_family, (unsigned int) pasic_revision);
	pasic_initialized = true;
	return true;
}

bool pasic_output_enable(
	uint8_t supply_mode_mask,
	uint8_t supply_1_mask,
	uint8_t supply_2_mask,
	uint32_t supply_settle_ms
) {
	if (!pasic_initialize())
		return false;

	uint8_t initial_supply_mode = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_MODE);
	uint8_t initial_supply_1 = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_1);
	uint8_t initial_supply_2 = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2);
	if (!pasic_register_write(PASIC_SUPPLY_MODE, initial_supply_mode | supply_mode_mask))
		return false;
	if (!pasic_register_write(PASIC_SUPPLY_ENABLE_1, initial_supply_1 | supply_1_mask))
		return false;
	if (!pasic_register_write(PASIC_SUPPLY_ENABLE_2, initial_supply_2 | supply_2_mask))
		return false;
	stopwatch_usleep_wd(supply_settle_ms * 1000);

	uint8_t supply_mode = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_MODE);
	uint8_t supply_1 = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_1);
	uint8_t supply_2 = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2);
	printf("# PASIC supplies: mode=%02X->%02X, enable1=%02X->%02X, enable2=%02X->%02X\n",
		initial_supply_mode, supply_mode, initial_supply_1, supply_1, initial_supply_2, supply_2);
	return true;
}

bool pasic_supply_mode_set(uint8_t value) {
	return pasic_register_write(PASIC_SUPPLY_MODE, value);
}

bool pasic_supply_enable_1_set(uint8_t value) {
	return pasic_register_write(PASIC_SUPPLY_ENABLE_1, value);
}

bool pasic_supply_enable_2_set(uint8_t value) {
	return pasic_register_write(PASIC_SUPPLY_ENABLE_2, value);
}

bool pasic_light_control_set(uint8_t value) {
	return pasic_register_write(PASIC_LIGHT_CONTROL, value);
}

bool pasic_audio_controls_reset(void) {
	static const struct {
		uint8_t reg;
		uint8_t value;
	} reset_values[] = {
		{ PASIC_AMPLIFIER_GAIN_0, 0x39 },
		{ PASIC_AMPLIFIER_GAIN_1, 0x39 },
		{ PASIC_AMPLIFIER_GAIN_2, 0x79 },
		{ PASIC_AMPLIFIER_GAIN_3, 0x39 },
		{ PASIC_AMPLIFIER_GAIN_4_5, 0x39 },
		{ PASIC_AMPLIFIER_GAIN_6, 0x39 },
		{ PASIC_AMPLIFIER_GAIN_7_8, 0x39 },
		{ PASIC_AMPLIFIER_GAIN_9, 0x39 },
		{ PASIC_MONO_CONTROL, 0x00 },
		{ PASIC_STEREO_CONTROL, 0x00 },
		{ PASIC_ADC_CONTROL, 0x00 },
		{ PASIC_DAC_CONTROL, 0x00 },
		{ PASIC_PP_AMPLIFIER_1, 0x00 },
		{ PASIC_PP_AMPLIFIER_2, 0x00 },
		{ PASIC_PP_ROUTE_1, 0x00 },
		{ PASIC_PP_ROUTE_2, 0x00 },
		{ PASIC_MUX_INPUT, 0x00 },
		{ PASIC_SWITCH_MUX_1, 0x00 },
		{ PASIC_SWITCH_MUX_2, 0x00 },
		{ PASIC_SWITCH_MUX_3, 0x00 },
		{ PASIC_SWITCH_MUX_4, 0x00 },
		{ PASIC_SWITCH_MUX_5, 0x00 },
	};

	for (size_t i = 0; i < ARRAY_SIZE(reset_values); i++)
		if (!pasic_register_write(reset_values[i].reg, reset_values[i].value))
			return false;
	return true;
}

bool pasic_amplifier_gain_set(enum pasic_amplifier_path path, uint8_t gain) {
	uint8_t reg;
	uint8_t value = gain;
	uint8_t mask = UINT8_MAX;

	switch (path) {
		case PASIC_AMPLIFIER_PATH_0:
			reg = PASIC_AMPLIFIER_GAIN_0;
			break;
		case PASIC_AMPLIFIER_PATH_1:
			reg = PASIC_AMPLIFIER_GAIN_1;
			break;
		case PASIC_AMPLIFIER_PATH_2:
			if ((gain & ~PASIC_AMPLIFIER_GAIN_2_GAIN) != 0)
				return false;
			reg = PASIC_AMPLIFIER_GAIN_2;
			mask = PASIC_AMPLIFIER_GAIN_2_GAIN;
			break;
		case PASIC_AMPLIFIER_PATH_3:
			reg = PASIC_AMPLIFIER_GAIN_3;
			break;
		case PASIC_AMPLIFIER_PATH_4:
			if ((gain & PASIC_AMPLIFIER_GAIN_4_5_PATH_4_SELECT) != 0)
				return false;
			reg = PASIC_AMPLIFIER_GAIN_4_5;
			value |= PASIC_AMPLIFIER_GAIN_4_5_PATH_4_SELECT;
			break;
		case PASIC_AMPLIFIER_PATH_5:
			if ((gain & PASIC_AMPLIFIER_GAIN_4_5_PATH_4_SELECT) != 0)
				return false;
			reg = PASIC_AMPLIFIER_GAIN_4_5;
			break;
		case PASIC_AMPLIFIER_PATH_6:
			reg = PASIC_AMPLIFIER_GAIN_6;
			break;
		case PASIC_AMPLIFIER_PATH_7:
			if ((gain & PASIC_AMPLIFIER_GAIN_7_8_PATH_7_SELECT) != 0)
				return false;
			reg = PASIC_AMPLIFIER_GAIN_7_8;
			value |= PASIC_AMPLIFIER_GAIN_7_8_PATH_7_SELECT;
			break;
		case PASIC_AMPLIFIER_PATH_8:
			if ((gain & PASIC_AMPLIFIER_GAIN_7_8_PATH_7_SELECT) != 0)
				return false;
			reg = PASIC_AMPLIFIER_GAIN_7_8;
			break;
		case PASIC_AMPLIFIER_PATH_9:
			reg = PASIC_AMPLIFIER_GAIN_9;
			break;
		default:
			return false;
	}

	if (!pasic_register_update(reg, value, mask))
		return false;
	if (path == PASIC_AMPLIFIER_PATH_6)
		return pasic_register_update(PASIC_AMPLIFIER_GAIN_4_5, 0, PASIC_AMPLIFIER_GAIN_4_5_PATH_4_SELECT);
	if (path == PASIC_AMPLIFIER_PATH_9)
		return pasic_register_update(PASIC_AMPLIFIER_GAIN_7_8, 0, PASIC_AMPLIFIER_GAIN_7_8_PATH_7_SELECT);
	return true;
}

bool pasic_push_pull_configure(
	enum pasic_push_pull_path path,
	bool enabled,
	uint8_t source
) {
	if (source > 8)
		return false;
	if (path == PASIC_PUSH_PULL_ROUTE_1)
		return pasic_register_write(PASIC_PP_ROUTE_1, source);
	if (path == PASIC_PUSH_PULL_ROUTE_2)
		return pasic_register_write(PASIC_PP_ROUTE_2, source);
	if ((source & 1) != 0)
		return false;

	uint8_t reg;
	uint8_t mask;
	uint8_t value = source / 2;
	switch (path) {
		case PASIC_PUSH_PULL_PATH_A:
			reg = PASIC_PP_AMPLIFIER_1;
			mask = PASIC_PP_AMPLIFIER_1_PATH_1_SOURCE | PASIC_PP_AMPLIFIER_1_PATH_1_EN;
			if (enabled)
				value |= PASIC_PP_AMPLIFIER_1_PATH_1_EN;
			break;
		case PASIC_PUSH_PULL_PATH_B:
			reg = PASIC_PP_AMPLIFIER_1;
			mask = PASIC_PP_AMPLIFIER_1_PATH_2_SOURCE | PASIC_PP_AMPLIFIER_1_PATH_2_EN;
			value <<= PASIC_PP_AMPLIFIER_1_PATH_2_SOURCE_SHIFT;
			if (enabled)
				value |= PASIC_PP_AMPLIFIER_1_PATH_2_EN;
			break;
		case PASIC_PUSH_PULL_PATH_C:
			reg = PASIC_PP_AMPLIFIER_2;
			mask = PASIC_PP_AMPLIFIER_2_PATH_1_SOURCE | PASIC_PP_AMPLIFIER_2_PATH_1_EN;
			if (enabled)
				value |= PASIC_PP_AMPLIFIER_2_PATH_1_EN;
			break;
		case PASIC_PUSH_PULL_PATH_D:
			reg = PASIC_PP_AMPLIFIER_2;
			mask = PASIC_PP_AMPLIFIER_2_PATH_2_SOURCE | PASIC_PP_AMPLIFIER_2_PATH_2_EN;
			value <<= PASIC_PP_AMPLIFIER_2_PATH_2_SOURCE_SHIFT;
			if (enabled)
				value |= PASIC_PP_AMPLIFIER_2_PATH_2_EN;
			break;
		default:
			return false;
	}
	return pasic_register_update(reg, value, mask);
}

bool pasic_switch_mux_set(enum pasic_switch_mux_path path, uint8_t value) {
	uint8_t reg;
	uint8_t mask = UINT8_MAX;

	switch (path) {
		case PASIC_SWITCH_MUX_PATH_1:
			reg = PASIC_SWITCH_MUX_1;
			break;
		case PASIC_SWITCH_MUX_PATH_2:
			reg = PASIC_SWITCH_MUX_2;
			mask = PASIC_SWITCH_MUX_2_VALUE;
			break;
		case PASIC_SWITCH_MUX_PATH_3:
			reg = PASIC_SWITCH_MUX_3;
			break;
		case PASIC_SWITCH_MUX_PATH_4:
			reg = PASIC_SWITCH_MUX_4;
			break;
		case PASIC_SWITCH_MUX_PATH_5: {
			reg = PASIC_SWITCH_MUX_5;
			bool is_model_4 = pasic_family == PASIC_FAMILY_DIALOG_MODEL_4 ||
				pasic_family == PASIC_FAMILY_ST_MODEL_4;
			if (is_model_4)
				value &= ~BIT(6);
			break;
		}
		default:
			return false;
	}
	return pasic_register_update(reg, value, mask);
}

bool pasic_mux_input_set(uint8_t value) {
	return pasic_register_write(PASIC_MUX_INPUT, value);
}

bool pasic_adc_sample_rate_set(uint8_t code) {
	if (code > 8)
		return false;
	return pasic_register_update(PASIC_SAMPLE_RATE,
		(code << PASIC_SAMPLE_RATE_ADC_SHIFT), PASIC_SAMPLE_RATE_ADC);
}

bool pasic_dac_sample_rate_set(uint8_t code) {
	if (code > 8)
		return false;
	return pasic_register_update(PASIC_SAMPLE_RATE,
		(code << PASIC_SAMPLE_RATE_DAC_SHIFT), PASIC_SAMPLE_RATE_DAC);
}

bool pasic_stereo_mode_set(uint8_t value) {
	return pasic_register_write(PASIC_STEREO_MODE, value);
}

bool pasic_mono_control_set(uint8_t value) {
	return pasic_register_write(PASIC_MONO_CONTROL, value);
}

bool pasic_stereo_control_set(uint8_t value) {
	return pasic_register_write(PASIC_STEREO_CONTROL, value);
}

bool pasic_adc_control_set(uint8_t value) {
	return pasic_register_write(PASIC_ADC_CONTROL, value);
}

bool pasic_dac_control_set(uint8_t value) {
	return pasic_register_write(PASIC_DAC_CONTROL, value);
}

bool pasic_i2s_rx_control_set(uint8_t value) {
	return pasic_register_write(PASIC_I2S_RX_CONTROL, value);
}

bool pasic_i2s_tx_control_set(uint8_t value) {
	return pasic_register_write(PASIC_I2S_TX_CONTROL, value);
}

bool pasic_output_is_muted(void) {
	if (!pasic_initialize())
		return false;
	uint8_t mono_control = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_MONO_CONTROL);

	printf("# PASIC output: mono-control=%02X\n", mono_control);
	return mono_control == 0;
}
