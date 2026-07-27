#pragma once

#include <bitops.h> // IWYU pragma: export

// PMB6812
// Infineon PMB6812 SM-POWER power-management IC
#define	PMB6812_I2C_ADDR							0x08

/* Reset and shutdown control */
#define	PMB6812_RESCTRL								0x01
#define	PMB6812_RESCTRL_RES							BIT(0)			 // Trigger internal and external reset
#define	PMB6812_RESCTRL_ALLOFF						BIT(6)			 // Shut down all regulators except LRTC
#define	PMB6812_RESCTRL_RESDN						BIT(7)			 // Generate external reset during power-down

/* LINT, LANA, LBB1, and LBB2 control */
#define	PMB6812_PWCTRL1								0x02
#define	PMB6812_PWCTRL1_LANAMD						BIT(0)			 // Enable LANA
#define	PMB6812_PWCTRL1_LBBMD						GENMASK(2, 1)	 // LBB1 and LBB2 operating mode
#define	PMB6812_PWCTRL1_LBBMD_SHIFT					1
#define	PMB6812_PWCTRL1_LBBMD_OFF					0x0
#define	PMB6812_PWCTRL1_LBBMD_LBB1_VCXO_LBB2_ON		0x2
#define	PMB6812_PWCTRL1_LBBMD_LBB1_OFF_LBB2_ON		0x4
#define	PMB6812_PWCTRL1_LBBMD_ON					0x6
#define	PMB6812_PWCTRL1_LBBV						BIT(3)			 // LBB1 and LBB2 voltage
#define	PMB6812_PWCTRL1_LBBV_1V5					0x0
#define	PMB6812_PWCTRL1_LBBV_1V65					0x8
#define	PMB6812_PWCTRL1_LINTMD						BIT(4)			 // LINT operating mode
#define	PMB6812_PWCTRL1_LINTMD_STANDBY				0x0
#define	PMB6812_PWCTRL1_LINTMD_ON					0x10

/* SDBB converter control */
#define	PMB6812_PWCTRL2								0x03
#define	PMB6812_PWCTRL2_VSEL						GENMASK(1, 0)	 // SDBB output voltage
#define	PMB6812_PWCTRL2_VSEL_SHIFT					0
#define	PMB6812_PWCTRL2_VSEL_1V5					0x0
#define	PMB6812_PWCTRL2_VSEL_1V8					0x1
#define	PMB6812_PWCTRL2_VSEL_TEST					0x2
#define	PMB6812_PWCTRL2_VSEL_1V92					0x3
#define	PMB6812_PWCTRL2_LPEN						BIT(4)			 // Enable PFM low-power mode
#define	PMB6812_PWCTRL2_ASPWM						BIT(5)			 // Enable automatic switchback to PWM
#define	PMB6812_PWCTRL2_SDBBMD						GENMASK(7, 6)	 // SDBB operating mode
#define	PMB6812_PWCTRL2_SDBBMD_SHIFT				6
#define	PMB6812_PWCTRL2_SDBBMD_FORCED_PWM			0x0
#define	PMB6812_PWCTRL2_SDBBMD_AUTO_PFM				0x40
#define	PMB6812_PWCTRL2_SDBBMD_TEST					0x80
#define	PMB6812_PWCTRL2_SDBBMD_FORCED_PFM			0xC0

