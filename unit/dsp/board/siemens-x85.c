#include <pmb887x.h>
#include <gen/peripheral/pmic/D1094XX.h>
#include <i2c.h>
#include <stopwatch.h>

#include "dsp/gsm-trx.h"
#include "eeprom.h"
#include "gsm.h"
#include "test.h"

#if defined(BOARD_PLATFORM_SIEMENS_X85)

#if !defined(BOARD_HAS_PMIC_D1094XX)
#error "the Siemens X85 GSM board backend requires a D1094XX PMIC"
#endif

#define CGU_LOCK_TIMEOUT_MS 20
#define SYSTEM_FREQUENCY_HZ 52000000
#define CPU_FREQUENCY_HZ 104000000
#define CGU_CON2_BASE_MASK (PLL_CON2_EBU_CLKSEL | PLL_CON2_CPU_DIV | PLL_CON2_CPU_DIV_EN)
#define CGU_CON2_BASE_CONFIGURATION 0x00000070
#define CGU_L1_CLOCK_MASK (PLL_CON2_EBU_CLKSEL | PLL_CON2_CPU_DIV | PLL_CON2_CPU_DIV_EN)
#define CGU_L1_CLOCK_CONFIGURATION 0x00001120
#define CGU_LOW_RAM_CLOCK_CONFIGURATION 0x00000120
#define CGU_CPU_MODE_MASK 0x30000007
#define CGU_CPU_MODE_CONFIGURATION 0x20000003
#define CGU_CON3_DIVIDER_MASK 0x00000300
#define CGU_CON3_DIVIDER_CONFIGURATION 0x00000300
#define CGU_CON3_CONTROL_MASK 0x00000033
#define CGU_CON3_CONTROL_CONFIGURATION 0x00000002
#define RF_POWER_SETTLE_US 1000
#define AFC_STARTUP_VALUE 0x4800
#define AFC_FREQUENCY_SCALE 64
#define AFC_CONFIGURATION_FORMAT 1
#define AGC_CONFIGURATION_FORMAT 1
#define AGC_INITIAL_GAIN_STATE 0x004F
#define AGC_RANGE_COUNT 3
#define AGC_EELITE_BLOCK 55
#define AGC_EELITE_VERSION 16
#define AGC_CHANNEL_EELITE_BLOCK 56
#define AGC_CHANNEL_EELITE_VERSION 16
#define AGC_CHANNEL_EELITE_SIZE 648
#define AGC_CHANNEL_BAND_SIZE 156
#define AGC_CHANNEL_POINT_COUNT_MAX 12
#define AGC_GAIN_EELITE_BLOCK 58
#define AGC_GAIN_EELITE_VERSION 16
#define AGC_GAIN_ROW_SIZE 64
#define AGC_GAIN_ROW_COUNT 11
#define AGC_GAIN_NOMINAL_ROW 4
#define AGC_THRESHOLD_EELITE_BLOCK 285
#define AGC_THRESHOLD_EELITE_VERSION 16
#define AGC_MONITOR_REFERENCE 800
#define AFC_EELITE_BLOCK 306
#define AFC_EELITE_VERSION 1
#define MON_RAW_VALUES_PER_DB 16
#define RX_POWER_MAX_X8 1000

struct afc_calibration_point {
	uint16_t value;
	int16_t frequency_div64;
};

struct agc_band_eelite_data {
	int16_t range_offsets[AGC_RANGE_COUNT];
	int16_t band_offset;
} __attribute__((packed));

struct agc_eelite_data {
	uint16_t format;
	uint8_t metadata_02[12];
	uint16_t range_count;
	uint8_t metadata_10[8];
	struct agc_band_eelite_data bands[GSM_BAND_COUNT];
} __attribute__((packed));

struct agc_channel_record {
	uint16_t point_count;
	uint16_t reserved;
	uint16_t channels[AGC_CHANNEL_POINT_COUNT_MAX];
	int16_t corrections[AGC_CHANNEL_POINT_COUNT_MAX];
} __attribute__((packed));

struct agc_channel_band_data {
	uint8_t metadata[24];
	struct agc_channel_record ranges[AGC_RANGE_COUNT];
} __attribute__((packed));

struct agc_gain_eelite_data {
	uint16_t format;
	uint8_t metadata[10];
	int8_t rows[AGC_GAIN_ROW_COUNT][AGC_GAIN_ROW_SIZE];
	uint32_t table_size;
} __attribute__((packed));

