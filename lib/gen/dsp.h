#pragma once

#ifdef PMB8875
#include "pmb8875_dsp.h" // IWYU pragma: export
#endif

#ifdef PMB8876
#include "pmb8876_dsp.h" // IWYU pragma: export
#endif

// TeakLite peripheral registers
/* Interrupt A0 flag register (read-only, modified by hardware) */
#define	TEAK_INT_FINTA0							(TEAK_INT_BASE + 0x00)
/* MCU interrupt 0 */
#define	TEAK_INT_FINTA0_MCU0					0x0001
#define	TEAK_INT_FINTA0_MCU0_SHIFT				0
/* MCU interrupt 1 */
#define	TEAK_INT_FINTA0_MCU1					0x0002
#define	TEAK_INT_FINTA0_MCU1_SHIFT				1
/* MCU interrupt 2 */
#define	TEAK_INT_FINTA0_MCU2					0x0004
#define	TEAK_INT_FINTA0_MCU2_SHIFT				2
/* MCU interrupt 3 */
#define	TEAK_INT_FINTA0_MCU3					0x0008
#define	TEAK_INT_FINTA0_MCU3_SHIFT				3
/* GSM frame interrupt */
#define	TEAK_INT_FINTA0_FRAME					0x0010
#define	TEAK_INT_FINTA0_FRAME_SHIFT				4
/* GSM timer CODON rising edge */
#define	TEAK_INT_FINTA0_CODONHI					0x0020
#define	TEAK_INT_FINTA0_CODONHI_SHIFT			5
/* GSM timer CODON falling edge */
#define	TEAK_INT_FINTA0_CODONLO					0x0040
#define	TEAK_INT_FINTA0_CODONLO_SHIFT			6
/* Modulator interrupt */
#define	TEAK_INT_FINTA0_MODU					0x0080
#define	TEAK_INT_FINTA0_MODU_SHIFT				7
/* Channel decoder interrupt */
#define	TEAK_INT_FINTA0_CHADEC					0x0100
#define	TEAK_INT_FINTA0_CHADEC_SHIFT			8
/* Equalizer interrupt */
#define	TEAK_INT_FINTA0_EQ						0x0200
#define	TEAK_INT_FINTA0_EQ_SHIFT				9
/* Baseband start signal */
#define	TEAK_INT_FINTA0_BBHI					0x0400
#define	TEAK_INT_FINTA0_BBHI_SHIFT				10
/* Baseband stop signal */
#define	TEAK_INT_FINTA0_BBLO					0x0800
#define	TEAK_INT_FINTA0_BBLO_SHIFT				11
/* Baseband buffer full interrupt */
#define	TEAK_INT_FINTA0_BB_FULL					0x1000
#define	TEAK_INT_FINTA0_BB_FULL_SHIFT			12
/* Interrupt A0 enable register */
#define	TEAK_INT_EINTA0							(TEAK_INT_BASE + 0x01)
/* MCU interrupt 0 enable */
#define	TEAK_INT_EINTA0_MCU0					0x0001
#define	TEAK_INT_EINTA0_MCU0_SHIFT				0
/* MCU interrupt 1 enable */
#define	TEAK_INT_EINTA0_MCU1					0x0002
#define	TEAK_INT_EINTA0_MCU1_SHIFT				1
/* MCU interrupt 2 enable */
#define	TEAK_INT_EINTA0_MCU2					0x0004
#define	TEAK_INT_EINTA0_MCU2_SHIFT				2
/* MCU interrupt 3 enable */
#define	TEAK_INT_EINTA0_MCU3					0x0008
#define	TEAK_INT_EINTA0_MCU3_SHIFT				3
/* GSM frame interrupt enable */
#define	TEAK_INT_EINTA0_FRAME					0x0010
#define	TEAK_INT_EINTA0_FRAME_SHIFT				4
/* GSM timer CODON rising-edge interrupt enable */
#define	TEAK_INT_EINTA0_CODONHI					0x0020
#define	TEAK_INT_EINTA0_CODONHI_SHIFT			5
/* GSM timer CODON falling-edge interrupt enable */
#define	TEAK_INT_EINTA0_CODONLO					0x0040
#define	TEAK_INT_EINTA0_CODONLO_SHIFT			6
/* Modulator interrupt enable */
#define	TEAK_INT_EINTA0_MODU					0x0080
#define	TEAK_INT_EINTA0_MODU_SHIFT				7
/* Channel decoder interrupt enable */
#define	TEAK_INT_EINTA0_CHADEC					0x0100
#define	TEAK_INT_EINTA0_CHADEC_SHIFT			8
/* Equalizer interrupt enable */
#define	TEAK_INT_EINTA0_EQ						0x0200
#define	TEAK_INT_EINTA0_EQ_SHIFT				9
/* Baseband start interrupt enable */
#define	TEAK_INT_EINTA0_BBHI					0x0400
#define	TEAK_INT_EINTA0_BBHI_SHIFT				10
/* Baseband stop interrupt enable */
#define	TEAK_INT_EINTA0_BBLO					0x0800
#define	TEAK_INT_EINTA0_BBLO_SHIFT				11
/* Baseband buffer full interrupt enable */
#define	TEAK_INT_EINTA0_BB_FULL					0x1000
#define	TEAK_INT_EINTA0_BB_FULL_SHIFT			12
/* Interrupt A0 reset register (write-only) */
#define	TEAK_INT_RINTA0							(TEAK_INT_BASE + 0x02)
/* MCU interrupt 0 reset */
#define	TEAK_INT_RINTA0_MCU0					0x0001
#define	TEAK_INT_RINTA0_MCU0_SHIFT				0
/* MCU interrupt 1 reset */
#define	TEAK_INT_RINTA0_MCU1					0x0002
#define	TEAK_INT_RINTA0_MCU1_SHIFT				1
/* MCU interrupt 2 reset */
#define	TEAK_INT_RINTA0_MCU2					0x0004
#define	TEAK_INT_RINTA0_MCU2_SHIFT				2
/* MCU interrupt 3 reset */
#define	TEAK_INT_RINTA0_MCU3					0x0008
#define	TEAK_INT_RINTA0_MCU3_SHIFT				3
/* GSM frame interrupt reset */
#define	TEAK_INT_RINTA0_FRAME					0x0010
#define	TEAK_INT_RINTA0_FRAME_SHIFT				4
/* GSM timer CODON rising-edge interrupt reset */
#define	TEAK_INT_RINTA0_CODONHI					0x0020
#define	TEAK_INT_RINTA0_CODONHI_SHIFT			5
/* GSM timer CODON falling-edge interrupt reset */
#define	TEAK_INT_RINTA0_CODONLO					0x0040
#define	TEAK_INT_RINTA0_CODONLO_SHIFT			6
/* Modulator interrupt reset */
#define	TEAK_INT_RINTA0_MODU					0x0080
#define	TEAK_INT_RINTA0_MODU_SHIFT				7
/* Channel decoder interrupt reset */
#define	TEAK_INT_RINTA0_CHADEC					0x0100
#define	TEAK_INT_RINTA0_CHADEC_SHIFT			8
/* Equalizer interrupt reset */
#define	TEAK_INT_RINTA0_EQ						0x0200
#define	TEAK_INT_RINTA0_EQ_SHIFT				9
/* Baseband start interrupt reset */
#define	TEAK_INT_RINTA0_BBHI					0x0400
#define	TEAK_INT_RINTA0_BBHI_SHIFT				10
/* Baseband stop interrupt reset */
#define	TEAK_INT_RINTA0_BBLO					0x0800
#define	TEAK_INT_RINTA0_BBLO_SHIFT				11
/* Baseband buffer full interrupt reset */
#define	TEAK_INT_RINTA0_BB_FULL					0x1000
#define	TEAK_INT_RINTA0_BB_FULL_SHIFT			12
/* Interrupt A0 set register (write-only) */
#define	TEAK_INT_SINTA0							(TEAK_INT_BASE + 0x03)
/* MCU interrupt 0 set */
#define	TEAK_INT_SINTA0_MCU0					0x0001
#define	TEAK_INT_SINTA0_MCU0_SHIFT				0
/* MCU interrupt 1 set */
#define	TEAK_INT_SINTA0_MCU1					0x0002
#define	TEAK_INT_SINTA0_MCU1_SHIFT				1
/* MCU interrupt 2 set */
#define	TEAK_INT_SINTA0_MCU2					0x0004
#define	TEAK_INT_SINTA0_MCU2_SHIFT				2
/* MCU interrupt 3 set */
#define	TEAK_INT_SINTA0_MCU3					0x0008
#define	TEAK_INT_SINTA0_MCU3_SHIFT				3
/* GSM frame interrupt set */
#define	TEAK_INT_SINTA0_FRAME					0x0010
#define	TEAK_INT_SINTA0_FRAME_SHIFT				4
/* GSM timer CODON rising-edge interrupt set */
#define	TEAK_INT_SINTA0_CODONHI					0x0020
#define	TEAK_INT_SINTA0_CODONHI_SHIFT			5
/* GSM timer CODON falling-edge interrupt set */
#define	TEAK_INT_SINTA0_CODONLO					0x0040
#define	TEAK_INT_SINTA0_CODONLO_SHIFT			6
/* Modulator interrupt set */
#define	TEAK_INT_SINTA0_MODU					0x0080
#define	TEAK_INT_SINTA0_MODU_SHIFT				7
/* Channel decoder interrupt set */
#define	TEAK_INT_SINTA0_CHADEC					0x0100
#define	TEAK_INT_SINTA0_CHADEC_SHIFT			8
/* Equalizer interrupt set */
#define	TEAK_INT_SINTA0_EQ						0x0200
#define	TEAK_INT_SINTA0_EQ_SHIFT				9
/* Baseband start interrupt set */
#define	TEAK_INT_SINTA0_BBHI					0x0400
#define	TEAK_INT_SINTA0_BBHI_SHIFT				10
/* Baseband stop interrupt set */
#define	TEAK_INT_SINTA0_BBLO					0x0800
#define	TEAK_INT_SINTA0_BBLO_SHIFT				11
/* Baseband buffer full interrupt set */
#define	TEAK_INT_SINTA0_BB_FULL					0x1000
#define	TEAK_INT_SINTA0_BB_FULL_SHIFT			12
/* Interrupt B0 flag register (read-only, modified by hardware) */
#define	TEAK_INT_FINTB0							(TEAK_INT_BASE + 0x04)
/* I2S1 transmit interrupt */
#define	TEAK_INT_FINTB0_I2S1TX					0x0001
#define	TEAK_INT_FINTB0_I2S1TX_SHIFT			0
/* I2S1 receive interrupt */
#define	TEAK_INT_FINTB0_I2S1RX					0x0002
#define	TEAK_INT_FINTB0_I2S1RX_SHIFT			1
/* I2S2 transmit interrupt */
#define	TEAK_INT_FINTB0_I2S2TX					0x0004
#define	TEAK_INT_FINTB0_I2S2TX_SHIFT			2
/* I2S2 receive interrupt */
#define	TEAK_INT_FINTB0_I2S2RX					0x0008
#define	TEAK_INT_FINTB0_I2S2RX_SHIFT			3
/* Voiceband receive interrupt */
#define	TEAK_INT_FINTB0_VBRX					0x0020
#define	TEAK_INT_FINTB0_VBRX_SHIFT				5
/* Voiceband transmit interrupt */
#define	TEAK_INT_FINTB0_VBTX					0x0040
#define	TEAK_INT_FINTB0_VBTX_SHIFT				6
/* SSC receive interrupt */
#define	TEAK_INT_FINTB0_SSC1RX					0x0080
#define	TEAK_INT_FINTB0_SSC1RX_SHIFT			7
/* SSC transmit interrupt */
#define	TEAK_INT_FINTB0_SSC1TX					0x0100
#define	TEAK_INT_FINTB0_SSC1TX_SHIFT			8
/* SSC error interrupt */
#define	TEAK_INT_FINTB0_SSC1ERR					0x0200
#define	TEAK_INT_FINTB0_SSC1ERR_SHIFT			9
/* System interface MCU interrupt */
#define	TEAK_INT_FINTB0_SYSMCU					0x0400
#define	TEAK_INT_FINTB0_SYSMCU_SHIFT			10
/* I2S3 transmit interrupt */
#define	TEAK_INT_FINTB0_I2S3TX					0x0800
#define	TEAK_INT_FINTB0_I2S3TX_SHIFT			11
/* Interrupt B0 enable register */
#define	TEAK_INT_EINTB0							(TEAK_INT_BASE + 0x05)
/* I2S1 transmit interrupt enable */
#define	TEAK_INT_EINTB0_I2S1TX					0x0001
#define	TEAK_INT_EINTB0_I2S1TX_SHIFT			0
/* I2S1 receive interrupt enable */
#define	TEAK_INT_EINTB0_I2S1RX					0x0002
#define	TEAK_INT_EINTB0_I2S1RX_SHIFT			1
/* I2S2 transmit interrupt enable */
#define	TEAK_INT_EINTB0_I2S2TX					0x0004
#define	TEAK_INT_EINTB0_I2S2TX_SHIFT			2
/* I2S2 receive interrupt enable */
#define	TEAK_INT_EINTB0_I2S2RX					0x0008
#define	TEAK_INT_EINTB0_I2S2RX_SHIFT			3
/* Voiceband receive interrupt enable */
#define	TEAK_INT_EINTB0_VBRX					0x0020
#define	TEAK_INT_EINTB0_VBRX_SHIFT				5
/* Voiceband transmit interrupt enable */
#define	TEAK_INT_EINTB0_VBTX					0x0040
#define	TEAK_INT_EINTB0_VBTX_SHIFT				6
/* SSC receive interrupt enable */
#define	TEAK_INT_EINTB0_SSC1RX					0x0080
#define	TEAK_INT_EINTB0_SSC1RX_SHIFT			7
/* SSC transmit interrupt enable */
#define	TEAK_INT_EINTB0_SSC1TX					0x0100
#define	TEAK_INT_EINTB0_SSC1TX_SHIFT			8
/* SSC error interrupt enable */
#define	TEAK_INT_EINTB0_SSC1ERR					0x0200
#define	TEAK_INT_EINTB0_SSC1ERR_SHIFT			9
/* System interface MCU interrupt enable */
#define	TEAK_INT_EINTB0_SYSMCU					0x0400
#define	TEAK_INT_EINTB0_SYSMCU_SHIFT			10
/* I2S3 transmit interrupt enable */
#define	TEAK_INT_EINTB0_I2S3TX					0x0800
#define	TEAK_INT_EINTB0_I2S3TX_SHIFT			11
/* Interrupt B0 reset register (write-only) */
#define	TEAK_INT_RINTB0							(TEAK_INT_BASE + 0x06)
/* I2S1 transmit interrupt reset */
#define	TEAK_INT_RINTB0_I2S1TX					0x0001
#define	TEAK_INT_RINTB0_I2S1TX_SHIFT			0
/* I2S1 receive interrupt reset */
#define	TEAK_INT_RINTB0_I2S1RX					0x0002
#define	TEAK_INT_RINTB0_I2S1RX_SHIFT			1
/* I2S2 transmit interrupt reset */
#define	TEAK_INT_RINTB0_I2S2TX					0x0004
#define	TEAK_INT_RINTB0_I2S2TX_SHIFT			2
/* I2S2 receive interrupt reset */
#define	TEAK_INT_RINTB0_I2S2RX					0x0008
#define	TEAK_INT_RINTB0_I2S2RX_SHIFT			3
/* Voiceband receive interrupt reset */
#define	TEAK_INT_RINTB0_VBRX					0x0020
#define	TEAK_INT_RINTB0_VBRX_SHIFT				5
/* Voiceband transmit interrupt reset */
#define	TEAK_INT_RINTB0_VBTX					0x0040
#define	TEAK_INT_RINTB0_VBTX_SHIFT				6
/* SSC receive interrupt reset */
#define	TEAK_INT_RINTB0_SSC1RX					0x0080
#define	TEAK_INT_RINTB0_SSC1RX_SHIFT			7
/* SSC transmit interrupt reset */
#define	TEAK_INT_RINTB0_SSC1TX					0x0100
#define	TEAK_INT_RINTB0_SSC1TX_SHIFT			8
/* SSC error interrupt reset */
#define	TEAK_INT_RINTB0_SSC1ERR					0x0200
#define	TEAK_INT_RINTB0_SSC1ERR_SHIFT			9
/* System interface MCU interrupt reset */
#define	TEAK_INT_RINTB0_SYSMCU					0x0400
#define	TEAK_INT_RINTB0_SYSMCU_SHIFT			10
/* I2S3 transmit interrupt reset */
#define	TEAK_INT_RINTB0_I2S3TX					0x0800
#define	TEAK_INT_RINTB0_I2S3TX_SHIFT			11
/* Interrupt B0 set register (write-only) */
#define	TEAK_INT_SINTB0							(TEAK_INT_BASE + 0x07)
/* I2S1 transmit interrupt set */
#define	TEAK_INT_SINTB0_I2S1TX					0x0001
#define	TEAK_INT_SINTB0_I2S1TX_SHIFT			0
/* I2S1 receive interrupt set */
#define	TEAK_INT_SINTB0_I2S1RX					0x0002
#define	TEAK_INT_SINTB0_I2S1RX_SHIFT			1
/* I2S2 transmit interrupt set */
#define	TEAK_INT_SINTB0_I2S2TX					0x0004
#define	TEAK_INT_SINTB0_I2S2TX_SHIFT			2
/* I2S2 receive interrupt set */
#define	TEAK_INT_SINTB0_I2S2RX					0x0008
#define	TEAK_INT_SINTB0_I2S2RX_SHIFT			3
/* Voiceband receive interrupt set */
#define	TEAK_INT_SINTB0_VBRX					0x0020
#define	TEAK_INT_SINTB0_VBRX_SHIFT				5
/* Voiceband transmit interrupt set */
#define	TEAK_INT_SINTB0_VBTX					0x0040
#define	TEAK_INT_SINTB0_VBTX_SHIFT				6
/* SSC receive interrupt set */
#define	TEAK_INT_SINTB0_SSC1RX					0x0080
#define	TEAK_INT_SINTB0_SSC1RX_SHIFT			7
/* SSC transmit interrupt set */
#define	TEAK_INT_SINTB0_SSC1TX					0x0100
#define	TEAK_INT_SINTB0_SSC1TX_SHIFT			8
/* SSC error interrupt set */
#define	TEAK_INT_SINTB0_SSC1ERR					0x0200
#define	TEAK_INT_SINTB0_SSC1ERR_SHIFT			9
/* System interface MCU interrupt set */
#define	TEAK_INT_SINTB0_SYSMCU					0x0400
#define	TEAK_INT_SINTB0_SYSMCU_SHIFT			10
/* I2S3 transmit interrupt set */
#define	TEAK_INT_SINTB0_I2S3TX					0x0800
#define	TEAK_INT_SINTB0_I2S3TX_SHIFT			11
/* Interrupt 1 flag register (read-only, modified by hardware) */
#define	TEAK_INT_FINT1							(TEAK_INT_BASE + 0x08)
/* Cipher interrupt */
#define	TEAK_INT_FINT1_CIPH						0x0001
#define	TEAK_INT_FINT1_CIPH_SHIFT				0
/* Timer 1 compare 0 interrupt */
#define	TEAK_INT_FINT1_TMR10					0x0002
#define	TEAK_INT_FINT1_TMR10_SHIFT				1
/* Timer 1 compare 1 interrupt */
#define	TEAK_INT_FINT1_TMR11					0x0004
#define	TEAK_INT_FINT1_TMR11_SHIFT				2
/* Timer 2 interrupt */
#define	TEAK_INT_FINT1_TMR2						0x0008
#define	TEAK_INT_FINT1_TMR2_SHIFT				3
/* DSP input 0 rising-edge interrupt */
#define	TEAK_INT_FINT1_DSPIN0HI					0x0010
#define	TEAK_INT_FINT1_DSPIN0HI_SHIFT			4
/* DSP input 0 falling-edge interrupt */
#define	TEAK_INT_FINT1_DSPIN0LO					0x0020
#define	TEAK_INT_FINT1_DSPIN0LO_SHIFT			5
/* DSP input 1 rising-edge interrupt */
#define	TEAK_INT_FINT1_DSPIN1HI					0x0040
#define	TEAK_INT_FINT1_DSPIN1HI_SHIFT			6
/* DSP input 1 falling-edge interrupt */
#define	TEAK_INT_FINT1_DSPIN1LO					0x0080
#define	TEAK_INT_FINT1_DSPIN1LO_SHIFT			7
/* Monitor input 1 rising-edge interrupt */
#define	TEAK_INT_FINT1_MONIN1HI					0x0100
#define	TEAK_INT_FINT1_MONIN1HI_SHIFT			8
/* Monitor input 1 falling-edge interrupt */
#define	TEAK_INT_FINT1_MONIN1LO					0x0200
#define	TEAK_INT_FINT1_MONIN1LO_SHIFT			9
/* Monitor input 2 rising-edge interrupt */
#define	TEAK_INT_FINT1_MONIN2HI					0x0400
#define	TEAK_INT_FINT1_MONIN2HI_SHIFT			10
/* Monitor input 2 falling-edge interrupt */
#define	TEAK_INT_FINT1_MONIN2LO					0x0800
#define	TEAK_INT_FINT1_MONIN2LO_SHIFT			11
/* Monitor input 3 rising-edge interrupt */
#define	TEAK_INT_FINT1_MONIN3HI					0x1000
#define	TEAK_INT_FINT1_MONIN3HI_SHIFT			12
/* Monitor input 3 falling-edge interrupt */
#define	TEAK_INT_FINT1_MONIN3LO					0x2000
#define	TEAK_INT_FINT1_MONIN3LO_SHIFT			13
/* Monitor input 4 rising-edge interrupt */
#define	TEAK_INT_FINT1_MONIN4HI					0x4000
#define	TEAK_INT_FINT1_MONIN4HI_SHIFT			14
/* Monitor input 4 falling-edge interrupt */
#define	TEAK_INT_FINT1_MONIN4LO					0x8000
#define	TEAK_INT_FINT1_MONIN4LO_SHIFT			15
/* Interrupt 1 enable register */
#define	TEAK_INT_EINT1							(TEAK_INT_BASE + 0x09)
/* Cipher interrupt enable */
#define	TEAK_INT_EINT1_CIPH						0x0001
#define	TEAK_INT_EINT1_CIPH_SHIFT				0
/* Timer 1 compare 0 interrupt enable */
#define	TEAK_INT_EINT1_TMR10					0x0002
#define	TEAK_INT_EINT1_TMR10_SHIFT				1
/* Timer 1 compare 1 interrupt enable */
#define	TEAK_INT_EINT1_TMR11					0x0004
#define	TEAK_INT_EINT1_TMR11_SHIFT				2
/* Timer 2 interrupt enable */
#define	TEAK_INT_EINT1_TMR2						0x0008
#define	TEAK_INT_EINT1_TMR2_SHIFT				3
/* DSP input 0 rising-edge interrupt enable */
#define	TEAK_INT_EINT1_DSPIN0HI					0x0010
#define	TEAK_INT_EINT1_DSPIN0HI_SHIFT			4
/* DSP input 0 falling-edge interrupt enable */
#define	TEAK_INT_EINT1_DSPIN0LO					0x0020
#define	TEAK_INT_EINT1_DSPIN0LO_SHIFT			5
/* DSP input 1 rising-edge interrupt enable */
#define	TEAK_INT_EINT1_DSPIN1HI					0x0040
#define	TEAK_INT_EINT1_DSPIN1HI_SHIFT			6
/* DSP input 1 falling-edge interrupt enable */
#define	TEAK_INT_EINT1_DSPIN1LO					0x0080
#define	TEAK_INT_EINT1_DSPIN1LO_SHIFT			7
/* Monitor input 1 rising-edge interrupt enable */
#define	TEAK_INT_EINT1_MONIN1HI					0x0100
#define	TEAK_INT_EINT1_MONIN1HI_SHIFT			8
/* Monitor input 1 falling-edge interrupt enable */
#define	TEAK_INT_EINT1_MONIN1LO					0x0200
#define	TEAK_INT_EINT1_MONIN1LO_SHIFT			9
/* Monitor input 2 rising-edge interrupt enable */
#define	TEAK_INT_EINT1_MONIN2HI					0x0400
#define	TEAK_INT_EINT1_MONIN2HI_SHIFT			10
/* Monitor input 2 falling-edge interrupt enable */
#define	TEAK_INT_EINT1_MONIN2LO					0x0800
#define	TEAK_INT_EINT1_MONIN2LO_SHIFT			11
/* Monitor input 3 rising-edge interrupt enable */
#define	TEAK_INT_EINT1_MONIN3HI					0x1000
#define	TEAK_INT_EINT1_MONIN3HI_SHIFT			12
/* Monitor input 3 falling-edge interrupt enable */
#define	TEAK_INT_EINT1_MONIN3LO					0x2000
#define	TEAK_INT_EINT1_MONIN3LO_SHIFT			13
/* Monitor input 4 rising-edge interrupt enable */
#define	TEAK_INT_EINT1_MONIN4HI					0x4000
#define	TEAK_INT_EINT1_MONIN4HI_SHIFT			14
/* Monitor input 4 falling-edge interrupt enable */
#define	TEAK_INT_EINT1_MONIN4LO					0x8000
#define	TEAK_INT_EINT1_MONIN4LO_SHIFT			15
/* Interrupt 1 reset register (write-only) */
#define	TEAK_INT_RINT1							(TEAK_INT_BASE + 0x0A)
/* Cipher interrupt reset */
#define	TEAK_INT_RINT1_CIPH						0x0001
#define	TEAK_INT_RINT1_CIPH_SHIFT				0
/* Timer 1 compare 0 interrupt reset */
#define	TEAK_INT_RINT1_TMR10					0x0002
#define	TEAK_INT_RINT1_TMR10_SHIFT				1
/* Timer 1 compare 1 interrupt reset */
#define	TEAK_INT_RINT1_TMR11					0x0004
#define	TEAK_INT_RINT1_TMR11_SHIFT				2
/* Timer 2 interrupt reset */
#define	TEAK_INT_RINT1_TMR2						0x0008
#define	TEAK_INT_RINT1_TMR2_SHIFT				3
/* DSP input 0 rising-edge interrupt reset */
#define	TEAK_INT_RINT1_DSPIN0HI					0x0010
#define	TEAK_INT_RINT1_DSPIN0HI_SHIFT			4
/* DSP input 0 falling-edge interrupt reset */
#define	TEAK_INT_RINT1_DSPIN0LO					0x0020
#define	TEAK_INT_RINT1_DSPIN0LO_SHIFT			5
/* DSP input 1 rising-edge interrupt reset */
#define	TEAK_INT_RINT1_DSPIN1HI					0x0040
#define	TEAK_INT_RINT1_DSPIN1HI_SHIFT			6
/* DSP input 1 falling-edge interrupt reset */
#define	TEAK_INT_RINT1_DSPIN1LO					0x0080
#define	TEAK_INT_RINT1_DSPIN1LO_SHIFT			7
/* Monitor input 1 rising-edge interrupt reset */
#define	TEAK_INT_RINT1_MONIN1HI					0x0100
#define	TEAK_INT_RINT1_MONIN1HI_SHIFT			8
/* Monitor input 1 falling-edge interrupt reset */
#define	TEAK_INT_RINT1_MONIN1LO					0x0200
#define	TEAK_INT_RINT1_MONIN1LO_SHIFT			9
/* Monitor input 2 rising-edge interrupt reset */
#define	TEAK_INT_RINT1_MONIN2HI					0x0400
#define	TEAK_INT_RINT1_MONIN2HI_SHIFT			10
/* Monitor input 2 falling-edge interrupt reset */
#define	TEAK_INT_RINT1_MONIN2LO					0x0800
#define	TEAK_INT_RINT1_MONIN2LO_SHIFT			11
/* Monitor input 3 rising-edge interrupt reset */
#define	TEAK_INT_RINT1_MONIN3HI					0x1000
#define	TEAK_INT_RINT1_MONIN3HI_SHIFT			12
/* Monitor input 3 falling-edge interrupt reset */
#define	TEAK_INT_RINT1_MONIN3LO					0x2000
#define	TEAK_INT_RINT1_MONIN3LO_SHIFT			13
/* Monitor input 4 rising-edge interrupt reset */
#define	TEAK_INT_RINT1_MONIN4HI					0x4000
#define	TEAK_INT_RINT1_MONIN4HI_SHIFT			14
/* Monitor input 4 falling-edge interrupt reset */
#define	TEAK_INT_RINT1_MONIN4LO					0x8000
#define	TEAK_INT_RINT1_MONIN4LO_SHIFT			15
/* Interrupt 1 set register (write-only) */
#define	TEAK_INT_SINT1							(TEAK_INT_BASE + 0x0B)
/* Cipher interrupt set */
#define	TEAK_INT_SINT1_CIPH						0x0001
#define	TEAK_INT_SINT1_CIPH_SHIFT				0
/* Timer 1 compare 0 interrupt set */
#define	TEAK_INT_SINT1_TMR10					0x0002
#define	TEAK_INT_SINT1_TMR10_SHIFT				1
/* Timer 1 compare 1 interrupt set */
#define	TEAK_INT_SINT1_TMR11					0x0004
#define	TEAK_INT_SINT1_TMR11_SHIFT				2
/* Timer 2 interrupt set */
#define	TEAK_INT_SINT1_TMR2						0x0008
#define	TEAK_INT_SINT1_TMR2_SHIFT				3
/* DSP input 0 rising-edge interrupt set */
#define	TEAK_INT_SINT1_DSPIN0HI					0x0010
#define	TEAK_INT_SINT1_DSPIN0HI_SHIFT			4
/* DSP input 0 falling-edge interrupt set */
#define	TEAK_INT_SINT1_DSPIN0LO					0x0020
#define	TEAK_INT_SINT1_DSPIN0LO_SHIFT			5
/* DSP input 1 rising-edge interrupt set */
#define	TEAK_INT_SINT1_DSPIN1HI					0x0040
#define	TEAK_INT_SINT1_DSPIN1HI_SHIFT			6
/* DSP input 1 falling-edge interrupt set */
#define	TEAK_INT_SINT1_DSPIN1LO					0x0080
#define	TEAK_INT_SINT1_DSPIN1LO_SHIFT			7
/* Monitor input 1 rising-edge interrupt set */
#define	TEAK_INT_SINT1_MONIN1HI					0x0100
#define	TEAK_INT_SINT1_MONIN1HI_SHIFT			8
/* Monitor input 1 falling-edge interrupt set */
#define	TEAK_INT_SINT1_MONIN1LO					0x0200
#define	TEAK_INT_SINT1_MONIN1LO_SHIFT			9
/* Monitor input 2 rising-edge interrupt set */
#define	TEAK_INT_SINT1_MONIN2HI					0x0400
#define	TEAK_INT_SINT1_MONIN2HI_SHIFT			10
/* Monitor input 2 falling-edge interrupt set */
#define	TEAK_INT_SINT1_MONIN2LO					0x0800
#define	TEAK_INT_SINT1_MONIN2LO_SHIFT			11
/* Monitor input 3 rising-edge interrupt set */
#define	TEAK_INT_SINT1_MONIN3HI					0x1000
#define	TEAK_INT_SINT1_MONIN3HI_SHIFT			12
/* Monitor input 3 falling-edge interrupt set */
#define	TEAK_INT_SINT1_MONIN3LO					0x2000
#define	TEAK_INT_SINT1_MONIN3LO_SHIFT			13
/* Monitor input 4 rising-edge interrupt set */
#define	TEAK_INT_SINT1_MONIN4HI					0x4000
#define	TEAK_INT_SINT1_MONIN4HI_SHIFT			14
/* Monitor input 4 falling-edge interrupt set */
#define	TEAK_INT_SINT1_MONIN4LO					0x8000
#define	TEAK_INT_SINT1_MONIN4LO_SHIFT			15
/* Interrupt 2 flag register (read-only) */
#define	TEAK_INT_FINT2							(TEAK_INT_BASE + 0x0C)
/* Firmware interrupt 0 flag */
#define	TEAK_INT_FINT2_FW0						0x0001
#define	TEAK_INT_FINT2_FW0_SHIFT				0
/* Firmware interrupt 1 flag */
#define	TEAK_INT_FINT2_FW1						0x0002
#define	TEAK_INT_FINT2_FW1_SHIFT				1
/* Firmware interrupt 2 flag */
#define	TEAK_INT_FINT2_FW2						0x0004
#define	TEAK_INT_FINT2_FW2_SHIFT				2
/* Firmware interrupt 3 flag */
#define	TEAK_INT_FINT2_FW3						0x0008
#define	TEAK_INT_FINT2_FW3_SHIFT				3
/* Firmware interrupt 4 flag */
#define	TEAK_INT_FINT2_FW4						0x0010
#define	TEAK_INT_FINT2_FW4_SHIFT				4
/* Firmware interrupt 5 flag */
#define	TEAK_INT_FINT2_FW5						0x0020
#define	TEAK_INT_FINT2_FW5_SHIFT				5
/* Firmware interrupt 6 flag */
#define	TEAK_INT_FINT2_FW6						0x0040
#define	TEAK_INT_FINT2_FW6_SHIFT				6
/* Firmware interrupt 7 flag */
#define	TEAK_INT_FINT2_FW7						0x0080
#define	TEAK_INT_FINT2_FW7_SHIFT				7
/* Firmware interrupt 8 flag */
#define	TEAK_INT_FINT2_FW8						0x0100
#define	TEAK_INT_FINT2_FW8_SHIFT				8
/* Firmware interrupt 9 flag */
#define	TEAK_INT_FINT2_FW9						0x0200
#define	TEAK_INT_FINT2_FW9_SHIFT				9
/* Firmware interrupt 10 flag */
#define	TEAK_INT_FINT2_FW10						0x0400
#define	TEAK_INT_FINT2_FW10_SHIFT				10
/* Firmware interrupt 11 flag */
#define	TEAK_INT_FINT2_FW11						0x0800
#define	TEAK_INT_FINT2_FW11_SHIFT				11
/* Firmware interrupt 12 flag */
#define	TEAK_INT_FINT2_FW12						0x1000
#define	TEAK_INT_FINT2_FW12_SHIFT				12
/* Firmware interrupt 13 flag */
#define	TEAK_INT_FINT2_FW13						0x2000
#define	TEAK_INT_FINT2_FW13_SHIFT				13
/* Firmware interrupt 14 flag */
#define	TEAK_INT_FINT2_FW14						0x4000
#define	TEAK_INT_FINT2_FW14_SHIFT				14
/* Firmware interrupt 15 flag */
#define	TEAK_INT_FINT2_FW15						0x8000
#define	TEAK_INT_FINT2_FW15_SHIFT				15
/* Interrupt 2 enable register */
#define	TEAK_INT_EINT2							(TEAK_INT_BASE + 0x0D)
/* Firmware interrupt 0 enable */
#define	TEAK_INT_EINT2_EFW0						0x0001
#define	TEAK_INT_EINT2_EFW0_SHIFT				0
/* Firmware interrupt 1 enable */
#define	TEAK_INT_EINT2_EFW1						0x0002
#define	TEAK_INT_EINT2_EFW1_SHIFT				1
/* Firmware interrupt 2 enable */
#define	TEAK_INT_EINT2_EFW2						0x0004
#define	TEAK_INT_EINT2_EFW2_SHIFT				2
/* Firmware interrupt 3 enable */
#define	TEAK_INT_EINT2_EFW3						0x0008
#define	TEAK_INT_EINT2_EFW3_SHIFT				3
/* Firmware interrupt 4 enable */
#define	TEAK_INT_EINT2_EFW4						0x0010
#define	TEAK_INT_EINT2_EFW4_SHIFT				4
/* Firmware interrupt 5 enable */
#define	TEAK_INT_EINT2_EFW5						0x0020
#define	TEAK_INT_EINT2_EFW5_SHIFT				5
/* Firmware interrupt 6 enable */
#define	TEAK_INT_EINT2_EFW6						0x0040
#define	TEAK_INT_EINT2_EFW6_SHIFT				6
/* Firmware interrupt 7 enable */
#define	TEAK_INT_EINT2_EFW7						0x0080
#define	TEAK_INT_EINT2_EFW7_SHIFT				7
/* Firmware interrupt 8 enable */
#define	TEAK_INT_EINT2_EFW8						0x0100
#define	TEAK_INT_EINT2_EFW8_SHIFT				8
/* Firmware interrupt 9 enable */
#define	TEAK_INT_EINT2_EFW9						0x0200
#define	TEAK_INT_EINT2_EFW9_SHIFT				9
/* Firmware interrupt 10 enable */
#define	TEAK_INT_EINT2_EFW10					0x0400
#define	TEAK_INT_EINT2_EFW10_SHIFT				10
/* Firmware interrupt 11 enable */
#define	TEAK_INT_EINT2_EFW11					0x0800
#define	TEAK_INT_EINT2_EFW11_SHIFT				11
/* Firmware interrupt 12 enable */
#define	TEAK_INT_EINT2_EFW12					0x1000
#define	TEAK_INT_EINT2_EFW12_SHIFT				12
/* Firmware interrupt 13 enable */
#define	TEAK_INT_EINT2_EFW13					0x2000
#define	TEAK_INT_EINT2_EFW13_SHIFT				13
/* Firmware interrupt 14 enable */
#define	TEAK_INT_EINT2_EFW14					0x4000
#define	TEAK_INT_EINT2_EFW14_SHIFT				14
/* Firmware interrupt 15 enable */
#define	TEAK_INT_EINT2_EFW15					0x8000
#define	TEAK_INT_EINT2_EFW15_SHIFT				15
/* Interrupt 2 reset register (write-only) */
#define	TEAK_INT_RINT2							(TEAK_INT_BASE + 0x0E)
/* Firmware interrupt 0 reset */
#define	TEAK_INT_RINT2_RFW0						0x0001
#define	TEAK_INT_RINT2_RFW0_SHIFT				0
/* Firmware interrupt 1 reset */
#define	TEAK_INT_RINT2_RFW1						0x0002
#define	TEAK_INT_RINT2_RFW1_SHIFT				1
/* Firmware interrupt 2 reset */
#define	TEAK_INT_RINT2_RFW2						0x0004
#define	TEAK_INT_RINT2_RFW2_SHIFT				2
/* Firmware interrupt 3 reset */
#define	TEAK_INT_RINT2_RFW3						0x0008
#define	TEAK_INT_RINT2_RFW3_SHIFT				3
/* Firmware interrupt 4 reset */
#define	TEAK_INT_RINT2_RFW4						0x0010
#define	TEAK_INT_RINT2_RFW4_SHIFT				4
/* Firmware interrupt 5 reset */
#define	TEAK_INT_RINT2_RFW5						0x0020
#define	TEAK_INT_RINT2_RFW5_SHIFT				5
/* Firmware interrupt 6 reset */
#define	TEAK_INT_RINT2_RFW6						0x0040
#define	TEAK_INT_RINT2_RFW6_SHIFT				6
/* Firmware interrupt 7 reset */
#define	TEAK_INT_RINT2_RFW7						0x0080
#define	TEAK_INT_RINT2_RFW7_SHIFT				7
/* Firmware interrupt 8 reset */
#define	TEAK_INT_RINT2_RFW8						0x0100
#define	TEAK_INT_RINT2_RFW8_SHIFT				8
/* Firmware interrupt 9 reset */
#define	TEAK_INT_RINT2_RFW9						0x0200
#define	TEAK_INT_RINT2_RFW9_SHIFT				9
/* Firmware interrupt 10 reset */
#define	TEAK_INT_RINT2_RFW10					0x0400
#define	TEAK_INT_RINT2_RFW10_SHIFT				10
/* Firmware interrupt 11 reset */
#define	TEAK_INT_RINT2_RFW11					0x0800
#define	TEAK_INT_RINT2_RFW11_SHIFT				11
/* Firmware interrupt 12 reset */
#define	TEAK_INT_RINT2_RFW12					0x1000
#define	TEAK_INT_RINT2_RFW12_SHIFT				12
/* Firmware interrupt 13 reset */
#define	TEAK_INT_RINT2_RFW13					0x2000
#define	TEAK_INT_RINT2_RFW13_SHIFT				13
/* Firmware interrupt 14 reset */
#define	TEAK_INT_RINT2_RFW14					0x4000
#define	TEAK_INT_RINT2_RFW14_SHIFT				14
/* Firmware interrupt 15 reset */
#define	TEAK_INT_RINT2_RFW15					0x8000
#define	TEAK_INT_RINT2_RFW15_SHIFT				15
/* Interrupt 2 set register (write-only) */
#define	TEAK_INT_SINT2							(TEAK_INT_BASE + 0x0F)
/* Firmware interrupt 0 set */
#define	TEAK_INT_SINT2_SFW0						0x0001
#define	TEAK_INT_SINT2_SFW0_SHIFT				0
/* Firmware interrupt 1 set */
#define	TEAK_INT_SINT2_SFW1						0x0002
#define	TEAK_INT_SINT2_SFW1_SHIFT				1
/* Firmware interrupt 2 set */
#define	TEAK_INT_SINT2_SFW2						0x0004
#define	TEAK_INT_SINT2_SFW2_SHIFT				2
/* Firmware interrupt 3 set */
#define	TEAK_INT_SINT2_SFW3						0x0008
#define	TEAK_INT_SINT2_SFW3_SHIFT				3
/* Firmware interrupt 4 set */
#define	TEAK_INT_SINT2_SFW4						0x0010
#define	TEAK_INT_SINT2_SFW4_SHIFT				4
/* Firmware interrupt 5 set */
#define	TEAK_INT_SINT2_SFW5						0x0020
#define	TEAK_INT_SINT2_SFW5_SHIFT				5
/* Firmware interrupt 6 set */
#define	TEAK_INT_SINT2_SFW6						0x0040
#define	TEAK_INT_SINT2_SFW6_SHIFT				6
/* Firmware interrupt 7 set */
#define	TEAK_INT_SINT2_SFW7						0x0080
#define	TEAK_INT_SINT2_SFW7_SHIFT				7
/* Firmware interrupt 8 set */
#define	TEAK_INT_SINT2_SFW8						0x0100
#define	TEAK_INT_SINT2_SFW8_SHIFT				8
/* Firmware interrupt 9 set */
#define	TEAK_INT_SINT2_SFW9						0x0200
#define	TEAK_INT_SINT2_SFW9_SHIFT				9
/* Firmware interrupt 10 set */
#define	TEAK_INT_SINT2_SFW10					0x0400
#define	TEAK_INT_SINT2_SFW10_SHIFT				10
/* Firmware interrupt 11 set */
#define	TEAK_INT_SINT2_SFW11					0x0800
#define	TEAK_INT_SINT2_SFW11_SHIFT				11
/* Firmware interrupt 12 set */
#define	TEAK_INT_SINT2_SFW12					0x1000
#define	TEAK_INT_SINT2_SFW12_SHIFT				12
/* Firmware interrupt 13 set */
#define	TEAK_INT_SINT2_SFW13					0x2000
#define	TEAK_INT_SINT2_SFW13_SHIFT				13
/* Firmware interrupt 14 set */
#define	TEAK_INT_SINT2_SFW14					0x4000
#define	TEAK_INT_SINT2_SFW14_SHIFT				14
/* Firmware interrupt 15 set */
#define	TEAK_INT_SINT2_SFW15					0x8000
#define	TEAK_INT_SINT2_SFW15_SHIFT				15
/* DSP-to-MCU interrupt register (write-only) */
#define	TEAK_INT_TOMCU							(TEAK_INT_BASE + 0x10)
/* MCU interrupt line 0 request */
#define	TEAK_INT_TOMCU_TOMCU0					0x0001
#define	TEAK_INT_TOMCU_TOMCU0_SHIFT				0
/* MCU interrupt line 1 request */
#define	TEAK_INT_TOMCU_TOMCU1					0x0002
#define	TEAK_INT_TOMCU_TOMCU1_SHIFT				1
/* MCU interrupt line 2 request */
#define	TEAK_INT_TOMCU_TOMCU2					0x0004
#define	TEAK_INT_TOMCU_TOMCU2_SHIFT				2
/* MCU interrupt line 3 request */
#define	TEAK_INT_TOMCU_TOMCU3					0x0008
#define	TEAK_INT_TOMCU_TOMCU3_SHIFT				3
/* Undocumented register written by Mask ROM 0801 */
#define	TEAK_INT_UNK11							(TEAK_INT_BASE + 0x11)
/* Undocumented register read by Mask ROM 0801 */
#define	TEAK_INT_UNK12							(TEAK_INT_BASE + 0x12)
/* Undocumented register written by Mask ROM 0801 */
#define	TEAK_INT_UNK13							(TEAK_INT_BASE + 0x13)
/* Undocumented register read by Mask ROM 0801 */
#define	TEAK_INT_UNK14							(TEAK_INT_BASE + 0x14)
/* Undocumented register written by Mask ROM 0801 */
#define	TEAK_INT_UNK15							(TEAK_INT_BASE + 0x15)
/* Status and control register */
#define	TEAK_CIPH_CSTAT							(TEAK_CIPH_BASE + 0x00)
/* Cipher unit active and start */
#define	TEAK_CIPH_CSTAT_CACT					0x0001
#define	TEAK_CIPH_CSTAT_CACT_SHIFT				0
/* A52 algorithm select in A51/A52 mode */
#define	TEAK_CIPH_CSTAT_A52						0x0002
#define	TEAK_CIPH_CSTAT_A52_SHIFT				1
/* EDGE mode select */
#define	TEAK_CIPH_CSTAT_EDGE					0x0004
#define	TEAK_CIPH_CSTAT_EDGE_SHIFT				2
/* A53 initialization phase active */
#define	TEAK_CIPH_CSTAT_INIT					0x0008
#define	TEAK_CIPH_CSTAT_INIT_SHIFT				3
/* A53 cipher block enable and select */
#define	TEAK_CIPH_CSTAT_A53						0x0010
#define	TEAK_CIPH_CSTAT_A53_SHIFT				4
/* Key word 0 */
#define	TEAK_CIPH_KEY0							(TEAK_CIPH_BASE + 0x01)
/* Cipher key bits 15:0 */
#define	TEAK_CIPH_KEY0_VALUE					0xFFFF
#define	TEAK_CIPH_KEY0_VALUE_SHIFT				0
/* Key word 1 */
#define	TEAK_CIPH_KEY1							(TEAK_CIPH_BASE + 0x02)
/* Cipher key bits 31:16 */
#define	TEAK_CIPH_KEY1_VALUE					0xFFFF
#define	TEAK_CIPH_KEY1_VALUE_SHIFT				0
/* Key word 2 */
#define	TEAK_CIPH_KEY2							(TEAK_CIPH_BASE + 0x03)
/* Cipher key bits 47:32 */
#define	TEAK_CIPH_KEY2_VALUE					0xFFFF
#define	TEAK_CIPH_KEY2_VALUE_SHIFT				0
/* Key word 3 */
#define	TEAK_CIPH_KEY3							(TEAK_CIPH_BASE + 0x04)
/* Cipher key bits 63:48 */
#define	TEAK_CIPH_KEY3_VALUE					0xFFFF
#define	TEAK_CIPH_KEY3_VALUE_SHIFT				0
/* A51/A52 T modulo 26; reserved in A53 mode */
#define	TEAK_CIPH_TMOD26						(TEAK_CIPH_BASE + 0x05)
/* TDMA frame number modulo 26 */
#define	TEAK_CIPH_TMOD26_T26N					0x001F
#define	TEAK_CIPH_TMOD26_T26N_SHIFT				0
/* A51/A52 T modulo 51; reserved in A53 mode */
#define	TEAK_CIPH_TMOD51						(TEAK_CIPH_BASE + 0x06)
/* TDMA frame number modulo 51 */
#define	TEAK_CIPH_TMOD51_T51N					0x003F
#define	TEAK_CIPH_TMOD51_T51N_SHIFT				0
/* A51/A52 superframe number; reserved in A53 mode */
#define	TEAK_CIPH_SFNUM							(TEAK_CIPH_BASE + 0x07)
/* Superframe number within a hyperframe */
#define	TEAK_CIPH_SFNUM_SFN						0x07FF
#define	TEAK_CIPH_SFNUM_SFN_SHIFT				0
/* A53 key word 4 */
#define	TEAK_CIPH_KEY4							(TEAK_CIPH_BASE + 0x08)
/* Cipher key bits 79:64 */
#define	TEAK_CIPH_KEY4_VALUE					0xFFFF
#define	TEAK_CIPH_KEY4_VALUE_SHIFT				0
/* A53 key word 5 */
#define	TEAK_CIPH_KEY5							(TEAK_CIPH_BASE + 0x09)
/* Cipher key bits 95:80 */
#define	TEAK_CIPH_KEY5_VALUE					0xFFFF
#define	TEAK_CIPH_KEY5_VALUE_SHIFT				0
/* A53 key word 6 */
#define	TEAK_CIPH_KEY6							(TEAK_CIPH_BASE + 0x0A)
/* Cipher key bits 111:96 */
#define	TEAK_CIPH_KEY6_VALUE					0xFFFF
#define	TEAK_CIPH_KEY6_VALUE_SHIFT				0
/* A53 key word 7 */
#define	TEAK_CIPH_KEY7							(TEAK_CIPH_BASE + 0x0B)
/* Cipher key bits 127:112 */
#define	TEAK_CIPH_KEY7_VALUE					0xFFFF
#define	TEAK_CIPH_KEY7_VALUE_SHIFT				0
/* A53 key data word 1 */
#define	TEAK_CIPH_KDATA1						(TEAK_CIPH_BASE + 0x0C)
/* KGCORE CE parameter */
#define	TEAK_CIPH_KDATA1_CE						0xFFFF
#define	TEAK_CIPH_KDATA1_CE_SHIFT				0
/* A53 key data word 2 */
#define	TEAK_CIPH_KDATA2						(TEAK_CIPH_BASE + 0x0D)
/* KGCORE CA parameter */
#define	TEAK_CIPH_KDATA2_CA						0x00FF
#define	TEAK_CIPH_KDATA2_CA_SHIFT				0
/* KGCORE CD parameter */
#define	TEAK_CIPH_KDATA2_CD						0x0400
#define	TEAK_CIPH_KDATA2_CD_SHIFT				10
/* KGCORE CB parameter */
#define	TEAK_CIPH_KDATA2_CB						0xF800
#define	TEAK_CIPH_KDATA2_CB_SHIFT				11
/* A53 key data word 3 */
#define	TEAK_CIPH_KDATA3						(TEAK_CIPH_BASE + 0x0E)
/* TDMA frame number modulo 26 */
#define	TEAK_CIPH_KDATA3_T26N					0x001F
#define	TEAK_CIPH_KDATA3_T26N_SHIFT				0
/* TDMA frame number modulo 51 */
#define	TEAK_CIPH_KDATA3_T51N					0x07E0
#define	TEAK_CIPH_KDATA3_T51N_SHIFT				5
/* Superframe number bits 4:0 */
#define	TEAK_CIPH_KDATA3_SFN					0xF800
#define	TEAK_CIPH_KDATA3_SFN_SHIFT				11
/* A53 key data word 4 */
#define	TEAK_CIPH_KDATA4						(TEAK_CIPH_BASE + 0x0F)
/* Superframe number bits 10:5 */
#define	TEAK_CIPH_KDATA4_SFN					0x003F
#define	TEAK_CIPH_KDATA4_SFN_SHIFT				0
/* Control register */
#define	TEAK_TMR1_CTRL							(TEAK_TMR1_BASE + 0x00)
/* Timer clock enable */
#define	TEAK_TMR1_CTRL_DT1ENA					0x0001
#define	TEAK_TMR1_CTRL_DT1ENA_SHIFT				0
/* Restart timer from zero */
#define	TEAK_TMR1_CTRL_RESTART					0x0002
#define	TEAK_TMR1_CTRL_RESTART_SHIFT			1
/* Timer active status */
#define	TEAK_TMR1_CTRL_DT1ACT					0x0004
#define	TEAK_TMR1_CTRL_DT1ACT_SHIFT				2
/* Counter register (read-only) */
#define	TEAK_TMR1_CNT							(TEAK_TMR1_BASE + 0x01)
/* Current counter value */
#define	TEAK_TMR1_CNT_T1CNT						0x0FFF
#define	TEAK_TMR1_CNT_T1CNT_SHIFT				0
/* Interrupt compare register 0 */
#define	TEAK_TMR1_INT0							(TEAK_TMR1_BASE + 0x02)
/* Interrupt compare value 0 */
#define	TEAK_TMR1_INT0_T1INT0					0x0FFF
#define	TEAK_TMR1_INT0_T1INT0_SHIFT				0
/* Interrupt compare register 1 */
#define	TEAK_TMR1_INT1							(TEAK_TMR1_BASE + 0x03)
/* Interrupt compare value 1 */
#define	TEAK_TMR1_INT1_T1INT1					0x0FFF
#define	TEAK_TMR1_INT1_T1INT1_SHIFT				0
/* Control register */
#define	TEAK_TMR2_CTRL							(TEAK_TMR2_BASE + 0x00)
/* Timer active and clock enable */
#define	TEAK_TMR2_CTRL_DT2ACT					0x0001
#define	TEAK_TMR2_CTRL_DT2ACT_SHIFT				0
/* Counter register */
#define	TEAK_TMR2_CNT							(TEAK_TMR2_BASE + 0x01)
/* Counter and start value */
#define	TEAK_TMR2_CNT_T2CNT						0xFFFF
#define	TEAK_TMR2_CNT_T2CNT_SHIFT				0
/* Maximum register */
#define	TEAK_TMR2_MAX							(TEAK_TMR2_BASE + 0x02)
/* Interrupt and wrap-around value */
#define	TEAK_TMR2_MAX_T2MAX						0xFFFF
#define	TEAK_TMR2_MAX_T2MAX_SHIFT				0
/* Configuration register 1 (write-only) */
#define	TEAK_EQ_CONF1							(TEAK_EQ_BASE + 0x00)
/* Select RAMW1 or RAMW2 */
#define	TEAK_EQ_CONF1_RES_RW1_RW2				0x0001
#define	TEAK_EQ_CONF1_RES_RW1_RW2_SHIFT			0
/* Reset RAM1 pointer to received-value base */
#define	TEAK_EQ_CONF1_RES_RX_BASE				0x0004
#define	TEAK_EQ_CONF1_RES_RX_BASE_SHIFT			2
/* Reset RAM1 pointer to branch-partial-sum base */
#define	TEAK_EQ_CONF1_RES_BPAR_BASE				0x0008
#define	TEAK_EQ_CONF1_RES_BPAR_BASE_SHIFT		3
/* Reset RAM2 pointer to soft-output base */
#define	TEAK_EQ_CONF1_RES_SOUT_BASE				0x0010
#define	TEAK_EQ_CONF1_RES_SOUT_BASE_SHIFT		4
/* Reset RAM2 pointer to hard-output base */
#define	TEAK_EQ_CONF1_RES_HOUT_BASE				0x0020
#define	TEAK_EQ_CONF1_RES_HOUT_BASE_SHIFT		5
/* Reset RAM2 pointer to equalizer-latency base */
#define	TEAK_EQ_CONF1_RES_ELAT_BASE				0x0080
#define	TEAK_EQ_CONF1_RES_ELAT_BASE_SHIFT		7
/* Reset RAMW1/RAMW2 pointer to right metric base */
#define	TEAK_EQ_CONF1_RES_EMR_BASE				0x0800
#define	TEAK_EQ_CONF1_RES_EMR_BASE_SHIFT		11
/* Reset RAMW1/RAMW2 pointer to left metric base */
#define	TEAK_EQ_CONF1_RES_EML_BASE				0x1000
#define	TEAK_EQ_CONF1_RES_EML_BASE_SHIFT		12
/* Reset RAMW1/RAMW2 pointer to right path base */
#define	TEAK_EQ_CONF1_RES_EPR_BASE				0x2000
#define	TEAK_EQ_CONF1_RES_EPR_BASE_SHIFT		13
/* Reset RAMW1/RAMW2 pointer to left path base */
#define	TEAK_EQ_CONF1_RES_EPL_BASE				0x4000
#define	TEAK_EQ_CONF1_RES_EPL_BASE_SHIFT		14
/* Reset RAMW1/RAMW2 pointer to branch-metric base */
#define	TEAK_EQ_CONF1_RES_EB_BASE				0x8000
#define	TEAK_EQ_CONF1_RES_EB_BASE_SHIFT			15
/* Configuration register 2 */
#define	TEAK_EQ_CONF2							(TEAK_EQ_BASE + 0x01)
/* Equalizer hardware clock enable */
#define	TEAK_EQ_CONF2_HW_ENA_EQ					0x0001
#define	TEAK_EQ_CONF2_HW_ENA_EQ_SHIFT			0
/* Reset equalization */
#define	TEAK_EQ_CONF2_RES_EQ					0x0002
#define	TEAK_EQ_CONF2_RES_EQ_SHIFT				1
/* Start Viterbi equalization */
#define	TEAK_EQ_CONF2_EQ_ON						0x0004
#define	TEAK_EQ_CONF2_EQ_ON_SHIFT				2
/* Reset all internal registers */
#define	TEAK_EQ_CONF2_RES_ALL					0x0100
#define	TEAK_EQ_CONF2_RES_ALL_SHIFT				8
/* Combined soft-output packing mode */
#define	TEAK_EQ_CONF2_S_SEG						0x0200
#define	TEAK_EQ_CONF2_S_SEG_SHIFT				9
/* Combine soft and hard outputs */
#define	TEAK_EQ_CONF2_S_COMB					0x0400
#define	TEAK_EQ_CONF2_S_COMB_SHIFT				10
/* External RAM access direction */
#define	TEAK_EQ_CONF2_EQ_FLAG_RD				0x0800
#define	TEAK_EQ_CONF2_EQ_FLAG_RD_SHIFT			11
/* RAM1/RAMW1/RAMW2 packing mode */
#define	TEAK_EQ_CONF2_PC_EQ_0					0x1000
#define	TEAK_EQ_CONF2_PC_EQ_0_SHIFT				12
/* RAM2 packing mode */
#define	TEAK_EQ_CONF2_PC_EQ_1					0x2000
#define	TEAK_EQ_CONF2_PC_EQ_1_SHIFT				13
/* Right half-slot equalization select */
#define	TEAK_EQ_CONF2_EQ_RIGHT					0x4000
#define	TEAK_EQ_CONF2_EQ_RIGHT_SHIFT			14
/* EDGE equalization mode enable */
#define	TEAK_EQ_CONF2_EQ_EDGE					0x8000
#define	TEAK_EQ_CONF2_EQ_EDGE_SHIFT				15
/* Status register (read-only) */
#define	TEAK_EQ_STATUS							(TEAK_EQ_BASE + 0x02)
/* Equalization in progress */
#define	TEAK_EQ_STATUS_EQ_BUSY					0x8000
#define	TEAK_EQ_STATUS_EQ_BUSY_SHIFT			15
/* Configuration counter */
#define	TEAK_EQ_CONF_CNT						(TEAK_EQ_BASE + 0x03)
/* Number of symbols to equalize */
#define	TEAK_EQ_CONF_CNT_C_EQ					0x003F
#define	TEAK_EQ_CONF_CNT_C_EQ_SHIFT				0
/* Status counter (read-only) */
#define	TEAK_EQ_STAT_CNT						(TEAK_EQ_BASE + 0x04)
/* Number of symbols already equalized */
#define	TEAK_EQ_STAT_CNT_S_EQ					0x003F
#define	TEAK_EQ_STAT_CNT_S_EQ_SHIFT				0
/* Soft output register */
#define	TEAK_EQ_SC_SOUT							(TEAK_EQ_BASE + 0x05)
/* Soft-output shift selection */
#define	TEAK_EQ_SC_SOUT_SC7_0					0x00FF
#define	TEAK_EQ_SC_SOUT_SC7_0_SHIFT				0
/* Second shift stage enable */
#define	TEAK_EQ_SC_SOUT_SC8						0x0100
#define	TEAK_EQ_SC_SOUT_SC8_SHIFT				8
/* Soft-output saturation selection */
#define	TEAK_EQ_SC_SOUT_SC12_9					0x1E00
#define	TEAK_EQ_SC_SOUT_SC12_9_SHIFT			9
/* Scaling accuracy control bit 13 */
#define	TEAK_EQ_SC_SOUT_SC13					0x2000
#define	TEAK_EQ_SC_SOUT_SC13_SHIFT				13
/* Scaling accuracy control bit 14 */
#define	TEAK_EQ_SC_SOUT_SC14					0x4000
#define	TEAK_EQ_SC_SOUT_SC14_SHIFT				14
/* Scaling accuracy control bit 15 */
#define	TEAK_EQ_SC_SOUT_SC15					0x8000
#define	TEAK_EQ_SC_SOUT_SC15_SHIFT				15
/* Signal quality register (read-only) */
#define	TEAK_EQ_SQUAL							(TEAK_EQ_BASE + 0x06)
/* RX_QUAL histogram FIFO output */
#define	TEAK_EQ_SQUAL_SQ						0xFFFF
#define	TEAK_EQ_SQUAL_SQ_SHIFT					0
/* Configuration register 1 (write-only) */
#define	TEAK_CHDEC_CONF1						(TEAK_CHDEC_BASE + 0x00)
/* Select RAMW1 or RAMW2 */
#define	TEAK_CHDEC_CONF1_RES_RW1_RW2			0x0001
#define	TEAK_CHDEC_CONF1_RES_RW1_RW2_SHIFT		0
/* Reset RAM2 pointer to SIN01 base */
#define	TEAK_CHDEC_CONF1_RES_SIN01_BASE			0x0002
#define	TEAK_CHDEC_CONF1_RES_SIN01_BASE_SHIFT	1
/* Reset RAM2 pointer to SIN2 base */
#define	TEAK_CHDEC_CONF1_RES_SIN2_BASE			0x0004
#define	TEAK_CHDEC_CONF1_RES_SIN2_BASE_SHIFT	2
/* Reset RAM2 pointer to traceback base */
#define	TEAK_CHDEC_CONF1_RES_TR_BASE			0x0008
#define	TEAK_CHDEC_CONF1_RES_TR_BASE_SHIFT		3
/* Reset RAMW1/RAMW2 pointer to decision-metric base */
#define	TEAK_CHDEC_CONF1_RES_DM_BASE			0x0080
#define	TEAK_CHDEC_CONF1_RES_DM_BASE_SHIFT		7
/* Configuration register 2 */
#define	TEAK_CHDEC_CONF2						(TEAK_CHDEC_BASE + 0x01)
/* Decoder hardware clock enable */
#define	TEAK_CHDEC_CONF2_HW_ENA_DEC				0x0001
#define	TEAK_CHDEC_CONF2_HW_ENA_DEC_SHIFT		0
/* Reset Viterbi decoder */
#define	TEAK_CHDEC_CONF2_RES_DEC				0x0002
#define	TEAK_CHDEC_CONF2_RES_DEC_SHIFT			1
/* Start Viterbi decoding */
#define	TEAK_CHDEC_CONF2_DEC_ON					0x0004
#define	TEAK_CHDEC_CONF2_DEC_ON_SHIFT			2
/* Reset all internal registers */
#define	TEAK_CHDEC_CONF2_RES_ALL				0x0100
#define	TEAK_CHDEC_CONF2_RES_ALL_SHIFT			8
/* External RAM access direction */
#define	TEAK_CHDEC_CONF2_DEC_FLAG_RD			0x0800
#define	TEAK_CHDEC_CONF2_DEC_FLAG_RD_SHIFT		11
/* RAMW1/RAMW2 packing mode */
#define	TEAK_CHDEC_CONF2_PC_DEC_0				0x1000
#define	TEAK_CHDEC_CONF2_PC_DEC_0_SHIFT			12
/* RAM2 packing mode */
#define	TEAK_CHDEC_CONF2_PC_DEC_1				0x2000
#define	TEAK_CHDEC_CONF2_PC_DEC_1_SHIFT			13
/* Metric overflow protection enable */
#define	TEAK_CHDEC_CONF2_OFLOW_PROT				0x4000
#define	TEAK_CHDEC_CONF2_OFLOW_PROT_SHIFT		14
/* 64-state decoder select */
#define	TEAK_CHDEC_CONF2_DEC_64					0x8000
#define	TEAK_CHDEC_CONF2_DEC_64_SHIFT			15
/* Status register (read-only) */
#define	TEAK_CHDEC_STATUS						(TEAK_CHDEC_BASE + 0x02)
/* Viterbi decoding in progress */
#define	TEAK_CHDEC_STATUS_DEC_BUSY				0x4000
#define	TEAK_CHDEC_STATUS_DEC_BUSY_SHIFT		14
/* Configuration counter */
#define	TEAK_CHDEC_CONF_CNT						(TEAK_CHDEC_BASE + 0x03)
/* Number of bits to decode */
#define	TEAK_CHDEC_CONF_CNT_C_DEC				0x00FF
#define	TEAK_CHDEC_CONF_CNT_C_DEC_SHIFT			0
/* Status counter (read-only) */
#define	TEAK_CHDEC_STAT_CNT						(TEAK_CHDEC_BASE + 0x04)
/* Number of bits already decoded */
#define	TEAK_CHDEC_STAT_CNT_S_DEC				0x00FF
#define	TEAK_CHDEC_STAT_CNT_S_DEC_SHIFT			0
/* Butterfly reference register 0 */
#define	TEAK_CHDEC_REF_BR_BFLY0					(TEAK_CHDEC_BASE + 0x05)
/* Branch metric reference 1 */
#define	TEAK_CHDEC_REF_BR_BFLY0_REF1			0x000F
#define	TEAK_CHDEC_REF_BR_BFLY0_REF1_SHIFT		0
/* Branch metric reference 2 */
#define	TEAK_CHDEC_REF_BR_BFLY0_REF2			0x00F0
#define	TEAK_CHDEC_REF_BR_BFLY0_REF2_SHIFT		4
/* Branch metric reference 3 */
#define	TEAK_CHDEC_REF_BR_BFLY0_REF3			0x0F00
#define	TEAK_CHDEC_REF_BR_BFLY0_REF3_SHIFT		8
/* Branch metric reference 4 */
#define	TEAK_CHDEC_REF_BR_BFLY0_REF4			0xF000
#define	TEAK_CHDEC_REF_BR_BFLY0_REF4_SHIFT		12
/* Butterfly reference register 1 */
#define	TEAK_CHDEC_REF_BR_BFLY1					(TEAK_CHDEC_BASE + 0x06)
/* Branch metric reference 1 */
#define	TEAK_CHDEC_REF_BR_BFLY1_REF1			0x000F
#define	TEAK_CHDEC_REF_BR_BFLY1_REF1_SHIFT		0
/* Branch metric reference 2 */
#define	TEAK_CHDEC_REF_BR_BFLY1_REF2			0x00F0
#define	TEAK_CHDEC_REF_BR_BFLY1_REF2_SHIFT		4
/* Branch metric reference 3 */
#define	TEAK_CHDEC_REF_BR_BFLY1_REF3			0x0F00
#define	TEAK_CHDEC_REF_BR_BFLY1_REF3_SHIFT		8
/* Branch metric reference 4 */
#define	TEAK_CHDEC_REF_BR_BFLY1_REF4			0xF000
#define	TEAK_CHDEC_REF_BR_BFLY1_REF4_SHIFT		12
/* Butterfly reference register 2 */
#define	TEAK_CHDEC_REF_BR_BFLY2					(TEAK_CHDEC_BASE + 0x07)
/* Branch metric reference 1 */
#define	TEAK_CHDEC_REF_BR_BFLY2_REF1			0x000F
#define	TEAK_CHDEC_REF_BR_BFLY2_REF1_SHIFT		0
/* Branch metric reference 2 */
#define	TEAK_CHDEC_REF_BR_BFLY2_REF2			0x00F0
#define	TEAK_CHDEC_REF_BR_BFLY2_REF2_SHIFT		4
/* Branch metric reference 3 */
#define	TEAK_CHDEC_REF_BR_BFLY2_REF3			0x0F00
#define	TEAK_CHDEC_REF_BR_BFLY2_REF3_SHIFT		8
/* Branch metric reference 4 */
#define	TEAK_CHDEC_REF_BR_BFLY2_REF4			0xF000
#define	TEAK_CHDEC_REF_BR_BFLY2_REF4_SHIFT		12
/* Butterfly reference register 3 */
#define	TEAK_CHDEC_REF_BR_BFLY3					(TEAK_CHDEC_BASE + 0x08)
/* Branch metric reference 1 */
#define	TEAK_CHDEC_REF_BR_BFLY3_REF1			0x000F
#define	TEAK_CHDEC_REF_BR_BFLY3_REF1_SHIFT		0
/* Branch metric reference 2 */
#define	TEAK_CHDEC_REF_BR_BFLY3_REF2			0x00F0
#define	TEAK_CHDEC_REF_BR_BFLY3_REF2_SHIFT		4
/* Branch metric reference 3 */
#define	TEAK_CHDEC_REF_BR_BFLY3_REF3			0x0F00
#define	TEAK_CHDEC_REF_BR_BFLY3_REF3_SHIFT		8
/* Branch metric reference 4 */
#define	TEAK_CHDEC_REF_BR_BFLY3_REF4			0xF000
#define	TEAK_CHDEC_REF_BR_BFLY3_REF4_SHIFT		12
/* Butterfly reference register 4 */
#define	TEAK_CHDEC_REF_BR_BFLY4					(TEAK_CHDEC_BASE + 0x09)
/* Branch metric reference 1 */
#define	TEAK_CHDEC_REF_BR_BFLY4_REF1			0x000F
#define	TEAK_CHDEC_REF_BR_BFLY4_REF1_SHIFT		0
/* Branch metric reference 2 */
#define	TEAK_CHDEC_REF_BR_BFLY4_REF2			0x00F0
#define	TEAK_CHDEC_REF_BR_BFLY4_REF2_SHIFT		4
/* Branch metric reference 3 */
#define	TEAK_CHDEC_REF_BR_BFLY4_REF3			0x0F00
#define	TEAK_CHDEC_REF_BR_BFLY4_REF3_SHIFT		8
/* Branch metric reference 4 */
#define	TEAK_CHDEC_REF_BR_BFLY4_REF4			0xF000
#define	TEAK_CHDEC_REF_BR_BFLY4_REF4_SHIFT		12
/* Butterfly reference register 5 */
#define	TEAK_CHDEC_REF_BR_BFLY5					(TEAK_CHDEC_BASE + 0x0A)
/* Branch metric reference 1 */
#define	TEAK_CHDEC_REF_BR_BFLY5_REF1			0x000F
#define	TEAK_CHDEC_REF_BR_BFLY5_REF1_SHIFT		0
/* Branch metric reference 2 */
#define	TEAK_CHDEC_REF_BR_BFLY5_REF2			0x00F0
#define	TEAK_CHDEC_REF_BR_BFLY5_REF2_SHIFT		4
/* Branch metric reference 3 */
#define	TEAK_CHDEC_REF_BR_BFLY5_REF3			0x0F00
#define	TEAK_CHDEC_REF_BR_BFLY5_REF3_SHIFT		8
/* Branch metric reference 4 */
#define	TEAK_CHDEC_REF_BR_BFLY5_REF4			0xF000
#define	TEAK_CHDEC_REF_BR_BFLY5_REF4_SHIFT		12
/* Butterfly reference register 6 */
#define	TEAK_CHDEC_REF_BR_BFLY6					(TEAK_CHDEC_BASE + 0x0B)
/* Branch metric reference 1 */
#define	TEAK_CHDEC_REF_BR_BFLY6_REF1			0x000F
#define	TEAK_CHDEC_REF_BR_BFLY6_REF1_SHIFT		0
/* Branch metric reference 2 */
#define	TEAK_CHDEC_REF_BR_BFLY6_REF2			0x00F0
#define	TEAK_CHDEC_REF_BR_BFLY6_REF2_SHIFT		4
/* Branch metric reference 3 */
#define	TEAK_CHDEC_REF_BR_BFLY6_REF3			0x0F00
#define	TEAK_CHDEC_REF_BR_BFLY6_REF3_SHIFT		8
/* Branch metric reference 4 */
#define	TEAK_CHDEC_REF_BR_BFLY6_REF4			0xF000
#define	TEAK_CHDEC_REF_BR_BFLY6_REF4_SHIFT		12
/* Butterfly reference register 7 */
#define	TEAK_CHDEC_REF_BR_BFLY7					(TEAK_CHDEC_BASE + 0x0C)
/* Branch metric reference 1 */
#define	TEAK_CHDEC_REF_BR_BFLY7_REF1			0x000F
#define	TEAK_CHDEC_REF_BR_BFLY7_REF1_SHIFT		0
/* Branch metric reference 2 */
#define	TEAK_CHDEC_REF_BR_BFLY7_REF2			0x00F0
#define	TEAK_CHDEC_REF_BR_BFLY7_REF2_SHIFT		4
/* Branch metric reference 3 */
#define	TEAK_CHDEC_REF_BR_BFLY7_REF3			0x0F00
#define	TEAK_CHDEC_REF_BR_BFLY7_REF3_SHIFT		8
/* Branch metric reference 4 */
#define	TEAK_CHDEC_REF_BR_BFLY7_REF4			0xF000
#define	TEAK_CHDEC_REF_BR_BFLY7_REF4_SHIFT		12
/* Interrupt pointer (write-only) */
#define	TEAK_AFE_INTPTR							(TEAK_AFE_BASE + 0x00)
/* Receive ring-buffer interrupt position */
#define	TEAK_AFE_INTPTR_RXINTPTR				0x003F
#define	TEAK_AFE_INTPTR_RXINTPTR_SHIFT			0
/* Transmit ring-buffer interrupt position */
#define	TEAK_AFE_INTPTR_TXINTPTR				0x3F00
#define	TEAK_AFE_INTPTR_TXINTPTR_SHIFT			8
/* Read/write address (read-only) */
#define	TEAK_AFE_RWADDR							(TEAK_AFE_BASE + 0x01)
/* Receive ring-buffer read position */
#define	TEAK_AFE_RWADDR_RDADDR					0x003F
#define	TEAK_AFE_RWADDR_RDADDR_SHIFT			0
/* Transmit ring-buffer write position */
#define	TEAK_AFE_RWADDR_WRADDR					0x3F00
#define	TEAK_AFE_RWADDR_WRADDR_SHIFT			8
/* Buffer control register */
#define	TEAK_AFE_BCON							(TEAK_AFE_BASE + 0x02)
/* Audio front-end operation enable */
#define	TEAK_AFE_BCON_MODE						0x0001
#define	TEAK_AFE_BCON_MODE_SHIFT				0
/* Receive path start */
#define	TEAK_AFE_BCON_RXSTART					0x0002
#define	TEAK_AFE_BCON_RXSTART_SHIFT				1
/* Receive interpolation rate */
#define	TEAK_AFE_BCON_RXRATE					0x001C
#define	TEAK_AFE_BCON_RXRATE_SHIFT				2
#define	TEAK_AFE_BCON_RXRATE_KHZ_8				0x0
#define	TEAK_AFE_BCON_RXRATE_KHZ_16				0x4
#define	TEAK_AFE_BCON_RXRATE_KHZ_31_746			0x8
#define	TEAK_AFE_BCON_RXRATE_KHZ_44_444			0xC
#define	TEAK_AFE_BCON_RXRATE_KHZ_47_619			0x10
/* Transmit path start */
#define	TEAK_AFE_BCON_TXSTART					0x0020
#define	TEAK_AFE_BCON_TXSTART_SHIFT				5
/* Transmit output rate */
#define	TEAK_AFE_BCON_TXRATE					0x0040
#define	TEAK_AFE_BCON_TXRATE_SHIFT				6
#define	TEAK_AFE_BCON_TXRATE_KHZ_8				0x0
#define	TEAK_AFE_BCON_TXRATE_KHZ_16				0x40
/* Voice receive control register 1 */
#define	TEAK_AFE_VRXCTRL1						(TEAK_AFE_BASE + 0x03)
/* Earpiece power-save mode */
#define	TEAK_AFE_VRXCTRL1_EPSAV					0x0004
#define	TEAK_AFE_VRXCTRL1_EPSAV_SHIFT			2
/* Earpiece buffer common-mode voltage */
#define	TEAK_AFE_VRXCTRL1_VCMEPB				0x0070
#define	TEAK_AFE_VRXCTRL1_VCMEPB_SHIFT			4
/* Line-out buffer gain */
#define	TEAK_AFE_VRXCTRL1_RXGAINSP				0x0380
#define	TEAK_AFE_VRXCTRL1_RXGAINSP_SHIFT		7
/* Power audio buffer gain */
#define	TEAK_AFE_VRXCTRL1_RXGAINPA				0x1C00
#define	TEAK_AFE_VRXCTRL1_RXGAINPA_SHIFT		10
/* Receive DAC dither control */
#define	TEAK_AFE_VRXCTRL1_RXDITH				0xC000
#define	TEAK_AFE_VRXCTRL1_RXDITH_SHIFT			14
/* Voice receive control register 2 */
#define	TEAK_AFE_VRXCTRL2						(TEAK_AFE_BASE + 0x04)
/* Receive channel DAC enable */
#define	TEAK_AFE_VRXCTRL2_RXDAC					0x0001
#define	TEAK_AFE_VRXCTRL2_RXDAC_SHIFT			0
/* Line-out buffer ringer mode */
#define	TEAK_AFE_VRXCTRL2_RINGSELSP				0x0004
#define	TEAK_AFE_VRXCTRL2_RINGSELSP_SHIFT		2
/* Power audio buffer ringer mode */
#define	TEAK_AFE_VRXCTRL2_RINGSELPA				0x0008
#define	TEAK_AFE_VRXCTRL2_RINGSELPA_SHIFT		3
/* Power audio buffer enable */
#define	TEAK_AFE_VRXCTRL2_VEPPA					0x0010
#define	TEAK_AFE_VRXCTRL2_VEPPA_SHIFT			4
/* Line-out buffer enable */
#define	TEAK_AFE_VRXCTRL2_VEPSP					0x0040
#define	TEAK_AFE_VRXCTRL2_VEPSP_SHIFT			6
/* Zero-detect enable */
#define	TEAK_AFE_VRXCTRL2_ZERODETECT			0x0080
#define	TEAK_AFE_VRXCTRL2_ZERODETECT_SHIFT		7
/* Analog ringer mode select */
#define	TEAK_AFE_VRXCTRL2_MODE					0x4000
#define	TEAK_AFE_VRXCTRL2_MODE_SHIFT			14
/* Voice transmit control register */
#define	TEAK_AFE_VTXCTRL						(TEAK_AFE_BASE + 0x05)
/* Transmit ADC dither enable */
#define	TEAK_AFE_VTXCTRL_TXDITH					0x0001
#define	TEAK_AFE_VTXCTRL_TXDITH_SHIFT			0
/* 3 dB gain stage enable */
#define	TEAK_AFE_VTXCTRL_TXGAIN0				0x0002
#define	TEAK_AFE_VTXCTRL_TXGAIN0_SHIFT			1
/* 6 dB gain stage enable */
#define	TEAK_AFE_VTXCTRL_TXGAIN1				0x0004
#define	TEAK_AFE_VTXCTRL_TXGAIN1_SHIFT			2
/* 12 dB gain stage enable */
#define	TEAK_AFE_VTXCTRL_TXGAIN2				0x0008
#define	TEAK_AFE_VTXCTRL_TXGAIN2_SHIFT			3
/* 24 dB gain stage enable */
#define	TEAK_AFE_VTXCTRL_TXGAIN3				0x0010
#define	TEAK_AFE_VTXCTRL_TXGAIN3_SHIFT			4
/* Transmit analog operation mode */
#define	TEAK_AFE_VTXCTRL_TXMODE					0x0060
#define	TEAK_AFE_VTXCTRL_TXMODE_SHIFT			5
#define	TEAK_AFE_VTXCTRL_TXMODE_POWER_DOWN		0x0
#define	TEAK_AFE_VTXCTRL_TXMODE_TALK			0x20
/* Transmit input source */
#define	TEAK_AFE_VTXCTRL_TXINSEL				0x0700
#define	TEAK_AFE_VTXCTRL_TXINSEL_SHIFT			8
#define	TEAK_AFE_VTXCTRL_TXINSEL_MIC2			0x200
#define	TEAK_AFE_VTXCTRL_TXINSEL_MIC1			0x400
/* Microphone supply voltage */
#define	TEAK_AFE_VTXCTRL_VMIC					0x1800
#define	TEAK_AFE_VTXCTRL_VMIC_SHIFT				11
#define	TEAK_AFE_VTXCTRL_VMIC_POWER_DOWN		0x0
#define	TEAK_AFE_VTXCTRL_VMIC_VOLT_1_8			0x800
#define	TEAK_AFE_VTXCTRL_VMIC_VOLT_2_0			0x1000
#define	TEAK_AFE_VTXCTRL_VMIC_VOLT_2_2			0x1800
/* Ringer control register */
#define	TEAK_AFE_RINGCTRL						(TEAK_AFE_BASE + 0x06)
/* Alternative ringer output enable */
#define	TEAK_AFE_RINGCTRL_ALTRIN				0x0001
#define	TEAK_AFE_RINGCTRL_ALTRIN_SHIFT			0
/* Ringer input source */
#define	TEAK_AFE_RINGCTRL_RINS					0x0006
#define	TEAK_AFE_RINGCTRL_RINS_SHIFT			1
#define	TEAK_AFE_RINGCTRL_RINS_CAPCOM0			0x0
#define	TEAK_AFE_RINGCTRL_RINS_CAPCOM1			0x2
#define	TEAK_AFE_RINGCTRL_RINS_ALTERNATIVE		0x4
/* Control register */
#define	TEAK_BB_CTRL							(TEAK_BB_BASE + 0x00)
/* Override hardware control and stop filter */
#define	TEAK_BB_CTRL_BB_STOP					0x0001
#define	TEAK_BB_CTRL_BB_STOP_SHIFT				0
/* CORDIC block enable */
#define	TEAK_BB_CTRL_CORDICON					0x0002
#define	TEAK_BB_CTRL_CORDICON_SHIFT				1
/* Baseband clock enable */
#define	TEAK_BB_CTRL_BB_ON						0x0004
#define	TEAK_BB_CTRL_BB_ON_SHIFT				2
/* Enhanced ADC mode enable */
#define	TEAK_BB_CTRL_BB_ADCMODE					0x0008
#define	TEAK_BB_CTRL_BB_ADCMODE_SHIFT			3
/* Adaptive filter mode enable */
#define	TEAK_BB_CTRL_BBADAP_EN					0x0010
#define	TEAK_BB_CTRL_BBADAP_EN_SHIFT			4
/* Interrupt pointer */
#define	TEAK_BB_INT_POINTER						(TEAK_BB_BASE + 0x01)
/* Baseband RAM interrupt position */
#define	TEAK_BB_INT_POINTER_VALUE				0x03FF
#define	TEAK_BB_INT_POINTER_VALUE_SHIFT			0
/* Write pointer (read-only) */
#define	TEAK_BB_WR_POINTER						(TEAK_BB_BASE + 0x02)
/* Current baseband RAM write position */
#define	TEAK_BB_WR_POINTER_VALUE				0x03FF
#define	TEAK_BB_WR_POINTER_VALUE_SHIFT			0
/* Status register (read-only) */
#define	TEAK_BB_STATUS							(TEAK_BB_BASE + 0x03)
/* Equalizer trigger status */
#define	TEAK_BB_STATUS_EQON						0x0001
#define	TEAK_BB_STATUS_EQON_SHIFT				0
/* Frequency-correction trigger status */
#define	TEAK_BB_STATUS_FCON						0x0002
#define	TEAK_BB_STATUS_FCON_SHIFT				1
/* Monitor trigger status */
#define	TEAK_BB_STATUS_MONON					0x0004
#define	TEAK_BB_STATUS_MONON_SHIFT				2
/* Synchronization trigger status */
#define	TEAK_BB_STATUS_SCON						0x0008
#define	TEAK_BB_STATUS_SCON_SHIFT				3
/* Receiver trigger status */
#define	TEAK_BB_STATUS_RXON						0x0010
#define	TEAK_BB_STATUS_RXON_SHIFT				4
/* I DC offset register */
#define	TEAK_BB_DCOFFSET_I						(TEAK_BB_BASE + 0x04)
/* I-component DC offset correction */
#define	TEAK_BB_DCOFFSET_I_VALUE				0xFFFF
#define	TEAK_BB_DCOFFSET_I_VALUE_SHIFT			0
/* Q DC offset register */
#define	TEAK_BB_DCOFFSET_Q						(TEAK_BB_BASE + 0x05)
/* Q-component DC offset correction */
#define	TEAK_BB_DCOFFSET_Q_VALUE				0xFFFF
#define	TEAK_BB_DCOFFSET_Q_VALUE_SHIFT			0
/* Frequency shift register */
#define	TEAK_BB_FSHIFT							(TEAK_BB_BASE + 0x06)
/* Frequency shift correction */
#define	TEAK_BB_FSHIFT_VALUE					0xFFFF
#define	TEAK_BB_FSHIFT_VALUE_SHIFT				0
/* Baseband receive filter control register */
#define	TEAK_BB_BRFILTER_CTRL					(TEAK_BB_BASE + 0x07)
/* Filter order divided by two */
#define	TEAK_BB_BRFILTER_CTRL_LENGTH			0x00FF
#define	TEAK_BB_BRFILTER_CTRL_LENGTH_SHIFT		0
/* Filter gain scaling */
#define	TEAK_BB_BRFILTER_CTRL_SCALING			0x0F00
#define	TEAK_BB_BRFILTER_CTRL_SCALING_SHIFT		8
/* Decimation filter enable */
#define	TEAK_BB_BRFILTER_CTRL_DECIMATION		0x1000
#define	TEAK_BB_BRFILTER_CTRL_DECIMATION_SHIFT	12
/* Pbase high word (read-only) */
#define	TEAK_BB_PBASE_MSB						(TEAK_BB_BASE + 0x08)
/* Estimated wanted-signal power bits 23:16 */
#define	TEAK_BB_PBASE_MSB_VALUE					0x00FF
#define	TEAK_BB_PBASE_MSB_VALUE_SHIFT			0
/* Pbase low word (read-only) */
#define	TEAK_BB_PBASE_LSB						(TEAK_BB_BASE + 0x09)
/* Estimated wanted-signal power bits 15:0 */
#define	TEAK_BB_PBASE_LSB_VALUE					0xFFFF
#define	TEAK_BB_PBASE_LSB_VALUE_SHIFT			0
/* Padj high word (read-only) */
#define	TEAK_BB_PADJ_MSB						(TEAK_BB_BASE + 0x0A)
/* Estimated adjacent-channel power bits 23:16 */
#define	TEAK_BB_PADJ_MSB_VALUE					0x00FF
#define	TEAK_BB_PADJ_MSB_VALUE_SHIFT			0
/* Padj low word (read-only) */
#define	TEAK_BB_PADJ_LSB						(TEAK_BB_BASE + 0x0B)
/* Estimated adjacent-channel power bits 15:0 */
#define	TEAK_BB_PADJ_LSB_VALUE					0xFFFF
#define	TEAK_BB_PADJ_LSB_VALUE_SHIFT			0
/* I/Q imbalance register */
#define	TEAK_BB_IQ_IMBALANCE					(TEAK_BB_BASE + 0x0C)
/* DSP communication flag status register (read-only) */
#define	TEAK_MCS_CFSTA							(TEAK_MCS_BASE + 0x00)
/* Communication flag 0 status */
#define	TEAK_MCS_CFSTA_CF0						0x0001
#define	TEAK_MCS_CFSTA_CF0_SHIFT				0
/* Communication flag 1 status */
#define	TEAK_MCS_CFSTA_CF1						0x0002
#define	TEAK_MCS_CFSTA_CF1_SHIFT				1
/* Communication flag 2 status */
#define	TEAK_MCS_CFSTA_CF2						0x0004
#define	TEAK_MCS_CFSTA_CF2_SHIFT				2
/* Communication flag 3 status */
#define	TEAK_MCS_CFSTA_CF3						0x0008
#define	TEAK_MCS_CFSTA_CF3_SHIFT				3
/* Communication flag 4 status */
#define	TEAK_MCS_CFSTA_CF4						0x0010
#define	TEAK_MCS_CFSTA_CF4_SHIFT				4
/* Communication flag 5 status */
#define	TEAK_MCS_CFSTA_CF5						0x0020
#define	TEAK_MCS_CFSTA_CF5_SHIFT				5
/* Communication flag 6 status */
#define	TEAK_MCS_CFSTA_CF6						0x0040
#define	TEAK_MCS_CFSTA_CF6_SHIFT				6
/* Communication flag 7 status */
#define	TEAK_MCS_CFSTA_CF7						0x0080
#define	TEAK_MCS_CFSTA_CF7_SHIFT				7
/* Communication flag 8 status */
#define	TEAK_MCS_CFSTA_CF8						0x0100
#define	TEAK_MCS_CFSTA_CF8_SHIFT				8
/* Communication flag 9 status */
#define	TEAK_MCS_CFSTA_CF9						0x0200
#define	TEAK_MCS_CFSTA_CF9_SHIFT				9
/* Communication flag 10 status */
#define	TEAK_MCS_CFSTA_CF10						0x0400
#define	TEAK_MCS_CFSTA_CF10_SHIFT				10
/* Communication flag 11 status */
#define	TEAK_MCS_CFSTA_CF11						0x0800
#define	TEAK_MCS_CFSTA_CF11_SHIFT				11
/* Communication flag 12 status */
#define	TEAK_MCS_CFSTA_CF12						0x1000
#define	TEAK_MCS_CFSTA_CF12_SHIFT				12
/* Communication flag 13 status */
#define	TEAK_MCS_CFSTA_CF13						0x2000
#define	TEAK_MCS_CFSTA_CF13_SHIFT				13
/* Communication flag 14 status */
#define	TEAK_MCS_CFSTA_CF14						0x4000
#define	TEAK_MCS_CFSTA_CF14_SHIFT				14
/* Communication flag 15 status */
#define	TEAK_MCS_CFSTA_CF15						0x8000
#define	TEAK_MCS_CFSTA_CF15_SHIFT				15
/* DSP communication flag set register (write-only) */
#define	TEAK_MCS_CFSET							(TEAK_MCS_BASE + 0x01)
/* Set communication flag 0 */
#define	TEAK_MCS_CFSET_CF0						0x0001
#define	TEAK_MCS_CFSET_CF0_SHIFT				0
/* Set communication flag 1 */
#define	TEAK_MCS_CFSET_CF1						0x0002
#define	TEAK_MCS_CFSET_CF1_SHIFT				1
/* Set communication flag 2 */
#define	TEAK_MCS_CFSET_CF2						0x0004
#define	TEAK_MCS_CFSET_CF2_SHIFT				2
/* Set communication flag 3 */
#define	TEAK_MCS_CFSET_CF3						0x0008
#define	TEAK_MCS_CFSET_CF3_SHIFT				3
/* Set communication flag 4 */
#define	TEAK_MCS_CFSET_CF4						0x0010
#define	TEAK_MCS_CFSET_CF4_SHIFT				4
/* Set communication flag 5 */
#define	TEAK_MCS_CFSET_CF5						0x0020
#define	TEAK_MCS_CFSET_CF5_SHIFT				5
/* Set communication flag 6 */
#define	TEAK_MCS_CFSET_CF6						0x0040
#define	TEAK_MCS_CFSET_CF6_SHIFT				6
/* Set communication flag 7 */
#define	TEAK_MCS_CFSET_CF7						0x0080
#define	TEAK_MCS_CFSET_CF7_SHIFT				7
/* Set communication flag 8 */
#define	TEAK_MCS_CFSET_CF8						0x0100
#define	TEAK_MCS_CFSET_CF8_SHIFT				8
/* Set communication flag 9 */
#define	TEAK_MCS_CFSET_CF9						0x0200
#define	TEAK_MCS_CFSET_CF9_SHIFT				9
/* Set communication flag 10 */
#define	TEAK_MCS_CFSET_CF10						0x0400
#define	TEAK_MCS_CFSET_CF10_SHIFT				10
/* Set communication flag 11 */
#define	TEAK_MCS_CFSET_CF11						0x0800
#define	TEAK_MCS_CFSET_CF11_SHIFT				11
/* Set communication flag 12 */
#define	TEAK_MCS_CFSET_CF12						0x1000
#define	TEAK_MCS_CFSET_CF12_SHIFT				12
/* Set communication flag 13 */
#define	TEAK_MCS_CFSET_CF13						0x2000
#define	TEAK_MCS_CFSET_CF13_SHIFT				13
/* Set communication flag 14 */
#define	TEAK_MCS_CFSET_CF14						0x4000
#define	TEAK_MCS_CFSET_CF14_SHIFT				14
/* Set communication flag 15 */
#define	TEAK_MCS_CFSET_CF15						0x8000
#define	TEAK_MCS_CFSET_CF15_SHIFT				15
/* DSP communication flag reset register (write-only) */
#define	TEAK_MCS_CFR							(TEAK_MCS_BASE + 0x02)
/* Reset communication flag 0 */
#define	TEAK_MCS_CFR_CF0						0x0001
#define	TEAK_MCS_CFR_CF0_SHIFT					0
/* Reset communication flag 1 */
#define	TEAK_MCS_CFR_CF1						0x0002
#define	TEAK_MCS_CFR_CF1_SHIFT					1
/* Reset communication flag 2 */
#define	TEAK_MCS_CFR_CF2						0x0004
#define	TEAK_MCS_CFR_CF2_SHIFT					2
/* Reset communication flag 3 */
#define	TEAK_MCS_CFR_CF3						0x0008
#define	TEAK_MCS_CFR_CF3_SHIFT					3
/* Reset communication flag 4 */
#define	TEAK_MCS_CFR_CF4						0x0010
#define	TEAK_MCS_CFR_CF4_SHIFT					4
/* Reset communication flag 5 */
#define	TEAK_MCS_CFR_CF5						0x0020
#define	TEAK_MCS_CFR_CF5_SHIFT					5
/* Reset communication flag 6 */
#define	TEAK_MCS_CFR_CF6						0x0040
#define	TEAK_MCS_CFR_CF6_SHIFT					6
/* Reset communication flag 7 */
#define	TEAK_MCS_CFR_CF7						0x0080
#define	TEAK_MCS_CFR_CF7_SHIFT					7
/* Reset communication flag 8 */
#define	TEAK_MCS_CFR_CF8						0x0100
#define	TEAK_MCS_CFR_CF8_SHIFT					8
/* Reset communication flag 9 */
#define	TEAK_MCS_CFR_CF9						0x0200
#define	TEAK_MCS_CFR_CF9_SHIFT					9
/* Reset communication flag 10 */
#define	TEAK_MCS_CFR_CF10						0x0400
#define	TEAK_MCS_CFR_CF10_SHIFT					10
/* Reset communication flag 11 */
#define	TEAK_MCS_CFR_CF11						0x0800
#define	TEAK_MCS_CFR_CF11_SHIFT					11
/* Reset communication flag 12 */
#define	TEAK_MCS_CFR_CF12						0x1000
#define	TEAK_MCS_CFR_CF12_SHIFT					12
/* Reset communication flag 13 */
#define	TEAK_MCS_CFR_CF13						0x2000
#define	TEAK_MCS_CFR_CF13_SHIFT					13
/* Reset communication flag 14 */
#define	TEAK_MCS_CFR_CF14						0x4000
#define	TEAK_MCS_CFR_CF14_SHIFT					14
/* Reset communication flag 15 */
#define	TEAK_MCS_CFR_CF15						0x8000
#define	TEAK_MCS_CFR_CF15_SHIFT					15
/* MCU semaphore status register (read-only) */
#define	TEAK_MCS_MCU_SEM						(TEAK_MCS_BASE + 0x03)
/* Semaphore 0 status */
#define	TEAK_MCS_MCU_SEM_SEM0					0x0001
#define	TEAK_MCS_MCU_SEM_SEM0_SHIFT				0
/* Semaphore 1 status */
#define	TEAK_MCS_MCU_SEM_SEM1					0x0002
#define	TEAK_MCS_MCU_SEM_SEM1_SHIFT				1
/* Semaphore 2 status */
#define	TEAK_MCS_MCU_SEM_SEM2					0x0004
#define	TEAK_MCS_MCU_SEM_SEM2_SHIFT				2
/* Semaphore 3 status */
#define	TEAK_MCS_MCU_SEM_SEM3					0x0008
#define	TEAK_MCS_MCU_SEM_SEM3_SHIFT				3
/* Semaphore 4 status */
#define	TEAK_MCS_MCU_SEM_SEM4					0x0010
#define	TEAK_MCS_MCU_SEM_SEM4_SHIFT				4
/* Semaphore 5 status */
#define	TEAK_MCS_MCU_SEM_SEM5					0x0020
#define	TEAK_MCS_MCU_SEM_SEM5_SHIFT				5
/* Semaphore 6 status */
#define	TEAK_MCS_MCU_SEM_SEM6					0x0040
#define	TEAK_MCS_MCU_SEM_SEM6_SHIFT				6
/* Semaphore 7 status */
#define	TEAK_MCS_MCU_SEM_SEM7					0x0080
#define	TEAK_MCS_MCU_SEM_SEM7_SHIFT				7
/* Semaphore 8 status */
#define	TEAK_MCS_MCU_SEM_SEM8					0x0100
#define	TEAK_MCS_MCU_SEM_SEM8_SHIFT				8
/* Semaphore 9 status */
#define	TEAK_MCS_MCU_SEM_SEM9					0x0200
#define	TEAK_MCS_MCU_SEM_SEM9_SHIFT				9
/* Semaphore 10 status */
#define	TEAK_MCS_MCU_SEM_SEM10					0x0400
#define	TEAK_MCS_MCU_SEM_SEM10_SHIFT			10
/* Semaphore 11 status */
#define	TEAK_MCS_MCU_SEM_SEM11					0x0800
#define	TEAK_MCS_MCU_SEM_SEM11_SHIFT			11
/* Semaphore 12 status */
#define	TEAK_MCS_MCU_SEM_SEM12					0x1000
#define	TEAK_MCS_MCU_SEM_SEM12_SHIFT			12
/* Semaphore 13 status */
#define	TEAK_MCS_MCU_SEM_SEM13					0x2000
#define	TEAK_MCS_MCU_SEM_SEM13_SHIFT			13
/* Semaphore 14 status */
#define	TEAK_MCS_MCU_SEM_SEM14					0x4000
#define	TEAK_MCS_MCU_SEM_SEM14_SHIFT			14
/* Semaphore 15 status */
#define	TEAK_MCS_MCU_SEM_SEM15					0x8000
#define	TEAK_MCS_MCU_SEM_SEM15_SHIFT			15
/* MCU semaphore set register (write-only) */
#define	TEAK_MCS_MCU_SEMS						(TEAK_MCS_BASE + 0x04)
/* Request semaphore 0 */
#define	TEAK_MCS_MCU_SEMS_SEM0					0x0001
#define	TEAK_MCS_MCU_SEMS_SEM0_SHIFT			0
/* Request semaphore 1 */
#define	TEAK_MCS_MCU_SEMS_SEM1					0x0002
#define	TEAK_MCS_MCU_SEMS_SEM1_SHIFT			1
/* Request semaphore 2 */
#define	TEAK_MCS_MCU_SEMS_SEM2					0x0004
#define	TEAK_MCS_MCU_SEMS_SEM2_SHIFT			2
/* Request semaphore 3 */
#define	TEAK_MCS_MCU_SEMS_SEM3					0x0008
#define	TEAK_MCS_MCU_SEMS_SEM3_SHIFT			3
/* Request semaphore 4 */
#define	TEAK_MCS_MCU_SEMS_SEM4					0x0010
#define	TEAK_MCS_MCU_SEMS_SEM4_SHIFT			4
/* Request semaphore 5 */
#define	TEAK_MCS_MCU_SEMS_SEM5					0x0020
#define	TEAK_MCS_MCU_SEMS_SEM5_SHIFT			5
/* Request semaphore 6 */
#define	TEAK_MCS_MCU_SEMS_SEM6					0x0040
#define	TEAK_MCS_MCU_SEMS_SEM6_SHIFT			6
/* Request semaphore 7 */
#define	TEAK_MCS_MCU_SEMS_SEM7					0x0080
#define	TEAK_MCS_MCU_SEMS_SEM7_SHIFT			7
/* Request semaphore 8 */
#define	TEAK_MCS_MCU_SEMS_SEM8					0x0100
#define	TEAK_MCS_MCU_SEMS_SEM8_SHIFT			8
/* Request semaphore 9 */
#define	TEAK_MCS_MCU_SEMS_SEM9					0x0200
#define	TEAK_MCS_MCU_SEMS_SEM9_SHIFT			9
/* Request semaphore 10 */
#define	TEAK_MCS_MCU_SEMS_SEM10					0x0400
#define	TEAK_MCS_MCU_SEMS_SEM10_SHIFT			10
/* Request semaphore 11 */
#define	TEAK_MCS_MCU_SEMS_SEM11					0x0800
#define	TEAK_MCS_MCU_SEMS_SEM11_SHIFT			11
/* Request semaphore 12 */
#define	TEAK_MCS_MCU_SEMS_SEM12					0x1000
#define	TEAK_MCS_MCU_SEMS_SEM12_SHIFT			12
/* Request semaphore 13 */
#define	TEAK_MCS_MCU_SEMS_SEM13					0x2000
#define	TEAK_MCS_MCU_SEMS_SEM13_SHIFT			13
/* Request semaphore 14 */
#define	TEAK_MCS_MCU_SEMS_SEM14					0x4000
#define	TEAK_MCS_MCU_SEMS_SEM14_SHIFT			14
/* Request semaphore 15 */
#define	TEAK_MCS_MCU_SEMS_SEM15					0x8000
#define	TEAK_MCS_MCU_SEMS_SEM15_SHIFT			15
/* MCU semaphore reset register (write-only) */
#define	TEAK_MCS_MCU_SEMR						(TEAK_MCS_BASE + 0x05)
/* Release semaphore 0 */
#define	TEAK_MCS_MCU_SEMR_SEM0					0x0001
#define	TEAK_MCS_MCU_SEMR_SEM0_SHIFT			0
/* Release semaphore 1 */
#define	TEAK_MCS_MCU_SEMR_SEM1					0x0002
#define	TEAK_MCS_MCU_SEMR_SEM1_SHIFT			1
/* Release semaphore 2 */
#define	TEAK_MCS_MCU_SEMR_SEM2					0x0004
#define	TEAK_MCS_MCU_SEMR_SEM2_SHIFT			2
/* Release semaphore 3 */
#define	TEAK_MCS_MCU_SEMR_SEM3					0x0008
#define	TEAK_MCS_MCU_SEMR_SEM3_SHIFT			3
/* Release semaphore 4 */
#define	TEAK_MCS_MCU_SEMR_SEM4					0x0010
#define	TEAK_MCS_MCU_SEMR_SEM4_SHIFT			4
/* Release semaphore 5 */
#define	TEAK_MCS_MCU_SEMR_SEM5					0x0020
#define	TEAK_MCS_MCU_SEMR_SEM5_SHIFT			5
/* Release semaphore 6 */
#define	TEAK_MCS_MCU_SEMR_SEM6					0x0040
#define	TEAK_MCS_MCU_SEMR_SEM6_SHIFT			6
/* Release semaphore 7 */
#define	TEAK_MCS_MCU_SEMR_SEM7					0x0080
#define	TEAK_MCS_MCU_SEMR_SEM7_SHIFT			7
/* Release semaphore 8 */
#define	TEAK_MCS_MCU_SEMR_SEM8					0x0100
#define	TEAK_MCS_MCU_SEMR_SEM8_SHIFT			8
/* Release semaphore 9 */
#define	TEAK_MCS_MCU_SEMR_SEM9					0x0200
#define	TEAK_MCS_MCU_SEMR_SEM9_SHIFT			9
/* Release semaphore 10 */
#define	TEAK_MCS_MCU_SEMR_SEM10					0x0400
#define	TEAK_MCS_MCU_SEMR_SEM10_SHIFT			10
/* Release semaphore 11 */
#define	TEAK_MCS_MCU_SEMR_SEM11					0x0800
#define	TEAK_MCS_MCU_SEMR_SEM11_SHIFT			11
/* Release semaphore 12 */
#define	TEAK_MCS_MCU_SEMR_SEM12					0x1000
#define	TEAK_MCS_MCU_SEMR_SEM12_SHIFT			12
/* Release semaphore 13 */
#define	TEAK_MCS_MCU_SEMR_SEM13					0x2000
#define	TEAK_MCS_MCU_SEMR_SEM13_SHIFT			13
/* Release semaphore 14 */
#define	TEAK_MCS_MCU_SEMR_SEM14					0x4000
#define	TEAK_MCS_MCU_SEMR_SEM14_SHIFT			14
/* Release semaphore 15 */
#define	TEAK_MCS_MCU_SEMR_SEM15					0x8000
#define	TEAK_MCS_MCU_SEMR_SEM15_SHIFT			15
/* DSP identification register (read-only, hard-wired) */
#define	TEAK_DSP_ID								(TEAK_DSP_BASE + 0x00)
/* Hardware revision number */
#define	TEAK_DSP_ID_DSPID						0xFFFF
#define	TEAK_DSP_ID_DSPID_SHIFT					0
/* DSP control register (write-only) */
#define	TEAK_DSP_CTRL							(TEAK_DSP_BASE + 0x01)
/* Disable the TeakLite core clock */
#define	TEAK_DSP_CTRL_DSPDIS					0x0001
#define	TEAK_DSP_CTRL_DSPDIS_SHIFT				0
/* DSP debug register */
#define	TEAK_DSP_DEBUG							(TEAK_DSP_BASE + 0x02)
/* OCEM peripheral clock status */
#define	TEAK_DSP_DEBUG_OCEM						0x0002
#define	TEAK_DSP_DEBUG_OCEM_SHIFT				1
/* Program and data ROM page register */
#define	TEAK_DSP_PAGE							(TEAK_DSP_BASE + 0x03)
#define	TEAK_DSP_PAGE_DATA_PAGE_SHIFT			0
/* Undocumented register used by Mask ROM 0604 and 0801 */
#define	TEAK_DSP_UNK6							(TEAK_DSP_BASE + 0x06)
/* DSP output register */
#define	TEAK_DSP_DSPOUT							(TEAK_DSP_BASE + 0x08)
/* DSP output pin 0 */
#define	TEAK_DSP_DSPOUT_DSPOUT0					0x0001
#define	TEAK_DSP_DSPOUT_DSPOUT0_SHIFT			0
/* DSP output pin 1 */
#define	TEAK_DSP_DSPOUT_DSPOUT1					0x0002
#define	TEAK_DSP_DSPOUT_DSPOUT1_SHIFT			1
/* DSP output pin 2 */
#define	TEAK_DSP_DSPOUT_DSPOUT2					0x0004
#define	TEAK_DSP_DSPOUT_DSPOUT2_SHIFT			2
/* Latched DSP input pin 0 */
#define	TEAK_DSP_DSPOUT_DSPIN0					0x0008
#define	TEAK_DSP_DSPOUT_DSPIN0_SHIFT			3
/* Latched DSP input pin 1 */
#define	TEAK_DSP_DSPOUT_DSPIN1					0x0010
#define	TEAK_DSP_DSPOUT_DSPIN1_SHIFT			4
/* Latched monitor input pin 1 */
#define	TEAK_DSP_DSPOUT_MONIN1					0x0020
#define	TEAK_DSP_DSPOUT_MONIN1_SHIFT			5
/* Latched monitor input pin 2 */
#define	TEAK_DSP_DSPOUT_MONIN2					0x0040
#define	TEAK_DSP_DSPOUT_MONIN2_SHIFT			6
/* Latched DSP_INT3 input */
#define	TEAK_DSP_DSPOUT_MONIN3					0x0080
#define	TEAK_DSP_DSPOUT_MONIN3_SHIFT			7
/* Latched DSP_INT4 input */
#define	TEAK_DSP_DSPOUT_MONIN4					0x0100
#define	TEAK_DSP_DSPOUT_MONIN4_SHIFT			8
/* Control register */
#define	TEAK_MOD_CTRL							(TEAK_MOD_BASE + 0x00)
/* Swap I and Q outputs */
#define	TEAK_MOD_CTRL_IQSWAP					0x0001
#define	TEAK_MOD_CTRL_IQSWAP_SHIFT				0
/* Software modulator clock enable */
#define	TEAK_MOD_CTRL_MSWACT					0x0100
#define	TEAK_MOD_CTRL_MSWACT_SHIFT				8
/* Status register (read-only) */
#define	TEAK_MOD_STAT							(TEAK_MOD_BASE + 0x01)
/* Modulator active status */
#define	TEAK_MOD_STAT_MSTAT						0x0001
#define	TEAK_MOD_STAT_MSTAT_SHIFT				0
/* Interrupt address */
#define	TEAK_MOD_INT_ADDR						(TEAK_MOD_BASE + 0x03)
/* Dual-port RAM interrupt address */
#define	TEAK_MOD_INT_ADDR_MINT_ADDR				0x01FF
#define	TEAK_MOD_INT_ADDR_MINT_ADDR_SHIFT		0
/* I offset correction register */
#define	TEAK_MOD_OCI							(TEAK_MOD_BASE + 0x04)
/* Signed I offset correction value */
#define	TEAK_MOD_OCI_VALUE						0x0FFF
#define	TEAK_MOD_OCI_VALUE_SHIFT				0
/* Q offset correction register */
#define	TEAK_MOD_OCQ							(TEAK_MOD_BASE + 0x05)
/* Signed Q offset correction value */
#define	TEAK_MOD_OCQ_VALUE						0x0FFF
#define	TEAK_MOD_OCQ_VALUE_SHIFT				0
/* I amplitude correction register */
#define	TEAK_MOD_ACI							(TEAK_MOD_BASE + 0x06)
/* I amplitude correction value */
#define	TEAK_MOD_ACI_VALUE						0x00FF
#define	TEAK_MOD_ACI_VALUE_SHIFT				0
/* Q amplitude correction register */
#define	TEAK_MOD_ACQ							(TEAK_MOD_BASE + 0x07)
/* Q amplitude correction value */
#define	TEAK_MOD_ACQ_VALUE						0x00FF
#define	TEAK_MOD_ACQ_VALUE_SHIFT				0
/* Frequency correction register */
#define	TEAK_MOD_FC								(TEAK_MOD_BASE + 0x08)
/* Signed GMSK frequency correction value */
#define	TEAK_MOD_FC_VALUE						0x0FFF
#define	TEAK_MOD_FC_VALUE_SHIFT					0
/* Undocumented MODU_INIT command parameter register */
#define	TEAK_MOD_UNK9							(TEAK_MOD_BASE + 0x09)
/* Undocumented MODU_INIT command parameter register */
#define	TEAK_MOD_UNKA							(TEAK_MOD_BASE + 0x0A)
/* Control register */
#define	TEAK_SSC_CON							(TEAK_SSC_BASE + 0x00)
/* Shifted bit count in operating mode */
#define	TEAK_SSC_CON_BC							0x000F
#define	TEAK_SSC_CON_BC_SHIFT					0
/* Transfer data width minus one in programming mode */
#define	TEAK_SSC_CON_BM							0x000F
#define	TEAK_SSC_CON_BM_SHIFT					0
/* LSB/MSB heading control */
#define	TEAK_SSC_CON_HB							0x0010
#define	TEAK_SSC_CON_HB_SHIFT					4
/* Clock phase */
#define	TEAK_SSC_CON_PH							0x0020
#define	TEAK_SSC_CON_PH_SHIFT					5
/* Clock polarity */
#define	TEAK_SSC_CON_PO							0x0040
#define	TEAK_SSC_CON_PO_SHIFT					6
/* Loopback enable */
#define	TEAK_SSC_CON_LB							0x0080
#define	TEAK_SSC_CON_LB_SHIFT					7
/* Transmit error flag in operating mode */
#define	TEAK_SSC_CON_TE							0x0100
#define	TEAK_SSC_CON_TE_SHIFT					8
/* Transmit error checking enable */
#define	TEAK_SSC_CON_TEN						0x0100
#define	TEAK_SSC_CON_TEN_SHIFT					8
/* Receive error flag in operating mode */
#define	TEAK_SSC_CON_RE							0x0200
#define	TEAK_SSC_CON_RE_SHIFT					9
/* Receive error checking enable */
#define	TEAK_SSC_CON_REN						0x0200
#define	TEAK_SSC_CON_REN_SHIFT					9
/* Phase error flag in operating mode */
#define	TEAK_SSC_CON_PE							0x0400
#define	TEAK_SSC_CON_PE_SHIFT					10
/* Phase error checking enable */
#define	TEAK_SSC_CON_PEN						0x0400
#define	TEAK_SSC_CON_PEN_SHIFT					10
/* Baud-rate error flag in operating mode */
#define	TEAK_SSC_CON_BE							0x0800
#define	TEAK_SSC_CON_BE_SHIFT					11
/* Baud-rate error checking enable */
#define	TEAK_SSC_CON_BEN						0x0800
#define	TEAK_SSC_CON_BEN_SHIFT					11
/* Automatic reset on baud-rate error */
#define	TEAK_SSC_CON_AREN						0x1000
#define	TEAK_SSC_CON_AREN_SHIFT					12
/* Transfer busy flag in operating mode */
#define	TEAK_SSC_CON_BSY						0x1000
#define	TEAK_SSC_CON_BSY_SHIFT					12
/* Module clock enable */
#define	TEAK_SSC_CON_CLKON						0x2000
#define	TEAK_SSC_CON_CLKON_SHIFT				13
/* Master mode select */
#define	TEAK_SSC_CON_MS							0x4000
#define	TEAK_SSC_CON_MS_SHIFT					14
/* Module enable and programming/operating mode select */
#define	TEAK_SSC_CON_EN							0x8000
#define	TEAK_SSC_CON_EN_SHIFT					15
/* Write hardware-modified control register */
#define	TEAK_SSC_WHBCON							(TEAK_SSC_BASE + 0x01)
/* Clear transmit error flag */
#define	TEAK_SSC_WHBCON_CLRTE					0x0100
#define	TEAK_SSC_WHBCON_CLRTE_SHIFT				8
/* Clear receive error flag */
#define	TEAK_SSC_WHBCON_CLRRE					0x0200
#define	TEAK_SSC_WHBCON_CLRRE_SHIFT				9
/* Clear phase error flag */
#define	TEAK_SSC_WHBCON_CLRPE					0x0400
#define	TEAK_SSC_WHBCON_CLRPE_SHIFT				10
/* Clear baud-rate error flag */
#define	TEAK_SSC_WHBCON_CLRBE					0x0800
#define	TEAK_SSC_WHBCON_CLRBE_SHIFT				11
/* Set transmit error flag */
#define	TEAK_SSC_WHBCON_SETTE					0x1000
#define	TEAK_SSC_WHBCON_SETTE_SHIFT				12
/* Set receive error flag */
#define	TEAK_SSC_WHBCON_SETRE					0x2000
#define	TEAK_SSC_WHBCON_SETRE_SHIFT				13
/* Set phase error flag */
#define	TEAK_SSC_WHBCON_SETPE					0x4000
#define	TEAK_SSC_WHBCON_SETPE_SHIFT				14
/* Set baud-rate error flag */
#define	TEAK_SSC_WHBCON_SETBE					0x8000
#define	TEAK_SSC_WHBCON_SETBE_SHIFT				15
/* Transmit buffer */
#define	TEAK_SSC_TXB							(TEAK_SSC_BASE + 0x02)
/* Transmit data */
#define	TEAK_SSC_TXB_VALUE						0xFFFF
#define	TEAK_SSC_TXB_VALUE_SHIFT				0
/* Receive buffer */
#define	TEAK_SSC_RXB							(TEAK_SSC_BASE + 0x03)
/* Received data */
#define	TEAK_SSC_RXB_VALUE						0xFFFF
#define	TEAK_SSC_RXB_VALUE_SHIFT				0
/* Receive FIFO control register */
#define	TEAK_SSC_RXFCON							(TEAK_SSC_BASE + 0x04)
/* Receive FIFO enable */
#define	TEAK_SSC_RXFCON_EN						0x0001
#define	TEAK_SSC_RXFCON_EN_SHIFT				0
/* Receive FIFO flush */
#define	TEAK_SSC_RXFCON_FLU						0x0002
#define	TEAK_SSC_RXFCON_FLU_SHIFT				1
/* Receive FIFO transparent mode enable */
#define	TEAK_SSC_RXFCON_TMEN					0x0004
#define	TEAK_SSC_RXFCON_TMEN_SHIFT				2
/* Receive FIFO interrupt trigger level */
#define	TEAK_SSC_RXFCON_ITL						0x3F00
#define	TEAK_SSC_RXFCON_ITL_SHIFT				8
/* Transmit FIFO control register */
#define	TEAK_SSC_TXFCON							(TEAK_SSC_BASE + 0x05)
/* Transmit FIFO enable */
#define	TEAK_SSC_TXFCON_EN						0x0001
#define	TEAK_SSC_TXFCON_EN_SHIFT				0
/* Transmit FIFO flush */
#define	TEAK_SSC_TXFCON_FLU						0x0002
#define	TEAK_SSC_TXFCON_FLU_SHIFT				1
/* Transmit FIFO transparent mode enable */
#define	TEAK_SSC_TXFCON_TMEN					0x0004
#define	TEAK_SSC_TXFCON_TMEN_SHIFT				2
/* Transmit FIFO interrupt trigger level */
#define	TEAK_SSC_TXFCON_ITL						0x3F00
#define	TEAK_SSC_TXFCON_ITL_SHIFT				8
/* FIFO status register (read-only) */
#define	TEAK_SSC_FSTAT							(TEAK_SSC_BASE + 0x06)
/* Receive FIFO fill level */
#define	TEAK_SSC_FSTAT_RXFFL					0x003F
#define	TEAK_SSC_FSTAT_RXFFL_SHIFT				0
/* Transmit FIFO fill level */
#define	TEAK_SSC_FSTAT_TXFFL					0x3F00
#define	TEAK_SSC_FSTAT_TXFFL_SHIFT				8
/* Baud-rate register */
#define	TEAK_SSC_BR								(TEAK_SSC_BASE + 0x07)
/* Baud-rate timer reload value */
#define	TEAK_SSC_BR_VALUE						0xFFFF
#define	TEAK_SSC_BR_VALUE_SHIFT					0
/* Fractional divider register */
#define	TEAK_SSC_FDV							(TEAK_SSC_BASE + 0x08)
/* Fractional divider value */
#define	TEAK_SSC_FDV_VALUE						0x01FF
#define	TEAK_SSC_FDV_VALUE_SHIFT				0
/* Control register */
#define	TEAK_I2S1_CTRL							(TEAK_I2S1_BASE + 0x00)
/* Module clock enable */
#define	TEAK_I2S1_CTRL_I2SON					0x0001
#define	TEAK_I2S1_CTRL_I2SON_SHIFT				0
/* Start data transmission */
#define	TEAK_I2S1_CTRL_I2STXSTART				0x0002
#define	TEAK_I2S1_CTRL_I2STXSTART_SHIFT			1
/* Start data reception */
#define	TEAK_I2S1_CTRL_I2SRXSTART				0x0004
#define	TEAK_I2S1_CTRL_I2SRXSTART_SHIFT			2
/* Transmit PCM mode select */
#define	TEAK_I2S1_CTRL_TXPCM					0x0020
#define	TEAK_I2S1_CTRL_TXPCM_SHIFT				5
/* Receive PCM mode select */
#define	TEAK_I2S1_CTRL_RXPCM					0x0040
#define	TEAK_I2S1_CTRL_RXPCM_SHIFT				6
/* DAI mode enable */
#define	TEAK_I2S1_CTRL_DAI_EN					0x0080
#define	TEAK_I2S1_CTRL_DAI_EN_SHIFT				7
/* Clock select register */
#define	TEAK_I2S1_CSEL							(TEAK_I2S1_BASE + 0x01)
/* Transmit clock source */
#define	TEAK_I2S1_CSEL_TXCLKSEL					0x0003
#define	TEAK_I2S1_CSEL_TXCLKSEL_SHIFT			0
/* CLK0 source */
#define	TEAK_I2S1_CSEL_CLK0SEL					0x000C
#define	TEAK_I2S1_CSEL_CLK0SEL_SHIFT			2
/* Receive clock source */
#define	TEAK_I2S1_CSEL_RXCLKSEL					0x0030
#define	TEAK_I2S1_CSEL_RXCLKSEL_SHIFT			4
/* CLK1 source */
#define	TEAK_I2S1_CSEL_CLK1SEL					0x00C0
#define	TEAK_I2S1_CSEL_CLK1SEL_SHIFT			6
/* Read/write address register (read-only) */
#define	TEAK_I2S1_RWADDR						(TEAK_I2S1_BASE + 0x02)
/* Transmit ring-buffer read position */
#define	TEAK_I2S1_RWADDR_RDADDR					0x003F
#define	TEAK_I2S1_RWADDR_RDADDR_SHIFT			0
/* Receive ring-buffer write position */
#define	TEAK_I2S1_RWADDR_WRADDR					0x3F00
#define	TEAK_I2S1_RWADDR_WRADDR_SHIFT			8
/* Clock numerator 0 */
#define	TEAK_I2S1_NUM0							(TEAK_I2S1_BASE + 0x03)
/* Fractional divider numerator */
#define	TEAK_I2S1_NUM0_NUMERATOR				0x07FF
#define	TEAK_I2S1_NUM0_NUMERATOR_SHIFT			0
/* Reference frequency select */
#define	TEAK_I2S1_NUM0_FREF						0x3000
#define	TEAK_I2S1_NUM0_FREF_SHIFT				12
#define	TEAK_I2S1_NUM0_FREF_MODULE_CLOCK		0x0
#define	TEAK_I2S1_NUM0_FREF_CLOCK_104MHZ		0x1000
/* Clock denominator 0 */
#define	TEAK_I2S1_DEN0							(TEAK_I2S1_BASE + 0x04)
/* Fractional divider denominator */
#define	TEAK_I2S1_DEN0_DENOMINATOR				0xFFFF
#define	TEAK_I2S1_DEN0_DENOMINATOR_SHIFT		0
/* Clock numerator 1 */
#define	TEAK_I2S1_NUM1							(TEAK_I2S1_BASE + 0x05)
/* Fractional divider numerator */
#define	TEAK_I2S1_NUM1_NUMERATOR				0x07FF
#define	TEAK_I2S1_NUM1_NUMERATOR_SHIFT			0
/* Reference frequency select */
#define	TEAK_I2S1_NUM1_FREF						0x3000
#define	TEAK_I2S1_NUM1_FREF_SHIFT				12
#define	TEAK_I2S1_NUM1_FREF_MODULE_CLOCK		0x0
#define	TEAK_I2S1_NUM1_FREF_CLOCK_104MHZ		0x1000
/* Clock denominator 1 */
#define	TEAK_I2S1_DEN1							(TEAK_I2S1_BASE + 0x06)
/* Fractional divider denominator */
#define	TEAK_I2S1_DEN1_DENOMINATOR				0xFFFF
#define	TEAK_I2S1_DEN1_DENOMINATOR_SHIFT		0
/* Receive configuration register */
#define	TEAK_I2S1_RXCONF						(TEAK_I2S1_BASE + 0x07)
/* Receive sampling edge */
#define	TEAK_I2S1_RXCONF_EDGE					0x0001
#define	TEAK_I2S1_RXCONF_EDGE_SHIFT				0
/* First-bit delay */
#define	TEAK_I2S1_RXCONF_DEL					0x0002
#define	TEAK_I2S1_RXCONF_DEL_SHIFT				1
/* Word-address polarity */
#define	TEAK_I2S1_RXCONF_POL					0x0004
#define	TEAK_I2S1_RXCONF_POL_SHIFT				2
/* Frame period */
#define	TEAK_I2S1_RXCONF_PERIOD					0x0018
#define	TEAK_I2S1_RXCONF_PERIOD_SHIFT			3
#define	TEAK_I2S1_RXCONF_PERIOD_CLOCKS_64		0x0
#define	TEAK_I2S1_RXCONF_PERIOD_CLOCKS_48		0x8
#define	TEAK_I2S1_RXCONF_PERIOD_CLOCKS_32		0x10
/* Sample width */
#define	TEAK_I2S1_RXCONF_WIDTH					0x00E0
#define	TEAK_I2S1_RXCONF_WIDTH_SHIFT			5
#define	TEAK_I2S1_RXCONF_WIDTH_BITS_16			0x0
#define	TEAK_I2S1_RXCONF_WIDTH_BITS_18			0x80
#define	TEAK_I2S1_RXCONF_WIDTH_BITS_20			0xA0
#define	TEAK_I2S1_RXCONF_WIDTH_BITS_24			0xC0
#define	TEAK_I2S1_RXCONF_WIDTH_BITS_32			0xE0
/* Data alignment */
#define	TEAK_I2S1_RXCONF_ALIGN					0x0100
#define	TEAK_I2S1_RXCONF_ALIGN_SHIFT			8
/* PCM burst clock output level */
#define	TEAK_I2S1_RXCONF_CLK1_OUT				0x2000
#define	TEAK_I2S1_RXCONF_CLK1_OUT_SHIFT			13
/* Continuous PCM burst clock */
#define	TEAK_I2S1_RXCONF_CLK1_CONT				0x4000
#define	TEAK_I2S1_RXCONF_CLK1_CONT_SHIFT		14
/* PCM word-address pulse length */
#define	TEAK_I2S1_RXCONF_WA1_LEN				0x8000
#define	TEAK_I2S1_RXCONF_WA1_LEN_SHIFT			15
/* Receive interrupt address */
#define	TEAK_I2S1_RXINTADDR						(TEAK_I2S1_BASE + 0x08)
/* Receive ring-buffer interrupt position */
#define	TEAK_I2S1_RXINTADDR_RXINTPTR			0x003F
#define	TEAK_I2S1_RXINTADDR_RXINTPTR_SHIFT		0
/* Transmit configuration register */
#define	TEAK_I2S1_TXCONF						(TEAK_I2S1_BASE + 0x09)
/* Transmit output edge */
#define	TEAK_I2S1_TXCONF_EDGE					0x0001
#define	TEAK_I2S1_TXCONF_EDGE_SHIFT				0
/* First-bit delay */
#define	TEAK_I2S1_TXCONF_DEL					0x0002
#define	TEAK_I2S1_TXCONF_DEL_SHIFT				1
/* Word-address polarity */
#define	TEAK_I2S1_TXCONF_POL					0x0004
#define	TEAK_I2S1_TXCONF_POL_SHIFT				2
/* Frame period */
#define	TEAK_I2S1_TXCONF_PERIOD					0x0018
#define	TEAK_I2S1_TXCONF_PERIOD_SHIFT			3
#define	TEAK_I2S1_TXCONF_PERIOD_CLOCKS_64		0x0
#define	TEAK_I2S1_TXCONF_PERIOD_CLOCKS_48		0x8
#define	TEAK_I2S1_TXCONF_PERIOD_CLOCKS_32		0x10
/* Sample width */
#define	TEAK_I2S1_TXCONF_WIDTH					0x00E0
#define	TEAK_I2S1_TXCONF_WIDTH_SHIFT			5
#define	TEAK_I2S1_TXCONF_WIDTH_BITS_16			0x0
#define	TEAK_I2S1_TXCONF_WIDTH_BITS_18			0x80
#define	TEAK_I2S1_TXCONF_WIDTH_BITS_20			0xA0
#define	TEAK_I2S1_TXCONF_WIDTH_BITS_24			0xC0
#define	TEAK_I2S1_TXCONF_WIDTH_BITS_32			0xE0
/* Data alignment */
#define	TEAK_I2S1_TXCONF_ALIGN					0x0100
#define	TEAK_I2S1_TXCONF_ALIGN_SHIFT			8
/* Channel duplication mode */
#define	TEAK_I2S1_TXCONF_MONO					0x0600
#define	TEAK_I2S1_TXCONF_MONO_SHIFT				9
#define	TEAK_I2S1_TXCONF_MONO_STEREO			0x0
#define	TEAK_I2S1_TXCONF_MONO_RIGHT				0x400
#define	TEAK_I2S1_TXCONF_MONO_LEFT				0x600
/* Mute left channel */
#define	TEAK_I2S1_TXCONF_MUTE_L					0x0800
#define	TEAK_I2S1_TXCONF_MUTE_L_SHIFT			11
/* Mute right channel */
#define	TEAK_I2S1_TXCONF_MUTE_R					0x1000
#define	TEAK_I2S1_TXCONF_MUTE_R_SHIFT			12
/* PCM burst clock output level */
#define	TEAK_I2S1_TXCONF_CLK0_OUT				0x2000
#define	TEAK_I2S1_TXCONF_CLK0_OUT_SHIFT			13
/* Continuous PCM burst clock */
#define	TEAK_I2S1_TXCONF_CLK0_CONT				0x4000
#define	TEAK_I2S1_TXCONF_CLK0_CONT_SHIFT		14
/* PCM word-address pulse length */
#define	TEAK_I2S1_TXCONF_WA0_LEN				0x8000
#define	TEAK_I2S1_TXCONF_WA0_LEN_SHIFT			15
/* Transmit interrupt address */
#define	TEAK_I2S1_TXINTADDR						(TEAK_I2S1_BASE + 0x0A)
/* Transmit ring-buffer interrupt position */
#define	TEAK_I2S1_TXINTADDR_TXINTPTR			0x003F
#define	TEAK_I2S1_TXINTADDR_TXINTPTR_SHIFT		0
/* Control register */
#define	TEAK_I2S2_CTRL							(TEAK_I2S2_BASE + 0x00)
/* Module clock enable */
#define	TEAK_I2S2_CTRL_I2SON					0x0001
#define	TEAK_I2S2_CTRL_I2SON_SHIFT				0
/* Start data transmission */
#define	TEAK_I2S2_CTRL_I2STXSTART				0x0002
#define	TEAK_I2S2_CTRL_I2STXSTART_SHIFT			1
/* Start data reception */
#define	TEAK_I2S2_CTRL_I2SRXSTART				0x0004
#define	TEAK_I2S2_CTRL_I2SRXSTART_SHIFT			2
/* Transmit PCM mode select */
#define	TEAK_I2S2_CTRL_TXPCM					0x0020
#define	TEAK_I2S2_CTRL_TXPCM_SHIFT				5
/* Receive PCM mode select */
#define	TEAK_I2S2_CTRL_RXPCM					0x0040
#define	TEAK_I2S2_CTRL_RXPCM_SHIFT				6
/* DAI mode enable */
#define	TEAK_I2S2_CTRL_DAI_EN					0x0080
#define	TEAK_I2S2_CTRL_DAI_EN_SHIFT				7
/* Clock select register */
#define	TEAK_I2S2_CSEL							(TEAK_I2S2_BASE + 0x01)
/* Transmit clock source */
#define	TEAK_I2S2_CSEL_TXCLKSEL					0x0003
#define	TEAK_I2S2_CSEL_TXCLKSEL_SHIFT			0
/* CLK0 source */
#define	TEAK_I2S2_CSEL_CLK0SEL					0x000C
#define	TEAK_I2S2_CSEL_CLK0SEL_SHIFT			2
/* Receive clock source */
#define	TEAK_I2S2_CSEL_RXCLKSEL					0x0030
#define	TEAK_I2S2_CSEL_RXCLKSEL_SHIFT			4
/* CLK1 source */
#define	TEAK_I2S2_CSEL_CLK1SEL					0x00C0
#define	TEAK_I2S2_CSEL_CLK1SEL_SHIFT			6
/* Read/write address register (read-only) */
#define	TEAK_I2S2_RWADDR						(TEAK_I2S2_BASE + 0x02)
/* Transmit ring-buffer read position */
#define	TEAK_I2S2_RWADDR_RDADDR					0x003F
#define	TEAK_I2S2_RWADDR_RDADDR_SHIFT			0
/* Receive ring-buffer write position */
#define	TEAK_I2S2_RWADDR_WRADDR					0x3F00
#define	TEAK_I2S2_RWADDR_WRADDR_SHIFT			8
/* Clock numerator 0 */
#define	TEAK_I2S2_NUM0							(TEAK_I2S2_BASE + 0x03)
/* Fractional divider numerator */
#define	TEAK_I2S2_NUM0_NUMERATOR				0x07FF
#define	TEAK_I2S2_NUM0_NUMERATOR_SHIFT			0
/* Reference frequency select */
#define	TEAK_I2S2_NUM0_FREF						0x3000
#define	TEAK_I2S2_NUM0_FREF_SHIFT				12
#define	TEAK_I2S2_NUM0_FREF_MODULE_CLOCK		0x0
#define	TEAK_I2S2_NUM0_FREF_CLOCK_104MHZ		0x1000
/* Clock denominator 0 */
#define	TEAK_I2S2_DEN0							(TEAK_I2S2_BASE + 0x04)
/* Fractional divider denominator */
#define	TEAK_I2S2_DEN0_DENOMINATOR				0xFFFF
#define	TEAK_I2S2_DEN0_DENOMINATOR_SHIFT		0
/* Clock numerator 1 */
#define	TEAK_I2S2_NUM1							(TEAK_I2S2_BASE + 0x05)
/* Fractional divider numerator */
#define	TEAK_I2S2_NUM1_NUMERATOR				0x07FF
#define	TEAK_I2S2_NUM1_NUMERATOR_SHIFT			0
/* Reference frequency select */
#define	TEAK_I2S2_NUM1_FREF						0x3000
#define	TEAK_I2S2_NUM1_FREF_SHIFT				12
#define	TEAK_I2S2_NUM1_FREF_MODULE_CLOCK		0x0
#define	TEAK_I2S2_NUM1_FREF_CLOCK_104MHZ		0x1000
/* Clock denominator 1 */
#define	TEAK_I2S2_DEN1							(TEAK_I2S2_BASE + 0x06)
/* Fractional divider denominator */
#define	TEAK_I2S2_DEN1_DENOMINATOR				0xFFFF
#define	TEAK_I2S2_DEN1_DENOMINATOR_SHIFT		0
/* Receive configuration register */
#define	TEAK_I2S2_RXCONF						(TEAK_I2S2_BASE + 0x07)
/* Receive sampling edge */
#define	TEAK_I2S2_RXCONF_EDGE					0x0001
#define	TEAK_I2S2_RXCONF_EDGE_SHIFT				0
/* First-bit delay */
#define	TEAK_I2S2_RXCONF_DEL					0x0002
#define	TEAK_I2S2_RXCONF_DEL_SHIFT				1
/* Word-address polarity */
#define	TEAK_I2S2_RXCONF_POL					0x0004
#define	TEAK_I2S2_RXCONF_POL_SHIFT				2
/* Frame period */
#define	TEAK_I2S2_RXCONF_PERIOD					0x0018
#define	TEAK_I2S2_RXCONF_PERIOD_SHIFT			3
#define	TEAK_I2S2_RXCONF_PERIOD_CLOCKS_64		0x0
#define	TEAK_I2S2_RXCONF_PERIOD_CLOCKS_48		0x8
#define	TEAK_I2S2_RXCONF_PERIOD_CLOCKS_32		0x10
/* Sample width */
#define	TEAK_I2S2_RXCONF_WIDTH					0x00E0
#define	TEAK_I2S2_RXCONF_WIDTH_SHIFT			5
#define	TEAK_I2S2_RXCONF_WIDTH_BITS_16			0x0
#define	TEAK_I2S2_RXCONF_WIDTH_BITS_18			0x80
#define	TEAK_I2S2_RXCONF_WIDTH_BITS_20			0xA0
#define	TEAK_I2S2_RXCONF_WIDTH_BITS_24			0xC0
#define	TEAK_I2S2_RXCONF_WIDTH_BITS_32			0xE0
/* Data alignment */
#define	TEAK_I2S2_RXCONF_ALIGN					0x0100
#define	TEAK_I2S2_RXCONF_ALIGN_SHIFT			8
/* PCM burst clock output level */
#define	TEAK_I2S2_RXCONF_CLK1_OUT				0x2000
#define	TEAK_I2S2_RXCONF_CLK1_OUT_SHIFT			13
/* Continuous PCM burst clock */
#define	TEAK_I2S2_RXCONF_CLK1_CONT				0x4000
#define	TEAK_I2S2_RXCONF_CLK1_CONT_SHIFT		14
/* PCM word-address pulse length */
#define	TEAK_I2S2_RXCONF_WA1_LEN				0x8000
#define	TEAK_I2S2_RXCONF_WA1_LEN_SHIFT			15
/* Receive interrupt address */
#define	TEAK_I2S2_RXINTADDR						(TEAK_I2S2_BASE + 0x08)
/* Receive ring-buffer interrupt position */
#define	TEAK_I2S2_RXINTADDR_RXINTPTR			0x003F
#define	TEAK_I2S2_RXINTADDR_RXINTPTR_SHIFT		0
/* Transmit configuration register */
#define	TEAK_I2S2_TXCONF						(TEAK_I2S2_BASE + 0x09)
/* Transmit output edge */
#define	TEAK_I2S2_TXCONF_EDGE					0x0001
#define	TEAK_I2S2_TXCONF_EDGE_SHIFT				0
/* First-bit delay */
#define	TEAK_I2S2_TXCONF_DEL					0x0002
#define	TEAK_I2S2_TXCONF_DEL_SHIFT				1
/* Word-address polarity */
#define	TEAK_I2S2_TXCONF_POL					0x0004
#define	TEAK_I2S2_TXCONF_POL_SHIFT				2
/* Frame period */
#define	TEAK_I2S2_TXCONF_PERIOD					0x0018
#define	TEAK_I2S2_TXCONF_PERIOD_SHIFT			3
#define	TEAK_I2S2_TXCONF_PERIOD_CLOCKS_64		0x0
#define	TEAK_I2S2_TXCONF_PERIOD_CLOCKS_48		0x8
#define	TEAK_I2S2_TXCONF_PERIOD_CLOCKS_32		0x10
/* Sample width */
#define	TEAK_I2S2_TXCONF_WIDTH					0x00E0
#define	TEAK_I2S2_TXCONF_WIDTH_SHIFT			5
#define	TEAK_I2S2_TXCONF_WIDTH_BITS_16			0x0
#define	TEAK_I2S2_TXCONF_WIDTH_BITS_18			0x80
#define	TEAK_I2S2_TXCONF_WIDTH_BITS_20			0xA0
#define	TEAK_I2S2_TXCONF_WIDTH_BITS_24			0xC0
#define	TEAK_I2S2_TXCONF_WIDTH_BITS_32			0xE0
/* Data alignment */
#define	TEAK_I2S2_TXCONF_ALIGN					0x0100
#define	TEAK_I2S2_TXCONF_ALIGN_SHIFT			8
/* Channel duplication mode */
#define	TEAK_I2S2_TXCONF_MONO					0x0600
#define	TEAK_I2S2_TXCONF_MONO_SHIFT				9
#define	TEAK_I2S2_TXCONF_MONO_STEREO			0x0
#define	TEAK_I2S2_TXCONF_MONO_RIGHT				0x400
#define	TEAK_I2S2_TXCONF_MONO_LEFT				0x600
/* Mute left channel */
#define	TEAK_I2S2_TXCONF_MUTE_L					0x0800
#define	TEAK_I2S2_TXCONF_MUTE_L_SHIFT			11
/* Mute right channel */
#define	TEAK_I2S2_TXCONF_MUTE_R					0x1000
#define	TEAK_I2S2_TXCONF_MUTE_R_SHIFT			12
/* PCM burst clock output level */
#define	TEAK_I2S2_TXCONF_CLK0_OUT				0x2000
#define	TEAK_I2S2_TXCONF_CLK0_OUT_SHIFT			13
/* Continuous PCM burst clock */
#define	TEAK_I2S2_TXCONF_CLK0_CONT				0x4000
#define	TEAK_I2S2_TXCONF_CLK0_CONT_SHIFT		14
/* PCM word-address pulse length */
#define	TEAK_I2S2_TXCONF_WA0_LEN				0x8000
#define	TEAK_I2S2_TXCONF_WA0_LEN_SHIFT			15
/* Transmit interrupt address */
#define	TEAK_I2S2_TXINTADDR						(TEAK_I2S2_BASE + 0x0A)
/* Transmit ring-buffer interrupt position */
#define	TEAK_I2S2_TXINTADDR_TXINTPTR			0x003F
#define	TEAK_I2S2_TXINTADDR_TXINTPTR_SHIFT		0
/* Control register */
#define	TEAK_I2S3_CTRL							(TEAK_I2S3_BASE + 0x00)
/* Module clock enable */
#define	TEAK_I2S3_CTRL_I2SON					0x0001
#define	TEAK_I2S3_CTRL_I2SON_SHIFT				0
/* Start data transmission */
#define	TEAK_I2S3_CTRL_I2STXSTART				0x0002
#define	TEAK_I2S3_CTRL_I2STXSTART_SHIFT			1
/* Transmit PCM mode select */
#define	TEAK_I2S3_CTRL_TXPCM					0x0020
#define	TEAK_I2S3_CTRL_TXPCM_SHIFT				5
/* Clock select register */
#define	TEAK_I2S3_CSEL							(TEAK_I2S3_BASE + 0x01)
/* Transmit clock source */
#define	TEAK_I2S3_CSEL_TXCLKSEL					0x0002
#define	TEAK_I2S3_CSEL_TXCLKSEL_SHIFT			1
/* External clock source */
#define	TEAK_I2S3_CSEL_CLKSEL					0x0008
#define	TEAK_I2S3_CSEL_CLKSEL_SHIFT				3
/* Read address register (read-only) */
#define	TEAK_I2S3_RADDR							(TEAK_I2S3_BASE + 0x02)
/* Transmit ring-buffer read position */
#define	TEAK_I2S3_RADDR_RDADDR					0x003F
#define	TEAK_I2S3_RADDR_RDADDR_SHIFT			0
/* Clock numerator */
#define	TEAK_I2S3_NUM							(TEAK_I2S3_BASE + 0x03)
/* Fractional divider numerator */
#define	TEAK_I2S3_NUM_NUMERATOR					0x07FF
#define	TEAK_I2S3_NUM_NUMERATOR_SHIFT			0
/* Reference frequency select */
#define	TEAK_I2S3_NUM_FREF						0x3000
#define	TEAK_I2S3_NUM_FREF_SHIFT				12
#define	TEAK_I2S3_NUM_FREF_MODULE_CLOCK			0x0
#define	TEAK_I2S3_NUM_FREF_CLOCK_104MHZ			0x1000
/* Clock denominator */
#define	TEAK_I2S3_DEN							(TEAK_I2S3_BASE + 0x04)
/* Fractional divider denominator */
#define	TEAK_I2S3_DEN_DENOMINATOR				0xFFFF
#define	TEAK_I2S3_DEN_DENOMINATOR_SHIFT			0
/* Transmit configuration register */
#define	TEAK_I2S3_TXCONF						(TEAK_I2S3_BASE + 0x09)
/* Transmit output edge */
#define	TEAK_I2S3_TXCONF_EDGE					0x0001
#define	TEAK_I2S3_TXCONF_EDGE_SHIFT				0
/* First-bit delay */
#define	TEAK_I2S3_TXCONF_DEL					0x0002
#define	TEAK_I2S3_TXCONF_DEL_SHIFT				1
/* Word-address polarity */
#define	TEAK_I2S3_TXCONF_POL					0x0004
#define	TEAK_I2S3_TXCONF_POL_SHIFT				2
/* Frame period */
#define	TEAK_I2S3_TXCONF_PERIOD					0x0018
#define	TEAK_I2S3_TXCONF_PERIOD_SHIFT			3
#define	TEAK_I2S3_TXCONF_PERIOD_CLOCKS_64		0x0
#define	TEAK_I2S3_TXCONF_PERIOD_CLOCKS_48		0x8
#define	TEAK_I2S3_TXCONF_PERIOD_CLOCKS_32		0x10
/* Sample width */
#define	TEAK_I2S3_TXCONF_WIDTH					0x00E0
#define	TEAK_I2S3_TXCONF_WIDTH_SHIFT			5
#define	TEAK_I2S3_TXCONF_WIDTH_BITS_16			0x0
#define	TEAK_I2S3_TXCONF_WIDTH_BITS_18			0x80
#define	TEAK_I2S3_TXCONF_WIDTH_BITS_20			0xA0
#define	TEAK_I2S3_TXCONF_WIDTH_BITS_24			0xC0
#define	TEAK_I2S3_TXCONF_WIDTH_BITS_32			0xE0
/* Data alignment */
#define	TEAK_I2S3_TXCONF_ALIGN					0x0100
#define	TEAK_I2S3_TXCONF_ALIGN_SHIFT			8
/* Channel duplication mode */
#define	TEAK_I2S3_TXCONF_MONO					0x0600
#define	TEAK_I2S3_TXCONF_MONO_SHIFT				9
#define	TEAK_I2S3_TXCONF_MONO_STEREO			0x0
#define	TEAK_I2S3_TXCONF_MONO_RIGHT				0x400
#define	TEAK_I2S3_TXCONF_MONO_LEFT				0x600
/* Mute left channel */
#define	TEAK_I2S3_TXCONF_MUTE_L					0x0800
#define	TEAK_I2S3_TXCONF_MUTE_L_SHIFT			11
/* Mute right channel */
#define	TEAK_I2S3_TXCONF_MUTE_R					0x1000
#define	TEAK_I2S3_TXCONF_MUTE_R_SHIFT			12
/* PCM burst clock output level */
#define	TEAK_I2S3_TXCONF_CLK_OUT				0x2000
#define	TEAK_I2S3_TXCONF_CLK_OUT_SHIFT			13
/* Continuous PCM burst clock */
#define	TEAK_I2S3_TXCONF_CLK_CONT				0x4000
#define	TEAK_I2S3_TXCONF_CLK_CONT_SHIFT			14
/* PCM word-address pulse length */
#define	TEAK_I2S3_TXCONF_WA_LEN					0x8000
#define	TEAK_I2S3_TXCONF_WA_LEN_SHIFT			15
/* Transmit interrupt address */
#define	TEAK_I2S3_TXINTADDR						(TEAK_I2S3_BASE + 0x0A)
/* Transmit ring-buffer interrupt position */
#define	TEAK_I2S3_TXINTADDR_TXINTPTR			0x003F
#define	TEAK_I2S3_TXINTADDR_TXINTPTR_SHIFT		0