/* Charger mode, voltage, and pulse length */
#define	PMB6812_CHCTRL1								0x04
#define	PMB6812_CHCTRL1_PL							GENMASK(2, 0)	 // Charging pulse length
#define	PMB6812_CHCTRL1_PL_SHIFT					0
#define	PMB6812_CHCTRL1_PL_2MS						0x0
#define	PMB6812_CHCTRL1_PL_4MS						0x1
#define	PMB6812_CHCTRL1_PL_8MS						0x2
#define	PMB6812_CHCTRL1_PL_16MS						0x3
#define	PMB6812_CHCTRL1_PL_33MS						0x4
#define	PMB6812_CHCTRL1_PL_66MS						0x5
#define	PMB6812_CHCTRL1_PL_131MS					0x6
#define	PMB6812_CHCTRL1_PL_262MS					0x7
#define	PMB6812_CHCTRL1_VL							BIT(3)			 // Pulse-mode voltage-limit handling
#define	PMB6812_CHCTRL1_VL_SHUTDOWN					0x0
#define	PMB6812_CHCTRL1_VL_VOLTAGE_LIMIT			0x8
#define	PMB6812_CHCTRL1_VMAX						GENMASK(5, 4)	 // Maximum charging voltage
#define	PMB6812_CHCTRL1_VMAX_SHIFT					4
#define	PMB6812_CHCTRL1_VMAX_VCHMAX1				0x0
#define	PMB6812_CHCTRL1_VMAX_VCHMAX2				0x10
#define	PMB6812_CHCTRL1_VMAX_RESERVED				0x20
#define	PMB6812_CHCTRL1_VMAX_VCHMAX3				0x30
#define	PMB6812_CHCTRL1_PCH							BIT(6)			 // Charging mode
#define	PMB6812_CHCTRL1_PCH_CONTINUOUS				0x0
#define	PMB6812_CHCTRL1_PCH_PULSE					0x40
#define	PMB6812_CHCTRL1_ON							BIT(7)			 // Enable charger

/* Charger and regulator interrupt control */
#define	PMB6812_INTCTRL1							0x05
#define	PMB6812_INTCTRL1_INTMD						BIT(0)			 // INTOUT interrupt edge
#define	PMB6812_INTCTRL1_INTMD_FALLING				0x0
#define	PMB6812_INTCTRL1_INTMD_RISING				0x1
#define	PMB6812_INTCTRL1_EIORLSIM					BIT(1)			 // Interrupt when LSIM regulation state changes
#define	PMB6812_INTCTRL1_EIORLMMC					BIT(2)			 // Interrupt when LMMC regulation state changes
#define	PMB6812_INTCTRL1_EISPWM						BIT(3)			 // Interrupt on automatic switchback to PWM
#define	PMB6812_INTCTRL1_EICHCAL					BIT(5)			 // Interrupt when charging-current limit state changes
#define	PMB6812_INTCTRL1_EICHMD						BIT(6)			 // Interrupt when charging mode changes
#define	PMB6812_INTCTRL1_EICHV						BIT(7)			 // Interrupt when charging-voltage availability changes

/* Temperature and power-button interrupt control */
#define	PMB6812_INTCTRL2							0x06
#define	PMB6812_INTCTRL2_DEBUG						BIT(0)			 // Error-flag reporting mode
#define	PMB6812_INTCTRL2_DEBUG_CURRENT				0x0
#define	PMB6812_INTCTRL2_DEBUG_LATCHED				0x1
#define	PMB6812_INTCTRL2_EION						BIT(1)			 // Interrupt when ON pin level changes
#define	PMB6812_INTCTRL2_OTSEN						BIT(4)			 // Enable overtemperature shutdown
#define	PMB6812_INTCTRL2_RAGOTW						BIT(6)			 // Reduce audio gain on overtemperature warning
#define	PMB6812_INTCTRL2_EIOTW						BIT(7)			 // Interrupt when overtemperature-warning state changes

/* LRF2 regulator control */
#define	PMB6812_PWCTRL3								0x07
#define	PMB6812_PWCTRL3_LRF2MD						GENMASK(1, 0)	 // LRF2 operating mode
#define	PMB6812_PWCTRL3_LRF2MD_SHIFT				0
#define	PMB6812_PWCTRL3_LRF2MD_OFF					0x0
#define	PMB6812_PWCTRL3_LRF2MD_VCXO					0x1
#define	PMB6812_PWCTRL3_LRF2MD_FORCED_OFF			0x2
#define	PMB6812_PWCTRL3_LRF2MD_ON					0x3
#define	PMB6812_PWCTRL3_LRF2V						BIT(2)			 // LRF2 output voltage
#define	PMB6812_PWCTRL3_LRF2V_2V7					0x0
#define	PMB6812_PWCTRL3_LRF2V_2V5					0x4

