#include <pmb887x.h>
#include <gen/dsp.h>
#include <gen/peripheral/pmic/PASIC.h>

#include "dsp/audio/board/siemens/commands.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/board/siemens/siemens.h"
#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"

static const uint16_t MASK_IDS[] = { 0x0801 };
static const uint8_t SPEAKER_GAIN_CODES[] = {
	0x39, 0x18, 0x16, 0x14, 0x12, 0x10, 0x0E, 0x0C,
	0x0A, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02,
};

static bool pcm_idle(void) {
	static const uint16_t PARAMETERS[] = { 0x0280, 0xD6DB, 0x0000, 0x0400 };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		PARAMETERS, ARRAY_SIZE(PARAMETERS));
}

static const uint16_t JTAB_PREPARE[] = {
	0x0006, 0x600D, 0xBBCB, 0x952B, 0x4D04, 0x7A14, 0x5036,
	0x0006, 0x610E, 0xBBCB, 0x952B, 0x4D04, 0x7A14, 0x5036,
	0x000B, 0x6208, 0x9CC9, 0xBBB2, 0x78ED, 0x323F,
	0x7FFF, 0x864A, 0x8671, 0x79B6, 0x73BC, 0x79B6,
	0x000B, 0x6309, 0x9CC9, 0xBBB2, 0x78ED, 0x323F,
	0x7FFF, 0x864A, 0x8671, 0x79B6, 0x73BC, 0x79B6,
	0x000A, 0x6460, 0x01C2, 0xFF74, 0x0025, 0x4000,
	0x0198, 0x032F, 0x7F8B, 0x0001, 0x0005,
	0x0001, 0x6502,
	0x0002, 0x6617, 0x32B7,
	0x0002, 0x6717, 0x32B7,
	0x0002, 0x68DB, 0xAC44,
	0x0001, 0x6982,
	0x000D, 0x6AB9, 0x0BFE, 0x3000, 0x0005, 0x0065, 0x0000,
	0xFE70, 0x4000, 0x00A0, 0x001E, 0xFE84, 0x0010, 0x0006,
	0x0001, 0x6BBA,
	0x0001, 0x6CBB,
	0x0001, 0x6DBC,
	0x0015, 0x6ED9, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x7FFF, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x2000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000,
	0x0015, 0x6FDA, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x7FFF, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x2000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000,
	0x0003, 0x70DE, 0xFFFF, 0x0005,
	0x0001, 0x71DF,
	0x0002, 0x7286, 0x0000,
	0x0001, 0x0000,
	0x0002, 0x0107, 0x0000,
	0x0002, 0x0218, 0x0333,
	0x0002, 0x0303, 0x0003,
	0x000B, 0x400C, 0x8000, 0xBA16, 0x7FFF, 0x320E,
	0x7FFF, 0xA460, 0xAEDC, 0x693A, 0x4DFC, 0x7FFF,
};

static bool job_table_prepare(void) {
	return dsp_siemens_jtab_exec(1, JTAB_PREPARE, ARRAY_SIZE(JTAB_PREPARE));
}

static const uint16_t JTAB_FINISH[] = {
	0x0002, 0x4104, 0x0003,
	0x0001, 0x803F,
};

static bool job_table_finish(void) {
	return dsp_siemens_jtab_exec(1, JTAB_FINISH, ARRAY_SIZE(JTAB_FINISH));
}

static bool runtime_prepare(void) {
	if (!dsp_siemens_cmd_exec(0, DSP_COMMAND_IDLE, NULL, 0))
		return false;

	static const uint16_t DSET_SCAL[] = { 0x1D9A, 0x2000 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_SCAL,
		DSET_SCAL, ARRAY_SIZE(DSET_SCAL))) {
		return false;
	}

	static const uint16_t DCONF_LIB[] = {
		0x0004, 0x003E, 0x0020, 0x8008, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0001, 0x0400, 0x0000, 0x0000, 0x0004, 0x0005,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB,
		DCONF_LIB, ARRAY_SIZE(DCONF_LIB))) {
		return false;
	}
	if (!pcm_idle())
		return false;
	if (!job_table_prepare())
		return false;
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_AUX, NULL, 0))
		return false;
	return job_table_finish();
}

