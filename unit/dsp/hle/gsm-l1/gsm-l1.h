#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gsm.h"

struct gsm_l1_runtime {
	void (*poll)(void *context);
	void *context;
	uint32_t command_timeout_ms;
};

struct gsm_l1_candidate {
	enum gsm_band band;
	uint16_t channel_index;
	uint16_t arfcn;
	uint16_t gain_state;
	int16_t rx_level_dbm_x16;
	int32_t afc_frequency_hz;
};

struct gsm_l1_fcch_result {
	bool detected;
	uint32_t attempt_count;
	uint16_t status;
	uint16_t start;
	uint16_t quality;
	int16_t rms;
	int16_t frequency;
	uint16_t communication_flags;
	uint16_t phase_tick;
	int32_t afc_frequency_hz;
};

struct gsm_l1_sch_result {
	bool decoded;
	uint32_t refinement_attempt_count;
	uint32_t attempt_count;
	uint16_t metric;
	uint16_t status;
	uint32_t data;
	int16_t equalizer_position;
	uint16_t communication_flags;
	uint8_t bsic;
	uint32_t frame_number;
};

bool gsm_l1_baseband_init(const struct gsm_l1_runtime *runtime);
bool gsm_l1_trx_init(void);
bool gsm_l1_scan_channels(struct gsm_l1_candidate *candidates, size_t capacity, size_t *count, uint32_t timeout_ms);
bool gsm_l1_search_fcch(const struct gsm_l1_candidate *candidate, struct gsm_l1_fcch_result *result, uint32_t timeout_ms);
bool gsm_l1_decode_sch(
	const struct gsm_l1_candidate *candidate,
	const struct gsm_l1_fcch_result *fcch,
	struct gsm_l1_sch_result *result,
	uint32_t timeout_ms
);
void gsm_l1_tpu_irq(void);
void gsm_l1_deinit(void);