/* Charger current control */
#define	PMB6812_CHCTRL2								0x08
#define	PMB6812_CHCTRL2_CHCLIM						GENMASK(2, 0)	 // Battery charging-current limit
#define	PMB6812_CHCTRL2_CHCLIM_SHIFT				0
#define	PMB6812_CHCTRL2_CHCLIM_400MA				0x0
#define	PMB6812_CHCTRL2_CHCLIM_500MA				0x1
#define	PMB6812_CHCTRL2_CHCLIM_600MA				0x2
#define	PMB6812_CHCTRL2_CHCLIM_700MA				0x3
#define	PMB6812_CHCTRL2_CHCLIM_800MA				0x4
#define	PMB6812_CHCTRL2_CHCLIM_900MA				0x5
#define	PMB6812_CHCTRL2_CHCLIM_1000MA				0x6
#define	PMB6812_CHCTRL2_CHCLIM_1100MA				0x7
#define	PMB6812_CHCTRL2_PREOFF						BIT(4)			 // Disable precharging
#define	PMB6812_CHCTRL2_RVM							GENMASK(7, 5)	 // Charging-current measurement reference
#define	PMB6812_CHCTRL2_RVM_SHIFT					5
#define	PMB6812_CHCTRL2_RVM_0MA						0x0
#define	PMB6812_CHCTRL2_RVM_100MA					0x20
#define	PMB6812_CHCTRL2_RVM_200MA					0x40
#define	PMB6812_CHCTRL2_RVM_300MA					0x60
#define	PMB6812_CHCTRL2_RVM_400MA					0x80
#define	PMB6812_CHCTRL2_RVM_500MA					0xA0
#define	PMB6812_CHCTRL2_RVM_600MA					0xC0
#define	PMB6812_CHCTRL2_RVM_700MA					0xE0

/* Main LED driver control */
#define	PMB6812_LEDCTRL1							0x0A
#define	PMB6812_LEDCTRL1_LEDON						BIT(0)			 // Enable main LED driver
#define	PMB6812_LEDCTRL1_LEDPWM						GENMASK(6, 1)	 // LED PWM duty-cycle code from 0 to 63
#define	PMB6812_LEDCTRL1_LEDPWM_SHIFT				1

/* Vibrator driver control */
#define	PMB6812_DRVCTRL								0x0B
#define	PMB6812_DRVCTRL_VVIB						GENMASK(3, 0)	 // Vibrator voltage code from 0.9 V to 2.0 V
#define	PMB6812_DRVCTRL_VVIB_SHIFT					0

/* USB control */
#define	PMB6812_USBCTRL								0x0C
#define	PMB6812_USBCTRL_VALUE						GENMASK(7, 0)	 // Raw PMB6812 USB control value
#define	PMB6812_USBCTRL_VALUE_SHIFT					0

/* Audio amplifier control */
#define	PMB6812_AUDCTRL								0x0D
#define	PMB6812_AUDCTRL_AUDON						BIT(0)			 // Enable audio amplifier
#define	PMB6812_AUDCTRL_MUTE						BIT(1)			 // Mute audio amplifier
#define	PMB6812_AUDCTRL_AUDGAIN						GENMASK(3, 2)	 // Audio amplifier gain
#define	PMB6812_AUDCTRL_AUDGAIN_SHIFT				2
#define	PMB6812_AUDCTRL_AUDGAIN_MINUS_6DB			0x0
#define	PMB6812_AUDCTRL_AUDGAIN_MINUS_1_2DB			0x4
#define	PMB6812_AUDCTRL_AUDGAIN_PLUS_2_7DB			0x8
#define	PMB6812_AUDCTRL_AUDGAIN_PLUS_7_6DB			0xC
#define	PMB6812_AUDCTRL_TRISTATE					BIT(4)			 // Make audio output high-impedance while powered down