struct agc_threshold_pair {
	int16_t enter;
	int16_t leave;
} __attribute__((packed));

struct agc_threshold_eelite_data {
	uint16_t format;
	uint8_t metadata_02[8];
	uint16_t monitor_offset;
	uint16_t range_count;
	uint16_t reserved;
	struct agc_threshold_pair range0[GSM_BAND_COUNT];
	struct agc_threshold_pair range2[GSM_BAND_COUNT];
	uint8_t reserved_30[40];
} __attribute__((packed));

struct afc_eelite_data {
	uint16_t format;
	uint8_t metadata_02[16];
	uint16_t point_count;
	uint8_t metadata_14[2];
	int16_t first_frequency_div64;
	struct afc_calibration_point remaining_points[5];
	uint8_t reserved_2c[176];
} __attribute__((packed));

struct band_calibration {
	/* EELITE block 55. */
	int16_t band_offset;
	int16_t range_offsets[GSM_TRX_GAIN_RANGE_COUNT];
	/* EELITE block 285. */
	int16_t range0_enter_threshold;
	int16_t range0_leave_threshold;
	int16_t range2_leave_threshold;
	int16_t range2_enter_threshold;
};

struct receiver_calibration {
	struct afc_calibration_point afc[6];
	int16_t rx_power_offset;
	struct band_calibration bands[GSM_BAND_COUNT];
	int16_t gain_values_x8[GSM_TRX_GAIN_STEP_COUNT];
	int8_t channel_corrections[GSM_TRX_GAIN_RANGE_COUNT][GSM_CHANNEL_COUNT];
};

static struct receiver_calibration calibration;

static int32_t frequency_correction_hz;

static bool configure_system_clocks(void) {
	uint32_t cgu_osc = 3 << PLL_OSC_NDIV_SHIFT;
	uint32_t cgu_con0 = 0x0B << PLL_CON0_PHASE0_CONFIG_SHIFT |
		0x08 << PLL_CON0_PHASE1_CONFIG_SHIFT |
		0x20 << PLL_CON0_PHASE2_CONFIG_SHIFT |
		0x11 << PLL_CON0_PHASE3_CONFIG_SHIFT;

	/* Firmware 0801 CGU initialization from FUN_a054e2e4. */
	PLL_OSC = cgu_osc;
	PLL_CON0 = cgu_con0;
	PLL_OSC = cgu_osc | PLL_OSC_PLL_POWER_UP;
	stopwatch_t start = stopwatch_get();
	while ((PLL_STAT & PLL_STAT_LOCK) == 0 && stopwatch_elapsed_ms(start) < CGU_LOCK_TIMEOUT_MS)
		test_watchdog_serve();
	if ((PLL_STAT & PLL_STAT_LOCK) == 0)
		return false;
	PLL_OSC = cgu_osc | PLL_OSC_PLL_POWER_UP | PLL_OSC_PLL_BYPASS_N;

	/* USART TX request means that TXB is empty, not that the frame has left the shifter. */
	stopwatch_usleep_wd(1000);
	USART_CLC(USART0) = 2 << MOD_CLC_RMC_SHIFT;
	PLL_CON1 = (PLL_CON1 & ~PLL_CON1_AHB_CLKSEL) | PLL_CON1_AHB_CLKSEL_PLL0;
	PLL_CON2 = (PLL_CON2 & ~CGU_CON2_BASE_MASK) | CGU_CON2_BASE_CONFIGURATION;
	PLL_CON2 = (PLL_CON2 & ~PLL_CON2_USB_CLKSEL) | PLL_CON2_USB_CLKSEL_DISABLE;
	PLL_CON3 = (PLL_CON3 & ~CGU_CON3_DIVIDER_MASK) | CGU_CON3_DIVIDER_CONFIGURATION;
	PLL_CON3 = (PLL_CON3 & ~CGU_CON3_CONTROL_MASK) | CGU_CON3_CONTROL_CONFIGURATION;
	PLL_CON1 = (PLL_CON1 & ~PLL_CON1_SYSTEM_OUT_CTRL) | PLL_CON1_SYSTEM_OUT_CTRL_ON;
	PLL_CON1 = (PLL_CON1 & ~PLL_CON1_FSYS_CLKSEL) | PLL_CON1_FSYS_CLKSEL_PLL;
	PLL_CON3 |= PLL_CON3_PERIPHERAL_CLK_EN;
	PLL_CON2 |= PLL_CON2_PERIPHERAL_CLKSEL_FSYS;

	return cpu_get_sys_freq() == SYSTEM_FREQUENCY_HZ && cpu_get_freq() == CPU_FREQUENCY_HZ;
}

