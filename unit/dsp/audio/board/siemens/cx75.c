#include <pmb887x.h>
#include <gen/dsp.h>
#include <gen/peripheral/pmic/PASIC.h>

#include "dsp/audio/board/siemens/commands.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/board/siemens/siemens.h"

static const uint16_t MASK_IDS[] = { 0x0602, 0x0605 };
static const uint8_t SPEAKER_GAIN_CODES[] = {
	0x39, 0x22, 0x20, 0x1E, 0x1C, 0x1A, 0x18, 0x16,
	0x14, 0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08,
};
static const uint16_t JTAB_PREPARE[] = {
	0x0002, 0x0001, 0x0001,
	0x0002, 0x6005, 0x6006,
};

static bool runtime_prepare(void) {
	// CX75 ARM audio route at A0826970 and its parameter block at A0A5624C.
	static const uint16_t DCONF_LIB[] = {
		0x0004, 0x0000, 0x0020, 0x0000, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0000, 0x05DC, 0x0000, 0x0000, 0x0004, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB,
		DCONF_LIB, ARRAY_SIZE(DCONF_LIB))) {
		return false;
	}

	// Audio routing selects the 8 kHz profile at A0A660EC in configuration mode 7.
	static const uint16_t DCONF_2S[] = {
		0x000C, 0x0001, 0x0007, 0x0047, 0x1010, 0x0659,
		0x1010, 0x0659, 0x0015, 0x0714, 0x0808,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		DCONF_2S, ARRAY_SIZE(DCONF_2S))) {
		return false;
	}

	// GenPlayer DConfPcmA record at A0A69440.
	static const uint16_t DCONF_PCMA[] = {
		0x0140, 0xDA2D, 0x00A0, 0x0000, 0x0000, 0x0000, 0x0000, 0x1F40,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		DCONF_PCMA, ARRAY_SIZE(DCONF_PCMA))) {
		return false;
	}

	static const uint16_t DSET_SCAL[] = { 0x2000, 0x2000 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_SCAL,
		DSET_SCAL, ARRAY_SIZE(DSET_SCAL))) {
		return false;
	}

	static const uint16_t DSET_VOL[] = { 0x4000, 0x0000, 0x0005, 0x0003 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL,
		DSET_VOL, ARRAY_SIZE(DSET_VOL))) {
		return false;
	}

	return dsp_siemens_jtab_exec(1, JTAB_PREPARE, ARRAY_SIZE(JTAB_PREPARE));
}

static bool audio_start(void) {
	// CX75 8 kHz I2S profile at A0A660EC; A096EF1C changes only the mode word to 3.
	static const uint16_t DCONF_2S[] = {
		0x000C, 0x0001, 0x0003, 0x0047, 0x1010, 0x0659,
		0x1010, 0x0659, 0x0015, 0x0714, 0x0808,
	};
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		DCONF_2S, ARRAY_SIZE(DCONF_2S));
}

static bool audio_stop(void) {
	static const uint16_t DCONF_PCMA[] = {
		0x0240, 0xFFD9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1F40,
	};
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		DCONF_PCMA, ARRAY_SIZE(DCONF_PCMA));
}

// CX75 firmware state machine at A0826774, in execution order.
static bool pasic_output_configure(void) {
	pasic_register_write(PASIC_MONO_CONTROL, 0x00);
	pasic_register_write(PASIC_STEREO_CONTROL, 0x00);
	pasic_register_write(PASIC_ADC_CONTROL, 0x0C);
	pasic_register_write(PASIC_DAC_CONTROL, 0x0C);
	pasic_register_write(PASIC_I2S_RX_CONTROL, 0x72);
	pasic_register_write(PASIC_I2S_TX_CONTROL, 0x72);
	pasic_register_write(PASIC_STEREO_MODE, 0x03);
	if (!pasic_adc_sample_rate_set(3))
		return false;
	pasic_register_write(PASIC_ADC_CONTROL, 0xF4);
	pasic_register_write(PASIC_DAC_CONTROL, 0x40);
	if (!pasic_push_pull_route_set(PASIC_PUSH_PULL_ROUTE_1, 3))
		return false;
	if (!pasic_push_pull_route_set(PASIC_PUSH_PULL_ROUTE_2, 3))
		return false;
	if (!pasic_output_gain_percent_set(PASIC_AMPLIFIER_PATH_4,
		&SIEMENS_AUDIO_BOARD.speaker_gain, PASIC_OUTPUT_GAIN_PERCENT)) {
		return false;
	}
	pasic_register_write(PASIC_MUX_INPUT, 0x03);
	if (!pasic_amplifier_gain_set(PASIC_AMPLIFIER_PATH_7, 0x0F))
		return false;
	if (!pasic_switch_mux_raw_set(PASIC_SWITCH_MUX_PATH_1, 0x00))
		return false;
	if (!pasic_switch_mux_raw_set(PASIC_SWITCH_MUX_PATH_2, 0x22))
		return false;
	if (!pasic_line_route_set(PASIC_LINE_ROUTE_DISABLED))
		return false;
	if (!pasic_mono_route_set(PASIC_MONO_ROUTE_DAC))
		return false;
	if (!pasic_stereo_route_set(PASIC_STEREO_ROUTE_DISABLED))
		return false;
	pasic_register_write(PASIC_SUPPLY_ENABLE_2, 0x19);
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_PATH_A, false, 8))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_PATH_B, false, 8))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_PATH_C, true, 6))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_PATH_D, true, 6))
		return false;
	pasic_register_write(PASIC_STEREO_CONTROL, 0x01);
	pasic_register_write(PASIC_MONO_CONTROL, 0x01);
	return true;
}

static void pasic_set_mute(void) {
	pasic_register_write(PASIC_MONO_CONTROL, 0x00);
	pasic_register_write(PASIC_STEREO_CONTROL, 0x00);
}

static bool audio_configure(void) {
	if (!runtime_prepare())
		return false;
	pasic_output_enable(
		PASIC_SUPPLY_MODE_VAUDREGA_MODE | PASIC_SUPPLY_MODE_VAUDREGB_MODE,
		PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN | PASIC_SUPPLY_ENABLE_1_VAUDREGB_EN,
		0, 25);
	return pasic_output_configure();
}

static bool audio_stop_and_mute(void) {
	bool stopped = audio_stop();
	pasic_set_mute();
	return stopped;
}

const struct siemens_audio_board SIEMENS_AUDIO_BOARD = {
	.name = "Siemens CX75",
	.mask_ids = MASK_IDS,
	.mask_id_count = ARRAY_SIZE(MASK_IDS),
	.buffer_word_offset = 0xDA2D - TEAK_SHARED_RAM_BASE,
	// A096EEA4 copies 0x140 16-bit words to D:DA2D before raising communication flag 8.
	.block_words = 0x0140,
	.buffer_channels = 1,
	.response_flag = BIT(8),
	.speaker_gain = {
		.codes = SPEAKER_GAIN_CODES,
		.code_count = ARRAY_SIZE(SPEAKER_GAIN_CODES),
	},
	.i2s = {
		.sample_rate_hz = 8000,
		.configure = audio_configure,
		.start = audio_start,
		.stop = audio_stop_and_mute,
	},
};
