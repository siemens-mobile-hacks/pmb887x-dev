#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gsm.h"

#define GSM_TRX_MONITOR_COUNT 4
#define GSM_TRX_TELEGRAM_COUNT 20
#define GSM_TRX_GAIN_RANGE_COUNT 3
#define GSM_TRX_GAIN_STEP_COUNT 46

void gsm_trx_init(void);
void gsm_trx_reset_telegram_table(void);
void gsm_trx_write_telegram_table(uint32_t bank);
void gsm_trx_configure_monitoring(uint32_t bank, const uint16_t *channel_indices, const uint16_t *gain_states);
void gsm_trx_configure_acquisition(uint32_t bank, uint16_t channel_index, uint16_t gain_state);
bool gsm_trx_is_idle(void);
uint32_t gsm_trx_get_gain_state_range(uint16_t gain_state);
uint32_t gsm_trx_get_gain_state_step(uint16_t gain_state);
uint32_t gsm_trx_get_max_gain_step(void);
uint16_t gsm_trx_encode_gain_state(int32_t residual, uint32_t range, uint32_t step);
