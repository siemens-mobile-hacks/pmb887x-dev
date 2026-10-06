#include <pmb887x.h>
#include <gen/dsp.h>
#include <gen/peripheral/pmic/PASIC.h>
#include <stopwatch.h>

#include "dsp/audio/board/siemens/commands.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/board/siemens/siemens.h"
#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"

static const uint16_t MASK_IDS[] = { 0x0800, 0x0801 };
static const uint8_t SPEAKER_GAIN_CODES[] = {
	0x39, 0x18, 0x16, 0x14, 0x12, 0x10, 0x0E, 0x0C,
	0x0A, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02,
};

static bool runtime_configure(void);
static bool runtime_output_configure(void);
static bool audio_start(void);

static const uint16_t JTAB_RESET[] = {
	0x0001, 0x0001,
};

static bool job_table_reset(void) {
	return dsp_siemens_jtab_exec(1, JTAB_RESET, ARRAY_SIZE(JTAB_RESET));
}

static const uint16_t JTAB_CONFIGURE[] = {
	0x0002, 0x6016, 0x1430,
	0x000B, 0x6108, 0x8574, 0x8104, 0x7A51, 0x7DFE,
	0x7AC7, 0xC96C, 0xE7B4, 0x3AD7, 0x1E7D, 0x7FFF,
	0x000A, 0x6260, 0x01C2, 0xFF38, 0x0025, 0x8CCC,
	0x0198, 0x032F, 0x7F8B, 0x0001, 0x0005,
	0x0002, 0x6317, 0x32B7,
	0x000B, 0x640A, 0xCD78, 0xEB85, 0x3F1C, 0x1BAA,
	0x65E1, 0xBF98, 0xE560, 0x2F3F, 0x0658, 0x782A,
	0x0002, 0x6515, 0x7FFF,
	0x0015, 0x6659, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x7FFF, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x2000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000,
	0x0003, 0x675E, 0xFFFF, 0x0005,
	0x0002, 0x6806, 0x0000,
	0x0001, 0x0000,
	0x0002, 0x0107, 0x0000,
	0x0002, 0x0203, 0x0003,
	0x000B, 0x400C, 0xB087, 0xBA16, 0x4F79, 0x320E,
	0x4F79, 0xAFE0, 0xAEDC, 0x5C05, 0x4DFC, 0x6FEF,
	0x0002, 0x4104, 0x0003,
	0x0001, 0x803F,
};

static bool job_table_configure(void) {
	return dsp_siemens_jtab_exec(1, JTAB_CONFIGURE, ARRAY_SIZE(JTAB_CONFIGURE));
}

static bool pcm_idle(void) {
	static const uint16_t PARAMETERS[] = { 0x0280, 0xD6DB, 0x0000, 0x0400 };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		PARAMETERS, ARRAY_SIZE(PARAMETERS));
}

static bool runtime_initialize(void) {
	if (!dsp_siemens_cmd_exec(0, DSP_COMMAND_IDLE, NULL, 0))
		return false;

	static const uint16_t DCONF_AUX[] = { 0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000, 0x7FFF, 0x0000 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_AUX,
		DCONF_AUX, ARRAY_SIZE(DCONF_AUX))) {
		return false;
	}

	static const uint16_t DSET_VOL[] = { 0x0000, 0x0000, 0x0010, 0x0001 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL,
		DSET_VOL, ARRAY_SIZE(DSET_VOL))) {
		return false;
	}

	static const uint16_t DSET_MIC_MODE[] = { 0x0001 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DSET_MIC_MODE,
		DSET_MIC_MODE, ARRAY_SIZE(DSET_MIC_MODE))) {
		return false;
	}
	if (!job_table_reset())
		return false;

	static const uint16_t MCU_INT[] = { 0x0000, 0x0000, 0x0000 };
	if (!dsp_siemens_cmd_exec(1, DSP_COMMAND_MCU_INT, MCU_INT, ARRAY_SIZE(MCU_INT)))
		return false;

	static const uint16_t DCONF_PCMA[] = { 0x0240, 0xD6DB, 0x0000, 0x0000 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		DCONF_PCMA, ARRAY_SIZE(DCONF_PCMA))) {
		return false;
	}

	static const uint16_t USER_15[] = { 0x0140, 0x4CAD, 0x0000, 0x05DD };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_USER_15, USER_15, ARRAY_SIZE(USER_15));
}

