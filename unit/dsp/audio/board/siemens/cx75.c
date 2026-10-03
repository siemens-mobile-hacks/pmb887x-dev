#include <pmb887x.h>
#include <gen/peripheral/pmic/PASIC.h>

#include "dsp/audio/board/siemens/commands.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/board/siemens/r65.h"

static const uint16_t MASK_IDS[] = { 0x0602, 0x0605 };
static const struct dconf_jtab_record JTAB_PREPARE_RECORDS[] = {
	DCONF_JTAB_RECORD(0x0002, 0x0001, 0x0001),
	DCONF_JTAB_RECORD(0x0002, 0x6005, 0x6006),
};

static bool prepare(void) {
	// CX75 ARM audio route at A0826970 and its parameter block at A0A5624C.
	static const uint16_t dconf_lib[] = {
		0x0004, 0x0000, 0x0020, 0x0000, 0x00A0, 0x00A0, 0x00A0,
		0x0960, 0x0000, 0x05DC, 0x0000, 0x0000, 0x0004, 0x0000,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_LIB,
		dconf_lib, ARRAY_SIZE(dconf_lib))) {
		return false;
	}

	static const uint16_t dconf_pcma[] = {
		0x0140, 0x531F, 0x0001, 0x0000, 0x0000, 0x0000, 0x4035, 0x1F40,
	};
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		dconf_pcma, ARRAY_SIZE(dconf_pcma))) {
		return false;
	}

	static const uint16_t dset_scal[] = { 0x2000, 0x2000 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_SCAL,
		dset_scal, ARRAY_SIZE(dset_scal))) {
		return false;
	}

	static const uint16_t dset_vol[] = { 0x4000, 0x0000, 0x0005, 0x0003 };
	if (!dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DSET_VOL,
		dset_vol, ARRAY_SIZE(dset_vol))) {
		return false;
	}

	return dsp_siemens_jtab_exec(1, JTAB_PREPARE_RECORDS, ARRAY_SIZE(JTAB_PREPARE_RECORDS));
}

static bool start(void) {
	static const uint16_t dconf_2s[] = {
		0x000C, 0x0001, 0x0003, 0x0047, 0x1020, 0x0659,
		0x1020, 0x0659, 0x0011, 0x1710, 0x2020,
	};
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_2S,
		dconf_2s, ARRAY_SIZE(dconf_2s));
}

static bool stop(void) {
	static const uint16_t dconf_pcma[] = {
		0x0240, 0xFFD9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1F40,
	};
	return dsp_siemens_cmd_exec(1, SIEMENS_AUDIO_COMMAND_DCONF_PCMA,
		dconf_pcma, ARRAY_SIZE(dconf_pcma));
}

// CX75 firmware state machine at A0826774, in execution order.
static bool pasic_output_configure(void) {
	if (!pasic_mono_control_set(0x00))
		return false;
	if (!pasic_stereo_control_set(0x00))
		return false;
	if (!pasic_adc_control_set(0x0C))
		return false;
	if (!pasic_dac_control_set(0x0C))
		return false;
	if (!pasic_i2s_rx_control_set(0x72))
		return false;
	if (!pasic_i2s_tx_control_set(0x72))
		return false;
	if (!pasic_stereo_mode_set(0x03))
		return false;
	if (!pasic_adc_sample_rate_set(3))
		return false;
	if (!pasic_adc_control_set(0xF4))
		return false;
	if (!pasic_dac_control_set(0x40))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_ROUTE_1, true, 3))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_ROUTE_2, true, 3))
		return false;
	if (!pasic_amplifier_gain_set(PASIC_AMPLIFIER_PATH_4, 0x0F))
		return false;
	if (!pasic_mux_input_set(0x03))
		return false;
	if (!pasic_amplifier_gain_set(PASIC_AMPLIFIER_PATH_7, 0x0F))
		return false;
	if (!pasic_switch_mux_set(PASIC_SWITCH_MUX_PATH_1, 0x00))
		return false;
	if (!pasic_switch_mux_set(PASIC_SWITCH_MUX_PATH_2, 0x22))
		return false;
	if (!pasic_switch_mux_set(PASIC_SWITCH_MUX_PATH_3, 0x00))
		return false;
	if (!pasic_switch_mux_set(PASIC_SWITCH_MUX_PATH_4, 0x03))
		return false;
	if (!pasic_switch_mux_set(PASIC_SWITCH_MUX_PATH_5, 0x00))
		return false;
	if (!pasic_supply_enable_2_set(0x19))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_PATH_A, false, 8))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_PATH_B, false, 8))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_PATH_C, true, 6))
		return false;
	if (!pasic_push_pull_configure(PASIC_PUSH_PULL_PATH_D, true, 6))
		return false;
	if (!pasic_stereo_control_set(0x01))
		return false;
	return pasic_mono_control_set(0x01);
}

static bool pasic_set_mute(void) {
	if (!pasic_mono_control_set(0x00))
		return false;
	return pasic_stereo_control_set(0x00);
}

const struct siemens_r65_audio_board siemens_r65_audio_board = {
	.name = "Siemens CX75",
	.mask_ids = MASK_IDS,
	.mask_id_count = ARRAY_SIZE(MASK_IDS),
	.sample_rate_hz = 8000,
	.buffer_address = 0x531F,
	.block_samples = 320,
	// 0602 PCM handler P:2D41..P:2DB9 advances the DConfPcmA source pointer at D:5DAB.
	.progress_address = 0x5DAB,
	.prepare = prepare,
	.start = start,
	.stop = stop,
	.output_configure = pasic_output_configure,
	.mute = pasic_set_mute,
	.supply_mode_mask = PASIC_SUPPLY_MODE_VAUDREGA_MODE | PASIC_SUPPLY_MODE_VAUDREGB_MODE,
	.supply_1_mask = PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN | PASIC_SUPPLY_ENABLE_1_VAUDREGB_EN,
	.supply_2_mask = 0,
	.supply_settle_ms = 25,
};
