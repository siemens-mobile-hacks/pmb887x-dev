#pragma once

#include <bitops.h> // IWYU pragma: export

// HD15515X
// Hitachi HD155153NP, HD155155NPEB and HD155165BP GSM RF transceivers
/* Channel synthesizer configuration */
#define	HD15515X_CHANNEL_PLL						0x01
#define	HD15515X_CHANNEL_PLL_BAND					GENMASK(1, 0)	 // Synthesizer band code
#define	HD15515X_CHANNEL_PLL_BAND_SHIFT				0
#define	HD15515X_CHANNEL_PLL_DIVIDER				GENMASK(15, 2)	 // Synthesizer divider in 200 kHz steps, 100 kHz steps in bands 0 and 1
#define	HD15515X_CHANNEL_PLL_DIVIDER_SHIFT			2
#define	HD15515X_CHANNEL_PLL_RECEIVE				BIT(16)			 // Receive path
#define	HD15515X_CHANNEL_PLL_TX_OFFSET				BIT(18)			 // Transmit offset below the synthesizer
#define	HD15515X_CHANNEL_PLL_TX_OFFSET_OFFSET_80MHZ	0x0
#define	HD15515X_CHANNEL_PLL_TX_OFFSET_OFFSET_82MHZ	0x40000
#define	HD15515X_CHANNEL_PLL_MODE					GENMASK(20, 19)	 // Operating mode
#define	HD15515X_CHANNEL_PLL_MODE_SHIFT				19

/* Receiver gain configuration */
#define	HD15515X_GAIN_CONTROL						0x02
#define	HD15515X_GAIN_CONTROL_PGA_GAIN				GENMASK(5, 0)	 // Programmable-gain amplifier code
#define	HD15515X_GAIN_CONTROL_PGA_GAIN_SHIFT		0
#define	HD15515X_GAIN_CONTROL_FRONT_END				GENMASK(7, 6)	 // Front-end gain stage control
#define	HD15515X_GAIN_CONTROL_FRONT_END_SHIFT		6
#define	HD15515X_GAIN_CONTROL_DATA					GENMASK(20, 8)	 // Raw configuration data
#define	HD15515X_GAIN_CONTROL_DATA_SHIFT			8

/* Undocumented control register */
#define	HD15515X_CONTROL_R3							0x03
#define	HD15515X_CONTROL_R3_DATA					GENMASK(20, 0)	 // Raw control data
#define	HD15515X_CONTROL_R3_DATA_SHIFT				0

/* Undocumented 8-bit control register */
#define	HD15515X_CONTROL_R4							0x04
#define	HD15515X_CONTROL_R4_DATA					GENMASK(4, 0)	 // Raw control data
#define	HD15515X_CONTROL_R4_DATA_SHIFT				0

/* Undocumented 24-bit configuration register */
#define	HD15515X_CONFIG_R5							0x05
#define	HD15515X_CONFIG_R5_DATA						GENMASK(20, 0)	 // Raw configuration data
#define	HD15515X_CONFIG_R5_DATA_SHIFT				0

/* Calibration and configuration commands */
#define	HD15515X_CALIBRATION_R6						0x06
#define	HD15515X_CALIBRATION_R6_SLOT				GENMASK(2, 0)	 // Command slot
#define	HD15515X_CALIBRATION_R6_SLOT_SHIFT			0
#define	HD15515X_CALIBRATION_R6_DATA				GENMASK(20, 3)	 // Raw command data
#define	HD15515X_CALIBRATION_R6_DATA_SHIFT			3

/* Undocumented 8-bit control register */
#define	HD15515X_CONTROL_R7							0x07
#define	HD15515X_CONTROL_R7_DATA					GENMASK(4, 0)	 // Raw control data
#define	HD15515X_CONTROL_R7_DATA_SHIFT				0