static void configure_rf_gpio(void) {
	GPIO_PIN(GPIO_RF_FE_CTR_GSM) = GPIO_OS_ALT0;
	GPIO_PIN(GPIO_RF_FE_CTR_DCS) = GPIO_OS_ALT0;
	GPIO_PIN(GPIO_RF_BAND_SW) = GPIO_OS_ALT0;
	GPIO_PIN(GPIO_RF_STR0) = GPIO_OS_ALT0;
	GPIO_PIN(GPIO_PM_RF2_EN) = GPIO_PS_MANUAL | GPIO_DATA_HIGH | GPIO_DIR_OUT;
}

static void enable_transceiver_power_rails(void) {
	i2c_init();
	i2c_smbus_write_byte(D1094XX_I2C_ADDR, D1094XX_RF_VOLTAGE,
		D1094XX_RF_VOLTAGE_VRF1_LEVEL_2850MV | D1094XX_RF_VOLTAGE_VRF2_LEVEL_2850MV);
	i2c_smbus_write_byte(D1094XX_I2C_ADDR, D1094XX_RF_ENABLE,
		D1094XX_RF_ENABLE_VRF1_EN | D1094XX_RF_ENABLE_VRF2_EN);
	stopwatch_usleep_wd(RF_POWER_SETTLE_US);
}

static void enable_afc_reference(void) {
	AFC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	/* Firmware 0801 initializes AFCVAL to 0x4800 before applying EELITE block 306. */
	AFC_AFCVAL = AFC_STARTUP_VALUE;
	stopwatch_usleep_wd(RF_POWER_SETTLE_US);
}

static bool load_eelite_block(uint32_t block_id, uint16_t version, uint32_t size, struct eeprom_block *block) {
	enum eeprom_result result = eeprom_find_block(EEPROM_PARTITION_EELITE, block_id, block);

	if (result != EEPROM_RESULT_OK) {
		printf("# RF_CALIBRATION,error=read,eelite_block=%u,result=%u\n", block_id, (uint32_t) result);
		return false;
	}
	if (block->version != version) {
		printf("# RF_CALIBRATION,error=format,eelite_block=%u,version=%u,size=%u\n",
			block_id, block->version, block->size);
		return false;
	}
	if (block->size != size) {
		printf("# RF_CALIBRATION,error=format,eelite_block=%u,version=%u,size=%u\n",
			block_id, block->version, block->size);
		return false;
	}

	return true;
}

static bool load_agc_calibration(void) {
	struct eeprom_block block;
	if (!load_eelite_block(AGC_EELITE_BLOCK, AGC_EELITE_VERSION, sizeof(struct agc_eelite_data), &block))
		return false;

	const struct agc_eelite_data *data = (const struct agc_eelite_data *) block.data;
	if (data->format != AGC_CONFIGURATION_FORMAT) {
		printf("# RF_CALIBRATION,error=data_format,eelite_block=%u\n", AGC_EELITE_BLOCK);
		return false;
	}
	if (data->range_count != AGC_RANGE_COUNT) {
		printf("# RF_CALIBRATION,error=data_format,eelite_block=%u\n", AGC_EELITE_BLOCK);
		return false;
	}

	for (size_t band = 0; band < GSM_BAND_COUNT; band++) {
		for (size_t range = 0; range < AGC_RANGE_COUNT; range++)
			calibration.bands[band].range_offsets[range] = data->bands[band].range_offsets[range];
		calibration.bands[band].band_offset = data->bands[band].band_offset;
	}

	return true;
}

static int32_t divide_floor(int32_t dividend, int32_t divisor) {
	int32_t quotient = dividend / divisor;

	if (dividend < 0 && dividend % divisor != 0)
		quotient--;
	return quotient;
}

