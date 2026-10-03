#include <pmb887x.h>
#include <gen/dsp.h>
#include <gen/peripheral/pmic/PASIC.h>

#include "dsp/audio/board/siemens/commands.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/board/siemens/x85.h"
#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"

static const uint16_t MASK_IDS[] = { 0x0801 };

static bool pcm_idle(void) {
	static const uint16_t parameters[] = { 0x0280, 0xD6DB, 0x0000, 0x0400 };
	return dsp_siemens_cmd_exec(2, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		parameters, ARRAY_SIZE(parameters));
}

static const struct dconf_jtab_record JTAB_PREPARE_RECORDS[] = {
	DCONF_JTAB_RECORD(0x0006, 0x600D, 0xBBCB, 0x952B, 0x4D04, 0x7A14, 0x5036),
	DCONF_JTAB_RECORD(0x0006, 0x610E, 0xBBCB, 0x952B, 0x4D04, 0x7A14, 0x5036),
	DCONF_JTAB_RECORD(
		0x000B, 0x6208, 0x9CC9, 0xBBB2, 0x78ED, 0x323F,
		0x7FFF, 0x864A, 0x8671, 0x79B6, 0x73BC, 0x79B6
	),
	DCONF_JTAB_RECORD(
		0x000B, 0x6309, 0x9CC9, 0xBBB2, 0x78ED, 0x323F,
		0x7FFF, 0x864A, 0x8671, 0x79B6, 0x73BC, 0x79B6
	),
	DCONF_JTAB_RECORD(
		0x000A, 0x6460, 0x01C2, 0xFF74, 0x0025, 0x4000,
		0x0198, 0x032F, 0x7F8B, 0x0001, 0x0005
	),
	DCONF_JTAB_RECORD(0x0001, 0x6502),
	DCONF_JTAB_RECORD(0x0002, 0x6617, 0x32B7),
	DCONF_JTAB_RECORD(0x0002, 0x6717, 0x32B7),
	DCONF_JTAB_RECORD(0x0002, 0x68DB, 0xAC44),
	DCONF_JTAB_RECORD(0x0001, 0x6982),
	DCONF_JTAB_RECORD(
		0x000D, 0x6AB9, 0x0BFE, 0x3000, 0x0005, 0x0065, 0x0000,
		0xFE70, 0x4000, 0x00A0, 0x001E, 0xFE84, 0x0010, 0x0006
	),
	DCONF_JTAB_RECORD(0x0001, 0x6BBA),
	DCONF_JTAB_RECORD(0x0001, 0x6CBB),
	DCONF_JTAB_RECORD(0x0001, 0x6DBC),
	DCONF_JTAB_RECORD(
		0x0015, 0x6ED9, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x7FFF, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x2000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000
	),
	DCONF_JTAB_RECORD(
		0x0015, 0x6FDA, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x7FFF, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x2000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x7FFF, 0x0000
	),
	DCONF_JTAB_RECORD(0x0003, 0x70DE, 0xFFFF, 0x0005),
	DCONF_JTAB_RECORD(0x0001, 0x71DF),
	DCONF_JTAB_RECORD(0x0002, 0x7286, 0x0000),
	DCONF_JTAB_RECORD(0x0001, 0x0000),
	DCONF_JTAB_RECORD(0x0002, 0x0107, 0x0000),
	DCONF_JTAB_RECORD(0x0002, 0x0218, 0x0333),
	DCONF_JTAB_RECORD(0x0002, 0x0303, 0x0003),
	DCONF_JTAB_RECORD(
		0x000B, 0x400C, 0x8000, 0xBA16, 0x7FFF, 0x320E,
		0x7FFF, 0xA460, 0xAEDC, 0x693A, 0x4DFC, 0x7FFF
	),
};

static bool job_table_prepare(void) {
	return dsp_siemens_jtab_exec(1, JTAB_PREPARE_RECORDS, ARRAY_SIZE(JTAB_PREPARE_RECORDS));
}

static const struct dconf_jtab_record JTAB_FINISH_RECORDS[] = {
	DCONF_JTAB_RECORD(0x0002, 0x4104, 0x0003),
	DCONF_JTAB_RECORD(0x0001, 0x803F),
};

static bool job_table_finish(void) {
	return dsp_siemens_jtab_exec(1, JTAB_FINISH_RECORDS, ARRAY_SIZE(JTAB_FINISH_RECORDS));
}

static bool runtime_prepare(void) {
	if (!dsp_siemens_cmd_exec(0, DSP_COMMAND_IDLE, NULL, 0))
		return false;

	static const uint16_t dset_scal[] = { 0x1D9A, 0x2000 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_SCAL,
		dset_scal, ARRAY_SIZE(dset_scal))) {
		return false;
	}

	static const uint16_t dconf_lib[] = {
		0x0004, 0x003E, 0x0020, 0x8008, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0001, 0x0400, 0x0000, 0x0000, 0x0004, 0x0005,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB,
		dconf_lib, ARRAY_SIZE(dconf_lib))) {
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
	if (!pasic_supply_mode_set(0x06))
		return false;
	if (!pasic_supply_enable_1_set(0x15))
		return false;
	if (!pasic_supply_enable_2_set(0x0D))
		return false;
	if (!pasic_audio_controls_reset())
		return false;
	if (!pasic_i2s_rx_control_set(0x72))
		return false;
	if (!pasic_stereo_mode_set(0x03))
		return false;
	if (!pasic_dac_sample_rate_set(7))
		return false;
	if (!pasic_dac_control_set(0x72))
		return false;
	return pasic_switch_mux_set(PASIC_SWITCH_MUX_PATH_4, 0x03);
}

static bool pasic_output_unmute(void) {
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
		0x000C, 0x0001, 0x0007, 0x0047, 0x11B9, 0x7EF4, 0x11B9,
		0x1FBD, 0x0015, 0x0010, 0x0808, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		dconf_2s, ARRAY_SIZE(dconf_2s))) {
		return false;
	}

	static const uint16_t dset_vol[] = { 0x4000, 0x0000, 0x0010, 0x0009 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL,
		dset_vol, ARRAY_SIZE(dset_vol))) {
		return false;
	}
	if (!pasic_output_unmute())
		return false;

	static const uint16_t dset_mic_mode[] = { 0x0000, 0x0000 };
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_MIC_MODE,
		dset_mic_mode, ARRAY_SIZE(dset_mic_mode));
}

static bool audio_configure(void) {
	if (!runtime_prepare())
		return false;
	if (!pasic_initialize())
		return false;
	if (!pasic_light_control_set(0xBA))
		return false;
	if (!pasic_output_enable(
		PASIC_SUPPLY_MODE_VAUDREGA_MODE | PASIC_SUPPLY_MODE_VAUDREGB_MODE,
		PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN,
		PASIC_SUPPLY_ENABLE_2_VBOOST_EN,
		25
	)) {
		return false;
	}
	if (!pasic_output_prepare())
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

static const struct siemens_x85_audio_route E71_AUDIO_ROUTE = {
	.mask_ids = MASK_IDS,
	.mask_id_count = ARRAY_SIZE(MASK_IDS),
	.source_sample_rate_hz = 44100,
	.i2s_max_large_steps = 1,
	.buffer_offset = 0xD6DB - TEAK_SHARED_RAM_BASE,
	.block_words = 640,
	.buffer_channels = 2,
	.response_flag = BIT(8),
	.configure = audio_configure,
	.start = audio_start,
	.stop = audio_stop,
};

const struct siemens_x85_audio_board siemens_x85_audio_board = {
	.name = "Siemens E71",
	.route = &E71_AUDIO_ROUTE,
};
