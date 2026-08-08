#include <pmb887x.h>
#include <gen/peripheral/rf/HD155153NP.h>

#include "dsp/gsm-l1/gsm-trx.h"

#if defined(BOARD_HAS_RF_HD155153NP)

#define TPU_RF_CONTROL_RAM_BASE 64
#define TPU_RF_TELEGRAM_WORDS 4
#define HD155153NP_REGISTER_VALUE_SHIFT 3

struct trx_telegram {
	uint16_t control;
	uint16_t data_23_16;
	uint16_t data_15_8;
	uint16_t data_7_0;
};

struct channel_telegram {
	uint8_t data_23_16;
	uint8_t data_15_8;
	uint8_t data_7_0;
};

#include "hd155153np-channel-telegrams.inc"

enum telegram_slot {
	TELEGRAM_SLOT_MONITOR_CHANNEL_FIRST = 0,
	TELEGRAM_SLOT_FC_GAIN = 2,
	TELEGRAM_SLOT_FC_CHANNEL = 3,
	TELEGRAM_SLOT_MONITOR_GAIN_FIRST = 4,
	TELEGRAM_SLOT_RX_CONTROL = 13,
};

/* EL71 v45 RF telegram work-table defaults. */
static const struct trx_telegram INITIAL_TELEGRAMS[GSM_TRX_TELEGRAM_COUNT] = {
	[0] = { 0x0320, 0x00, 0x00, 0x01 },
	[1] = { 0x0320, 0x00, 0x00, 0x45 },
	[2] = { 0x0320, 0x13, 0x03, 0x06 },
	[3] = { 0x0320, 0x00, 0x8A, 0x8E },
	[4] = { 0x0320, 0x07, 0x92, 0x96 },
	[5] = { 0x0320, 0x70, 0x00, 0x26 },
	[6] = { 0x0320, 0x76, 0xE0, 0x2E },
	[7] = { 0x0320, 0x76, 0xC0, 0x36 },
	[8] = { 0x0320, 0x76, 0xB8, 0x3E },
	[9] = { 0x0300, 0x07, 0x00, 0x00 },
	[13] = { 0x0300, 0x04, 0x00, 0x00 },
	[14] = { 0x0320, 0x01, 0xFC, 0x0B },
	[15] = { 0x0320, 0x05, 0x24, 0x1B },
	[16] = { 0x0320, 0x00, 0x20, 0xEB },
	[17] = { 0x0320, 0x00, 0x20, 0xC3 },
};

static struct trx_telegram telegrams[GSM_TRX_TELEGRAM_COUNT];

static struct trx_telegram encode_gain_telegram(uint16_t gain_state) {
	static const uint16_t RANGE_FIELDS[GSM_TRX_GAIN_RANGE_COUNT] = { 0x0600, 0x0400, 0x0000 };
	uint32_t range = gsm_trx_get_gain_state_range(gain_state);
	uint32_t step = gsm_trx_get_gain_state_step(gain_state);
	uint32_t telegram = RANGE_FIELDS[range] | (step << HD155153NP_REGISTER_VALUE_SHIFT) | HD155153NP_GAIN_CONTROL;

	return (struct trx_telegram) { 0x0310, (telegram >> 8), (telegram & 0xFF), 0 };
}

static void write_telegram_slot(uint32_t bank, size_t slot) {
	const struct trx_telegram *telegram = &telegrams[slot];
	uint32_t word = TPU_RF_CONTROL_RAM_BASE + (bank * GSM_TRX_TELEGRAM_COUNT + slot) * TPU_RF_TELEGRAM_WORDS;

	TPU_RAM(word++) = telegram->control;
	TPU_RAM(word++) = telegram->data_23_16;
	TPU_RAM(word++) = telegram->data_15_8;
	TPU_RAM(word) = telegram->data_7_0;
}

void gsm_trx_init(void) {
	TPU_RFCON1 = TPU_RFCON1_STBSEL_ACTIVE_HIGH | TPU_RFCON1_RFISSCP_IDLE_LOW_LEADING_RISE;
	TPU_RFCON2 = 0;
}

void gsm_trx_reset_telegram_table(void) {
	for (size_t slot = 0; slot < ARRAY_SIZE(telegrams); slot++)
		telegrams[slot] = INITIAL_TELEGRAMS[slot];
}

void gsm_trx_write_telegram_table(uint32_t bank) {
	for (size_t slot = 0; slot < ARRAY_SIZE(telegrams); slot++)
		write_telegram_slot(bank, slot);
}

void gsm_trx_configure_monitoring(uint32_t bank, const uint16_t *channel_indices, const uint16_t *gain_states) {
	const struct trx_telegram idle = { 0x0300, HD155153NP_CONTROL_R4, 0, 0 };

	for (size_t monitor = 0; monitor < GSM_TRX_MONITOR_COUNT; monitor++) {
		if (channel_indices[monitor] != GSM_CHANNEL_NONE) {
			const struct channel_telegram *channel = &CHANNEL_TELEGRAMS[channel_indices[monitor]];

			telegrams[TELEGRAM_SLOT_MONITOR_CHANNEL_FIRST + monitor] =
				(struct trx_telegram) { 0x0320, channel->data_23_16, channel->data_15_8, channel->data_7_0 };
		} else {
			telegrams[TELEGRAM_SLOT_MONITOR_CHANNEL_FIRST + monitor] = idle;
		}
		telegrams[TELEGRAM_SLOT_MONITOR_GAIN_FIRST + monitor] = encode_gain_telegram(gain_states[monitor]);
	}

	for (size_t slot = 0; slot < GSM_TRX_MONITOR_COUNT * 2; slot++)
		write_telegram_slot(bank, slot);
}

void gsm_trx_configure_acquisition(uint32_t bank, uint16_t channel_index, uint16_t gain_state) {
	const struct channel_telegram *channel = &CHANNEL_TELEGRAMS[channel_index];

	telegrams[TELEGRAM_SLOT_FC_GAIN] = encode_gain_telegram(gain_state);
	telegrams[TELEGRAM_SLOT_FC_CHANNEL] = (struct trx_telegram) {
		0x0320,
		channel->data_23_16,
		channel->data_15_8,
		channel->data_7_0,
	};
	write_telegram_slot(bank, TELEGRAM_SLOT_FC_GAIN);
	write_telegram_slot(bank, TELEGRAM_SLOT_FC_CHANNEL);
	write_telegram_slot(bank, TELEGRAM_SLOT_RX_CONTROL);
}

bool gsm_trx_is_idle(void) {
	return (TPU_RFCON2 & TPU_RFCON2_SSCEN) == 0;
}

uint32_t gsm_trx_get_gain_state_range(uint16_t gain_state) {
	return (gain_state >> 6) & 0x03;
}

uint32_t gsm_trx_get_gain_state_step(uint16_t gain_state) {
	return gain_state & 0x3F;
}

uint32_t gsm_trx_get_max_gain_step(void) {
	return 45;
}

uint16_t gsm_trx_encode_gain_state(int32_t residual, uint32_t range, uint32_t step) {
	return ((uint16_t) residual << 8) | (range << 6) | step;
}

#endif
