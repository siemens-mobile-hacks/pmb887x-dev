#pragma once

#include <bitops.h> // IWYU pragma: export

// TEA5761UK
// NXP TEA5761UK low-voltage single-chip FM stereo radio
#define	TEA5761UK_I2C_ADDR												0x10

/* Interrupt and tuner-state flags */
#define	TEA5761UK_INTREG_STATUS											0x00
#define	TEA5761UK_INTREG_STATUS_BLFLAG									BIT(0)			 // Search band-limit or timeout flag
#define	TEA5761UK_INTREG_STATUS_BLFLAG_BAND_LIMIT_NOT_REACHED			0x0
#define	TEA5761UK_INTREG_STATUS_BLFLAG_BAND_LIMIT_REACHED_OR_TIMEOUT	0x1
#define	TEA5761UK_INTREG_STATUS_FRRFLAG									BIT(1)			 // Tuner state-machine ready flag
#define	TEA5761UK_INTREG_STATUS_FRRFLAG_TUNER_NOT_READY					0x0
#define	TEA5761UK_INTREG_STATUS_FRRFLAG_TUNER_READY						0x2
#define	TEA5761UK_INTREG_STATUS_LEVFLAG									BIT(3)			 // RSSI threshold flag
#define	TEA5761UK_INTREG_STATUS_LEVFLAG_LEVEL_ABOVE_THRESHOLD			0x0
#define	TEA5761UK_INTREG_STATUS_LEVFLAG_LEVEL_BELOW_THRESHOLD			0x8
#define	TEA5761UK_INTREG_STATUS_IFFLAG									BIT(4)			 // IF count validity flag
#define	TEA5761UK_INTREG_STATUS_IFFLAG_IF_COUNT_CORRECT					0x0
#define	TEA5761UK_INTREG_STATUS_IFFLAG_IF_COUNT_INCORRECT				0x10

/* Tuner-flag hardware-interrupt enables */
#define	TEA5761UK_INTREG_MASK											0x01
#define	TEA5761UK_INTREG_MASK_BLMSK										BIT(0)			 // BLFLAG hardware-interrupt enable
#define	TEA5761UK_INTREG_MASK_BLMSK_DISABLED							0x0
#define	TEA5761UK_INTREG_MASK_BLMSK_ENABLED								0x1
#define	TEA5761UK_INTREG_MASK_FRRMSK									BIT(1)			 // FRRFLAG hardware-interrupt enable
#define	TEA5761UK_INTREG_MASK_FRRMSK_DISABLED							0x0
#define	TEA5761UK_INTREG_MASK_FRRMSK_ENABLED							0x2
#define	TEA5761UK_INTREG_MASK_LEVMSK									BIT(3)			 // LEVFLAG hardware-interrupt enable
#define	TEA5761UK_INTREG_MASK_LEVMSK_DISABLED							0x0
#define	TEA5761UK_INTREG_MASK_LEVMSK_ENABLED							0x8
#define	TEA5761UK_INTREG_MASK_IFMSK										BIT(4)			 // IFFLAG hardware-interrupt enable
#define	TEA5761UK_INTREG_MASK_IFMSK_DISABLED							0x0
#define	TEA5761UK_INTREG_MASK_IFMSK_ENABLED								0x10

/* Search direction, tuning mode, and frequency MSBs */
#define	TEA5761UK_FRQSET_HIGH											0x02
#define	TEA5761UK_FRQSET_HIGH_FR_13_08									GENMASK(5, 0)	 // Frequency setting bits 13 to 8
#define	TEA5761UK_FRQSET_HIGH_FR_13_08_SHIFT							0
#define	TEA5761UK_FRQSET_HIGH_SM										BIT(6)			 // Preset or search tuning mode
#define	TEA5761UK_FRQSET_HIGH_SM_PRESET									0x0
#define	TEA5761UK_FRQSET_HIGH_SM_SEARCH									0x40
#define	TEA5761UK_FRQSET_HIGH_SUD										BIT(7)			 // Search direction
#define	TEA5761UK_FRQSET_HIGH_SUD_SEARCH_DOWN							0x0
#define	TEA5761UK_FRQSET_HIGH_SUD_SEARCH_UP								0x80