static bool agc_channel_position(enum gsm_band band, uint16_t arfcn, uint16_t *position) {
	switch (band) {
		case GSM_BAND_900:
			if (arfcn >= 975 && arfcn <= 1023) {
				*position = arfcn - 975;
				return true;
			}
			if (arfcn <= 124) {
				*position = arfcn + 49;
				return true;
			}
			return false;

		case GSM_BAND_DCS1800:
			if (arfcn < 512 || arfcn > 885)
				return false;
			*position = arfcn - 512;
			return true;

		case GSM_BAND_PCS1900:
			if (arfcn < 512 || arfcn > 810)
				return false;
			*position = arfcn - 512;
			return true;

		case GSM_BAND_850:
			if (arfcn < 128 || arfcn > 251)
				return false;
			*position = arfcn - 128;
			return true;

		case GSM_BAND_COUNT:
			return false;
	}

	return false;
}

static uint16_t agc_channel_index(enum gsm_band band, uint16_t position) {
	switch (band) {
		case GSM_BAND_900:
			if (position < 49)
				return 126 + position;
			if (position == 49)
				return 125;
			return position - 49;

		case GSM_BAND_DCS1800:
			return 175 + position;

		case GSM_BAND_PCS1900:
			return 549 + position;

		case GSM_BAND_850:
			return 848 + position;

		case GSM_BAND_COUNT:
			return 0;
	}

	return 0;
}

static uint16_t agc_band_last_position(enum gsm_band band) {
	switch (band) {
		case GSM_BAND_900:
			return 173;
		case GSM_BAND_DCS1800:
			return 373;
		case GSM_BAND_PCS1900:
			return 298;
		case GSM_BAND_850:
			return 123;
		case GSM_BAND_COUNT:
			return 0;
	}

	return 0;
}

static bool load_agc_channel_record(enum gsm_band band, uint32_t range, const struct agc_channel_record *record) {
	if (record->point_count < 2 || record->point_count > AGC_CHANNEL_POINT_COUNT_MAX)
		return false;

	uint16_t point_positions[AGC_CHANNEL_POINT_COUNT_MAX];
	for (size_t point = 0; point < record->point_count; point++) {
		if (!agc_channel_position(band, record->channels[point], &point_positions[point]))
			return false;
		if (point != 0 && point_positions[point] <= point_positions[point - 1])
			return false;
	}
	if (point_positions[0] != 0)
		return false;
	if (point_positions[record->point_count - 1] != agc_band_last_position(band))
		return false;

	uint32_t lower = 0;
	for (uint16_t position = 0; position <= agc_band_last_position(band); position++) {
		while (lower + 1 < record->point_count && position >= point_positions[lower + 1])
			lower++;

		int32_t correction_x16 = record->corrections[lower] * 65536;
		if (lower + 1 < record->point_count) {
			int32_t position_delta = position - point_positions[lower];
			int32_t correction_delta = record->corrections[lower + 1] - record->corrections[lower];
			int32_t point_distance = point_positions[lower + 1] - point_positions[lower];
			int32_t correction_step_x16 = correction_delta * 65536 / point_distance;

			correction_x16 += position_delta * correction_step_x16;
		}

		calibration.channel_corrections[range][agc_channel_index(band, position)] = divide_floor(correction_x16, 65536);
	}

	return true;
}

static bool load_agc_channel_calibration(void) {
	struct eeprom_block block;
	if (!load_eelite_block(AGC_CHANNEL_EELITE_BLOCK, AGC_CHANNEL_EELITE_VERSION, AGC_CHANNEL_EELITE_SIZE, &block))
		return false;

	for (size_t band = 0; band < GSM_BAND_COUNT; band++) {
		const struct agc_channel_band_data *data =
			(const struct agc_channel_band_data *) (block.data + band * AGC_CHANNEL_BAND_SIZE);

		for (size_t range = 0; range < AGC_RANGE_COUNT; range++) {
			if (!load_agc_channel_record(band, range, &data->ranges[range])) {
				printf("# RF_CALIBRATION,error=data_format,eelite_block=%u,band=%u,range=%u\n",
					AGC_CHANNEL_EELITE_BLOCK, (uint32_t) band, (uint32_t) range);
				return false;
			}
		}
	}

	return true;
}

static bool load_agc_gain_calibration(void) {
	struct eeprom_block block;
	if (!load_eelite_block(AGC_GAIN_EELITE_BLOCK, AGC_GAIN_EELITE_VERSION,
		sizeof(struct agc_gain_eelite_data), &block))
		return false;

	const struct agc_gain_eelite_data *data = (const struct agc_gain_eelite_data *) block.data;
	if (data->format != AGC_CONFIGURATION_FORMAT) {
		printf("# RF_CALIBRATION,error=data_format,eelite_block=%u\n", AGC_GAIN_EELITE_BLOCK);
		return false;
	}

	for (size_t step = 0; step < GSM_TRX_GAIN_STEP_COUNT; step++)
		calibration.gain_values_x8[step] = step * MON_RAW_VALUES_PER_DB + data->rows[AGC_GAIN_NOMINAL_ROW][step];

	return true;
}