/* LRFC and LRF1 regulator control */
#define	PMB6812_PWCTRL4								0x0E
#define	PMB6812_PWCTRL4_LRF1MD						GENMASK(3, 2)	 // LRF1 operating mode
#define	PMB6812_PWCTRL4_LRF1MD_SHIFT				2
#define	PMB6812_PWCTRL4_LRF1MD_OFF					0x0
#define	PMB6812_PWCTRL4_LRF1MD_VCXO					0x4
#define	PMB6812_PWCTRL4_LRF1MD_FORCED_OFF			0x8
#define	PMB6812_PWCTRL4_LRF1MD_ON					0xC
#define	PMB6812_PWCTRL4_LRFCMD						GENMASK(7, 6)	 // LRFC operating mode
#define	PMB6812_PWCTRL4_LRFCMD_SHIFT				6
#define	PMB6812_PWCTRL4_LRFCMD_OFF					0x0
#define	PMB6812_PWCTRL4_LRFCMD_VCXO					0x40
#define	PMB6812_PWCTRL4_LRFCMD_FORCED_OFF			0x80
#define	PMB6812_PWCTRL4_LRFCMD_ON					0xC0

/* LSIM and LSIM2 regulator control */
#define	PMB6812_PWCTRL5								0x11
#define	PMB6812_PWCTRL5_LSIMMD						GENMASK(1, 0)	 // LSIM operating mode
#define	PMB6812_PWCTRL5_LSIMMD_SHIFT				0
#define	PMB6812_PWCTRL5_LSIMMD_OFF					0x0
#define	PMB6812_PWCTRL5_LSIMMD_VCXO_STANDBY			0x1
#define	PMB6812_PWCTRL5_LSIMMD_STANDBY				0x2
#define	PMB6812_PWCTRL5_LSIMMD_ON					0x3
#define	PMB6812_PWCTRL5_LSIMV						BIT(2)			 // LSIM output voltage
#define	PMB6812_PWCTRL5_LSIMV_2V85					0x0
#define	PMB6812_PWCTRL5_LSIMV_1V8					0x4
#define	PMB6812_PWCTRL5_LSIM2MD						GENMASK(5, 3)	 // LSIM2 operating mode
#define	PMB6812_PWCTRL5_LSIM2MD_SHIFT				3
#define	PMB6812_PWCTRL5_LSIM2MD_OFF					0x0
#define	PMB6812_PWCTRL5_LSIM2MD_VCXO_STANDBY		0x8
#define	PMB6812_PWCTRL5_LSIM2MD_ON					0x10
#define	PMB6812_PWCTRL5_LSIM2MD_VCXO_STANDBY_ALT	0x18
#define	PMB6812_PWCTRL5_LSIM2MD_STANDBY				0x20
#define	PMB6812_PWCTRL5_LSIM2MD_ON_ALT1				0x28
#define	PMB6812_PWCTRL5_LSIM2MD_OFF_ALT				0x30
#define	PMB6812_PWCTRL5_LSIM2MD_ON_ALT2				0x38
#define	PMB6812_PWCTRL5_LSIM2V						BIT(6)			 // LSIM2 output voltage
#define	PMB6812_PWCTRL5_LSIM2V_2V85					0x0
#define	PMB6812_PWCTRL5_LSIM2V_1V8					0x40

/* LMMC regulator control */
#define	PMB6812_PWCTRL6								0x12
#define	PMB6812_PWCTRL6_LMMCMD						GENMASK(1, 0)	 // LMMC operating mode
#define	PMB6812_PWCTRL6_LMMCMD_SHIFT				0
#define	PMB6812_PWCTRL6_LMMCMD_OFF					0x0
#define	PMB6812_PWCTRL6_LMMCMD_VCXO_STANDBY			0x1
#define	PMB6812_PWCTRL6_LMMCMD_STANDBY				0x2
#define	PMB6812_PWCTRL6_LMMCMD_ON					0x3
#define	PMB6812_PWCTRL6_LMMCV						BIT(2)			 // LMMC output voltage
#define	PMB6812_PWCTRL6_LMMCV_1V8					0x0
#define	PMB6812_PWCTRL6_LMMCV_2V85					0x4