/* Frequency setting LSBs */
#define	TEA5761UK_FRQSET_LOW											0x03
#define	TEA5761UK_FRQSET_LOW_FR_07_00									GENMASK(7, 0)	 // Frequency setting bits 7 to 0
#define	TEA5761UK_FRQSET_LOW_FR_07_00_SHIFT								0

/* Power, band, IF timing, and audio processing control */
#define	TEA5761UK_TNCTRL_CONTROL										0x04
#define	TEA5761UK_TNCTRL_CONTROL_SNC									BIT(0)			 // Stereo noise-cancellation enable
#define	TEA5761UK_TNCTRL_CONTROL_SNC_OFF								0x0
#define	TEA5761UK_TNCTRL_CONTROL_SNC_ON									0x1
#define	TEA5761UK_TNCTRL_CONTROL_SMUTE									BIT(1)			 // Soft-mute enable
#define	TEA5761UK_TNCTRL_CONTROL_SMUTE_OFF								0x0
#define	TEA5761UK_TNCTRL_CONTROL_SMUTE_ON								0x2
#define	TEA5761UK_TNCTRL_CONTROL_AFM									BIT(2)			 // Left and right audio mute
#define	TEA5761UK_TNCTRL_CONTROL_AFM_AUDIO_ACTIVE						0x0
#define	TEA5761UK_TNCTRL_CONTROL_AFM_AUDIO_MUTED						0x4
#define	TEA5761UK_TNCTRL_CONTROL_IFCTC									BIT(3)			 // IF count duration
#define	TEA5761UK_TNCTRL_CONTROL_IFCTC_COUNT_1_953_MS					0x0
#define	TEA5761UK_TNCTRL_CONTROL_IFCTC_COUNT_15_625_MS					0x8
#define	TEA5761UK_TNCTRL_CONTROL_SWPM									BIT(4)			 // Software-port signal source
#define	TEA5761UK_TNCTRL_CONTROL_SWPM_SWP_OUTPUT						0x0
#define	TEA5761UK_TNCTRL_CONTROL_SWPM_FRRFLAG_OUTPUT					0x10
#define	TEA5761UK_TNCTRL_CONTROL_BLIM									BIT(5)			 // FM tuning band selection
#define	TEA5761UK_TNCTRL_CONTROL_BLIM_US_EUROPE							0x0
#define	TEA5761UK_TNCTRL_CONTROL_BLIM_JAPAN								0x20
#define	TEA5761UK_TNCTRL_CONTROL_PUPD									BIT(6)			 // FM receiver power control
#define	TEA5761UK_TNCTRL_CONTROL_PUPD_FM_OFF							0x0
#define	TEA5761UK_TNCTRL_CONTROL_PUPD_FM_ON								0x40

/* Mute, search, injection, mono, port, and de-emphasis control */
#define	TEA5761UK_TNCTRL_AUDIO											0x05
#define	TEA5761UK_TNCTRL_AUDIO_AHLSI									BIT(0)			 // Stop search on failed IF count and valid level
#define	TEA5761UK_TNCTRL_AUDIO_AHLSI_CONTINUOUS_SEARCH					0x0
#define	TEA5761UK_TNCTRL_AUDIO_AHLSI_STOP_ON_FAILED_IF_COUNT			0x1
#define	TEA5761UK_TNCTRL_AUDIO_DTC										BIT(1)			 // De-emphasis time constant
#define	TEA5761UK_TNCTRL_AUDIO_DTC_DEEMPHASIS_75_US						0x0
#define	TEA5761UK_TNCTRL_AUDIO_DTC_DEEMPHASIS_50_US						0x2
#define	TEA5761UK_TNCTRL_AUDIO_SWP										BIT(2)			 // Software-port output level
#define	TEA5761UK_TNCTRL_AUDIO_SWP_LOW									0x0
#define	TEA5761UK_TNCTRL_AUDIO_SWP_HIGH									0x4
#define	TEA5761UK_TNCTRL_AUDIO_MST										BIT(3)			 // Stereo or forced-mono mode
#define	TEA5761UK_TNCTRL_AUDIO_MST_STEREO								0x0
#define	TEA5761UK_TNCTRL_AUDIO_MST_FORCED_MONO							0x8
#define	TEA5761UK_TNCTRL_AUDIO_HLSI										BIT(4)			 // Mixer injection side
#define	TEA5761UK_TNCTRL_AUDIO_HLSI_LOW_SIDE							0x0
#define	TEA5761UK_TNCTRL_AUDIO_HLSI_HIGH_SIDE							0x10
#define	TEA5761UK_TNCTRL_AUDIO_SSL										GENMASK(6, 5)	 // Search stop-level threshold
#define	TEA5761UK_TNCTRL_AUDIO_SSL_SHIFT								5
#define	TEA5761UK_TNCTRL_AUDIO_SSL_ADC3									0x0
#define	TEA5761UK_TNCTRL_AUDIO_SSL_ADC5									0x20
#define	TEA5761UK_TNCTRL_AUDIO_SSL_ADC7									0x40
#define	TEA5761UK_TNCTRL_AUDIO_SSL_ADC10								0x60
#define	TEA5761UK_TNCTRL_AUDIO_MU										BIT(7)			 // Left and right hard mute
#define	TEA5761UK_TNCTRL_AUDIO_MU_NOT_MUTED								0x0
#define	TEA5761UK_TNCTRL_AUDIO_MU_HARD_MUTED							0x80