static bool load_agc_threshold_calibration(void) {
	struct eeprom_block block;
	if (!load_eelite_block(AGC_THRESHOLD_EELITE_BLOCK, AGC_THRESHOLD_EELITE_VERSION,
		sizeof(struct agc_threshold_eelite_data), &block))
		return false;

	const struct agc_threshold_eelite_data *data = (const struct agc_threshold_eelite_data *) block.data;
	if (data->format != AGC_CONFIGURATION_FORMAT) {
		printf("# RF_CALIBRATION,error=data_format,eelite_block=%u\n", AGC_THRESHOLD_EELITE_BLOCK);
		return false;
	}
	if (data->range_count != AGC_RANGE_COUNT) {
		printf("# RF_CALIBRATION,error=data_format,eelite_block=%u\n", AGC_THRESHOLD_EELITE_BLOCK);
		return false;
	}
	if (data->monitor_offset < AGC_MONITOR_REFERENCE) {
		printf("# RF_CALIBRATION,error=data_format,eelite_block=%u\n", AGC_THRESHOLD_EELITE_BLOCK);
		return false;
	}

	calibration.rx_power_offset = data->monitor_offset - AGC_MONITOR_REFERENCE;
	for (size_t band = 0; band < GSM_BAND_COUNT; band++) {
		calibration.bands[band].range0_enter_threshold = data->range0[band].enter;
		calibration.bands[band].range0_leave_threshold = data->range0[band].leave;
		calibration.bands[band].range2_leave_threshold = data->range2[band].enter;
		calibration.bands[band].range2_enter_threshold = data->range2[band].leave;
	}

	return true;
}