static bool pasic_output_prepare(void) {
	pasic_register_write(PASIC_SUPPLY_MODE, 0x06);
	pasic_register_write(PASIC_SUPPLY_ENABLE_1, 0x15);
	pasic_register_write(PASIC_SUPPLY_ENABLE_2, 0x0D);
	pasic_audio_controls_reset();
	pasic_register_write(PASIC_I2S_RX_CONTROL, 0x72);
	pasic_register_write(PASIC_STEREO_MODE, 0x03);
	if (!pasic_dac_sample_rate_set(7))
		return false;
	pasic_register_write(PASIC_DAC_CONTROL, 0x72);
	return pasic_mono_route_set(PASIC_MONO_ROUTE_DAC);
}

static bool pasic_output_unmute(void) {
	pasic_register_write(PASIC_MONO_CONTROL, 0x02);
	return pasic_output_gain_percent_set(PASIC_AMPLIFIER_PATH_4,
		&SIEMENS_AUDIO_BOARD.speaker_gain, PASIC_OUTPUT_GAIN_PERCENT);
}

static bool pasic_set_mute(void) {
	if (!pasic_amplifier_gain_set(PASIC_AMPLIFIER_PATH_5, 0x00))
		return false;
	pasic_register_write(PASIC_MONO_CONTROL, 0x00);
	return true;
}

static bool runtime_output_configure(void) {
	static const uint16_t DCONF_2S[] = {
		0x000C, 0x0001, 0x0007, 0x0047, 0x11B9, 0x7EF4, 0x11B9,
		0x1FBD, 0x0015, 0x0010, 0x0808, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		DCONF_2S, ARRAY_SIZE(DCONF_2S))) {
		return false;
	}

	static const uint16_t DSET_VOL[] = { 0x4000, 0x0000, 0x0010, 0x0009 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL,
		DSET_VOL, ARRAY_SIZE(DSET_VOL))) {
		return false;
	}
	if (!pasic_output_unmute())
		return false;

	static const uint16_t DSET_MIC_MODE[] = { 0x0000, 0x0000 };
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_MIC_MODE,
		DSET_MIC_MODE, ARRAY_SIZE(DSET_MIC_MODE));
}

static bool audio_configure(void) {
	if (!runtime_prepare())
		return false;
	pasic_register_write(PASIC_LIGHT_CONTROL, 0xBA);
	pasic_output_enable(
		PASIC_SUPPLY_MODE_VAUDREGA_MODE | PASIC_SUPPLY_MODE_VAUDREGB_MODE,
		PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN,
		PASIC_SUPPLY_ENABLE_2_VBOOST_EN,
		25
	);
	if (!pasic_output_prepare())
		return false;
	return runtime_output_configure();
}

static bool audio_start(void) {
	static const uint16_t PARAMETERS[] = { 0x0280, 0xD6DB, 0x0280, 0x0400 };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		PARAMETERS, ARRAY_SIZE(PARAMETERS));
}

static bool audio_stop(void) {
	bool stopped = pcm_idle();
	bool muted = pasic_set_mute();
	return stopped && muted;
}

const struct siemens_audio_board SIEMENS_AUDIO_BOARD = {
	.name = "Siemens E71",
	.mask_ids = MASK_IDS,
	.mask_id_count = ARRAY_SIZE(MASK_IDS),
	.buffer_word_offset = 0xD6DB - TEAK_SHARED_RAM_BASE,
	.block_words = 640,
	.buffer_channels = 2,
	.response_flag = BIT(8),
	.speaker_gain = {
		.codes = SPEAKER_GAIN_CODES,
		.code_count = ARRAY_SIZE(SPEAKER_GAIN_CODES),
	},
	.i2s = {
		.sample_rate_hz = 44100,
		.configure = audio_configure,
		.start = audio_start,
		.stop = audio_stop,
	},
};
