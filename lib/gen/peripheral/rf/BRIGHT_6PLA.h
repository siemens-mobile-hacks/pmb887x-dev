#pragma once

#include <bitops.h> // IWYU pragma: export

// BRIGHT_6PLA
// Renesas Bright 6PLA GSM/EDGE RF transceiver
/* Undocumented control register */
#define	BRIGHT_6PLA_CONTROL_R0				0x00
#define	BRIGHT_6PLA_CONTROL_R0_DATA			GENMASK(19, 0)	 // Raw control data
#define	BRIGHT_6PLA_CONTROL_R0_DATA_SHIFT	0

/* Channel synthesizer configuration */
#define	BRIGHT_6PLA_CHANNEL					0x01
#define	BRIGHT_6PLA_CHANNEL_CONTROL			GENMASK(2, 0)	 // Undocumented control bits
#define	BRIGHT_6PLA_CHANNEL_CONTROL_SHIFT	0
#define	BRIGHT_6PLA_CHANNEL_RECEIVE			BIT(3)			 // Receive path
#define	BRIGHT_6PLA_CHANNEL_FREQUENCY		GENMASK(17, 4)	 // Channel frequency in 100 kHz steps in bands 0 and 1, 200 kHz steps in bands 2 and 3
#define	BRIGHT_6PLA_CHANNEL_FREQUENCY_SHIFT	4
#define	BRIGHT_6PLA_CHANNEL_BAND			GENMASK(19, 18)	 // Synthesizer band code
#define	BRIGHT_6PLA_CHANNEL_BAND_SHIFT		18

/* Receiver gain configuration */
#define	BRIGHT_6PLA_GAIN					0x02
#define	BRIGHT_6PLA_GAIN_LNA				GENMASK(5, 4)	 // LNA and mixer gain mode
#define	BRIGHT_6PLA_GAIN_LNA_SHIFT			4
#define	BRIGHT_6PLA_GAIN_PGA				GENMASK(11, 6)	 // Baseband amplifier gain in 2 dB steps
#define	BRIGHT_6PLA_GAIN_PGA_SHIFT			6

/* Undocumented control register */
#define	BRIGHT_6PLA_CONTROL_R4				0x04
#define	BRIGHT_6PLA_CONTROL_R4_DATA			GENMASK(19, 0)	 // Raw control data
#define	BRIGHT_6PLA_CONTROL_R4_DATA_SHIFT	0

/* Undocumented control register */
#define	BRIGHT_6PLA_CONTROL_R5				0x05
#define	BRIGHT_6PLA_CONTROL_R5_DATA			GENMASK(19, 0)	 // Raw control data
#define	BRIGHT_6PLA_CONTROL_R5_DATA_SHIFT	0

/* Undocumented control register */
#define	BRIGHT_6PLA_CONTROL_R6				0x06
#define	BRIGHT_6PLA_CONTROL_R6_DATA			GENMASK(19, 0)	 // Raw control data
#define	BRIGHT_6PLA_CONTROL_R6_DATA_SHIFT	0

/* Undocumented control register */
#define	BRIGHT_6PLA_CONTROL_R7				0x07
#define	BRIGHT_6PLA_CONTROL_R7_DATA			GENMASK(19, 0)	 // Raw control data
#define	BRIGHT_6PLA_CONTROL_R7_DATA_SHIFT	0
