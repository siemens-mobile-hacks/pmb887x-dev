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

void pasic_register_write(uint8_t reg, uint8_t value) {
	i2c_smbus_write_byte(PASIC_I2C_ADDR, reg, value);
}

static void pasic_register_update(uint8_t reg, uint8_t value, uint8_t mask) {
	uint8_t old_value = 0;

	i2c_smbus_read_byte(PASIC_I2C_ADDR, reg, &old_value);
	pasic_register_write(reg, (old_value & ~mask) | (value & mask));
}

bool pasic_detect(void) {
	uint8_t identification;

	if (!i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_IDENTIFICATION, &identification))
		return false;
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

	uint8_t revision = (identification & PASIC_IDENTIFICATION_REVISION) >>
		PASIC_IDENTIFICATION_REVISION_SHIFT;
	printf("# PASIC identification: %02X, family=%u, revision=%u\n",
		identification, (unsigned int) pasic_family, (unsigned int) revision);
	return true;
}

void pasic_output_enable(
	uint8_t supply_mode_mask,
	uint8_t supply_1_mask,
	uint8_t supply_2_mask,
	uint32_t supply_settle_ms
) {
	uint8_t initial_supply_mode = 0;
	uint8_t initial_supply_1 = 0;
	uint8_t initial_supply_2 = 0;

	i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_MODE, &initial_supply_mode);
	i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_1, &initial_supply_1);
	i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2, &initial_supply_2);
	pasic_register_write(PASIC_SUPPLY_MODE, initial_supply_mode | supply_mode_mask);
	pasic_register_write(PASIC_SUPPLY_ENABLE_1, initial_supply_1 | supply_1_mask);
	pasic_register_write(PASIC_SUPPLY_ENABLE_2, initial_supply_2 | supply_2_mask);
	stopwatch_usleep_wd(supply_settle_ms * 1000);
}

void pasic_audio_controls_reset(void) {
	static const struct {
		uint8_t reg;
		uint8_t value;
	} RESET_VALUES[] = {
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

	for (size_t i = 0; i < ARRAY_SIZE(RESET_VALUES); i++)
		pasic_register_write(RESET_VALUES[i].reg, RESET_VALUES[i].value);
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

	pasic_register_update(reg, value, mask);
	if (path == PASIC_AMPLIFIER_PATH_6)
		pasic_register_update(PASIC_AMPLIFIER_GAIN_4_5, 0, PASIC_AMPLIFIER_GAIN_4_5_PATH_4_SELECT);
	if (path == PASIC_AMPLIFIER_PATH_9)
		pasic_register_update(PASIC_AMPLIFIER_GAIN_7_8, 0, PASIC_AMPLIFIER_GAIN_7_8_PATH_7_SELECT);
	return true;
}

bool pasic_output_gain_percent_set(
	enum pasic_amplifier_path path,
	const struct pasic_gain_table *table,
	uint8_t percent
) {
	if (percent > 100)
		return false;
	if (path != PASIC_AMPLIFIER_PATH_4 && path != PASIC_AMPLIFIER_PATH_7)
		return false;
	if (table == NULL)
		return false;
	if (table->codes == NULL)
		return false;
	if (table->code_count < 2)
		return false;

	// Round a nonzero percentage up to the first audible firmware step.
	size_t index = ((size_t) percent * (table->code_count - 1) + 99) / 100;
	return pasic_amplifier_gain_set(path, table->codes[index]);
}

bool pasic_push_pull_configure(
	enum pasic_push_pull_path path,
	bool enabled,
	uint8_t source
) {
	if (source > 8)
		return false;
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

	pasic_register_update(reg, value, mask);
	return true;
}

bool pasic_push_pull_route_set(enum pasic_push_pull_route route, uint8_t source) {
	if (source > 8)
		return false;

	uint8_t reg;
	switch (route) {
		case PASIC_PUSH_PULL_ROUTE_1:
			reg = PASIC_PP_ROUTE_1;
			break;
		case PASIC_PUSH_PULL_ROUTE_2:
			reg = PASIC_PP_ROUTE_2;
			break;
		default:
			return false;
	}

	pasic_register_write(reg, source);
	return true;
}

bool pasic_switch_mux_raw_set(enum pasic_switch_mux_path path, uint8_t value) {
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

	pasic_register_update(reg, value, mask);
	return true;
}

bool pasic_mono_route_set(enum pasic_mono_route route) {
	uint8_t value;

	switch (route) {
		case PASIC_MONO_ROUTE_DISABLED:
			value = 0x00;
			break;
		case PASIC_MONO_ROUTE_DAC:
			value = 0x03;
			break;
		default:
			return false;
	}

	return pasic_switch_mux_raw_set(PASIC_SWITCH_MUX_PATH_4, value);
}

bool pasic_stereo_route_set(enum pasic_stereo_route route) {
	uint8_t value;

	switch (route) {
		case PASIC_STEREO_ROUTE_DISABLED:
			value = 0x00;
			break;
		case PASIC_STEREO_ROUTE_DAC:
			value = 0x0C;
			break;
		default:
			return false;
	}

	return pasic_switch_mux_raw_set(PASIC_SWITCH_MUX_PATH_5, value);
}

bool pasic_line_route_set(enum pasic_line_route route) {
	uint8_t value;

	switch (route) {
		case PASIC_LINE_ROUTE_DISABLED:
			value = 0x00;
			break;
		case PASIC_LINE_ROUTE_MONO:
			value = 0x03;
			break;
		case PASIC_LINE_ROUTE_DIFFERENTIAL_STEREO:
			value = 0x18;
			break;
		default:
			return false;
	}

	return pasic_switch_mux_raw_set(PASIC_SWITCH_MUX_PATH_3, value);
}

bool pasic_adc_sample_rate_set(uint8_t code) {
	if (code > 8)
		return false;
	pasic_register_update(PASIC_SAMPLE_RATE,
		(code << PASIC_SAMPLE_RATE_ADC_SHIFT), PASIC_SAMPLE_RATE_ADC);
	return true;
}

bool pasic_dac_sample_rate_set(uint8_t code) {
	if (code > 8)
		return false;
	pasic_register_update(PASIC_SAMPLE_RATE,
		(code << PASIC_SAMPLE_RATE_DAC_SHIFT), PASIC_SAMPLE_RATE_DAC);
	return true;
}