/* Secondary LED and current control */
#define	PMB6812_LEDCTRL2							0x13
#define	PMB6812_LEDCTRL2_LEDCUR						GENMASK(5, 3)	 // Main LED current
#define	PMB6812_LEDCTRL2_LEDCUR_SHIFT				3
#define	PMB6812_LEDCTRL2_LEDCUR_POWER_DOWN			0x0
#define	PMB6812_LEDCTRL2_LEDCUR_20MA				0x8
#define	PMB6812_LEDCTRL2_LEDCUR_40MA				0x10
#define	PMB6812_LEDCTRL2_LEDCUR_60MA				0x18
#define	PMB6812_LEDCTRL2_LEDCUR_80MA				0x20
#define	PMB6812_LEDCTRL2_LEDCUR_100MA				0x28
#define	PMB6812_LEDCTRL2_LEDCUR_120MA				0x30
#define	PMB6812_LEDCTRL2_LEDCUR_140MA				0x38
#define	PMB6812_LEDCTRL2_SLED1ON					BIT(6)			 // Enable secondary LED driver 1
#define	PMB6812_LEDCTRL2_SLED2ON					BIT(7)			 // Enable secondary LED driver 2

/* First general error flags */
#define	PMB6812_GEF1								0x80
#define	PMB6812_GEF1_LINT							BIT(0)			 // LINT current limit exceeded
#define	PMB6812_GEF1_LANA							BIT(1)			 // LANA current limit exceeded
#define	PMB6812_GEF1_LRF2							BIT(2)			 // LRF2 current limit exceeded
#define	PMB6812_GEF1_LRF1							BIT(3)			 // LRF1 current limit exceeded
#define	PMB6812_GEF1_LRFC							BIT(4)			 // LRFC current limit exceeded

/* Interrupt source flags */
#define	PMB6812_ISF									0x81
#define	PMB6812_ISF_LON								BIT(0)			 // ON pin level changed
#define	PMB6812_ISF_OTS								BIT(1)			 // Overtemperature shutdown caused reset
#define	PMB6812_ISF_SPWM							BIT(2)			 // Automatic switchback from PFM to PWM occurred
#define	PMB6812_ISF_LSIM							BIT(3)			 // LSIM out of regulation
#define	PMB6812_ISF_LMMC							BIT(4)			 // LMMC out of regulation
#define	PMB6812_ISF_OTW								BIT(7)			 // Overtemperature warning

/* Charger status and PMIC identification */
#define	PMB6812_CHST								0x82
#define	PMB6812_CHST_PMUID							BIT(0)			 // Power-management IC family
#define	PMB6812_CHST_PMUID_SM_POWER					0x0
#define	PMB6812_CHST_PMUID_E_POWERLITE				0x1
#define	PMB6812_CHST_CCAL							BIT(5)			 // Charging current exceeds the programmed limit
#define	PMB6812_CHST_CHMD							BIT(6)			 // Charger is current-limited
#define	PMB6812_CHST_CHV							BIT(7)			 // Charging voltage is available

/* Second general error flags */
#define	PMB6812_GEF2								0x83
#define	PMB6812_GEF2_SDBB							BIT(0)			 // SDBB current limit exceeded
#define	PMB6812_GEF2_LBB1							BIT(1)			 // LBB1 current limit exceeded
#define	PMB6812_GEF2_LBB2							BIT(2)			 // LBB2 current limit exceeded
#define	PMB6812_GEF2_LSIM2							BIT(3)			 // LSIM2 current limit exceeded
#define	PMB6812_GEF2_OTWD							BIT(7)			 // Overtemperature warning in debug mode