/* Detected frequency PLL word MSBs */
#define	TEA5761UK_FRQCHK_HIGH											0x06
#define	TEA5761UK_FRQCHK_HIGH_PLL_13_08									GENMASK(5, 0)	 // Detected frequency bits 13 to 8
#define	TEA5761UK_FRQCHK_HIGH_PLL_13_08_SHIFT							0

/* Detected frequency PLL word LSBs */
#define	TEA5761UK_FRQCHK_LOW											0x07
#define	TEA5761UK_FRQCHK_LOW_PLL_07_00									GENMASK(7, 0)	 // Detected frequency bits 7 to 0
#define	TEA5761UK_FRQCHK_LOW_PLL_07_00_SHIFT							0

/* IF count and PLL tuning timeout */
#define	TEA5761UK_TUNCHK_IF												0x08
#define	TEA5761UK_TUNCHK_IF_TUNTO										BIT(0)			 // PLL tuning timeout flag
#define	TEA5761UK_TUNCHK_IF_TUNTO_PLL_SETTLED							0x0
#define	TEA5761UK_TUNCHK_IF_TUNTO_TUNING_TIMEOUT						0x1
#define	TEA5761UK_TUNCHK_IF_IF_6_0										GENMASK(7, 1)	 // Intermediate-frequency count
#define	TEA5761UK_TUNCHK_IF_IF_6_0_SHIFT								1

/* Signal level, PLL lock, and pilot-detection status */
#define	TEA5761UK_TUNCHK_STATUS											0x09
#define	TEA5761UK_TUNCHK_STATUS_STEREO									BIT(2)			 // Stereo-pilot detection flag
#define	TEA5761UK_TUNCHK_STATUS_STEREO_NO_PILOT							0x0
#define	TEA5761UK_TUNCHK_STATUS_STEREO_PILOT_DETECTED					0x4
#define	TEA5761UK_TUNCHK_STATUS_LD										BIT(3)			 // PLL lock flag
#define	TEA5761UK_TUNCHK_STATUS_LD_PLL_NOT_LOCKED						0x0
#define	TEA5761UK_TUNCHK_STATUS_LD_PLL_LOCKED							0x8
#define	TEA5761UK_TUNCHK_STATUS_LEV_3_0									GENMASK(7, 4)	 // Received-signal level count
#define	TEA5761UK_TUNCHK_STATUS_LEV_3_0_SHIFT							4

