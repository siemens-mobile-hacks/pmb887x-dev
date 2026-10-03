#include <pmb887x.h>
#include <gen/dsp.h>
#include <gen/peripheral/pmic/PASIC.h>

#include "dsp/audio/board/siemens/commands.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/board/siemens/x85.h"
#include "dsp/runtime.h"

static const uint16_t MASK_IDS[] = { 0x0800, 0x0801 };

static const struct dconf_jtab_record JTAB_RESET_RECORDS[] = {
	DCONF_JTAB_RECORD(0x0001, 0x0001),
};

static bool job_table_reset(void) {
	return dsp_siemens_jtab_exec(1, JTAB_RESET_RECORDS, ARRAY_SIZE(JTAB_RESET_RECORDS));
}

static const struct dconf_jtab_record JTAB_CONFIGURE_RECORDS[] = {
	DCONF_JTAB_RECORD(0x0002, 0x6016, 0x1430),
	DCONF_JTAB_RECORD(
		0x000B, 0x6108, 0x8574, 0x8104, 0x7A51, 0x7DFE,
		0x7AC7, 0xC96C, 0xE7B4, 0x3AD7, 0x1E7D, 0x7FFF
	),
	DCONF_JTAB_RECORD(
		0x000A, 0x6260, 0x01C2, 0xFF38, 0x0025, 0x8CCC,
		0x0198, 0x032F, 0x7F8B, 0x0001, 0x0005
	),
	DCONF_JTAB_RECORD(0x0002, 0x6317, 0x32B7),
	DCONF_JTAB_RECORD(
		0x000B, 0x640A, 0xCD78, 0xEB85, 0x3F1C, 0x1BAA,
		0x65E1, 0xBF98, 0xE560, 0x2F3F, 0x0658, 0x782A
	),
	DCONF_JTAB_RECORD(0x0002, 0x6515, 0x7FFF),
	DCONF_JTAB_RECORD(
		0x0015, 0x6659, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x7FFF, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x2000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000
	),
	DCONF_JTAB_RECORD(0x0003, 0x675E, 0xFFFF, 0x0005),
	DCONF_JTAB_RECORD(0x0002, 0x6806, 0x0000),
	DCONF_JTAB_RECORD(0x0001, 0x0000),
	DCONF_JTAB_RECORD(0x0002, 0x0107, 0x0000),
	DCONF_JTAB_RECORD(0x0002, 0x0203, 0x0003),
	DCONF_JTAB_RECORD(
		0x000B, 0x400C, 0xB087, 0xBA16, 0x4F79, 0x320E,
		0x4F79, 0xAFE0, 0xAEDC, 0x5C05, 0x4DFC, 0x6FEF
	),
	DCONF_JTAB_RECORD(0x0002, 0x4104, 0x0003),
	DCONF_JTAB_RECORD(0x0001, 0x803F),
};

static bool job_table_configure(void) {
	return dsp_siemens_jtab_exec(1, JTAB_CONFIGURE_RECORDS, ARRAY_SIZE(JTAB_CONFIGURE_RECORDS));
}

static bool pcm_idle(void) {
	static const uint16_t parameters[] = { 0x0280, 0xD6DB, 0x0000, 0x0400 };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		parameters, ARRAY_SIZE(parameters));
}

static bool runtime_initialize(void) {
	if (!dsp_siemens_cmd_exec(0, DSP_COMMAND_IDLE, NULL, 0))
		return false;

	static const uint16_t dconf_aux[] = { 0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000, 0x7FFF, 0x0000 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_AUX,
		dconf_aux, ARRAY_SIZE(dconf_aux))) {
		return false;
	}

	static const uint16_t dset_vol[] = { 0x0000, 0x0000, 0x0010, 0x0001 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL,
		dset_vol, ARRAY_SIZE(dset_vol))) {
		return false;
	}

	static const uint16_t dset_mic_mode[] = { 0x0001 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DSET_MIC_MODE,
		dset_mic_mode, ARRAY_SIZE(dset_mic_mode))) {
		return false;
	}
	if (!job_table_reset())
		return false;

	static const uint16_t mcu_int[] = { 0x0000, 0x0000, 0x0000 };
	if (!dsp_siemens_cmd_exec(1, DSP_COMMAND_MCU_INT, mcu_int, ARRAY_SIZE(mcu_int)))
		return false;

	static const uint16_t dconf_pcma[] = { 0x0240, 0xD6DB, 0x0000, 0x0000 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		dconf_pcma, ARRAY_SIZE(dconf_pcma))) {
		return false;
	}

	static const uint16_t user_15[] = { 0x0140, 0x4CAD, 0x0000, 0x05DD };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_USER_15, user_15, ARRAY_SIZE(user_15));
}

