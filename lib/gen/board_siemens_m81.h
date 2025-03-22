#pragma once
#include <stdint.h>

#define PMB8876

// GPIO numbers
#define	GPIO_KP_OUT6			GPIO_KP_IN5
#define	GPIO_BT_WAKEUP_GSM		GPIO_KP_IN6
#define	GPIO_AC_RX				GPIO_USART0_RXD
#define	GPIO_AC_TX				GPIO_USART0_TXD
#define	GPIO_AC_CTS				GPIO_USART0_RTS
#define	GPIO_AC_RTS				GPIO_USART0_CTS
#define	GPIO_AC_DCD				GPIO_DSPOUT0
#define	GPIO_BT_RX				GPIO_USART1_RXD
#define	GPIO_LIGHT_PWM3			GPIO_USART1_TXD
#define	GPIO_BT_RTS				GPIO_USART1_RTS
#define	GPIO_BT_CTS				GPIO_USART1_CTS
#define	GPIO_PM_CHARGE_UC		GPIO_T_OUT1
#define	GPIO_RF_FE_CTR_GSM		GPIO_T_OUT2
#define	GPIO_RF_FE_CTR_DCS		GPIO_T_OUT3
#define	GPIO_RF_FE_CTR_RX		GPIO_T_OUT4
#define	GPIO_RF_ANT_DET			GPIO_T_OUT5
#define	GPIO_PM_SSC_CS			GPIO_T_OUT6
#define	GPIO_PM_RF2_EN			GPIO_T_OUT7
#define	GPIO_RF_BAND_SW			GPIO_T_OUT8
#define	GPIO_SERIAL_EN			GPIO_T_OUT10
#define	GPIO_IR_SEL				GPIO_T_OUT12
#define	GPIO_PM_RINGIN			GPIO_RF_STR1
#define	GPIO_MMC_VCC_EN			GPIO_CLKOUT0
#define	GPIO_PM_WADOG			GPIO_DSPOUT1
#define	GPIO_MM_INT1			GPIO_DSPIN1
#define	GPIO_HW_DET_MOB_TYPE3	GPIO_PIPESTAT2
#define	GPIO_HW_DET_MOB_TYPE2	GPIO_PIPESTAT1
#define	GPIO_HW_DET_MOB_TYPE1	GPIO_PIPESTAT0
#define	GPIO_HW_DET_MOB_TYPE4	GPIO_TRACEPKT0
#define	GPIO_HW_DET_BLUETOOTH	GPIO_TRACEPKT1
#define	GPIO_HW_DET_BAND_SEL	GPIO_TRACEPKT3
#define	GPIO_MMC_CD				GPIO_FCDP_RB
#define	GPIO_EXT_FL_TRIG		GPIO_DIF_VD
#define	GPIO_MM_EN				GPIO_I2S1_CLK1


// Keypad
#define	KP_NUM1			0x00000102
#define	KP_NUM4			0x00000104
#define	KP_NUM7			0x00000108
#define	KP_STAR			0x00000110
#define	KP_NUM2			0x00000202
#define	KP_NUM5			0x00000204
#define	KP_NUM8			0x00000208
#define	KP_NUM0			0x00000210
#define	KP_NUM3			0x00000402
#define	KP_NUM6			0x00000404
#define	KP_NUM9			0x00000408
#define	KP_HASH			0x00000410
#define	KP_NAV_UP		0x00000801
#define	KP_NAV_RIGHT	0x00000802
#define	KP_NAV_CENTER	0x00000804
#define	KP_NAV_LEFT		0x00000808
#define	KP_NAV_DOWN		0x00000810
#define	KP_SEND			0x00004001
#define	KP_BROWSER		0x00004002
#define	KP_MUSIC		0x00004004
#define	KP_SOFT_LEFT	0x00004008
#define	KP_SOFT_RIGHT	0x00004010
#define	KP_END_CALL		0x0001FF01
#define	KP_VOLUME_DOWN	0x0001FF02
#define	KP_VOLUME_UP	0x0001FF04