/* Audio mute, hysteresis, reference, LNA, RF AGC, and interrupt control */
#define	TEA5761UK_TESTREG_CONTROL										0x0A
#define	TEA5761UK_TESTREG_CONTROL_INTCTRL								BIT(0)			 // INTX interrupt generation enable
#define	TEA5761UK_TESTREG_CONTROL_INTCTRL_DISABLED						0x0
#define	TEA5761UK_TESTREG_CONTROL_INTCTRL_ENABLED						0x1
#define	TEA5761UK_TESTREG_CONTROL_RFAGC									BIT(1)			 // RF automatic gain control
#define	TEA5761UK_TESTREG_CONTROL_RFAGC_ON								0x0
#define	TEA5761UK_TESTREG_CONTROL_RFAGC_OFF								0x2
#define	TEA5761UK_TESTREG_CONTROL_LDX									BIT(2)			 // LNA local/DX gain mode
#define	TEA5761UK_TESTREG_CONTROL_LDX_NORMAL_GAIN						0x0
#define	TEA5761UK_TESTREG_CONTROL_LDX_REDUCED_GAIN_6_DB					0x4
#define	TEA5761UK_TESTREG_CONTROL_TRIGFR								BIT(3)			 // Reference-frequency input source
#define	TEA5761UK_TESTREG_CONTROL_TRIGFR_XTAL							0x0
#define	TEA5761UK_TESTREG_CONTROL_TRIGFR_FREQIN							0x8
#define	TEA5761UK_TESTREG_CONTROL_LHSW									BIT(4)			 // RSSI level hysteresis
#define	TEA5761UK_TESTREG_CONTROL_LHSW_SMALL							0x0
#define	TEA5761UK_TESTREG_CONTROL_LHSW_LARGE							0x10
#define	TEA5761UK_TESTREG_CONTROL_RHM									BIT(6)			 // Right audio hard mute
#define	TEA5761UK_TESTREG_CONTROL_RHM_NOT_MUTED							0x0
#define	TEA5761UK_TESTREG_CONTROL_RHM_HARD_MUTED						0x40
#define	TEA5761UK_TESTREG_CONTROL_LHM									BIT(7)			 // Left audio hard mute
#define	TEA5761UK_TESTREG_CONTROL_LHM_NOT_MUTED							0x0
#define	TEA5761UK_TESTREG_CONTROL_LHM_HARD_MUTED						0x80

/* Detection time and software-port test mode */
#define	TEA5761UK_TESTREG_CONFIG										0x0B
#define	TEA5761UK_TESTREG_CONFIG_TB_3_0									GENMASK(3, 0)	 // Software-port test signal selection
#define	TEA5761UK_TESTREG_CONFIG_TB_3_0_SHIFT							0
#define	TEA5761UK_TESTREG_CONFIG_TM										BIT(4)			 // Test mode and software-port test output enable
#define	TEA5761UK_TESTREG_CONFIG_TM_NORMAL								0x0
#define	TEA5761UK_TESTREG_CONFIG_TM_TEST_MODE							0x10
#define	TEA5761UK_TESTREG_CONFIG_DETT									BIT(7)			 // Detection time
#define	TEA5761UK_TESTREG_CONFIG_DETT_SLOW_8_MS							0x0
#define	TEA5761UK_TESTREG_CONFIG_DETT_FAST_4_MS							0x80

/* Version code and manufacturer ID MSBs */
#define	TEA5761UK_MANID_HIGH											0x0C
#define	TEA5761UK_MANID_HIGH_MAN_ID_10_07								GENMASK(3, 0)	 // Manufacturer ID bits 10 to 7
#define	TEA5761UK_MANID_HIGH_MAN_ID_10_07_SHIFT							0
#define	TEA5761UK_MANID_HIGH_VERSION									GENMASK(7, 4)	 // Device version code
#define	TEA5761UK_MANID_HIGH_VERSION_SHIFT								4

/* Manufacturer ID LSBs and availability */
#define	TEA5761UK_MANID_LOW												0x0D
#define	TEA5761UK_MANID_LOW_IDAV										BIT(0)			 // Manufacturer ID availability flag
#define	TEA5761UK_MANID_LOW_IDAV_NOT_AVAILABLE							0x0
#define	TEA5761UK_MANID_LOW_IDAV_AVAILABLE								0x1
#define	TEA5761UK_MANID_LOW_MAN_ID_06_00								GENMASK(7, 1)	 // Manufacturer ID bits 6 to 0
#define	TEA5761UK_MANID_LOW_MAN_ID_06_00_SHIFT							1

/* Chip identification code MSBs */
#define	TEA5761UK_CHIPID_HIGH											0x0E
#define	TEA5761UK_CHIPID_HIGH_CHIP_ID_15_08								GENMASK(7, 0)	 // Chip ID bits 15 to 8
#define	TEA5761UK_CHIPID_HIGH_CHIP_ID_15_08_SHIFT						0

/* Chip identification code LSBs */
#define	TEA5761UK_CHIPID_LOW											0x0F
#define	TEA5761UK_CHIPID_LOW_CHIP_ID_07_00								GENMASK(7, 0)	 // Chip ID bits 7 to 0
#define	TEA5761UK_CHIPID_LOW_CHIP_ID_07_00_SHIFT						0