static bool i2s_initialize(void) {
	static const uint16_t profile_a[] = {
		0x0000, 0x8004, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0659, 0x0015, 0x0710, 0x0808, 0x0000, 0x0047, 0x1010,
		0x0659, 0x1010, 0x0659, 0x0015, 0x0710, 0x0808, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		profile_a, ARRAY_SIZE(profile_a))) {
		return false;
	}

	static const uint16_t profile_b[] = {
		0x0000, 0x8002, 0x0000, 0x0047, 0x1010, 0x0659, 0x1010,
		0x0659, 0x0015, 0x0710, 0x0808, 0x0000, 0x0047, 0x1010,
		0x0659, 0x1010, 0x0659, 0x0015, 0x0710, 0x0808, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		profile_b, ARRAY_SIZE(profile_b))) {
		return false;
	}

	static const uint16_t profile_c[] = {
		0x0000, 0x8001, 0x0000, 0x0047, 0x1010, 0x0659, 0x1010,
		0x0659, 0x0015, 0x0710, 0x0808, 0x0000, 0x0047, 0x1010,
		0x0659, 0x1010, 0x0659, 0x0015, 0x0710, 0x0808, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		profile_c, ARRAY_SIZE(profile_c))) {
		return false;
	}

	static const uint16_t profile_d[] = {
		0x0000, 0x2007, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
	};
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		profile_d, ARRAY_SIZE(profile_d));
}

static bool audio_profile_configure(void) {
	if (!i2s_initialize())
		return false;

	static const uint16_t dconf_lib_initialize[] = {
		0x0001, 0x0000, 0x0001, 0x0000, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0000, 0x0000, 0x0000, 0x0000, 0x0004, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB,
		dconf_lib_initialize, ARRAY_SIZE(dconf_lib_initialize))) {
		return false;
	}

	static const uint16_t user_14_a[] = { 0x0008, 0x0000 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_USER_14, user_14_a, ARRAY_SIZE(user_14_a)))
		return false;

	static const uint16_t user_14_b[] = { 0x0004, 0x0000 };
	if (!dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_USER_14, user_14_b, ARRAY_SIZE(user_14_b)))
		return false;

	static const uint16_t dset_scal[] = { 0x2000, 0x2000 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_SCAL, dset_scal, ARRAY_SIZE(dset_scal)))
		return false;

	static const uint16_t dconf_lib[] = {
		0x0004, 0x003E, 0x0010, 0x8008, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0000, 0x06A5, 0x0004, 0x0000, 0x0004, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB, dconf_lib, ARRAY_SIZE(dconf_lib)))
		return false;
	if (!pcm_idle())
		return false;
	if (!job_table_configure())
		return false;

	static const uint16_t dconf_aux[] = { 0, 0, 0, 0, 0, 0, 0 };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_AUX,
		dconf_aux, ARRAY_SIZE(dconf_aux));
}

static bool pasic_output_configure(void) {
	if (!pasic_i2s_rx_control_set(0x72))
		return false;
	if (!pasic_stereo_mode_set(0x03))
		return false;
	if (!pasic_dac_sample_rate_set(6))
		return false;
	if (!pasic_dac_control_set(0x72))
		return false;
	if (!pasic_switch_mux_set(PASIC_SWITCH_MUX_PATH_4, 0x03))
		return false;
	if (!pasic_mono_control_set(0x02))
		return false;
	return pasic_amplifier_gain_set(PASIC_AMPLIFIER_PATH_4, 0x07);
}

static bool pasic_set_mute(void) {
	if (!pasic_amplifier_gain_set(PASIC_AMPLIFIER_PATH_5, 0x00))
		return false;
	return pasic_mono_control_set(0x00);
}

static bool runtime_output_configure(void) {
	static const uint16_t dconf_2s[] = {
		0x000C, 0x0001, 0x0007, 0x0047, 0x11B9, 0x7EF4,
		0x11B9, 0x1FBD, 0x0015, 0x0710, 0x0808, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		dconf_2s, ARRAY_SIZE(dconf_2s))) {
		return false;
	}

	static const uint16_t dset_vol[] = { 0x4000, 0x0000, 0x0010, 0x000B };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL,
		dset_vol, ARRAY_SIZE(dset_vol))) {
		return false;
	}

	static const uint16_t dset_mic_mode[] = { 0x0000, 0x0000 };
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_MIC_MODE,
		dset_mic_mode, ARRAY_SIZE(dset_mic_mode));
}

static bool audio_configure(void) {
	if (!runtime_initialize())
		return false;
	if (!audio_profile_configure())
		return false;
	if (!pasic_output_enable(
		PASIC_SUPPLY_MODE_VAUDREGA_MODE | PASIC_SUPPLY_MODE_VAUDREGB_MODE,
		PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN | PASIC_SUPPLY_ENABLE_1_VAUDREGB_EN,
		0, 25)) {
		return false;
	}
	if (!pasic_output_configure())
		return false;
	return runtime_output_configure();
}

static bool audio_start(void) {
	static const uint16_t parameters[] = { 0x0280, 0xD6DB, 0x0280, 0x0400 };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		parameters, ARRAY_SIZE(parameters));
}

static bool audio_stop(void) {
	bool stopped = pcm_idle();
	bool muted = pasic_set_mute();
	return stopped && muted;
}

static const struct siemens_x85_audio_route EL71_AUDIO_ROUTE = {
	.mask_ids = MASK_IDS,
	.mask_id_count = ARRAY_SIZE(MASK_IDS),
	.source_sample_rate_hz = 44100,
	// EL71 post-processing may introduce four half-range steps per 32-sample TX snapshot.
	.i2s_max_large_steps = 4,
	.buffer_offset = 0xD6DB - TEAK_SHARED_RAM_BASE,
	.block_words = 640,
	.buffer_channels = 1,
	.response_flag = BIT(8),
	.configure = audio_configure,
	.start = audio_start,
	.stop = audio_stop,
};

const struct siemens_x85_audio_board siemens_x85_audio_board = {
	.name = "Siemens EL71",
	.route = &EL71_AUDIO_ROUTE,
};
