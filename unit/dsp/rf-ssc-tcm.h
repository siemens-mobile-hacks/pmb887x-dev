#pragma once

#include <stdbool.h>
#include <stdint.h>

void rf_ssc_tcm_init(void);
bool rf_ssc_tcm_transfer(uint32_t control, uint16_t first, uint16_t second, bool has_second);
