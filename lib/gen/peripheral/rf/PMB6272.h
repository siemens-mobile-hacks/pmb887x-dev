#pragma once

#include <bitops.h> // IWYU pragma: export

// PMB6272
// Infineon PMB6272 SMARTi RF transceiver
/* Fractional synthesizer channel word, low part */
#define	PMB6272_CHANNEL1						0x00
#define	PMB6272_CHANNEL1_FRACTION				GENMASK(23, 4)	 // Fractional channel word bits 19..0
#define	PMB6272_CHANNEL1_FRACTION_SHIFT			4

/* Synthesizer channel and operating mode */
#define	PMB6272_CHANNEL2						0x02
#define	PMB6272_CHANNEL2_FRACTION				GENMASK(6, 4)	 // Fractional channel word bits 22..20
#define	PMB6272_CHANNEL2_FRACTION_SHIFT			4
#define	PMB6272_CHANNEL2_INTEGER				GENMASK(14, 7)	 // Integer channel word
#define	PMB6272_CHANNEL2_INTEGER_SHIFT			7
#define	PMB6272_CHANNEL2_POWER_MODE				GENMASK(16, 15)	 // Transceiver power mode
#define	PMB6272_CHANNEL2_POWER_MODE_SHIFT		15
#define	PMB6272_CHANNEL2_POWER_MODE_ALL_OFF		0x0
#define	PMB6272_CHANNEL2_POWER_MODE_UNUSED		0x8000
#define	PMB6272_CHANNEL2_POWER_MODE_PLL_TEST	0x10000
#define	PMB6272_CHANNEL2_POWER_MODE_ALL_ON		0x18000
#define	PMB6272_CHANNEL2_TRX					BIT(18)			 // Receive or transmit path
#define	PMB6272_CHANNEL2_TRX_RX					0x0
#define	PMB6272_CHANNEL2_TRX_TX					0x40000
#define	PMB6272_CHANNEL2_BAND					GENMASK(20, 19)	 // RF band
#define	PMB6272_CHANNEL2_BAND_SHIFT				19
#define	PMB6272_CHANNEL2_BAND_GSM850			0x0
#define	PMB6272_CHANNEL2_BAND_GSM900			0x80000
#define	PMB6272_CHANNEL2_BAND_DCS1800			0x100000
#define	PMB6272_CHANNEL2_BAND_PCS1900			0x180000

/* Receiver gain and burst activation */
#define	PMB6272_RXTX							0x04
#define	PMB6272_RXTX_RXGAIN						GENMASK(5, 4)	 // Receiver gain
#define	PMB6272_RXTX_RXGAIN_SHIFT				4
#define	PMB6272_RXTX_RXGAIN_LOW					0x0
#define	PMB6272_RXTX_RXGAIN_MEDIUM				0x10
#define	PMB6272_RXTX_RXGAIN_UNSUPPORTED			0x20
#define	PMB6272_RXTX_RXGAIN_HIGH				0x30
#define	PMB6272_RXTX_RXCORR						GENMASK(9, 6)	 // Receiver gain correction
#define	PMB6272_RXTX_RXCORR_SHIFT				6
#define	PMB6272_RXTX_RXCM						GENMASK(12, 11)	 // Mandatory receiver common-mode setting
#define	PMB6272_RXTX_RXCM_SHIFT					11
#define	PMB6272_RXTX_RXGS0						BIT(13)			 // Enable +12 dB receiver gain step
#define	PMB6272_RXTX_RXGS1						BIT(14)			 // Enable +6 dB receiver gain step
#define	PMB6272_RXTX_RXGS2						BIT(15)			 // Enable -6 dB receiver gain step
#define	PMB6272_RXTX_RXGS3						BIT(16)			 // Enable -3 dB receiver gain step
#define	PMB6272_RXTX_OFC						BIT(17)			 // Enable DC-offset compensation

/* Crystal oscillator mode and calibration */
#define	PMB6272_XO_INIT1						0x06
#define	PMB6272_XO_INIT1_XOMODE0				BIT(4)			 // Use an external oscillator signal
#define	PMB6272_XO_INIT1_XOMODE1				BIT(5)			 // Mandatory oscillator-mode bit
#define	PMB6272_XO_INIT1_XOMODE3				BIT(7)			 // Mandatory oscillator-mode bit
#define	PMB6272_XO_INIT1_XOSETUP				GENMASK(19, 16)	 // Crystal-core capacitance adaptation
#define	PMB6272_XO_INIT1_XOSETUP_SHIFT			16
#define	PMB6272_XO_INIT1_XOCAL					GENMASK(22, 20)	 // Crystal subrange selection
#define	PMB6272_XO_INIT1_XOCAL_SHIFT			20
#define	PMB6272_XO_INIT1_CRYSTAL_SIZE			BIT(23)			 // Crystal load capacitance
#define	PMB6272_XO_INIT1_CRYSTAL_SIZE_10PF		0x0
#define	PMB6272_XO_INIT1_CRYSTAL_SIZE_8PF		0x800000

/* Crystal oscillator linearization configuration */
#define	PMB6272_XO_INIT3						0x08
#define	PMB6272_XO_INIT3_VALUE					GENMASK(23, 4)	 // Undocumented configuration value
#define	PMB6272_XO_INIT3_VALUE_SHIFT			4

/* Crystal oscillator linearization coefficients */
#define	PMB6272_XO_INIT2						0x0A
#define	PMB6272_XO_INIT2_ALPHA					GENMASK(13, 4)	 // LUXO alpha coefficient
#define	PMB6272_XO_INIT2_ALPHA_SHIFT			4
#define	PMB6272_XO_INIT2_GAMMA					GENMASK(23, 14)	 // LUXO gamma coefficient
#define	PMB6272_XO_INIT2_GAMMA_SHIFT			14

/* Crystal oscillator frequency correction */
#define	PMB6272_XO_TUNE							0x0C
#define	PMB6272_XO_TUNE_AFC						GENMASK(20, 4)	 // Frequency-correction value
#define	PMB6272_XO_TUNE_AFC_SHIFT				4