static bool i2s_initialize(void) {
	static const uint16_t PROFILE_A[] = {
		0x0000, 0x8004, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0659, 0x0015, 0x0710, 0x0808, 0x0000, 0x0047, 0x1010,
		0x0659, 0x1010, 0x0659, 0x0015, 0x0710, 0x0808, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		PROFILE_A, ARRAY_SIZE(PROFILE_A))) {
		return false;
	}

	static const uint16_t PROFILE_B[] = {
		0x0000, 0x8002, 0x0000, 0x0047, 0x1010, 0x0659, 0x1010,
		0x0659, 0x0015, 0x0710, 0x0808, 0x0000, 0x0047, 0x1010,
		0x0659, 0x1010, 0x0659, 0x0015, 0x0710, 0x0808, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		PROFILE_B, ARRAY_SIZE(PROFILE_B))) {
		return false;
	}

	static const uint16_t PROFILE_C[] = {
		0x0000, 0x8001, 0x0000, 0x0047, 0x1010, 0x0659, 0x1010,
		0x0659, 0x0015, 0x0710, 0x0808, 0x0000, 0x0047, 0x1010,
		0x0659, 0x1010, 0x0659, 0x0015, 0x0710, 0x0808, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		PROFILE_C, ARRAY_SIZE(PROFILE_C))) {
		return false;
	}

	static const uint16_t PROFILE_D[] = {
		0x0000, 0x2007, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
	};
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		PROFILE_D, ARRAY_SIZE(PROFILE_D));
}

static bool audio_profile_configure(void) {
	if (!i2s_initialize())
		return false;

	static const uint16_t DCONF_LIB_INITIALIZE[] = {
		0x0001, 0x0000, 0x0001, 0x0000, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0000, 0x0000, 0x0000, 0x0000, 0x0004, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB,
		DCONF_LIB_INITIALIZE, ARRAY_SIZE(DCONF_LIB_INITIALIZE))) {
		return false;
	}

	static const uint16_t USER_14_A[] = { 0x0008, 0x0000 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_USER_14, USER_14_A, ARRAY_SIZE(USER_14_A)))
		return false;

	static const uint16_t USER_14_B[] = { 0x0004, 0x0000 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_USER_14, USER_14_B, ARRAY_SIZE(USER_14_B)))
		return false;

	static const uint16_t DSET_SCAL[] = { 0x2000, 0x2000 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_SCAL, DSET_SCAL, ARRAY_SIZE(DSET_SCAL)))
		return false;

	static const uint16_t DCONF_LIB[] = {
		0x0004, 0x003E, 0x0010, 0x8008, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0000, 0x06A5, 0x0004, 0x0000, 0x0004, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB, DCONF_LIB, ARRAY_SIZE(DCONF_LIB)))
		return false;
	if (!pcm_idle())
		return false;
	if (!job_table_configure())
		return false;

	static const uint16_t DCONF_AUX[] = { 0, 0, 0, 0, 0, 0, 0 };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_AUX,
		DCONF_AUX, ARRAY_SIZE(DCONF_AUX));
}

static bool pasic_output_configure(void) {
	pasic_register_write(PASIC_I2S_RX_CONTROL, 0x72);
	pasic_register_write(PASIC_STEREO_MODE, 0x03);
	if (!pasic_dac_sample_rate_set(6))
		return false;
	pasic_register_write(PASIC_DAC_CONTROL, 0x72);
	if (!pasic_mono_route_set(PASIC_MONO_ROUTE_DAC))
		return false;
	pasic_register_write(PASIC_MONO_CONTROL, 0x02);
	return pasic_output_gain_percent_set(PASIC_AMPLIFIER_PATH_4,
		&SIEMENS_AUDIO_BOARD.speaker_gain, PASIC_OUTPUT_GAIN_PERCENT);
}

/*
 * Hands-free AFE route: the multimedia job graph plays the PCM ring at the AFE rate and its output stage
 * writes the AFE DAC ring. The AFE line output reaches the loudspeaker through PASIC mux 3.
 */
