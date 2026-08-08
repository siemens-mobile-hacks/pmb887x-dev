#pragma once

#include <stdint.h>

#define GSM_CHANNEL_COUNT 972
#define GSM_CHANNEL_NONE 0xFFFF

enum gsm_band {
	GSM_BAND_900,
	GSM_BAND_DCS1800,
	GSM_BAND_PCS1900,
	GSM_BAND_850,
	GSM_BAND_COUNT,
};
