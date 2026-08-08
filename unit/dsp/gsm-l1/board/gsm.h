#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "dsp/gsm-l1/gsm.h"

bool gsm_board_init(void);
void gsm_board_enable_transceiver_power(void);
uint16_t gsm_board_get_initial_gain_state(void);
uint16_t gsm_board_calculate_next_gain_state(enum gsm_band band, uint16_t channel_index, uint16_t gain_state, uint16_t raw);
int32_t gsm_board_calculate_rx_level_dbm_x16(enum gsm_band band, uint16_t channel_index, uint16_t gain_state, uint16_t raw);
void gsm_board_set_frequency_correction_hz(int32_t frequency_hz);
int32_t gsm_board_get_frequency_correction_hz(void);
uint16_t gsm_board_get_afc_value(void);