static bool afe_configure(void) {
	ADC_CLC = (1 << MOD_CLC_RMC_SHIFT) | MOD_CLC_EDIS;
	ADC_ANA_CTRL |= ADC_ANA_CTRL_BG_PWUP;
	stopwatch_usleep_wd(150);
	if (!runtime_configure())
		return false;
	pasic_output_enable(
		PASIC_SUPPLY_MODE_VAUDREGA_MODE | PASIC_SUPPLY_MODE_VAUDREGB_MODE,
		PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN | PASIC_SUPPLY_ENABLE_1_VAUDREGB_EN,
		0, 25);

	/* Multimedia library profile with no input sink and the AFE as the output sink. */
	static const uint16_t DCONF_LIB_AFE[] = {
		0x0004, 0x003E, 0x0010, 0x8008, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0000, 0x06A5, 0x0000, 0x0000, 0x0006, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB,
		DCONF_LIB_AFE, ARRAY_SIZE(DCONF_LIB_AFE))) {
		return false;
	}

	/* Enable VBRX/VBTX; AFE at 8 kHz with the RX DAC, line and earpiece buffers on, microphone off. */
	static const uint16_t AFE_PROFILE[23] = {
		0x0060, 0x8004, 0x0000, 0x0023, 0x4A80, 0x00E3, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S, AFE_PROFILE, ARRAY_SIZE(AFE_PROFILE)))
		return false;
	static const uint16_t AFE_START[23] = { 0x0000, 0x4000 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S, AFE_START, ARRAY_SIZE(AFE_START)))
		return false;

	/* The stop-script volume leaves the job graph output gain at zero. */
	static const uint16_t DSET_VOL[] = { 0x4000, 0x0000, 0x0010, 0x000B };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL, DSET_VOL, ARRAY_SIZE(DSET_VOL)))
		return false;

	if (!pasic_line_route_set(PASIC_LINE_ROUTE_MONO))
		return false;
	pasic_register_write(PASIC_MONO_CONTROL, 0x02);
	return pasic_output_gain_percent_set(PASIC_AMPLIFIER_PATH_4,
		&SIEMENS_AUDIO_BOARD.speaker_gain, PASIC_OUTPUT_GAIN_PERCENT);
}

static bool afe_stop(void) {
	bool pcm_stopped = pcm_idle();
	bool muted = pasic_amplifier_gain_set(PASIC_AMPLIFIER_PATH_4, 0x39);
	pasic_register_write(PASIC_MONO_CONTROL, 0x00);
	ADC_ANA_CTRL &= ~ADC_ANA_CTRL_BG_PWUP;
	ADC_CLC = MOD_CLC_DISR;
	return muted && pcm_stopped;
}

static bool pasic_set_mute(void) {
	if (!pasic_amplifier_gain_set(PASIC_AMPLIFIER_PATH_5, 0x00))
		return false;
	pasic_register_write(PASIC_MONO_CONTROL, 0x00);
	return true;
}

static bool runtime_output_configure(void) {
	static const uint16_t DCONF_2S[] = {
		0x000C, 0x0001, 0x0007, 0x0047, 0x11B9, 0x7EF4,
		0x11B9, 0x1FBD, 0x0015, 0x0710, 0x0808, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		DCONF_2S, ARRAY_SIZE(DCONF_2S))) {
		return false;
	}

	static const uint16_t DSET_VOL[] = { 0x4000, 0x0000, 0x0010, 0x000B };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL,
		DSET_VOL, ARRAY_SIZE(DSET_VOL))) {
		return false;
	}

	static const uint16_t DSET_MIC_MODE[] = { 0x0000, 0x0000 };
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_MIC_MODE,
		DSET_MIC_MODE, ARRAY_SIZE(DSET_MIC_MODE));
}

static bool runtime_configure(void) {
	if (!runtime_initialize())
		return false;
	return audio_profile_configure();
}

static bool audio_configure(void) {
	if (!runtime_configure())
		return false;
	pasic_output_enable(
		PASIC_SUPPLY_MODE_VAUDREGA_MODE | PASIC_SUPPLY_MODE_VAUDREGB_MODE,
		PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN | PASIC_SUPPLY_ENABLE_1_VAUDREGB_EN,
		0, 25);
	if (!pasic_output_configure())
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
	.name = "Siemens EL71",
	.mask_ids = MASK_IDS,
	.mask_id_count = ARRAY_SIZE(MASK_IDS),
	.buffer_word_offset = 0xD6DB - TEAK_SHARED_RAM_BASE,
	.block_words = 640,
	.buffer_channels = 1,
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
	.afe = {
		.sample_rate_hz = 8000,
		.configure = afe_configure,
		.start = audio_start,
		.stop = afe_stop,
	},
};