static bool load_afc_calibration(void) {
	struct eeprom_block block;
	if (!load_eelite_block(AFC_EELITE_BLOCK, AFC_EELITE_VERSION, sizeof(struct afc_eelite_data), &block))
		return false;

	const struct afc_eelite_data *data = (const struct afc_eelite_data *) block.data;
	if (data->format != AFC_CONFIGURATION_FORMAT) {
		printf("# RF_CALIBRATION,error=data_format,eelite_block=%u\n", AFC_EELITE_BLOCK);
		return false;
	}
	if (data->point_count != ARRAY_SIZE(calibration.afc)) {
		printf("# RF_CALIBRATION,error=data_format,eelite_block=%u\n", AFC_EELITE_BLOCK);
		return false;
	}

	struct afc_calibration_point points[ARRAY_SIZE(calibration.afc)];
	points[0].value = 0;
	points[0].frequency_div64 = data->first_frequency_div64;
	for (size_t i = 1; i < ARRAY_SIZE(points); i++) {
		points[i] = data->remaining_points[i - 1];
		if (points[i].frequency_div64 <= points[i - 1].frequency_div64) {
			printf("# RF_CALIBRATION,error=afc_frequency,eelite_block=%u,index=%u\n",
				AFC_EELITE_BLOCK, (uint32_t) i);
			return false;
		}
		if (points[i].value <= points[i - 1].value) {
			printf("# RF_CALIBRATION,error=afc_value,eelite_block=%u,index=%u\n",
				AFC_EELITE_BLOCK, (uint32_t) i);
			return false;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(points); i++)
		calibration.afc[i] = points[i];

	return true;
}

static bool load_receiver_calibration(void) {
	if (!load_agc_calibration())
		return false;
	if (!load_agc_channel_calibration())
		return false;
	if (!load_agc_gain_calibration())
		return false;
	if (!load_agc_threshold_calibration())
		return false;
	if (!load_afc_calibration())
		return false;

	/* EEFULL 5437 is learned AFC state; a fresh scan only needs factory EELITE calibration. */
	printf("# RF_CALIBRATION,source=EELITE,blocks=%u,%u,%u,%u,%u\n",
		AGC_EELITE_BLOCK, AGC_CHANNEL_EELITE_BLOCK, AGC_GAIN_EELITE_BLOCK,
		AGC_THRESHOLD_EELITE_BLOCK, AFC_EELITE_BLOCK);
	return true;
}

static uint16_t frequency_hz_to_afc_value(int32_t frequency_hz) {
	int32_t minimum_frequency = calibration.afc[0].frequency_div64 * AFC_FREQUENCY_SCALE;
	if (frequency_hz <= minimum_frequency)
		return calibration.afc[0].value;

	for (size_t upper = 1; upper < ARRAY_SIZE(calibration.afc); upper++) {
		const struct afc_calibration_point *lower_point = &calibration.afc[upper - 1];
		const struct afc_calibration_point *upper_point = &calibration.afc[upper];
		int32_t upper_frequency = upper_point->frequency_div64 * AFC_FREQUENCY_SCALE;

		if (frequency_hz <= upper_frequency) {
			int32_t lower_frequency = lower_point->frequency_div64 * AFC_FREQUENCY_SCALE;
			int32_t frequency_range = upper_frequency - lower_frequency;
			int32_t frequency_offset = frequency_hz - lower_frequency;
			int32_t value_range = (int32_t) upper_point->value - lower_point->value;

			return (uint16_t) (lower_point->value + (int64_t) frequency_offset * value_range / frequency_range);
		}
	}

	return calibration.afc[ARRAY_SIZE(calibration.afc) - 1].value;
}

static void enable_baseband_analog_reference(void) {
	ADC_CLC = 1 << MOD_CLC_RMC_SHIFT | MOD_CLC_EDIS;
	ADC_ANA_CTRL = ADC_ANA_CTRL_BG_PWUP;
	ADC_CLK = 4 << ADC_CLK_K_SHIFT | 13 << ADC_CLK_L_SHIFT;
	stopwatch_usleep_wd(150);
}

static void request_ebu_clock_transition(volatile uint32_t *control, uint32_t request_mask, uint32_t ready_mask) {
	*control |= request_mask;
	while ((*control & ready_mask) == 0)
		;
}

static void configure_l1_clocks(void) {
	/* Firmware 0801 L1CTRL clock mode 3 from FUN_a054e734. */
	PLL_CON2 = (PLL_CON2 & ~CGU_L1_CLOCK_MASK) | CGU_L1_CLOCK_CONFIGURATION;

	/* Firmware 0801 low-RAM clock mode from PWR_SetLowRamClock. */
	PLL_OSC |= PLL_OSC_PHASE1_POWER_UP | PLL_OSC_PHASE1_BYPASS_N;
	PLL_CON2 = (PLL_CON2 & ~CGU_L1_CLOCK_MASK) | CGU_LOW_RAM_CLOCK_CONFIGURATION;
	request_ebu_clock_transition(&SCU_EBUCLC2, 1 << SCU_EBUCLC2_FLAG1_SHIFT, SCU_EBUCLC2_READY);
	PLL_CON1 = (PLL_CON1 & ~PLL_CON1_AHB_CLKSEL) | PLL_CON1_AHB_CLKSEL_PLL2;
	request_ebu_clock_transition(&SCU_EBUCLC1, 1 << SCU_EBUCLC1_FLAG1_SHIFT, SCU_EBUCLC1_READY);
	EBU_CON &= ~EBU_CON_SDCMSEL;
	SCU_EBUCLC1 = 0;

	/* Firmware 0801 CPU mode from FUN_a054e660. */
	PLL_OSC |= PLL_OSC_PHASE0_POWER_UP | PLL_OSC_PHASE0_BYPASS_N;
	PLL_CON2 = (PLL_CON2 & ~CGU_CPU_MODE_MASK) | CGU_CPU_MODE_CONFIGURATION;
}

static int32_t decode_monitor_value(uint16_t raw) {
	int32_t monitor = (int16_t) raw;

	if (raw == 0xFF80)
		return 0;
	if (monitor >= 0)
		return raw >> 2;
	return monitor;
}

static int32_t calculate_rx_gain_x8(enum gsm_band band, uint16_t channel_index, uint16_t gain_state) {
	const struct band_calibration *band_calibration = &calibration.bands[band];
	uint32_t range = gsm_trx_get_gain_state_range(gain_state);
	uint32_t step = gsm_trx_get_gain_state_step(gain_state);

	return calibration.gain_values_x8[step] +
		calibration.channel_corrections[range][channel_index] +
		band_calibration->range_offsets[range] + band_calibration->band_offset +
		calibration.rx_power_offset * 2;
}

static int32_t calculate_rx_power_x8(enum gsm_band band, uint16_t channel_index, uint16_t gain_state, uint16_t raw) {
	int32_t power_x8 = calculate_rx_gain_x8(band, channel_index, gain_state) - decode_monitor_value(raw) * 2;

	if (power_x8 < 0)
		return 0;
	if (power_x8 > RX_POWER_MAX_X8)
		return RX_POWER_MAX_X8;
	return power_x8;
}

bool gsm_board_init(void) {
	configure_rf_gpio();
	if (!load_receiver_calibration())
		return false;
	if (!configure_system_clocks())
		return false;

	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	enable_baseband_analog_reference();
	enable_afc_reference();
	return true;
}

void gsm_board_configure_transceiver_clocks(void) {
	configure_l1_clocks();
}

void gsm_board_enable_transceiver_power(void) {
	enable_transceiver_power_rails();
}

uint16_t gsm_board_get_initial_gain_state(void) {
	/* Firmware 0801 algorithm-3 startup state: range 1, step 15, zero residual. */
	return AGC_INITIAL_GAIN_STATE;
}

uint16_t gsm_board_calculate_next_gain_state(enum gsm_band band, uint16_t channel_index, uint16_t gain_state, uint16_t raw) {
	/* Firmware 0801 gsm_convert_monitor_and_update_gain at 0xA0B220FE. */
	const struct band_calibration *band_calibration = &calibration.bands[band];
	uint32_t range = gsm_trx_get_gain_state_range(gain_state);
	uint32_t step = gsm_trx_get_gain_state_step(gain_state);
	int32_t monitor_delta_x8 = (decode_monitor_value(raw) - calibration.rx_power_offset) * 2;
	int32_t power_x8 = calculate_rx_power_x8(band, channel_index, gain_state, raw);
	uint32_t next_range = range;

	if (power_x8 < band_calibration->range0_leave_threshold) {
		if (power_x8 < band_calibration->range0_enter_threshold) {
			next_range = 0;
		} else if (range > 1) {
			next_range = 1;
		}
	} else if (power_x8 < band_calibration->range2_enter_threshold) {
		if (power_x8 < band_calibration->range2_leave_threshold || range == 0)
			next_range = 1;
	} else {
		next_range = 2;
	}

	int32_t gain_delta_x8 =
		band_calibration->range_offsets[range] +
		calibration.channel_corrections[range][channel_index] -
		band_calibration->range_offsets[next_range] -
		calibration.channel_corrections[next_range][channel_index] +
		(int8_t) (gain_state >> 8) - monitor_delta_x8;
	int32_t step_delta = (gain_delta_x8 + 4) >> 4;
	int32_t next_step = (int32_t) step + step_delta;
	int32_t maximum_step = gsm_trx_get_max_gain_step();

	if (next_step < 0) {
		step_delta -= next_step;
		next_step = 0;
	}
	if (next_step > maximum_step) {
		step_delta -= next_step - maximum_step;
		next_step = maximum_step;
	}

	int32_t residual = gain_delta_x8 - step_delta * MON_RAW_VALUES_PER_DB;
	if (residual < -127)
		residual = -127;
	if (residual > 127)
		residual = 127;

	return gsm_trx_encode_gain_state(residual, next_range, next_step);
}

int32_t gsm_board_calculate_rx_level_dbm_x16(enum gsm_band band, uint16_t channel_index, uint16_t gain_state, uint16_t raw) {
	return calculate_rx_power_x8(band, channel_index, gain_state, raw) * -2;
}

void gsm_board_set_frequency_correction_hz(int32_t frequency_hz) {
	int32_t minimum_frequency = calibration.afc[0].frequency_div64 * AFC_FREQUENCY_SCALE;
	int32_t maximum_frequency = calibration.afc[ARRAY_SIZE(calibration.afc) - 1].frequency_div64 * AFC_FREQUENCY_SCALE;

	if (frequency_hz < minimum_frequency)
		frequency_hz = minimum_frequency;
	if (frequency_hz > maximum_frequency)
		frequency_hz = maximum_frequency;

	frequency_correction_hz = frequency_hz;
	AFC_AFCVAL = frequency_hz_to_afc_value(frequency_hz) | AFC_AFCVAL_ENAFC;
}

int32_t gsm_board_get_frequency_correction_hz(void) {
	return frequency_correction_hz;
}

uint16_t gsm_board_get_afc_value(void) {
	return AFC_AFCVAL & AFC_AFCVAL_AFC;
}

#endif
