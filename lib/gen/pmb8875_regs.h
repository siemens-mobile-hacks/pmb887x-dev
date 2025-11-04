#pragma once
#include <pmb887x.h>

// GPIO numbers
#define	GPIO_KP_IN0		0
#define	GPIO_KP_IN1		1
#define	GPIO_KP_IN2		2
#define	GPIO_KP_IN3		3
#define	GPIO_KP_IN4		4
#define	GPIO_KP_IN5		5
#define	GPIO_KP_IN6		6
#define	GPIO_KP_OUT0	7
#define	GPIO_KP_OUT1	8
#define	GPIO_KP_OUT2	9
#define	GPIO_KP_OUT3	10
#define	GPIO_USART0_RXD	11
#define	GPIO_USART0_TXD	12
#define	GPIO_USART0_RTS	13
#define	GPIO_USART0_CTS	14
#define	GPIO_DSPOUT0	15
#define	GPIO_USART1_RXD	16
#define	GPIO_USART1_TXD	17
#define	GPIO_USART1_RTS	18
#define	GPIO_USART1_CTS	19
#define	GPIO_SSC2_SCLK	20
#define	GPIO_SSC2_MTSR	21
#define	GPIO_SSC2_MRST	22
#define	GPIO_DIF_CLK	23
#define	GPIO_DIF_DAT	24
#define	GPIO_DIF_RS		25
#define	GPIO_DIF_CS		26
#define	GPIO_DIF_RESET	27
#define	GPIO_I2C_SCL	28
#define	GPIO_I2C_SDA	29
#define	GPIO_PIN30		30
#define	GPIO_PIN31		31
#define	GPIO_PIN32		32
#define	GPIO_PIN33		33
#define	GPIO_PIN34		34
#define	GPIO_PIN35		35
#define	GPIO_PIN36		36
#define	GPIO_PIN37		37
#define	GPIO_PIN38		38
#define	GPIO_PIN39		39
#define	GPIO_PIN40		40
#define	GPIO_PIN41		41
#define	GPIO_PIN42		42
#define	GPIO_T_OUT0		43
#define	GPIO_T_OUT1		44
#define	GPIO_T_OUT2		45
#define	GPIO_T_OUT3		46
#define	GPIO_T_OUT4		47
#define	GPIO_T_OUT5		48
#define	GPIO_T_OUT6		49
#define	GPIO_T_OUT7		50
#define	GPIO_T_OUT8		51
#define	GPIO_T_OUT9		52
#define	GPIO_T_OUT10	53
#define	GPIO_T_OUT11	54
#define	GPIO_T_OUT12	55
#define	GPIO_RF_STR0	56
#define	GPIO_RF_STR1	57
#define	GPIO_CLKOUT0	58
#define	GPIO_RF_CLK		59
#define	GPIO_PIN60		60
#define	GPIO_PIN61		61
#define	GPIO_DSPOUT1	62
#define	GPIO_DSPIN1		63
#define	GPIO_PIN64		64
#define	GPIO_PIN65		65
#define	GPIO_PIPESTAT2	66
#define	GPIO_PIPESTAT1	67
#define	GPIO_PIPESTAT0	68
#define	GPIO_TRACEPKT0	69
#define	GPIO_TRACEPKT1	70
#define	GPIO_TRACEPKT2	71
#define	GPIO_TRACEPKT3	72
#define	GPIO_PIN73		73
#define	GPIO_PIN74		74
#define	GPIO_PIN75		75
#define	GPIO_PIN76		76


// IRQ numbers
#define	VIC_USART0_TX_IRQ		4
#define	VIC_USART0_TBUF_IRQ		5
#define	VIC_USART0_RX_IRQ		6
#define	VIC_USART0_ERR_IRQ		7
#define	VIC_USART0_CTS_IRQ		8
#define	VIC_USART0_ABDET_IRQ	9
#define	VIC_USART0_ABSTART_IRQ	10
#define	VIC_USART0_TMO_IRQ		11
#define	VIC_SSC_TX_IRQ			12
#define	VIC_SSC_RX_IRQ			13
#define	VIC_SSC_ERR_IRQ			14
#define	VIC_SSC_TMO_IRQ			15
#define	VIC_DIF_TX_IRQ			18
#define	VIC_DIF_RX_IRQ			19
#define	VIC_DIF_ERR_IRQ			20
#define	VIC_DIF_TMO_IRQ			21
#define	VIC_SIM_UNK0_IRQ		22
#define	VIC_SIM_UNK1_IRQ		23
#define	VIC_SIM_UNK2_IRQ		24
#define	VIC_USB_IRQ				25
#define	VIC_USART1_TX_IRQ		26
#define	VIC_USART1_TBUF_IRQ		27
#define	VIC_USART1_RX_IRQ		28
#define	VIC_USART1_ERR_IRQ		29
#define	VIC_USART1_CTS_IRQ		30
#define	VIC_USART1_ABDET_IRQ	31
#define	VIC_USART1_ABSTART_IRQ	32
#define	VIC_USART1_TMO_IRQ		33
#define	VIC_DMAC_ERR_IRQ		35
#define	VIC_DMAC_CH0_IRQ		36
#define	VIC_DMAC_CH1_IRQ		37
#define	VIC_DMAC_CH2_IRQ		38
#define	VIC_DMAC_CH3_IRQ		39
#define	VIC_DMAC_CH4_IRQ		40
#define	VIC_DMAC_CH5_IRQ		41
#define	VIC_DMAC_CH6_IRQ		42
#define	VIC_DMAC_CH7_IRQ		43
#define	VIC_RTC_IRQ				46
#define	VIC_SCU_EXTI0_IRQ		48
#define	VIC_SCU_EXTI1_IRQ		49
#define	VIC_SCU_EXTI2_IRQ		50
#define	VIC_SCU_EXTI3_IRQ		51
#define	VIC_SCU_EXTI4_IRQ		52
#define	VIC_SCU_DSP0_IRQ		53
#define	VIC_SCU_DSP1_IRQ		54
#define	VIC_SCU_DSP2_IRQ		55
#define	VIC_SCU_DSP3_IRQ		56
#define	VIC_SCU_DSP4_IRQ		57
#define	VIC_SCU_UNK0_IRQ		58
#define	VIC_SCU_UNK1_IRQ		59
#define	VIC_SCU_UNK2_IRQ		60
#define	VIC_SCU_EXTI5_IRQ		61
#define	VIC_SCU_EXTI6_IRQ		62
#define	VIC_SCCU_UNK_IRQ		63
#define	VIC_SCU_EXTI7_IRQ		63
#define	VIC_SCCU_WAKE_IRQ		64
#define	VIC_PLL_IRQ				65
#define	VIC_I2C_DATA_IRQ		66
#define	VIC_I2C_PROTO_IRQ		67
#define	VIC_I2C_END_IRQ			68
#define	VIC_ADC_INT0_IRQ		70
#define	VIC_ADC_INT1_IRQ		71
#define	VIC_CAPCOM0_T0_IRQ		72
#define	VIC_CAPCOM0_T1_IRQ		73
#define	VIC_CAPCOM0_CC0_IRQ		74
#define	VIC_CAPCOM0_CC1_IRQ		75
#define	VIC_CAPCOM0_CC2_IRQ		76
#define	VIC_CAPCOM0_CC3_IRQ		77
#define	VIC_CAPCOM0_CC4_IRQ		78
#define	VIC_CAPCOM0_CC5_IRQ		79
#define	VIC_CAPCOM0_CC6_IRQ		80
#define	VIC_CAPCOM0_CC7_IRQ		81
#define	VIC_CAPCOM1_T0_IRQ		82
#define	VIC_CAPCOM1_T1_IRQ		83
#define	VIC_CAPCOM1_CC0_IRQ		84
#define	VIC_CAPCOM1_CC1_IRQ		85
#define	VIC_CAPCOM1_CC2_IRQ		86
#define	VIC_CAPCOM1_CC3_IRQ		87
#define	VIC_CAPCOM1_CC4_IRQ		88
#define	VIC_CAPCOM1_CC5_IRQ		89
#define	VIC_CAPCOM1_CC6_IRQ		90
#define	VIC_CAPCOM1_CC7_IRQ		91
#define	VIC_GPTU0_SRC7_IRQ		92
#define	VIC_GPTU0_SRC6_IRQ		93
#define	VIC_GPTU0_SRC5_IRQ		94
#define	VIC_GPTU0_SRC4_IRQ		95
#define	VIC_GPTU0_SRC3_IRQ		96
#define	VIC_GPTU0_SRC2_IRQ		97
#define	VIC_GPTU0_SRC1_IRQ		98
#define	VIC_GPTU0_SRC0_IRQ		99
#define	VIC_GPTU1_SRC7_IRQ		100
#define	VIC_GPTU1_SRC6_IRQ		101
#define	VIC_GPTU1_SRC5_IRQ		102
#define	VIC_GPTU1_SRC4_IRQ		103
#define	VIC_GPTU1_SRC3_IRQ		104
#define	VIC_GPTU1_SRC2_IRQ		105
#define	VIC_GPTU1_SRC1_IRQ		106
#define	VIC_GPTU1_SRC0_IRQ		107
#define	VIC_KEYPAD_PRESS_IRQ	108
#define	VIC_KEYPAD_UNK0_IRQ		109
#define	VIC_KEYPAD_UNK1_IRQ		110
#define	VIC_KEYPAD_RELEASE_IRQ	111
#define	VIC_TPU_INT_UNK0_IRQ	113
#define	VIC_TPU_INT_UNK1_IRQ	114
#define	VIC_TPU_INT_UNK2_IRQ	115
#define	VIC_TPU_INT_UNK3_IRQ	116
#define	VIC_TPU_INT_UNK4_IRQ	117
#define	VIC_TPU_INT_UNK5_IRQ	118
#define	VIC_TPU_INT0_IRQ		119
#define	VIC_TPU_INT1_IRQ		120
#define	VIC_GPRSCU_INT0_IRQ		121
#define	VIC_GPRSCU_INT1_IRQ		122


// Common regs for all modules
/* Clock Control Register */
#define	MOD_CLC_DISR						BIT(0)			 // Module Disable Request Bit
#define	MOD_CLC_DISS						BIT(1)			 // Module Disable Status Bit
#define	MOD_CLC_SPEN						BIT(2)			 // Module Suspend Enable Bit
#define	MOD_CLC_EDIS						BIT(3)			 // Module External Request Disable
#define	MOD_CLC_SBWE						BIT(4)			 // Module Suspend Bit Write Enable
#define	MOD_CLC_FSOE						BIT(5)			 // Module Fast Shut-Off Enable.
#define	MOD_CLC_RMC							GENMASK(15, 8)	 // Module Clock Divider for Normal Mode
#define	MOD_CLC_RMC_SHIFT					8

/* Module Identifier Register */
#define	MOD_ID_REV							GENMASK(7, 0)
#define	MOD_ID_REV_SHIFT					0
#define	MOD_ID_32B							GENMASK(15, 8)
#define	MOD_ID_32B_SHIFT					8
#define	MOD_ID_NUMBER						GENMASK(31, 16)
#define	MOD_ID_NUMBER_SHIFT					16

/* Service Routing Control Register */
#define	MOD_SRC_SRPN						GENMASK(7, 0)	 // IRQ priority number
#define	MOD_SRC_SRPN_SHIFT					0
#define	MOD_SRC_TOS							GENMASK(11, 10)	 // Type of service for node
#define	MOD_SRC_TOS_SHIFT					10
#define	MOD_SRC_SRE							BIT(12)			 // IRQ enable
#define	MOD_SRC_SRR							BIT(13)			 // IRQ Service Request Bit
#define	MOD_SRC_CLRR						BIT(14)			 // IRQ Request Clear Bit
#define	MOD_SRC_SETR						BIT(15)			 // IRQ Request Set Bit

#define	AMBA_PERIPH_ID0_PARTNUMBER0			GENMASK(7, 0)
#define	AMBA_PERIPH_ID0_PARTNUMBER0_SHIFT	0

#define	AMBA_PERIPH_ID1_PARTNUMBER1			GENMASK(3, 0)
#define	AMBA_PERIPH_ID1_PARTNUMBER1_SHIFT	0
#define	AMBA_PERIPH_ID1_DESIGNER0			GENMASK(11, 4)
#define	AMBA_PERIPH_ID1_DESIGNER0_SHIFT		4

#define	AMBA_PERIPH_ID2_DESIGNER1			GENMASK(3, 0)
#define	AMBA_PERIPH_ID2_DESIGNER1_SHIFT		0
#define	AMBA_PERIPH_ID2_REVISION			GENMASK(11, 4)
#define	AMBA_PERIPH_ID2_REVISION_SHIFT		4

#define	AMBA_PERIPH_ID3_CONFIGURATION		GENMASK(7, 0)
#define	AMBA_PERIPH_ID3_CONFIGURATION_SHIFT	0



// EBU [MOD_NUM=0014, MOD_REV=04, MOD_32BIT=C0]
// EBU from XMC4500 official public datasheet
#define	EBU_BASE					0xF0000000
/* Clock Control Register */
#define	EBU_CLC						MMIO32(EBU_BASE + 0x00)

/* Module Identifier Register */
#define	EBU_ID						MMIO32(EBU_BASE + 0x08)

#define	EBU_CON						MMIO32(EBU_BASE + 0x10)
#define	EBU_CON_EXTRECON			BIT(1)									 // External reconfiguration
#define	EBU_CON_EXTSVM				BIT(2)									 // Perform master in
#define	EBU_CON_EXTACC				BIT(3)									 // External access FPI-bus
#define	EBU_CON_EXTLOCK				BIT(4)									 // Lock external bus
#define	EBU_CON_ARBSYNC				BIT(5)									 // Arbitration evaluation
#define	EBU_CON_ARBMODE				GENMASK(7, 6)							 // Arbitration mode
#define	EBU_CON_ARBMODE_SHIFT		6
#define	EBU_CON_TOUTC				GENMASK(15, 8)							 // Time-out control
#define	EBU_CON_TOUTC_SHIFT			8
#define	EBU_CON_GLOBALCS			GENMASK(23, 16)							 // Global chip select signal
#define	EBU_CON_GLOBALCS_SHIFT		16
#define	EBU_CON_BUSCLK				GENMASK(25, 24)							 // Clock generation
#define	EBU_CON_BUSCLK_SHIFT		24
#define	EBU_CON_SDCMSEL				BIT(26)									 // SDRAM Clock Mode Select
#define	EBU_CON_CS0FAM				BIT(27)									 // CS0 Fills Address Map
#define	EBU_CON_EMUFAM				BIT(28)									 // CSEMU Fills Address Map
#define	EBU_CON_BFSSS				BIT(29)									 // Burst FLASH Single Stage Synchronization

#define	EBU_BFCON					MMIO32(EBU_BASE + 0x20)
#define	EBU_BFCON_FETBLEN0			GENMASK(3, 0)							 // Fetch Burst Length for Burst FLASH Type 0
#define	EBU_BFCON_FETBLEN0_SHIFT	0
#define	EBU_BFCON_FBBMSEL0			BIT(4)									 // FLASH Burst Buffer Mode Select for Burst FLASH Type 0
#define	EBU_BFCON_WAITFUNC0			BIT(5)									 // Function of WAIT Input for Burst FLASH Type 0
#define	EBU_BFCON_EXTCLOCK			GENMASK(7, 6)							 // Frequency of external clock
#define	EBU_BFCON_EXTCLOCK_SHIFT	6
#define	EBU_BFCON_BFCMSEL			BIT(8)									 // Burst FLASH Clock Mode Select
#define	EBU_BFCON_EBSE0				BIT(9)									 // Early Burst Signal Enable for Burst FLASH Type 0
#define	EBU_BFCON_DBA0				BIT(10)									 // Disable Burst Address Wrapping
#define	EBU_BFCON_FDBKEN			BIT(11)									 // Burst FLASH Clock Feedback Enable
#define	EBU_BFCON_DTALTNCY			GENMASK(15, 12)							 // Latency Cycle Control
#define	EBU_BFCON_DTALTNCY_SHIFT	12
#define	EBU_BFCON_FETBLEN1			GENMASK(19, 16)							 // Fetch Burst Length for Burst FLASH Type 1
#define	EBU_BFCON_FETBLEN1_SHIFT	16
#define	EBU_BFCON_FBBMSEL1			BIT(20)									 // FLASH Burst Buffer Mode Select for Burst FLASH Type 1
#define	EBU_BFCON_WAITFUNC1			BIT(21)									 // Function of WAIT Input for Burst FLASH Type 1
#define	EBU_BFCON_DBA1				BIT(23)									 // Disable Burst Address Wrapping
#define	EBU_BFCON_EBSE1				BIT(25)									 // Early Burst Signal Enable for Burst FLASH Type 1

#define	EBU_SDRMREF(n)				MMIO32(EBU_BASE + 0x40 + ((n) * 0x8))
#define	EBU_SDRMREF_REFRESHC		GENMASK(5, 0)							 // Refresh counter period
#define	EBU_SDRMREF_REFRESHC_SHIFT	0
#define	EBU_SDRMREF_REFRESHR		GENMASK(8, 6)							 // Number of refresh commands
#define	EBU_SDRMREF_REFRESHR_SHIFT	6
#define	EBU_SDRMREF_SELFREXST		BIT(9)									 // Self refresh exit status
#define	EBU_SDRMREF_SELFREX			BIT(10)									 // Self refresh exit
#define	EBU_SDRMREF_SELFRENST		BIT(11)									 // Self refresh entry status
#define	EBU_SDRMREF_SELFREN			BIT(12)									 // Self refresh entry
#define	EBU_SDRMREF_AUTOSELFR		BIT(13)									 // Automatic self refresh

#define	EBU_SDRMCON(n)				MMIO32(EBU_BASE + 0x50 + ((n) * 0x8))
#define	EBU_SDRMCON_CRAS			GENMASK(3, 0)							 // Row to precharge delay counter
#define	EBU_SDRMCON_CRAS_SHIFT		0
#define	EBU_SDRMCON_CRFSH			GENMASK(7, 4)							 // Refresh commands counter
#define	EBU_SDRMCON_CRFSH_SHIFT		4
#define	EBU_SDRMCON_CRSC			GENMASK(9, 8)							 // Mode register setup time
#define	EBU_SDRMCON_CRSC_SHIFT		8
#define	EBU_SDRMCON_CRP				GENMASK(11, 10)							 // Row precharge time counter
#define	EBU_SDRMCON_CRP_SHIFT		10
#define	EBU_SDRMCON_AWIDTH			GENMASK(13, 12)							 // Width of column address
#define	EBU_SDRMCON_AWIDTH_SHIFT	12
#define	EBU_SDRMCON_CRCD			GENMASK(15, 14)							 // Row to column delay counter
#define	EBU_SDRMCON_CRCD_SHIFT		14
#define	EBU_SDRMCON_CRC				GENMASK(18, 16)							 // Row cycle time counter
#define	EBU_SDRMCON_CRC_SHIFT		16
#define	EBU_SDRMCON_PAGEM			GENMASK(21, 19)							 // Mask for page tag
#define	EBU_SDRMCON_PAGEM_SHIFT		19
#define	EBU_SDRMCON_BANKM			GENMASK(24, 22)							 // Mask for bank tag
#define	EBU_SDRMCON_BANKM_SHIFT		22

#define	EBU_SDRMOD(n)				MMIO32(EBU_BASE + 0x60 + ((n) * 0x8))
#define	EBU_SDRMOD_BURSTL			GENMASK(2, 0)							 // Burst length
#define	EBU_SDRMOD_BURSTL_SHIFT		0
#define	EBU_SDRMOD_BTYP				BIT(3)									 // Burst type
#define	EBU_SDRMOD_CASLAT			GENMASK(6, 4)							 // CAS latency
#define	EBU_SDRMOD_CASLAT_SHIFT		4
#define	EBU_SDRMOD_OPMODE			GENMASK(13, 7)							 // Operation Mode
#define	EBU_SDRMOD_OPMODE_SHIFT		7

#define	EBU_SDRSTAT(n)				MMIO32(EBU_BASE + 0x70 + ((n) * 0x8))
#define	EBU_SDRSTAT_REFERR			BIT(0)									 // SDRAM Refresh Error
#define	EBU_SDRSTAT_SDRM_BUSY		BIT(1)									 // SDRAM Busy

#define	EBU_ADDRSEL(n)				MMIO32(EBU_BASE + 0x80 + ((n) * 0x8))
#define	EBU_ADDRSEL_REGENAB			BIT(0)									 // Memory Region
#define	EBU_ADDRSEL_ALTENAB			BIT(1)									 // Alternate Segment Comparison
#define	EBU_ADDRSEL_MASK			GENMASK(7, 4)							 // Address Mask
#define	EBU_ADDRSEL_MASK_SHIFT		4
#define	EBU_ADDRSEL_ALTSEG			GENMASK(11, 8)							 // Alternate Segment
#define	EBU_ADDRSEL_ALTSEG_SHIFT	8
#define	EBU_ADDRSEL_BASE			GENMASK(31, 12)							 // Base Address
#define	EBU_ADDRSEL_BASE_SHIFT		12

#define	EBU_BUSCON(n)				MMIO32(EBU_BASE + 0xC0 + ((n) * 0x8))
#define	EBU_BUSCON_MULTMAP			GENMASK(6, 0)							 // Multiplier map
#define	EBU_BUSCON_MULTMAP_SHIFT	0
#define	EBU_BUSCON_WPRE				BIT(8)									 // Weak prefetch
#define	EBU_BUSCON_AALIGN			BIT(9)									 // Address alignment
#define	EBU_BUSCON_CTYPE			GENMASK(11, 10)							 // Cycle Type
#define	EBU_BUSCON_CTYPE_SHIFT		10
#define	EBU_BUSCON_CMULT			GENMASK(15, 13)							 // Cycle multiplier
#define	EBU_BUSCON_CMULT_SHIFT		13
#define	EBU_BUSCON_ENDIAN			BIT(16)									 // Endian mode
#define	EBU_BUSCON_DLOAD			BIT(17)									 // Data upload
#define	EBU_BUSCON_PRE				BIT(18)									 // Prefetch mechanism
#define	EBU_BUSCON_WAITINV			BIT(19)									 // Reversed polarity at WAIT
#define	EBU_BUSCON_BCGEN			GENMASK(21, 20)							 // Signal timing mode
#define	EBU_BUSCON_BCGEN_SHIFT		20
#define	EBU_BUSCON_PORTW			GENMASK(23, 22)							 // Port width
#define	EBU_BUSCON_PORTW_SHIFT		22
#define	EBU_BUSCON_WAIT				GENMASK(25, 24)							 // External wait state
#define	EBU_BUSCON_WAIT_SHIFT		24
#define	EBU_BUSCON_XCMDDELAY		GENMASK(27, 26)							 // External command delay
#define	EBU_BUSCON_XCMDDELAY_SHIFT	26
#define	EBU_BUSCON_AGEN				GENMASK(30, 28)							 // Address generation
#define	EBU_BUSCON_AGEN_SHIFT		28
#define	EBU_BUSCON_WRITE			BIT(31)									 // Write protection

#define	EBU_BUSAP(n)				MMIO32(EBU_BASE + 0x100 + ((n) * 0x8))
#define	EBU_BUSAP_DTACS				GENMASK(3, 0)							 // Between different regions
#define	EBU_BUSAP_DTACS_SHIFT		0
#define	EBU_BUSAP_DTARDWR			GENMASK(7, 4)							 // Between read and write accesses
#define	EBU_BUSAP_DTARDWR_SHIFT		4
#define	EBU_BUSAP_WRRECOVC			GENMASK(10, 8)							 // After write accesses
#define	EBU_BUSAP_WRRECOVC_SHIFT	8
#define	EBU_BUSAP_RDRECOVC			GENMASK(13, 11)							 // After read accesses
#define	EBU_BUSAP_RDRECOVC_SHIFT	11
#define	EBU_BUSAP_DATAC				GENMASK(15, 14)							 // Write accesses
#define	EBU_BUSAP_DATAC_SHIFT		14
#define	EBU_BUSAP_BURSTC			GENMASK(18, 16)							 // During burst accesses
#define	EBU_BUSAP_BURSTC_SHIFT		16
#define	EBU_BUSAP_WAITWRC			GENMASK(21, 19)							 // Programmed for wait accesses
#define	EBU_BUSAP_WAITWRC_SHIFT		19
#define	EBU_BUSAP_WAITRDC			GENMASK(24, 22)							 // Programmed for read accesses
#define	EBU_BUSAP_WAITRDC_SHIFT		22
#define	EBU_BUSAP_CMDDELAY			GENMASK(27, 25)							 // Programmed command
#define	EBU_BUSAP_CMDDELAY_SHIFT	25
#define	EBU_BUSAP_AHOLDC			GENMASK(29, 28)							 // Multiplexed accesses
#define	EBU_BUSAP_AHOLDC_SHIFT		28
#define	EBU_BUSAP_ADDRC				GENMASK(31, 30)							 // Address Cycles
#define	EBU_BUSAP_ADDRC_SHIFT		30

#define	EBU_EMUAS					MMIO32(EBU_BASE + 0x160)
#define	EBU_EMUAS_REGENAB			BIT(0)									 // Memory region
#define	EBU_EMUAS_ALTENAB			BIT(1)									 // Alternate segment comparison
#define	EBU_EMUAS_MASK				GENMASK(7, 4)							 // Address mask
#define	EBU_EMUAS_MASK_SHIFT		4
#define	EBU_EMUAS_ALTSEG			GENMASK(11, 8)							 // Alternate segment
#define	EBU_EMUAS_ALTSEG_SHIFT		8
#define	EBU_EMUAS_BASE				GENMASK(31, 12)							 // Base address
#define	EBU_EMUAS_BASE_SHIFT		12

#define	EBU_EMUBC					MMIO32(EBU_BASE + 0x168)
#define	EBU_EMUBC_MULTMAP			GENMASK(6, 0)							 // Multiplier map
#define	EBU_EMUBC_MULTMAP_SHIFT		0
#define	EBU_EMUBC_WPRE				BIT(8)									 // Weak prefetch
#define	EBU_EMUBC_AALIGN			BIT(9)									 // Address alignment
#define	EBU_EMUBC_CTYPE				GENMASK(11, 10)							 // Cycle Type
#define	EBU_EMUBC_CTYPE_SHIFT		10
#define	EBU_EMUBC_CMULT				GENMASK(15, 13)							 // Cycle multiplier
#define	EBU_EMUBC_CMULT_SHIFT		13
#define	EBU_EMUBC_ENDIAN			BIT(16)									 // Endian mode
#define	EBU_EMUBC_DLOAD				BIT(17)									 // Data upload
#define	EBU_EMUBC_PRE				BIT(18)									 // Prefetch mechanism
#define	EBU_EMUBC_WAITINV			BIT(19)									 // Reversed polarity at WAIT
#define	EBU_EMUBC_BCGEN				GENMASK(21, 20)							 // Signal timing mode
#define	EBU_EMUBC_BCGEN_SHIFT		20
#define	EBU_EMUBC_PORTW				GENMASK(23, 22)							 // Port width
#define	EBU_EMUBC_PORTW_SHIFT		22
#define	EBU_EMUBC_WAIT				GENMASK(25, 24)							 // External wait state
#define	EBU_EMUBC_WAIT_SHIFT		24
#define	EBU_EMUBC_XCMDDELAY			GENMASK(27, 26)							 // External command delay
#define	EBU_EMUBC_XCMDDELAY_SHIFT	26
#define	EBU_EMUBC_AGEN				GENMASK(30, 28)							 // Address generation
#define	EBU_EMUBC_AGEN_SHIFT		28
#define	EBU_EMUBC_WRITE				BIT(31)									 // Write protection

#define	EBU_EMUBAP					MMIO32(EBU_BASE + 0x170)
#define	EBU_EMUBAP_DTACS			GENMASK(3, 0)							 // Between different regions
#define	EBU_EMUBAP_DTACS_SHIFT		0
#define	EBU_EMUBAP_DTARDWR			GENMASK(7, 4)							 // Between read and write accesses
#define	EBU_EMUBAP_DTARDWR_SHIFT	4
#define	EBU_EMUBAP_WRRECOVC			GENMASK(10, 8)							 // After write accesses
#define	EBU_EMUBAP_WRRECOVC_SHIFT	8
#define	EBU_EMUBAP_RDRECOVC			GENMASK(13, 11)							 // After read accesses
#define	EBU_EMUBAP_RDRECOVC_SHIFT	11
#define	EBU_EMUBAP_DATAC			GENMASK(15, 14)							 // Write accesses
#define	EBU_EMUBAP_DATAC_SHIFT		14
#define	EBU_EMUBAP_BURSTC			GENMASK(18, 16)							 // During burst accesses
#define	EBU_EMUBAP_BURSTC_SHIFT		16
#define	EBU_EMUBAP_WAITWRC			GENMASK(21, 19)							 // Programmed for wait accesses
#define	EBU_EMUBAP_WAITWRC_SHIFT	19
#define	EBU_EMUBAP_WAITRDC			GENMASK(24, 22)							 // Programmed for read accesses
#define	EBU_EMUBAP_WAITRDC_SHIFT	22
#define	EBU_EMUBAP_CMDDELAY			GENMASK(27, 25)							 // Programmed command
#define	EBU_EMUBAP_CMDDELAY_SHIFT	25
#define	EBU_EMUBAP_AHOLDC			GENMASK(29, 28)							 // Multiplexed accesses
#define	EBU_EMUBAP_AHOLDC_SHIFT		28
#define	EBU_EMUBAP_ADDRC			GENMASK(31, 30)							 // Address Cycles
#define	EBU_EMUBAP_ADDRC_SHIFT		30

#define	EBU_EMUOVL					MMIO32(EBU_BASE + 0x178)
#define	EBU_EMUOVL_OVERLAY			GENMASK(7, 0)							 // Overlay chip select
#define	EBU_EMUOVL_OVERLAY_SHIFT	0

#define	EBU_USERCON					MMIO32(EBU_BASE + 0x190)


// USART0 [MOD_NUM=0044, MOD_REV=E2, MOD_32BIT=00]
// ASC0 from Tricore TC1766 official public datasheet
#define	USART0_BASE						0xF1000000
#define	USART0							0xF1000000

#define	USART1_BASE						0xF1800000
#define	USART1							0xF1800000

/* Clock Control Register */
#define	USART_CLC(base)					MMIO32((base) + 0x00)

#define	USART_PISEL(base)				MMIO32((base) + 0x04)

/* Module Identifier Register */
#define	USART_ID(base)					MMIO32((base) + 0x08)

#define	USART_CON(base)					MMIO32((base) + 0x10)
#define	USART_CON_M						GENMASK(2, 0)			 // ASC Mode Control.
#define	USART_CON_M_SHIFT				0
#define	USART_CON_M_SYNC_8BIT			0x0
#define	USART_CON_M_ASYNC_8BIT			0x1
#define	USART_CON_M_ASYNC_IRDA_8BIT		0x2
#define	USART_CON_M_ASYNC_PARITY_7BIT	0x3
#define	USART_CON_M_ASYNC_9BIT			0x4
#define	USART_CON_M_ASYNC_WAKE_UP_8BIT	0x5
#define	USART_CON_M_ASYNC_PARITY_8BIT	0x7
#define	USART_CON_STP					BIT(3)					 // Number of stop bits (0: 1 stop bit; 1: two stop bits)
#define	USART_CON_STP_ONE				0x0
#define	USART_CON_STP_TWO				0x8
#define	USART_CON_REN					BIT(4)					 // Receiver bit enable (0: disable; 1: enable)
#define	USART_CON_PEN					BIT(5)					 // Parity check enable (0: ignore; 1: check)
#define	USART_CON_FEN					BIT(6)					 // Framing error check (0: ignore; 1: check)
#define	USART_CON_OEN					BIT(7)					 // Overrun check enable (0: ignore; 1: check)
#define	USART_CON_PE					BIT(8)					 // Parity error flag
#define	USART_CON_FE					BIT(9)					 // Framing error flag
#define	USART_CON_OE					BIT(10)					 // Overrun error flag
#define	USART_CON_FDE					BIT(11)					 // Fraction divider enable (0: disable; 1: enable)
#define	USART_CON_ODD					BIT(12)					 // Parity selection (0: even; 1: odd)
#define	USART_CON_BRS					BIT(13)					 // Baudrate selection (0: Pre-scaler /2; 1: Pre-scaler / 3)
#define	USART_CON_LB					BIT(14)					 // Loopback mode (0: disable; 1: enable)
#define	USART_CON_CON_R					BIT(15)					 // Baud rate generator run control (0: disable; 1: enable)

#define	USART_BG(base)					MMIO32((base) + 0x14)

#define	USART_FDV(base)					MMIO32((base) + 0x18)

#define	USART_PMW(base)					MMIO32((base) + 0x1C)
#define	USART_PMW_PW_VALUE				GENMASK(7, 0)			 // IrDA Pulse Width Value
#define	USART_PMW_PW_VALUE_SHIFT		0
#define	USART_PMW_IRPW					BIT(8)					 // IrDA Pulse Width Selection

#define	USART_TXB(base)					MMIO32((base) + 0x20)

#define	USART_RXB(base)					MMIO32((base) + 0x24)

#define	USART_ABCON(base)				MMIO32((base) + 0x30)
#define	USART_ABCON_ABEN				BIT(0)					 // Autobaud detection enable
#define	USART_ABCON_AUREN				BIT(1)					 // Auto control of CON.REN (too complex for here)
#define	USART_ABCON_ABSTEN				BIT(2)					 // Start of autobaud detect interrupt (0: dis; 1: en)
#define	USART_ABCON_ABDETEN				BIT(3)					 // Autobaud detection interrupt enable (0: dis; 1: en)
#define	USART_ABCON_FCDETEN				BIT(4)					 // Fir char of two byte frame detect
#define	USART_ABCON_ABEM_ECHO_DET		BIT(8)					 // Autobaud echo mode enabled during detection
#define	USART_ABCON_ABEM_ECHO_ALWAYS	BIT(9)					 // Autobaud echo mode always enabled
#define	USART_ABCON_TXINV				BIT(10)					 // Transmit invert enable (0: disable; 1: enable)
#define	USART_ABCON_RXINV				BIT(11)					 // Receive invert enable (0: disable; 1: enable)

#define	USART_ABSTAT(base)				MMIO32((base) + 0x34)
#define	USART_ABSTAT_FCSDET				BIT(0)					 // First character with small letter detected
#define	USART_ABSTAT_FCCDET				BIT(1)					 // First character with capital letter detected
#define	USART_ABSTAT_SCSDET				BIT(2)					 // Second character with small letter detected
#define	USART_ABSTAT_SCCDET				BIT(3)					 // Second character with capital letter detected
#define	USART_ABSTAT_DETWAIT			BIT(4)					 // Autobaud detect is waiting

#define	USART_RXFCON(base)				MMIO32((base) + 0x40)
#define	USART_RXFCON_RXFEN				BIT(0)					 // Receive FIFO enable
#define	USART_RXFCON_RXFFLU				BIT(1)					 // Receive FIFO flush
#define	USART_RXFCON_RXTMEN				BIT(2)					 // Receive FIFO transparent mode enable
#define	USART_RXFCON_RXFITL				GENMASK(11, 8)			 // Receive FIFO interrupt trigger level
#define	USART_RXFCON_RXFITL_SHIFT		8

#define	USART_TXFCON(base)				MMIO32((base) + 0x44)
#define	USART_TXFCON_TXFEN				BIT(0)					 // Transmit FIFO enable
#define	USART_TXFCON_TXFFLU				BIT(1)					 // Transmit FIFO flush
#define	USART_TXFCON_TXTMEN				BIT(2)					 // Transmit FIFO transparent mode enable
#define	USART_TXFCON_TXFITL				GENMASK(11, 8)			 // Transmit FIFO interrupt trigger level
#define	USART_TXFCON_TXFITL_SHIFT		8

#define	USART_FSTAT(base)				MMIO32((base) + 0x48)
#define	USART_FSTAT_RXFFL				GENMASK(3, 0)			 // Receive FIFO filling level mask
#define	USART_FSTAT_RXFFL_SHIFT			0
#define	USART_FSTAT_TXFFL				GENMASK(11, 8)			 // Transmit FIFO filling level mask
#define	USART_FSTAT_TXFFL_SHIFT			8

#define	USART_WHBCON(base)				MMIO32((base) + 0x50)
#define	USART_WHBCON_CLRREN				BIT(4)					 // Clear receiver enable bit
#define	USART_WHBCON_SETREN				BIT(5)					 // Set receiver enable bit
#define	USART_WHBCON_CLRPE				BIT(8)					 // Clear parity error flag
#define	USART_WHBCON_CLRFE				BIT(9)					 // Clear framing error flag
#define	USART_WHBCON_CLROE				BIT(10)					 // Clear overrun error flag
#define	USART_WHBCON_SETPE				BIT(11)					 // Set parity error flag
#define	USART_WHBCON_SETFE				BIT(12)					 // Set framing error flag
#define	USART_WHBCON_SETOE				BIT(13)					 // Set overrun error flag

#define	USART_WHBABCON(base)			MMIO32((base) + 0x54)

#define	USART_WHBABSTAT(base)			MMIO32((base) + 0x58)

#define	USART_FCCON(base)				MMIO32((base) + 0x5C)
#define	USART_FCCON_CTSEN				BIT(0)					 // RTS enbled (0: disable; 1: enable)
#define	USART_FCCON_RTSEN				BIT(1)					 // CTS enable (0: disable; 1: enable)
#define	USART_FCCON_RTS					BIT(4)					 // RTS control bit
#define	USART_FCCON_RTS_TRIGGER			GENMASK(13, 8)			 // RTS receive FIFO trigger level
#define	USART_FCCON_RTS_TRIGGER_SHIFT	8

#define	USART_FCSTAT(base)				MMIO32((base) + 0x60)
#define	USART_FCSTAT_CTS				BIT(0)					 // CTS Status (0: inactive; 1: active)
#define	USART_FCSTAT_RTS				BIT(1)					 // RTS Status (0: inactive; 1: active)

#define	USART_IMSC(base)				MMIO32((base) + 0x64)
#define	USART_IMSC_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_IMSC_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_IMSC_RX					BIT(2)					 // Receive interrupt mask
#define	USART_IMSC_ERR					BIT(3)					 // Error interrupt mask
#define	USART_IMSC_CTS					BIT(4)					 // CTS interrupt mask
#define	USART_IMSC_ABDET				BIT(5)					 // Autobaud detected interrupt mask
#define	USART_IMSC_ABSTART				BIT(6)					 // Autobaud start interrupt mask
#define	USART_IMSC_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_RIS(base)					MMIO32((base) + 0x68)
#define	USART_RIS_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_RIS_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_RIS_RX					BIT(2)					 // Receive interrupt mask
#define	USART_RIS_ERR					BIT(3)					 // Error interrupt mask
#define	USART_RIS_CTS					BIT(4)					 // CTS interrupt mask
#define	USART_RIS_ABDET					BIT(5)					 // Autobaud detected interrupt mask
#define	USART_RIS_ABSTART				BIT(6)					 // Autobaud start interrupt mask
#define	USART_RIS_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_MIS(base)					MMIO32((base) + 0x6C)
#define	USART_MIS_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_MIS_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_MIS_RX					BIT(2)					 // Receive interrupt mask
#define	USART_MIS_ERR					BIT(3)					 // Error interrupt mask
#define	USART_MIS_CTS					BIT(4)					 // CTS interrupt mask
#define	USART_MIS_ABDET					BIT(5)					 // Autobaud detected interrupt mask
#define	USART_MIS_ABSTART				BIT(6)					 // Autobaud start interrupt mask
#define	USART_MIS_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_ICR(base)					MMIO32((base) + 0x70)
#define	USART_ICR_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_ICR_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_ICR_RX					BIT(2)					 // Receive interrupt mask
#define	USART_ICR_ERR					BIT(3)					 // Error interrupt mask
#define	USART_ICR_CTS					BIT(4)					 // CTS interrupt mask
#define	USART_ICR_ABDET					BIT(5)					 // Autobaud detected interrupt mask
#define	USART_ICR_ABSTART				BIT(6)					 // Autobaud start interrupt mask
#define	USART_ICR_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_ISR(base)					MMIO32((base) + 0x74)
#define	USART_ISR_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_ISR_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_ISR_RX					BIT(2)					 // Receive interrupt mask
#define	USART_ISR_ERR					BIT(3)					 // Error interrupt mask
#define	USART_ISR_CTS					BIT(4)					 // CTS interrupt mask
#define	USART_ISR_ABDET					BIT(5)					 // Autobaud detected interrupt mask
#define	USART_ISR_ABSTART				BIT(6)					 // Autobaud start interrupt mask
#define	USART_ISR_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_DMACON(base)				MMIO32((base) + 0x78)
#define	USART_DMACON_TX					BIT(0)					 // Transmit DMA Enable. If this bit is set to 1, DMA for the transmit FIFO is enabled
#define	USART_DMACON_RX					BIT(1)					 // Receive DMA Enable. If this bit is set to 1, DMA for the receive FIFO is enabled.

#define	USART_TMO(base)					MMIO32((base) + 0x7C)


// SSC [MOD_NUM=0045, MOD_REV=25, MOD_32BIT=00]
// SSC (SPI)
#define	SSC_BASE				0xF1100000
/* Clock Control Register */
#define	SSC_CLC					MMIO32(SSC_BASE + 0x00)

/* Port Input Select Register */
#define	SSC_PISEL				MMIO32(SSC_BASE + 0x04)
#define	SSC_PISEL_MRIS			BIT(0)					 // Master Mode Receive Input Select
#define	SSC_PISEL_SRIS			BIT(1)					 // Slave Mode Receive Input Select
#define	SSC_PISEL_SCIS			BIT(2)					 // Slave Mode Clock Input Select
#define	SSC_PISEL_SLSIS			GENMASK(5, 3)			 // Slave Mode Slave Select Input Selection
#define	SSC_PISEL_SLSIS_SHIFT	3
#define	SSC_PISEL_STIP			BIT(8)					 // Slave Transmit Idle State Polarity

/* Module Identifier Register */
#define	SSC_ID					MMIO32(SSC_BASE + 0x08)

/* Control Register */
#define	SSC_CON					MMIO32(SSC_BASE + 0x10)
#define	SSC_CON_HB				BIT(4)					 // Heading Bit Control
#define	SSC_CON_HB_LSB			0x0
#define	SSC_CON_HB_MSB			0x10
#define	SSC_CON_PH				BIT(5)					 // Clock Phase Control (CPHA)
#define	SSC_CON_PH_0			0x0
#define	SSC_CON_PH_1			0x20
#define	SSC_CON_PO				BIT(6)					 // Clock Polarity Control (CPOL)
#define	SSC_CON_PO_0			0x0
#define	SSC_CON_PO_1			0x40
#define	SSC_CON_LB				BIT(7)					 // Loop-Back Control
#define	SSC_CON_TE				BIT(8)					 // Transmit Error Flag
#define	SSC_CON_TEN				BIT(8)					 // Transmit Error Enable
#define	SSC_CON_RE				BIT(9)					 // Receive Error Flag
#define	SSC_CON_REN				BIT(9)					 // Receive Error Enable
#define	SSC_CON_PE				BIT(10)					 // Phase Error Flag
#define	SSC_CON_PEN				BIT(10)					 // Phase Error Enable
#define	SSC_CON_BE				BIT(11)					 // Baud Rate Error Flag
#define	SSC_CON_BEN				BIT(11)					 // Baud Rate Error Enable
#define	SSC_CON_AREN			BIT(12)					 // Automatic Reset Enable
#define	SSC_CON_BSY				BIT(12)					 // Busy Flag
#define	SSC_CON_LOCK			BIT(13)					 // Lock bit for the 8 MSB bits of the Transmist data register
#define	SSC_CON_MS				BIT(14)					 // Master Select
#define	SSC_CON_MS_SLAVE		0x0
#define	SSC_CON_MS_MASTER		0x4000
#define	SSC_CON_EN				BIT(15)					 // Enable Bit
#define	SSC_CON_BC				GENMASK(19, 16)			 // Bit Count Status
#define	SSC_CON_BC_SHIFT		16
#define	SSC_CON_BM				GENMASK(19, 16)			 // Data Width Selection
#define	SSC_CON_BM_SHIFT		16
#define	SSC_CON_BM_1			0x0
#define	SSC_CON_BM_2			0x10000
#define	SSC_CON_BM_3			0x20000
#define	SSC_CON_BM_4			0x30000
#define	SSC_CON_BM_5			0x40000
#define	SSC_CON_BM_6			0x50000
#define	SSC_CON_BM_7			0x60000
#define	SSC_CON_BM_8			0x70000
#define	SSC_CON_BM_9			0x80000
#define	SSC_CON_BM_10			0x90000
#define	SSC_CON_BM_11			0xA0000
#define	SSC_CON_BM_12			0xB0000
#define	SSC_CON_BM_13			0xC0000
#define	SSC_CON_BM_14			0xD0000
#define	SSC_CON_BM_15			0xE0000
#define	SSC_CON_BM_16			0xF0000

/* Baud Rate Timer Reload Register */
#define	SSC_BR					MMIO32(SSC_BASE + 0x14)
#define	SSC_BR_BR_VALUE			GENMASK(15, 0)			 // Baud Rate Timer/Reload Register Value
#define	SSC_BR_BR_VALUE_SHIFT	0

/* Transmit Buffer Register */
#define	SSC_TB					MMIO32(SSC_BASE + 0x20)
#define	SSC_TB_TB_VALUE			GENMASK(15, 0)			 // Transmit Data Register Value
#define	SSC_TB_TB_VALUE_SHIFT	0

/* Receive Buffer Register */
#define	SSC_RB					MMIO32(SSC_BASE + 0x24)
#define	SSC_RB_RB_VALUE			GENMASK(15, 0)			 // Receive Data Register Value
#define	SSC_RB_RB_VALUE_SHIFT	0

/* Receive FIFO Control Register */
#define	SSC_RXFCON				MMIO32(SSC_BASE + 0x30)
#define	SSC_RXFCON_RXFEN		BIT(0)					 // Receive FIFO Enable
#define	SSC_RXFCON_RXFLU		BIT(1)					 // Receive FIFO Flush
#define	SSC_RXFCON_RXTMEN		BIT(2)					 // Receive FIFO Transparent Mode Enable
#define	SSC_RXFCON_RXFITL		GENMASK(13, 8)			 // Receive FIFO Interrupt Trigger Level
#define	SSC_RXFCON_RXFITL_SHIFT	8

#define	SSC_TXFCON				MMIO32(SSC_BASE + 0x34)
#define	SSC_TXFCON_TXFEN		BIT(0)					 // Receive FIFO Enable
#define	SSC_TXFCON_TXFLU		BIT(1)					 // Receive FIFO Flush
#define	SSC_TXFCON_TXTMEN		BIT(2)					 // Receive FIFO Transparent Mode Enable
#define	SSC_TXFCON_TXFITL		GENMASK(13, 8)			 // Receive FIFO Interrupt Trigger Level
#define	SSC_TXFCON_TXFITL_SHIFT	8

#define	SSC_FSTAT				MMIO32(SSC_BASE + 0x38)
#define	SSC_FSTAT_RXFFL			GENMASK(5, 0)			 // Receive FIFO Filling Level
#define	SSC_FSTAT_RXFFL_SHIFT	0
#define	SSC_FSTAT_TXFFL			GENMASK(13, 8)			 // Transmit FIFO Filling Level
#define	SSC_FSTAT_TXFFL_SHIFT	8

#define	SSC_UNK0				MMIO32(SSC_BASE + 0x40)

#define	SSC_UNK1				MMIO32(SSC_BASE + 0x44)

#define	SSC_IMSC				MMIO32(SSC_BASE + 0x48)
#define	SSC_IMSC_TX				BIT(0)					 // Transmit interrupt mask
#define	SSC_IMSC_RX				BIT(1)					 // Receive interrupt mask
#define	SSC_IMSC_ERR			BIT(3)					 // Error interrupt mask

#define	SSC_RIS					MMIO32(SSC_BASE + 0x4C)
#define	SSC_RIS_TX				BIT(0)					 // Transmit interrupt raw status
#define	SSC_RIS_RX				BIT(1)					 // Receive interrupt raw status
#define	SSC_RIS_ERR				BIT(2)					 // Error interrupt raw status

#define	SSC_MIS					MMIO32(SSC_BASE + 0x50)
#define	SSC_MIS_TX				BIT(0)					 // Transmit interrupt status
#define	SSC_MIS_RX				BIT(1)					 // Receive interrupt status
#define	SSC_MIS_ERR				BIT(2)					 // Error interrupt status

#define	SSC_ICR					MMIO32(SSC_BASE + 0x54)
#define	SSC_ICR_TX				BIT(0)					 // Transmit interrupt mask
#define	SSC_ICR_RX				BIT(1)					 // Receive interrupt mask
#define	SSC_ICR_ERR				BIT(2)					 // Error interrupt mask

#define	SSC_ISR					MMIO32(SSC_BASE + 0x58)
#define	SSC_ISR_TX				BIT(0)					 // Transmit interrupt set
#define	SSC_ISR_RX				BIT(1)					 // Receive interrupt set
#define	SSC_ISR_ERR				BIT(2)					 // Error interrupt set

#define	SSC_DMAE				MMIO32(SSC_BASE + 0x5C)
#define	SSC_DMAE_TX				BIT(0)					 // Transmit interrupt mask
#define	SSC_DMAE_RX				BIT(1)					 // Receive interrupt mask

#define	SSC_UNK2				MMIO32(SSC_BASE + 0x60)


// SIM [MOD_NUM=F000, MOD_REV=32, MOD_32BIT=C0]
// Looks like SIM IO module, but not sure.
#define	SIM_BASE	0xF1300000
/* Clock Control Register */
#define	SIM_CLC		MMIO32(SIM_BASE + 0x00)

/* Module Identifier Register */
#define	SIM_ID		MMIO32(SIM_BASE + 0x08)


// DIF [MOD_NUM=F043, MOD_REV=00, MOD_32BIT=C0]
// DIF (Display Interface)
#define	DIF_BASE				0xF1B00000
/* Clock Control Register */
#define	DIF_CLC					MMIO32(DIF_BASE + 0x00)

/* Port Input Select Register */
#define	DIF_PISEL				MMIO32(DIF_BASE + 0x04)
#define	DIF_PISEL_MRIS			BIT(0)					 // Master Mode Receive Input Select
#define	DIF_PISEL_SRIS			BIT(1)					 // Slave Mode Receive Input Select
#define	DIF_PISEL_SCIS			BIT(2)					 // Slave Mode Clock Input Select
#define	DIF_PISEL_SLSIS			GENMASK(5, 3)			 // Slave Mode Slave Select Input Selection
#define	DIF_PISEL_SLSIS_SHIFT	3
#define	DIF_PISEL_STIP			BIT(8)					 // Slave Transmit Idle State Polarity

/* Module Identifier Register */
#define	DIF_ID					MMIO32(DIF_BASE + 0x08)

/* Control Register */
#define	DIF_CON					MMIO32(DIF_BASE + 0x10)
#define	DIF_CON_HB				BIT(4)					 // Heading Bit Control
#define	DIF_CON_HB_LSB			0x0
#define	DIF_CON_HB_MSB			0x10
#define	DIF_CON_PH				BIT(5)					 // Clock Phase Control (CPHA)
#define	DIF_CON_PH_0			0x0
#define	DIF_CON_PH_1			0x20
#define	DIF_CON_PO				BIT(6)					 // Clock Polarity Control (CPOL)
#define	DIF_CON_PO_0			0x0
#define	DIF_CON_PO_1			0x40
#define	DIF_CON_LB				BIT(7)					 // Loop-Back Control
#define	DIF_CON_TE				BIT(8)					 // Transmit Error Flag
#define	DIF_CON_TEN				BIT(8)					 // Transmit Error Enable
#define	DIF_CON_RE				BIT(9)					 // Receive Error Flag
#define	DIF_CON_REN				BIT(9)					 // Receive Error Enable
#define	DIF_CON_PE				BIT(10)					 // Phase Error Flag
#define	DIF_CON_PEN				BIT(10)					 // Phase Error Enable
#define	DIF_CON_BE				BIT(11)					 // Baud Rate Error Flag
#define	DIF_CON_BEN				BIT(11)					 // Baud Rate Error Enable
#define	DIF_CON_AREN			BIT(12)					 // Automatic Reset Enable
#define	DIF_CON_BSY				BIT(12)					 // Busy Flag
#define	DIF_CON_LOCK			BIT(13)					 // Lock bit for the 8 MSB bits of the Transmist data register
#define	DIF_CON_MS				BIT(14)					 // Master Select
#define	DIF_CON_MS_SLAVE		0x0
#define	DIF_CON_MS_MASTER		0x4000
#define	DIF_CON_EN				BIT(15)					 // Enable Bit
#define	DIF_CON_BC				GENMASK(19, 16)			 // Bit Count Status
#define	DIF_CON_BC_SHIFT		16
#define	DIF_CON_BM				GENMASK(19, 16)			 // Data Width Selection
#define	DIF_CON_BM_SHIFT		16
#define	DIF_CON_BM_1			0x0
#define	DIF_CON_BM_2			0x10000
#define	DIF_CON_BM_3			0x20000
#define	DIF_CON_BM_4			0x30000
#define	DIF_CON_BM_5			0x40000
#define	DIF_CON_BM_6			0x50000
#define	DIF_CON_BM_7			0x60000
#define	DIF_CON_BM_8			0x70000
#define	DIF_CON_BM_9			0x80000
#define	DIF_CON_BM_10			0x90000
#define	DIF_CON_BM_11			0xA0000
#define	DIF_CON_BM_12			0xB0000
#define	DIF_CON_BM_13			0xC0000
#define	DIF_CON_BM_14			0xD0000
#define	DIF_CON_BM_15			0xE0000
#define	DIF_CON_BM_16			0xF0000

/* Baud Rate Timer Reload Register */
#define	DIF_BR					MMIO32(DIF_BASE + 0x14)
#define	DIF_BR_BR_VALUE			GENMASK(15, 0)			 // Baud Rate Timer/Reload Register Value
#define	DIF_BR_BR_VALUE_SHIFT	0

/* Transmit Buffer Register */
#define	DIF_TB					MMIO32(DIF_BASE + 0x20)
#define	DIF_TB_TB_VALUE			GENMASK(15, 0)			 // Transmit Data Register Value
#define	DIF_TB_TB_VALUE_SHIFT	0

/* Receive Buffer Register */
#define	DIF_RB					MMIO32(DIF_BASE + 0x24)
#define	DIF_RB_RB_VALUE			GENMASK(15, 0)			 // Receive Data Register Value
#define	DIF_RB_RB_VALUE_SHIFT	0

/* Receive FIFO Control Register */
#define	DIF_RXFCON				MMIO32(DIF_BASE + 0x30)
#define	DIF_RXFCON_RXFEN		BIT(0)					 // Receive FIFO Enable
#define	DIF_RXFCON_RXFLU		BIT(1)					 // Receive FIFO Flush
#define	DIF_RXFCON_RXTMEN		BIT(2)					 // Receive FIFO Transparent Mode Enable
#define	DIF_RXFCON_RXFITL		GENMASK(13, 8)			 // Receive FIFO Interrupt Trigger Level
#define	DIF_RXFCON_RXFITL_SHIFT	8

#define	DIF_TXFCON				MMIO32(DIF_BASE + 0x34)
#define	DIF_TXFCON_TXFEN		BIT(0)					 // Receive FIFO Enable
#define	DIF_TXFCON_TXFLU		BIT(1)					 // Receive FIFO Flush
#define	DIF_TXFCON_TXTMEN		BIT(2)					 // Receive FIFO Transparent Mode Enable
#define	DIF_TXFCON_TXFITL		GENMASK(13, 8)			 // Receive FIFO Interrupt Trigger Level
#define	DIF_TXFCON_TXFITL_SHIFT	8

#define	DIF_FSTAT				MMIO32(DIF_BASE + 0x38)
#define	DIF_FSTAT_RXFFL			GENMASK(5, 0)			 // Receive FIFO Filling Level
#define	DIF_FSTAT_RXFFL_SHIFT	0
#define	DIF_FSTAT_TXFFL			GENMASK(13, 8)			 // Transmit FIFO Filling Level
#define	DIF_FSTAT_TXFFL_SHIFT	8

#define	DIF_UNK0				MMIO32(DIF_BASE + 0x40)

#define	DIF_UNK1				MMIO32(DIF_BASE + 0x44)

#define	DIF_IMSC				MMIO32(DIF_BASE + 0x48)
#define	DIF_IMSC_TX				BIT(0)					 // Transmit interrupt mask
#define	DIF_IMSC_RX				BIT(1)					 // Receive interrupt mask
#define	DIF_IMSC_ERR			BIT(3)					 // Error interrupt mask

#define	DIF_RIS					MMIO32(DIF_BASE + 0x4C)
#define	DIF_RIS_TX				BIT(0)					 // Transmit interrupt raw status
#define	DIF_RIS_RX				BIT(1)					 // Receive interrupt raw status
#define	DIF_RIS_ERR				BIT(2)					 // Error interrupt raw status

#define	DIF_MIS					MMIO32(DIF_BASE + 0x50)
#define	DIF_MIS_TX				BIT(0)					 // Transmit interrupt status
#define	DIF_MIS_RX				BIT(1)					 // Receive interrupt status
#define	DIF_MIS_ERR				BIT(2)					 // Error interrupt status

#define	DIF_ICR					MMIO32(DIF_BASE + 0x54)
#define	DIF_ICR_TX				BIT(0)					 // Transmit interrupt mask
#define	DIF_ICR_RX				BIT(1)					 // Receive interrupt mask
#define	DIF_ICR_ERR				BIT(2)					 // Error interrupt mask

#define	DIF_ISR					MMIO32(DIF_BASE + 0x58)
#define	DIF_ISR_TX				BIT(0)					 // Transmit interrupt set
#define	DIF_ISR_RX				BIT(1)					 // Receive interrupt set
#define	DIF_ISR_ERR				BIT(2)					 // Error interrupt set

#define	DIF_DMAE				MMIO32(DIF_BASE + 0x5C)
#define	DIF_DMAE_TX				BIT(0)					 // Transmit interrupt mask
#define	DIF_DMAE_RX				BIT(1)					 // Receive interrupt mask

#define	DIF_UNK2				MMIO32(DIF_BASE + 0x60)

/* Pixel-Bit Conversion Register */
#define	DIF_PBCCON				MMIO32(DIF_BASE + 0x70)
#define	DIF_PBCCON_PBBCONV_MODE	BIT(0)

/* Bit Multiplex Configuration Register 0 */
#define	DIF_BMREG0				MMIO32(DIF_BASE + 0x74)
#define	DIF_BMREG0_MUX0			GENMASK(4, 0)
#define	DIF_BMREG0_MUX0_SHIFT	0
#define	DIF_BMREG0_MUX1			GENMASK(9, 5)
#define	DIF_BMREG0_MUX1_SHIFT	5
#define	DIF_BMREG0_MUX2			GENMASK(14, 10)
#define	DIF_BMREG0_MUX2_SHIFT	10
#define	DIF_BMREG0_MUX3			GENMASK(20, 16)
#define	DIF_BMREG0_MUX3_SHIFT	16
#define	DIF_BMREG0_MUX4			GENMASK(25, 21)
#define	DIF_BMREG0_MUX4_SHIFT	21
#define	DIF_BMREG0_MUX5			GENMASK(30, 26)
#define	DIF_BMREG0_MUX5_SHIFT	26

/* Bit Multiplex Configuration Register 1 */
#define	DIF_BMREG1				MMIO32(DIF_BASE + 0x78)
#define	DIF_BMREG1_MUX6			GENMASK(4, 0)
#define	DIF_BMREG1_MUX6_SHIFT	0
#define	DIF_BMREG1_MUX7			GENMASK(9, 5)
#define	DIF_BMREG1_MUX7_SHIFT	5
#define	DIF_BMREG1_MUX8			GENMASK(14, 10)
#define	DIF_BMREG1_MUX8_SHIFT	10
#define	DIF_BMREG1_MUX9			GENMASK(20, 16)
#define	DIF_BMREG1_MUX9_SHIFT	16
#define	DIF_BMREG1_MUX10		GENMASK(25, 21)
#define	DIF_BMREG1_MUX10_SHIFT	21
#define	DIF_BMREG1_MUX11		GENMASK(30, 26)
#define	DIF_BMREG1_MUX11_SHIFT	26

/* Bit Multiplex Configuration Register 2 */
#define	DIF_BMREG2				MMIO32(DIF_BASE + 0x7C)
#define	DIF_BMREG2_MUX12		GENMASK(4, 0)
#define	DIF_BMREG2_MUX12_SHIFT	0
#define	DIF_BMREG2_MUX13		GENMASK(9, 5)
#define	DIF_BMREG2_MUX13_SHIFT	5
#define	DIF_BMREG2_MUX14		GENMASK(14, 10)
#define	DIF_BMREG2_MUX14_SHIFT	10
#define	DIF_BMREG2_MUX15		GENMASK(20, 16)
#define	DIF_BMREG2_MUX15_SHIFT	16
#define	DIF_BMREG2_MUX16		GENMASK(25, 21)
#define	DIF_BMREG2_MUX16_SHIFT	21
#define	DIF_BMREG2_MUX17		GENMASK(30, 26)
#define	DIF_BMREG2_MUX17_SHIFT	26

/* Bit Multiplex Configuration Register 3 */
#define	DIF_BMREG3				MMIO32(DIF_BASE + 0x80)
#define	DIF_BMREG3_MUX18		GENMASK(4, 0)
#define	DIF_BMREG3_MUX18_SHIFT	0
#define	DIF_BMREG3_MUX19		GENMASK(9, 5)
#define	DIF_BMREG3_MUX19_SHIFT	5
#define	DIF_BMREG3_MUX20		GENMASK(14, 10)
#define	DIF_BMREG3_MUX20_SHIFT	10
#define	DIF_BMREG3_MUX21		GENMASK(20, 16)
#define	DIF_BMREG3_MUX21_SHIFT	16
#define	DIF_BMREG3_MUX22		GENMASK(25, 21)
#define	DIF_BMREG3_MUX22_SHIFT	21
#define	DIF_BMREG3_MUX23		GENMASK(30, 26)
#define	DIF_BMREG3_MUX23_SHIFT	26

/* Bit Multiplex Configuration Register 4 */
#define	DIF_BMREG4				MMIO32(DIF_BASE + 0x84)
#define	DIF_BMREG4_MUX24		GENMASK(4, 0)
#define	DIF_BMREG4_MUX24_SHIFT	0
#define	DIF_BMREG4_MUX25		GENMASK(9, 5)
#define	DIF_BMREG4_MUX25_SHIFT	5
#define	DIF_BMREG4_MUX26		GENMASK(14, 10)
#define	DIF_BMREG4_MUX26_SHIFT	10
#define	DIF_BMREG4_MUX27		GENMASK(20, 16)
#define	DIF_BMREG4_MUX27_SHIFT	16
#define	DIF_BMREG4_MUX28		GENMASK(25, 21)
#define	DIF_BMREG4_MUX28_SHIFT	21
#define	DIF_BMREG4_MUX29		GENMASK(30, 26)
#define	DIF_BMREG4_MUX29_SHIFT	26

/* Bit Multiplex Configuration Register 5 */
#define	DIF_BMREG5				MMIO32(DIF_BASE + 0x88)
#define	DIF_BMREG5_MUX30		GENMASK(4, 0)
#define	DIF_BMREG5_MUX30_SHIFT	0
#define	DIF_BMREG5_MUX31		GENMASK(9, 5)
#define	DIF_BMREG5_MUX31_SHIFT	5

/* Bit Clamp Value Register */
#define	DIF_BCREG				MMIO32(DIF_BASE + 0x8C)
#define	DIF_BCREG_B0			BIT(0)
#define	DIF_BCREG_B16			BIT(0)
#define	DIF_BCREG_B1			BIT(2)
#define	DIF_BCREG_B17			BIT(2)
#define	DIF_BCREG_B18			BIT(4)
#define	DIF_BCREG_B2			BIT(4)
#define	DIF_BCREG_B19			BIT(6)
#define	DIF_BCREG_B3			BIT(6)
#define	DIF_BCREG_B20			BIT(8)
#define	DIF_BCREG_B4			BIT(8)
#define	DIF_BCREG_B21			BIT(10)
#define	DIF_BCREG_B5			BIT(10)
#define	DIF_BCREG_B22			BIT(12)
#define	DIF_BCREG_B6			BIT(12)
#define	DIF_BCREG_B23			BIT(14)
#define	DIF_BCREG_B7			BIT(14)
#define	DIF_BCREG_B24			BIT(16)
#define	DIF_BCREG_B8			BIT(16)
#define	DIF_BCREG_B25			BIT(18)
#define	DIF_BCREG_B9			BIT(18)
#define	DIF_BCREG_B10			BIT(20)
#define	DIF_BCREG_B26			BIT(20)
#define	DIF_BCREG_B11			BIT(22)
#define	DIF_BCREG_B27			BIT(22)
#define	DIF_BCREG_B12			BIT(24)
#define	DIF_BCREG_B28			BIT(24)
#define	DIF_BCREG_B13			BIT(26)
#define	DIF_BCREG_B29			BIT(26)
#define	DIF_BCREG_B14			BIT(28)
#define	DIF_BCREG_B30			BIT(28)
#define	DIF_BCREG_B15			BIT(30)
#define	DIF_BCREG_B31			BIT(30)

/* Bit Control Register 0 */
#define	DIF_BCSEL0				MMIO32(DIF_BASE + 0x90)
#define	DIF_BCSEL0_B0			GENMASK(1, 0)
#define	DIF_BCSEL0_B0_SHIFT		0
#define	DIF_BCSEL0_B1			GENMASK(3, 2)
#define	DIF_BCSEL0_B1_SHIFT		2
#define	DIF_BCSEL0_B2			GENMASK(5, 4)
#define	DIF_BCSEL0_B2_SHIFT		4
#define	DIF_BCSEL0_B3			GENMASK(7, 6)
#define	DIF_BCSEL0_B3_SHIFT		6
#define	DIF_BCSEL0_B4			GENMASK(9, 8)
#define	DIF_BCSEL0_B4_SHIFT		8
#define	DIF_BCSEL0_B5			GENMASK(11, 10)
#define	DIF_BCSEL0_B5_SHIFT		10
#define	DIF_BCSEL0_B6			GENMASK(13, 12)
#define	DIF_BCSEL0_B6_SHIFT		12
#define	DIF_BCSEL0_B7			GENMASK(15, 14)
#define	DIF_BCSEL0_B7_SHIFT		14
#define	DIF_BCSEL0_B8			GENMASK(17, 16)
#define	DIF_BCSEL0_B8_SHIFT		16
#define	DIF_BCSEL0_B9			GENMASK(19, 18)
#define	DIF_BCSEL0_B9_SHIFT		18
#define	DIF_BCSEL0_B10			GENMASK(21, 20)
#define	DIF_BCSEL0_B10_SHIFT	20
#define	DIF_BCSEL0_B11			GENMASK(23, 22)
#define	DIF_BCSEL0_B11_SHIFT	22
#define	DIF_BCSEL0_B12			GENMASK(25, 24)
#define	DIF_BCSEL0_B12_SHIFT	24
#define	DIF_BCSEL0_B13			GENMASK(27, 26)
#define	DIF_BCSEL0_B13_SHIFT	26
#define	DIF_BCSEL0_B14			GENMASK(29, 28)
#define	DIF_BCSEL0_B14_SHIFT	28
#define	DIF_BCSEL0_B15			GENMASK(31, 30)
#define	DIF_BCSEL0_B15_SHIFT	30

/* Bit Control Register 1 */
#define	DIF_BCSEL1				MMIO32(DIF_BASE + 0x94)
#define	DIF_BCSEL1_B16			GENMASK(1, 0)
#define	DIF_BCSEL1_B16_SHIFT	0
#define	DIF_BCSEL1_B17			GENMASK(3, 2)
#define	DIF_BCSEL1_B17_SHIFT	2
#define	DIF_BCSEL1_B18			GENMASK(5, 4)
#define	DIF_BCSEL1_B18_SHIFT	4
#define	DIF_BCSEL1_B19			GENMASK(7, 6)
#define	DIF_BCSEL1_B19_SHIFT	6
#define	DIF_BCSEL1_B20			GENMASK(9, 8)
#define	DIF_BCSEL1_B20_SHIFT	8
#define	DIF_BCSEL1_B21			GENMASK(11, 10)
#define	DIF_BCSEL1_B21_SHIFT	10
#define	DIF_BCSEL1_B22			GENMASK(13, 12)
#define	DIF_BCSEL1_B22_SHIFT	12
#define	DIF_BCSEL1_B23			GENMASK(15, 14)
#define	DIF_BCSEL1_B23_SHIFT	14
#define	DIF_BCSEL1_B24			GENMASK(17, 16)
#define	DIF_BCSEL1_B24_SHIFT	16
#define	DIF_BCSEL1_B25			GENMASK(19, 18)
#define	DIF_BCSEL1_B25_SHIFT	18
#define	DIF_BCSEL1_B26			GENMASK(21, 20)
#define	DIF_BCSEL1_B26_SHIFT	20
#define	DIF_BCSEL1_B27			GENMASK(23, 22)
#define	DIF_BCSEL1_B27_SHIFT	22
#define	DIF_BCSEL1_B28			GENMASK(25, 24)
#define	DIF_BCSEL1_B28_SHIFT	24
#define	DIF_BCSEL1_B29			GENMASK(27, 26)
#define	DIF_BCSEL1_B29_SHIFT	26
#define	DIF_BCSEL1_B30			GENMASK(29, 28)
#define	DIF_BCSEL1_B30_SHIFT	28
#define	DIF_BCSEL1_B31			GENMASK(31, 30)
#define	DIF_BCSEL1_B31_SHIFT	30

#define	DIF_UNK3				MMIO32(DIF_BASE + 0x98)

#define	DIF_UNK4				MMIO32(DIF_BASE + 0x9C)


// USB [MOD_NUM=F047, MOD_REV=00, MOD_32BIT=C0]
// Looks like USB module, but not sure.
#define	USB_BASE	0xF2200800
/* Clock Control Register */
#define	USB_CLC		MMIO32(USB_BASE + 0x00)

/* Module Identifier Register */
#define	USB_ID		MMIO32(USB_BASE + 0x08)


// VIC [MOD_NUM=0031, MOD_REV=01, MOD_32BIT=C0]
// Vectored Interrupt Controller, registers collected using tests on real hardware (using "black box" method).
#define	VIC_BASE						0xF2800000
/* Module Identifier Register */
#define	VIC_ID							MMIO32(VIC_BASE + 0x00)

#define	VIC_FIQ_CON						MMIO32(VIC_BASE + 0x08)
#define	VIC_FIQ_CON_NUM					GENMASK(7, 0)							 // Pending fiq num
#define	VIC_FIQ_CON_NUM_SHIFT			0
#define	VIC_FIQ_CON_PRIORITY			GENMASK(19, 16)							 // Pending fiq priority
#define	VIC_FIQ_CON_PRIORITY_SHIFT		16
#define	VIC_FIQ_CON_MASK_PRIORITY		GENMASK(27, 24)							 // Mask fiq's' with priority <= MASK_PRIORITY
#define	VIC_FIQ_CON_MASK_PRIORITY_SHIFT	24

#define	VIC_IRQ_CON						MMIO32(VIC_BASE + 0x0C)
#define	VIC_IRQ_CON_NUM					GENMASK(7, 0)							 // Pending irq num
#define	VIC_IRQ_CON_NUM_SHIFT			0
#define	VIC_IRQ_CON_PRIORITY			GENMASK(19, 16)							 // Pending irq priority
#define	VIC_IRQ_CON_PRIORITY_SHIFT		16
#define	VIC_IRQ_CON_MASK_PRIORITY		GENMASK(27, 24)							 // Mask irq's' with priority <= MASK_PRIORITY
#define	VIC_IRQ_CON_MASK_PRIORITY_SHIFT	24

/* End of FIQ processing */
#define	VIC_FIQ_ACK						MMIO32(VIC_BASE + 0x10)

/* End of IRQ processing */
#define	VIC_IRQ_ACK						MMIO32(VIC_BASE + 0x14)

/* Start of FIQ processing */
#define	VIC_FIQ_CURRENT					MMIO32(VIC_BASE + 0x18)
#define	VIC_FIQ_CURRENT_NUM				GENMASK(7, 0)							 // fiq num
#define	VIC_FIQ_CURRENT_NUM_SHIFT		0

/* Start of IRQ processing */
#define	VIC_IRQ_CURRENT					MMIO32(VIC_BASE + 0x1C)
#define	VIC_IRQ_CURRENT_NUM				GENMASK(7, 0)							 // irq num
#define	VIC_IRQ_CURRENT_NUM_SHIFT		0

#define	VIC_CON(n)						MMIO32(VIC_BASE + 0x30 + ((n) * 0x4))
#define	VIC_CON_PRIORITY				GENMASK(3, 0)
#define	VIC_CON_PRIORITY_SHIFT			0
#define	VIC_CON_FIQ						BIT(8)


// DMAC [AMBA PL080]
// PrimeCell DMA Controller (PL080)
#define	DMAC_BASE								0xF3000000
/* Status of the DMA interrupts after masking */
#define	DMAC_INT_STATUS							MMIO32(DMAC_BASE + 0x00)
#define	DMAC_INT_STATUS_CH0						BIT(0)
#define	DMAC_INT_STATUS_CH1						BIT(1)
#define	DMAC_INT_STATUS_CH2						BIT(2)
#define	DMAC_INT_STATUS_CH3						BIT(3)
#define	DMAC_INT_STATUS_CH4						BIT(4)
#define	DMAC_INT_STATUS_CH5						BIT(5)
#define	DMAC_INT_STATUS_CH6						BIT(6)
#define	DMAC_INT_STATUS_CH7						BIT(7)

/* Interrupt terminal count request status */
#define	DMAC_TC_STATUS							MMIO32(DMAC_BASE + 0x04)
#define	DMAC_TC_STATUS_CH0						BIT(0)
#define	DMAC_TC_STATUS_CH1						BIT(1)
#define	DMAC_TC_STATUS_CH2						BIT(2)
#define	DMAC_TC_STATUS_CH3						BIT(3)
#define	DMAC_TC_STATUS_CH4						BIT(4)
#define	DMAC_TC_STATUS_CH5						BIT(5)
#define	DMAC_TC_STATUS_CH6						BIT(6)
#define	DMAC_TC_STATUS_CH7						BIT(7)

/* Terminal count request clear. */
#define	DMAC_TC_CLEAR							MMIO32(DMAC_BASE + 0x08)
#define	DMAC_TC_CLEAR_CH0						BIT(0)
#define	DMAC_TC_CLEAR_CH1						BIT(1)
#define	DMAC_TC_CLEAR_CH2						BIT(2)
#define	DMAC_TC_CLEAR_CH3						BIT(3)
#define	DMAC_TC_CLEAR_CH4						BIT(4)
#define	DMAC_TC_CLEAR_CH5						BIT(5)
#define	DMAC_TC_CLEAR_CH6						BIT(6)
#define	DMAC_TC_CLEAR_CH7						BIT(7)

/* Interrupt error status */
#define	DMAC_ERR_STATUS							MMIO32(DMAC_BASE + 0x0C)
#define	DMAC_ERR_STATUS_CH0						BIT(0)
#define	DMAC_ERR_STATUS_CH1						BIT(1)
#define	DMAC_ERR_STATUS_CH2						BIT(2)
#define	DMAC_ERR_STATUS_CH3						BIT(3)
#define	DMAC_ERR_STATUS_CH4						BIT(4)
#define	DMAC_ERR_STATUS_CH5						BIT(5)
#define	DMAC_ERR_STATUS_CH6						BIT(6)
#define	DMAC_ERR_STATUS_CH7						BIT(7)

/* Interrupt error clear. */
#define	DMAC_ERR_CLEAR							MMIO32(DMAC_BASE + 0x10)
#define	DMAC_ERR_CLEAR_CH0						BIT(0)
#define	DMAC_ERR_CLEAR_CH1						BIT(1)
#define	DMAC_ERR_CLEAR_CH2						BIT(2)
#define	DMAC_ERR_CLEAR_CH3						BIT(3)
#define	DMAC_ERR_CLEAR_CH4						BIT(4)
#define	DMAC_ERR_CLEAR_CH5						BIT(5)
#define	DMAC_ERR_CLEAR_CH6						BIT(6)
#define	DMAC_ERR_CLEAR_CH7						BIT(7)

/* Status of the terminal count interrupt prior to masking */
#define	DMAC_RAW_TC_STATUS						MMIO32(DMAC_BASE + 0x14)
#define	DMAC_RAW_TC_STATUS_CH0					BIT(0)
#define	DMAC_RAW_TC_STATUS_CH1					BIT(1)
#define	DMAC_RAW_TC_STATUS_CH2					BIT(2)
#define	DMAC_RAW_TC_STATUS_CH3					BIT(3)
#define	DMAC_RAW_TC_STATUS_CH4					BIT(4)
#define	DMAC_RAW_TC_STATUS_CH5					BIT(5)
#define	DMAC_RAW_TC_STATUS_CH6					BIT(6)
#define	DMAC_RAW_TC_STATUS_CH7					BIT(7)

/* Status of the error interrupt prior to masking */
#define	DMAC_RAW_ERR_STATUS						MMIO32(DMAC_BASE + 0x18)
#define	DMAC_RAW_ERR_STATUS_CH0					BIT(0)
#define	DMAC_RAW_ERR_STATUS_CH1					BIT(1)
#define	DMAC_RAW_ERR_STATUS_CH2					BIT(2)
#define	DMAC_RAW_ERR_STATUS_CH3					BIT(3)
#define	DMAC_RAW_ERR_STATUS_CH4					BIT(4)
#define	DMAC_RAW_ERR_STATUS_CH5					BIT(5)
#define	DMAC_RAW_ERR_STATUS_CH6					BIT(6)
#define	DMAC_RAW_ERR_STATUS_CH7					BIT(7)

/* Channel enable status */
#define	DMAC_EN_CHAN							MMIO32(DMAC_BASE + 0x1C)
#define	DMAC_EN_CHAN_CH0						BIT(0)
#define	DMAC_EN_CHAN_CH1						BIT(1)
#define	DMAC_EN_CHAN_CH2						BIT(2)
#define	DMAC_EN_CHAN_CH3						BIT(3)
#define	DMAC_EN_CHAN_CH4						BIT(4)
#define	DMAC_EN_CHAN_CH5						BIT(5)
#define	DMAC_EN_CHAN_CH6						BIT(6)
#define	DMAC_EN_CHAN_CH7						BIT(7)

/* Software burst request. */
#define	DMAC_SOFT_BREQ							MMIO32(DMAC_BASE + 0x20)
#define	DMAC_SOFT_BREQ_CH0_0					BIT(0)
#define	DMAC_SOFT_BREQ_CH0_1					BIT(1)
#define	DMAC_SOFT_BREQ_CH1_0					BIT(2)
#define	DMAC_SOFT_BREQ_CH1_1					BIT(3)
#define	DMAC_SOFT_BREQ_CH2_0					BIT(4)
#define	DMAC_SOFT_BREQ_CH2_1					BIT(5)
#define	DMAC_SOFT_BREQ_CH3_0					BIT(6)
#define	DMAC_SOFT_BREQ_CH3_1					BIT(7)
#define	DMAC_SOFT_BREQ_CH4_0					BIT(8)
#define	DMAC_SOFT_BREQ_CH4_1					BIT(9)
#define	DMAC_SOFT_BREQ_CH5_0					BIT(10)
#define	DMAC_SOFT_BREQ_CH5_1					BIT(11)
#define	DMAC_SOFT_BREQ_CH6_0					BIT(12)
#define	DMAC_SOFT_BREQ_CH6_1					BIT(13)
#define	DMAC_SOFT_BREQ_CH7_0					BIT(14)
#define	DMAC_SOFT_BREQ_CH7_1					BIT(15)

/* Software single request. */
#define	DMAC_SOFT_SREQ							MMIO32(DMAC_BASE + 0x24)
#define	DMAC_SOFT_SREQ_CH0_0					BIT(0)
#define	DMAC_SOFT_SREQ_CH0_1					BIT(1)
#define	DMAC_SOFT_SREQ_CH1_0					BIT(2)
#define	DMAC_SOFT_SREQ_CH1_1					BIT(3)
#define	DMAC_SOFT_SREQ_CH2_0					BIT(4)
#define	DMAC_SOFT_SREQ_CH2_1					BIT(5)
#define	DMAC_SOFT_SREQ_CH3_0					BIT(6)
#define	DMAC_SOFT_SREQ_CH3_1					BIT(7)
#define	DMAC_SOFT_SREQ_CH4_0					BIT(8)
#define	DMAC_SOFT_SREQ_CH4_1					BIT(9)
#define	DMAC_SOFT_SREQ_CH5_0					BIT(10)
#define	DMAC_SOFT_SREQ_CH5_1					BIT(11)
#define	DMAC_SOFT_SREQ_CH6_0					BIT(12)
#define	DMAC_SOFT_SREQ_CH6_1					BIT(13)
#define	DMAC_SOFT_SREQ_CH7_0					BIT(14)
#define	DMAC_SOFT_SREQ_CH7_1					BIT(15)

/* Software last burst request. */
#define	DMAC_SOFT_LBREQ							MMIO32(DMAC_BASE + 0x28)
#define	DMAC_SOFT_LBREQ_CH0_0					BIT(0)
#define	DMAC_SOFT_LBREQ_CH0_1					BIT(1)
#define	DMAC_SOFT_LBREQ_CH1_0					BIT(2)
#define	DMAC_SOFT_LBREQ_CH1_1					BIT(3)
#define	DMAC_SOFT_LBREQ_CH2_0					BIT(4)
#define	DMAC_SOFT_LBREQ_CH2_1					BIT(5)
#define	DMAC_SOFT_LBREQ_CH3_0					BIT(6)
#define	DMAC_SOFT_LBREQ_CH3_1					BIT(7)
#define	DMAC_SOFT_LBREQ_CH4_0					BIT(8)
#define	DMAC_SOFT_LBREQ_CH4_1					BIT(9)
#define	DMAC_SOFT_LBREQ_CH5_0					BIT(10)
#define	DMAC_SOFT_LBREQ_CH5_1					BIT(11)
#define	DMAC_SOFT_LBREQ_CH6_0					BIT(12)
#define	DMAC_SOFT_LBREQ_CH6_1					BIT(13)
#define	DMAC_SOFT_LBREQ_CH7_0					BIT(14)
#define	DMAC_SOFT_LBREQ_CH7_1					BIT(15)

/* Software last single request. */
#define	DMAC_SOFT_LSREQ							MMIO32(DMAC_BASE + 0x2C)
#define	DMAC_SOFT_LSREQ_CH0_0					BIT(0)
#define	DMAC_SOFT_LSREQ_CH0_1					BIT(1)
#define	DMAC_SOFT_LSREQ_CH1_0					BIT(2)
#define	DMAC_SOFT_LSREQ_CH1_1					BIT(3)
#define	DMAC_SOFT_LSREQ_CH2_0					BIT(4)
#define	DMAC_SOFT_LSREQ_CH2_1					BIT(5)
#define	DMAC_SOFT_LSREQ_CH3_0					BIT(6)
#define	DMAC_SOFT_LSREQ_CH3_1					BIT(7)
#define	DMAC_SOFT_LSREQ_CH4_0					BIT(8)
#define	DMAC_SOFT_LSREQ_CH4_1					BIT(9)
#define	DMAC_SOFT_LSREQ_CH5_0					BIT(10)
#define	DMAC_SOFT_LSREQ_CH5_1					BIT(11)
#define	DMAC_SOFT_LSREQ_CH6_0					BIT(12)
#define	DMAC_SOFT_LSREQ_CH6_1					BIT(13)
#define	DMAC_SOFT_LSREQ_CH7_0					BIT(14)
#define	DMAC_SOFT_LSREQ_CH7_1					BIT(15)

/* Configuration Register */
#define	DMAC_CONFIG								MMIO32(DMAC_BASE + 0x30)
#define	DMAC_CONFIG_ENABLE						BIT(0)										 // DMAC Enable
#define	DMAC_CONFIG_M1							BIT(1)										 // AHB Master 1 endianness configuration
#define	DMAC_CONFIG_M1_LE						0x0
#define	DMAC_CONFIG_M1_BE						0x2
#define	DMAC_CONFIG_M2							BIT(2)										 // AHB Master 2 endianness configuration
#define	DMAC_CONFIG_M2_LE						0x0
#define	DMAC_CONFIG_M2_BE						0x4

/* Synchronization Register */
#define	DMAC_SYNC								MMIO32(DMAC_BASE + 0x34)
#define	DMAC_SYNC_CH0_0							BIT(0)
#define	DMAC_SYNC_CH0_1							BIT(1)
#define	DMAC_SYNC_CH1_0							BIT(2)
#define	DMAC_SYNC_CH1_1							BIT(3)
#define	DMAC_SYNC_CH2_0							BIT(4)
#define	DMAC_SYNC_CH2_1							BIT(5)
#define	DMAC_SYNC_CH3_0							BIT(6)
#define	DMAC_SYNC_CH3_1							BIT(7)
#define	DMAC_SYNC_CH4_0							BIT(8)
#define	DMAC_SYNC_CH4_1							BIT(9)
#define	DMAC_SYNC_CH5_0							BIT(10)
#define	DMAC_SYNC_CH5_1							BIT(11)
#define	DMAC_SYNC_CH6_0							BIT(12)
#define	DMAC_SYNC_CH6_1							BIT(13)
#define	DMAC_SYNC_CH7_0							BIT(14)
#define	DMAC_SYNC_CH7_1							BIT(15)

#define	DMAC_CH_SRC_ADDR(n)						MMIO32(DMAC_BASE + 0x100 + ((n) * 0x20))

#define	DMAC_CH_DST_ADDR(n)						MMIO32(DMAC_BASE + 0x104 + ((n) * 0x20))

#define	DMAC_CH_LLI(n)							MMIO32(DMAC_BASE + 0x108 + ((n) * 0x20))
#define	DMAC_CH_LLI_LM							BIT(0)										 // AHB master select for loading the next LLI
#define	DMAC_CH_LLI_LM_AHB1						0x0
#define	DMAC_CH_LLI_LM_AHB2						0x1
#define	DMAC_CH_LLI_ITEM						GENMASK(30, 2)								 // Linked list item
#define	DMAC_CH_LLI_ITEM_SHIFT					2

#define	DMAC_CH_CONTROL(n)						MMIO32(DMAC_BASE + 0x10C + ((n) * 0x20))
#define	DMAC_CH_CONTROL_TRANSFER_SIZE			GENMASK(11, 0)								 // Transfer size.
#define	DMAC_CH_CONTROL_TRANSFER_SIZE_SHIFT		0
#define	DMAC_CH_CONTROL_SB_SIZE					GENMASK(14, 12)								 // Source burst size
#define	DMAC_CH_CONTROL_SB_SIZE_SHIFT			12
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_1			0x0
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_4			0x1000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_8			0x2000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_16			0x3000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_32			0x4000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_64			0x5000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_128			0x6000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_256			0x7000
#define	DMAC_CH_CONTROL_DB_SIZE					GENMASK(17, 15)								 // Destination burst size
#define	DMAC_CH_CONTROL_DB_SIZE_SHIFT			15
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_1			0x0
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_4			0x8000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_8			0x10000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_16			0x18000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_32			0x20000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_64			0x28000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_128			0x30000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_256			0x38000
#define	DMAC_CH_CONTROL_S_WIDTH					GENMASK(20, 18)								 // Source transfer width
#define	DMAC_CH_CONTROL_S_WIDTH_SHIFT			18
#define	DMAC_CH_CONTROL_S_WIDTH_BYTE			0x0
#define	DMAC_CH_CONTROL_S_WIDTH_WORD			0x40000
#define	DMAC_CH_CONTROL_S_WIDTH_DWORD			0x80000
#define	DMAC_CH_CONTROL_D_WIDTH					GENMASK(23, 21)								 // Destination transfer width
#define	DMAC_CH_CONTROL_D_WIDTH_SHIFT			21
#define	DMAC_CH_CONTROL_D_WIDTH_BYTE			0x0
#define	DMAC_CH_CONTROL_D_WIDTH_WORD			0x200000
#define	DMAC_CH_CONTROL_D_WIDTH_DWORD			0x400000
#define	DMAC_CH_CONTROL_S						BIT(24)										 // Source AHB master select
#define	DMAC_CH_CONTROL_S_AHB1					0x0
#define	DMAC_CH_CONTROL_S_AHB2					0x1000000
#define	DMAC_CH_CONTROL_D						BIT(25)										 // Destination AHB master select
#define	DMAC_CH_CONTROL_D_AHB1					0x0
#define	DMAC_CH_CONTROL_D_AHB2					0x2000000
#define	DMAC_CH_CONTROL_SI						BIT(26)										 // Source increment.
#define	DMAC_CH_CONTROL_DI						BIT(27)										 // Destination increment.
#define	DMAC_CH_CONTROL_PROTECTION				GENMASK(30, 28)								 // Protection.
#define	DMAC_CH_CONTROL_PROTECTION_SHIFT		28
#define	DMAC_CH_CONTROL_I						BIT(31)										 // Terminal count interrupt enable bit.

#define	DMAC_CH_CONFIG(n)						MMIO32(DMAC_BASE + 0x110 + ((n) * 0x20))
#define	DMAC_CH_CONFIG_ENABLE					BIT(0)										 // Channel enable.
#define	DMAC_CH_CONFIG_SRC_PERIPH				GENMASK(4, 1)								 // Source peripheral.
#define	DMAC_CH_CONFIG_SRC_PERIPH_SHIFT			1
#define	DMAC_CH_CONFIG_DST_PERIPH				GENMASK(9, 6)								 // Destination peripheral.
#define	DMAC_CH_CONFIG_DST_PERIPH_SHIFT			6
#define	DMAC_CH_CONFIG_FLOW_CTRL				GENMASK(13, 11)								 // Flow control and transfer type
#define	DMAC_CH_CONFIG_FLOW_CTRL_SHIFT			11
#define	DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM		0x0
#define	DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER		0x800
#define	DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM		0x1000
#define	DMAC_CH_CONFIG_FLOW_CTRL_PER2PER		0x1800
#define	DMAC_CH_CONFIG_FLOW_CTRL_PER2PER_DST	0x2000
#define	DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER	0x2800
#define	DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER	0x3000
#define	DMAC_CH_CONFIG_FLOW_CTRL_PER2PER_SRC	0x3800
#define	DMAC_CH_CONFIG_INT_MASK_ERR				BIT(14)										 // Interrupt error mask.
#define	DMAC_CH_CONFIG_INT_MASK_TC				BIT(15)										 // Terminal count interrupt mask.
#define	DMAC_CH_CONFIG_LOCK						BIT(16)										 // Lock.
#define	DMAC_CH_CONFIG_ACTIVE					BIT(17)										 // Active.
#define	DMAC_CH_CONFIG_HALT						BIT(18)										 // Halt.

#define	DMAC_PERIPH_ID0							MMIO32(DMAC_BASE + 0xFE0)

#define	DMAC_PERIPH_ID1							MMIO32(DMAC_BASE + 0xFE4)

#define	DMAC_PERIPH_ID2							MMIO32(DMAC_BASE + 0xFE8)

#define	DMAC_PERIPH_ID3							MMIO32(DMAC_BASE + 0xFEC)

#define	DMAC_PCELL_ID0							MMIO32(DMAC_BASE + 0xFF0)

#define	DMAC_PCELL_ID1							MMIO32(DMAC_BASE + 0xFF4)

#define	DMAC_PCELL_ID2							MMIO32(DMAC_BASE + 0xFF8)

#define	DMAC_PCELL_ID3							MMIO32(DMAC_BASE + 0xFFC)


// CAPCOM0 [MOD_NUM=0050, MOD_REV=03, MOD_32BIT=00]
// CAPCOM from drivers/clocksource/xgold_capcom_timer.c (GPL2)
#define	CAPCOM0_BASE							0xF4000000
#define	CAPCOM0									0xF4000000

#define	CAPCOM1_BASE							0xF4100000
#define	CAPCOM1									0xF4100000

/* Clock Control Register */
#define	CAPCOM_CLC(base)						MMIO32((base) + 0x00)

#define	CAPCOM_PISEL(base)						MMIO32((base) + 0x04)
#define	CAPCOM_PISEL_C1C0IS						BIT(0)
#define	CAPCOM_PISEL_C3C2IS						BIT(1)
#define	CAPCOM_PISEL_C5C4IS						BIT(2)
#define	CAPCOM_PISEL_C7C6IS						BIT(3)
#define	CAPCOM_PISEL_T0INIS						BIT(4)
#define	CAPCOM_PISEL_T1INIS						BIT(5)

/* Module Identifier Register */
#define	CAPCOM_ID(base)							MMIO32((base) + 0x08)

#define	CAPCOM_T01CON(base)						MMIO32((base) + 0x10)
#define	CAPCOM_T01CON_T0I						GENMASK(2, 0)
#define	CAPCOM_T01CON_T0I_SHIFT					0
#define	CAPCOM_T01CON_T0I_OVERFLOW_UNDERFLOW	0x0
#define	CAPCOM_T01CON_T0I_RISING_EDGE			0x1
#define	CAPCOM_T01CON_T0I_FALLING_EDGE			0x2
#define	CAPCOM_T01CON_T0I_BOTH_EDGES			0x3
#define	CAPCOM_T01CON_T0M						BIT(3)
#define	CAPCOM_T01CON_T0M_TIMER					0x0
#define	CAPCOM_T01CON_T0M_COUNTER				0x8
#define	CAPCOM_T01CON_T0R						BIT(6)
#define	CAPCOM_T01CON_T0R_DISABLED				0x0
#define	CAPCOM_T01CON_T0R_ENABLED				0x40
#define	CAPCOM_T01CON_T1I						GENMASK(10, 8)
#define	CAPCOM_T01CON_T1I_SHIFT					8
#define	CAPCOM_T01CON_T1I_OVERFLOW_UNDERFLOW	0x0
#define	CAPCOM_T01CON_T1I_RISING_EDGE			0x100
#define	CAPCOM_T01CON_T1I_FALLING_EDGE			0x200
#define	CAPCOM_T01CON_T1I_BOTH_EDGES			0x300
#define	CAPCOM_T01CON_T1M						BIT(11)
#define	CAPCOM_T01CON_T1M_TIMER					0x0
#define	CAPCOM_T01CON_T1M_COUNTER				0x800
#define	CAPCOM_T01CON_T1R						BIT(14)
#define	CAPCOM_T01CON_T1R_DISABLED				0x0
#define	CAPCOM_T01CON_T1R_ENABLED				0x4000

#define	CAPCOM_CCM0(base)						MMIO32((base) + 0x14)
#define	CAPCOM_CCM0_MOD0						GENMASK(2, 0)
#define	CAPCOM_CCM0_MOD0_SHIFT					0
#define	CAPCOM_CCM0_MOD0_DISABLE				0x0
#define	CAPCOM_CCM0_MOD0_RISING_EDGE			0x1
#define	CAPCOM_CCM0_MOD0_FALLING_EDGE			0x2
#define	CAPCOM_CCM0_MOD0_BOTH_EDGES				0x3
#define	CAPCOM_CCM0_MOD0_MODE0					0x4
#define	CAPCOM_CCM0_MOD0_MODE1					0x5
#define	CAPCOM_CCM0_MOD0_MODE2					0x6
#define	CAPCOM_CCM0_MOD0_MODE3					0x7
#define	CAPCOM_CCM0_ACC0						BIT(3)
#define	CAPCOM_CCM0_ACC0_T0						0x0
#define	CAPCOM_CCM0_ACC0_T1						0x8
#define	CAPCOM_CCM0_MOD1						GENMASK(6, 4)
#define	CAPCOM_CCM0_MOD1_SHIFT					4
#define	CAPCOM_CCM0_MOD1_DISABLE				0x0
#define	CAPCOM_CCM0_MOD1_RISING_EDGE			0x10
#define	CAPCOM_CCM0_MOD1_FALLING_EDGE			0x20
#define	CAPCOM_CCM0_MOD1_BOTH_EDGES				0x30
#define	CAPCOM_CCM0_MOD1_MODE0					0x40
#define	CAPCOM_CCM0_MOD1_MODE1					0x50
#define	CAPCOM_CCM0_MOD1_MODE2					0x60
#define	CAPCOM_CCM0_MOD1_MODE3					0x70
#define	CAPCOM_CCM0_ACC1						BIT(7)
#define	CAPCOM_CCM0_ACC1_T0						0x0
#define	CAPCOM_CCM0_ACC1_T1						0x80
#define	CAPCOM_CCM0_MOD2						GENMASK(10, 8)
#define	CAPCOM_CCM0_MOD2_SHIFT					8
#define	CAPCOM_CCM0_MOD2_DISABLE				0x0
#define	CAPCOM_CCM0_MOD2_RISING_EDGE			0x100
#define	CAPCOM_CCM0_MOD2_FALLING_EDGE			0x200
#define	CAPCOM_CCM0_MOD2_BOTH_EDGES				0x300
#define	CAPCOM_CCM0_MOD2_MODE0					0x400
#define	CAPCOM_CCM0_MOD2_MODE1					0x500
#define	CAPCOM_CCM0_MOD2_MODE2					0x600
#define	CAPCOM_CCM0_MOD2_MODE3					0x700
#define	CAPCOM_CCM0_ACC2						BIT(11)
#define	CAPCOM_CCM0_ACC2_T0						0x0
#define	CAPCOM_CCM0_ACC2_T1						0x800
#define	CAPCOM_CCM0_MOD3						GENMASK(14, 12)
#define	CAPCOM_CCM0_MOD3_SHIFT					12
#define	CAPCOM_CCM0_MOD3_DISABLE				0x0
#define	CAPCOM_CCM0_MOD3_RISING_EDGE			0x1000
#define	CAPCOM_CCM0_MOD3_FALLING_EDGE			0x2000
#define	CAPCOM_CCM0_MOD3_BOTH_EDGES				0x3000
#define	CAPCOM_CCM0_MOD3_MODE0					0x4000
#define	CAPCOM_CCM0_MOD3_MODE1					0x5000
#define	CAPCOM_CCM0_MOD3_MODE2					0x6000
#define	CAPCOM_CCM0_MOD3_MODE3					0x7000
#define	CAPCOM_CCM0_ACC3						BIT(15)
#define	CAPCOM_CCM0_ACC3_T0						0x0
#define	CAPCOM_CCM0_ACC3_T1						0x8000

#define	CAPCOM_CCM1(base)						MMIO32((base) + 0x18)
#define	CAPCOM_CCM1_MOD4						GENMASK(2, 0)
#define	CAPCOM_CCM1_MOD4_SHIFT					0
#define	CAPCOM_CCM1_MOD4_DISABLE				0x0
#define	CAPCOM_CCM1_MOD4_RISING_EDGE			0x1
#define	CAPCOM_CCM1_MOD4_FALLING_EDGE			0x2
#define	CAPCOM_CCM1_MOD4_BOTH_EDGES				0x3
#define	CAPCOM_CCM1_MOD4_MODE0					0x4
#define	CAPCOM_CCM1_MOD4_MODE1					0x5
#define	CAPCOM_CCM1_MOD4_MODE2					0x6
#define	CAPCOM_CCM1_MOD4_MODE3					0x7
#define	CAPCOM_CCM1_ACC4						BIT(3)
#define	CAPCOM_CCM1_ACC4_T0						0x0
#define	CAPCOM_CCM1_ACC4_T1						0x8
#define	CAPCOM_CCM1_MOD5						GENMASK(6, 4)
#define	CAPCOM_CCM1_MOD5_SHIFT					4
#define	CAPCOM_CCM1_MOD5_DISABLE				0x0
#define	CAPCOM_CCM1_MOD5_RISING_EDGE			0x10
#define	CAPCOM_CCM1_MOD5_FALLING_EDGE			0x20
#define	CAPCOM_CCM1_MOD5_BOTH_EDGES				0x30
#define	CAPCOM_CCM1_MOD5_MODE0					0x40
#define	CAPCOM_CCM1_MOD5_MODE1					0x50
#define	CAPCOM_CCM1_MOD5_MODE2					0x60
#define	CAPCOM_CCM1_MOD5_MODE3					0x70
#define	CAPCOM_CCM1_ACC5						BIT(7)
#define	CAPCOM_CCM1_ACC5_T0						0x0
#define	CAPCOM_CCM1_ACC5_T1						0x80
#define	CAPCOM_CCM1_MOD6						GENMASK(10, 8)
#define	CAPCOM_CCM1_MOD6_SHIFT					8
#define	CAPCOM_CCM1_MOD6_DISABLE				0x0
#define	CAPCOM_CCM1_MOD6_RISING_EDGE			0x100
#define	CAPCOM_CCM1_MOD6_FALLING_EDGE			0x200
#define	CAPCOM_CCM1_MOD6_BOTH_EDGES				0x300
#define	CAPCOM_CCM1_MOD6_MODE0					0x400
#define	CAPCOM_CCM1_MOD6_MODE1					0x500
#define	CAPCOM_CCM1_MOD6_MODE2					0x600
#define	CAPCOM_CCM1_MOD6_MODE3					0x700
#define	CAPCOM_CCM1_ACC6						BIT(11)
#define	CAPCOM_CCM1_ACC6_T0						0x0
#define	CAPCOM_CCM1_ACC6_T1						0x800
#define	CAPCOM_CCM1_MOD7						GENMASK(14, 12)
#define	CAPCOM_CCM1_MOD7_SHIFT					12
#define	CAPCOM_CCM1_MOD7_DISABLE				0x0
#define	CAPCOM_CCM1_MOD7_RISING_EDGE			0x1000
#define	CAPCOM_CCM1_MOD7_FALLING_EDGE			0x2000
#define	CAPCOM_CCM1_MOD7_BOTH_EDGES				0x3000
#define	CAPCOM_CCM1_MOD7_MODE0					0x4000
#define	CAPCOM_CCM1_MOD7_MODE1					0x5000
#define	CAPCOM_CCM1_MOD7_MODE2					0x6000
#define	CAPCOM_CCM1_MOD7_MODE3					0x7000
#define	CAPCOM_CCM1_ACC7						BIT(15)
#define	CAPCOM_CCM1_ACC7_T0						0x0
#define	CAPCOM_CCM1_ACC7_T1						0x8000

#define	CAPCOM_OUT(base)						MMIO32((base) + 0x24)
#define	CAPCOM_OUT_O0							BIT(0)
#define	CAPCOM_OUT_O1							BIT(1)
#define	CAPCOM_OUT_O2							BIT(2)
#define	CAPCOM_OUT_O3							BIT(3)
#define	CAPCOM_OUT_O4							BIT(4)
#define	CAPCOM_OUT_O5							BIT(5)
#define	CAPCOM_OUT_O6							BIT(6)
#define	CAPCOM_OUT_O7							BIT(7)

#define	CAPCOM_IOC(base)						MMIO32((base) + 0x28)
#define	CAPCOM_IOC_PL							BIT(1)								 // Port Lock
#define	CAPCOM_IOC_STAG							BIT(2)								 // Stagger
#define	CAPCOM_IOC_PDS							BIT(3)								 // Port Direction Select
#define	CAPCOM_IOC_PDS_OUT						0x0
#define	CAPCOM_IOC_PDS_IN						0x8

#define	CAPCOM_SEM(base)						MMIO32((base) + 0x2C)
#define	CAPCOM_SEM_SEM0							BIT(0)
#define	CAPCOM_SEM_SEM1							BIT(1)
#define	CAPCOM_SEM_SEM2							BIT(2)
#define	CAPCOM_SEM_SEM3							BIT(3)
#define	CAPCOM_SEM_SEM4							BIT(4)
#define	CAPCOM_SEM_SEM5							BIT(5)
#define	CAPCOM_SEM_SEM6							BIT(6)
#define	CAPCOM_SEM_SEM7							BIT(7)

#define	CAPCOM_SEE(base)						MMIO32((base) + 0x30)
#define	CAPCOM_SEE_SEE0							BIT(0)
#define	CAPCOM_SEE_SEE1							BIT(1)
#define	CAPCOM_SEE_SEE2							BIT(2)
#define	CAPCOM_SEE_SEE3							BIT(3)
#define	CAPCOM_SEE_SEE4							BIT(4)
#define	CAPCOM_SEE_SEE5							BIT(5)
#define	CAPCOM_SEE_SEE6							BIT(6)
#define	CAPCOM_SEE_SEE7							BIT(7)

#define	CAPCOM_DRM(base)						MMIO32((base) + 0x34)
#define	CAPCOM_DRM_DR0M							GENMASK(1, 0)
#define	CAPCOM_DRM_DR0M_SHIFT					0
#define	CAPCOM_DRM_DR0M_CON						0x0
#define	CAPCOM_DRM_DR0M_DIS						0x1
#define	CAPCOM_DRM_DR0M_EN						0x2
#define	CAPCOM_DRM_DR0M_RES						0x3
#define	CAPCOM_DRM_DR1M							GENMASK(3, 2)
#define	CAPCOM_DRM_DR1M_SHIFT					2
#define	CAPCOM_DRM_DR1M_CON						0x0
#define	CAPCOM_DRM_DR1M_DIS						0x4
#define	CAPCOM_DRM_DR1M_EN						0x8
#define	CAPCOM_DRM_DR1M_RES						0xC
#define	CAPCOM_DRM_DR2M							GENMASK(5, 4)
#define	CAPCOM_DRM_DR2M_SHIFT					4
#define	CAPCOM_DRM_DR2M_CON						0x0
#define	CAPCOM_DRM_DR2M_DIS						0x10
#define	CAPCOM_DRM_DR2M_EN						0x20
#define	CAPCOM_DRM_DR2M_RES						0x30
#define	CAPCOM_DRM_DR3M							GENMASK(7, 6)
#define	CAPCOM_DRM_DR3M_SHIFT					6
#define	CAPCOM_DRM_DR3M_CON						0x0
#define	CAPCOM_DRM_DR3M_DIS						0x40
#define	CAPCOM_DRM_DR3M_EN						0x80
#define	CAPCOM_DRM_DR3M_RES						0xC0

#define	CAPCOM_WHBSSEE(base)					MMIO32((base) + 0x38)
#define	CAPCOM_WHBSSEE_SETSEE0					BIT(0)
#define	CAPCOM_WHBSSEE_SETSEE0_NOE				0x0
#define	CAPCOM_WHBSSEE_SETSEE0_SET				0x1
#define	CAPCOM_WHBSSEE_SETSEE1					BIT(1)
#define	CAPCOM_WHBSSEE_SETSEE1_NOE				0x0
#define	CAPCOM_WHBSSEE_SETSEE1_SET				0x2
#define	CAPCOM_WHBSSEE_SETSEE2					BIT(2)
#define	CAPCOM_WHBSSEE_SETSEE2_NOE				0x0
#define	CAPCOM_WHBSSEE_SETSEE2_SET				0x4
#define	CAPCOM_WHBSSEE_SETSEE3					BIT(3)
#define	CAPCOM_WHBSSEE_SETSEE3_NOE				0x0
#define	CAPCOM_WHBSSEE_SETSEE3_SET				0x8
#define	CAPCOM_WHBSSEE_SETSEE4					BIT(4)
#define	CAPCOM_WHBSSEE_SETSEE4_NOE				0x0
#define	CAPCOM_WHBSSEE_SETSEE4_SET				0x10
#define	CAPCOM_WHBSSEE_SETSEE5					BIT(5)
#define	CAPCOM_WHBSSEE_SETSEE5_NOE				0x0
#define	CAPCOM_WHBSSEE_SETSEE5_SET				0x20
#define	CAPCOM_WHBSSEE_SETSEE6					BIT(6)
#define	CAPCOM_WHBSSEE_SETSEE6_NOE				0x0
#define	CAPCOM_WHBSSEE_SETSEE6_SET				0x40
#define	CAPCOM_WHBSSEE_SETSEE7					BIT(7)
#define	CAPCOM_WHBSSEE_SETSEE7_NOE				0x0
#define	CAPCOM_WHBSSEE_SETSEE7_SET				0x80

#define	CAPCOM_WHBCSEE(base)					MMIO32((base) + 0x3C)
#define	CAPCOM_WHBCSEE_CLRSEE0					BIT(0)
#define	CAPCOM_WHBCSEE_CLRSEE0_NOE				0x0
#define	CAPCOM_WHBCSEE_CLRSEE0_CLR				0x1
#define	CAPCOM_WHBCSEE_CLRSEE1					BIT(1)
#define	CAPCOM_WHBCSEE_CLRSEE1_NOE				0x0
#define	CAPCOM_WHBCSEE_CLRSEE1_CLR				0x2
#define	CAPCOM_WHBCSEE_CLRSEE2					BIT(2)
#define	CAPCOM_WHBCSEE_CLRSEE2_NOE				0x0
#define	CAPCOM_WHBCSEE_CLRSEE2_CLR				0x4
#define	CAPCOM_WHBCSEE_CLRSEE3					BIT(3)
#define	CAPCOM_WHBCSEE_CLRSEE3_NOE				0x0
#define	CAPCOM_WHBCSEE_CLRSEE3_CLR				0x8
#define	CAPCOM_WHBCSEE_CLRSEE4					BIT(4)
#define	CAPCOM_WHBCSEE_CLRSEE4_NOE				0x0
#define	CAPCOM_WHBCSEE_CLRSEE4_CLR				0x10
#define	CAPCOM_WHBCSEE_CLRSEE5					BIT(5)
#define	CAPCOM_WHBCSEE_CLRSEE5_NOE				0x0
#define	CAPCOM_WHBCSEE_CLRSEE5_CLR				0x20
#define	CAPCOM_WHBCSEE_CLRSEE6					BIT(6)
#define	CAPCOM_WHBCSEE_CLRSEE6_NOE				0x0
#define	CAPCOM_WHBCSEE_CLRSEE6_CLR				0x40
#define	CAPCOM_WHBCSEE_CLRSEE7					BIT(7)
#define	CAPCOM_WHBCSEE_CLRSEE7_NOE				0x0
#define	CAPCOM_WHBCSEE_CLRSEE7_CLR				0x80

#define	CAPCOM_T0(base)							MMIO32((base) + 0x40)
#define	CAPCOM_T0_T0							GENMASK(30, 0)
#define	CAPCOM_T0_T0_SHIFT						0
#define	CAPCOM_T0_OVF0							BIT(31)
#define	CAPCOM_T0_OVF0_CLEARED					0x0
#define	CAPCOM_T0_OVF0_SET						0x80000000

#define	CAPCOM_T0REL(base)						MMIO32((base) + 0x44)
#define	CAPCOM_T0REL_T0REL						GENMASK(30, 0)
#define	CAPCOM_T0REL_T0REL_SHIFT				0

#define	CAPCOM_T1(base)							MMIO32((base) + 0x48)
#define	CAPCOM_T1_T1							GENMASK(30, 0)
#define	CAPCOM_T1_T1_SHIFT						0
#define	CAPCOM_T1_OVF1							BIT(31)
#define	CAPCOM_T1_OVF1_CLEARED					0x0
#define	CAPCOM_T1_OVF1_SET						0x80000000

#define	CAPCOM_T1REL(base)						MMIO32((base) + 0x4C)
#define	CAPCOM_T1REL_T1REL						GENMASK(30, 0)
#define	CAPCOM_T1REL_T1REL_SHIFT				0

#define	CAPCOM_CC(base, n)						MMIO32(base + 0x50 + ((n) * 0x4))

#define	CAPCOM_T01OCR(base)						MMIO32((base) + 0x94)
#define	CAPCOM_T01OCR_CT0						BIT(0)
#define	CAPCOM_T01OCR_CT0_NOC					0x0
#define	CAPCOM_T01OCR_CT0_CSR					0x1
#define	CAPCOM_T01OCR_CT1						BIT(1)
#define	CAPCOM_T01OCR_CT1_NOC					0x0
#define	CAPCOM_T01OCR_CT1_CSR					0x2

#define	CAPCOM_WHBSOUT(base)					MMIO32((base) + 0x98)
#define	CAPCOM_WHBSOUT_SET0O					BIT(0)
#define	CAPCOM_WHBSOUT_SET1O					BIT(1)
#define	CAPCOM_WHBSOUT_SET2O					BIT(2)
#define	CAPCOM_WHBSOUT_SET3O					BIT(3)
#define	CAPCOM_WHBSOUT_SET4O					BIT(4)
#define	CAPCOM_WHBSOUT_SET5O					BIT(5)
#define	CAPCOM_WHBSOUT_SET6O					BIT(6)
#define	CAPCOM_WHBSOUT_SET7O					BIT(7)

#define	CAPCOM_WHBCOUT(base)					MMIO32((base) + 0x9C)
#define	CAPCOM_WHBCOUT_CLR0O					BIT(0)
#define	CAPCOM_WHBCOUT_CLR1O					BIT(1)
#define	CAPCOM_WHBCOUT_CLR2O					BIT(2)
#define	CAPCOM_WHBCOUT_CLR3O					BIT(3)
#define	CAPCOM_WHBCOUT_CLR4O					BIT(4)
#define	CAPCOM_WHBCOUT_CLR5O					BIT(5)
#define	CAPCOM_WHBCOUT_CLR6O					BIT(6)
#define	CAPCOM_WHBCOUT_CLR7O					BIT(7)

/* Service Routing Control Register */
#define	CAPCOM_CC7_SRC(base)					MMIO32((base) + 0xD8)

/* Service Routing Control Register */
#define	CAPCOM_CC6_SRC(base)					MMIO32((base) + 0xDC)

/* Service Routing Control Register */
#define	CAPCOM_CC5_SRC(base)					MMIO32((base) + 0xE0)

/* Service Routing Control Register */
#define	CAPCOM_CC4_SRC(base)					MMIO32((base) + 0xE4)

/* Service Routing Control Register */
#define	CAPCOM_CC3_SRC(base)					MMIO32((base) + 0xE8)

/* Service Routing Control Register */
#define	CAPCOM_CC2_SRC(base)					MMIO32((base) + 0xEC)

/* Service Routing Control Register */
#define	CAPCOM_CC1_SRC(base)					MMIO32((base) + 0xF0)

/* Service Routing Control Register */
#define	CAPCOM_CC0_SRC(base)					MMIO32((base) + 0xF4)

/* Service Routing Control Register */
#define	CAPCOM_T1_SRC(base)						MMIO32((base) + 0xF8)

/* Service Routing Control Register */
#define	CAPCOM_T0_SRC(base)						MMIO32((base) + 0xFC)


// GPIO [MOD_NUM=F023, MOD_REV=00, MOD_32BIT=C0]
// PCL (Port Control Logic), registers from drivers/pinctrl/pinctrl-thunderbay.c
#define	GPIO_BASE			0xF4300000
/* Clock Control Register */
#define	GPIO_CLC			MMIO32(GPIO_BASE + 0x00)

/* Module Identifier Register */
#define	GPIO_ID				MMIO32(GPIO_BASE + 0x08)

#define	GPIO_MON_CR1		MMIO32(GPIO_BASE + 0x10)

#define	GPIO_MON_CR2		MMIO32(GPIO_BASE + 0x14)

#define	GPIO_MON_CR3		MMIO32(GPIO_BASE + 0x18)

#define	GPIO_MON_CR4		MMIO32(GPIO_BASE + 0x1C)

#define	GPIO_PIN(n)			MMIO32(GPIO_BASE + 0x20 + ((n) * 0x4))
#define	GPIO_IS				GENMASK(2, 0)
#define	GPIO_IS_SHIFT		0
#define	GPIO_IS_NONE		0x0
#define	GPIO_IS_ALT0		0x1
#define	GPIO_IS_ALT1		0x2
#define	GPIO_IS_ALT2		0x3
#define	GPIO_IS_ALT3		0x4
#define	GPIO_IS_ALT4		0x5
#define	GPIO_IS_ALT5		0x6
#define	GPIO_IS_ALT6		0x7
#define	GPIO_OS				GENMASK(6, 4)
#define	GPIO_OS_SHIFT		4
#define	GPIO_OS_NONE		0x0
#define	GPIO_OS_ALT0		0x10
#define	GPIO_OS_ALT1		0x20
#define	GPIO_OS_ALT2		0x30
#define	GPIO_OS_ALT3		0x40
#define	GPIO_OS_ALT4		0x50
#define	GPIO_OS_ALT5		0x60
#define	GPIO_OS_ALT6		0x70
#define	GPIO_PS				BIT(8)
#define	GPIO_PS_ALT			0x0
#define	GPIO_PS_MANUAL		0x100
#define	GPIO_DATA			BIT(9)
#define	GPIO_DATA_LOW		0x0
#define	GPIO_DATA_HIGH		0x200
#define	GPIO_DIR			BIT(10)
#define	GPIO_DIR_IN			0x0
#define	GPIO_DIR_OUT		0x400
#define	GPIO_PPEN			BIT(12)
#define	GPIO_PPEN_PUSHPULL	0x0
#define	GPIO_PPEN_OPENDRAIN	0x1000
#define	GPIO_PDPU			GENMASK(14, 13)
#define	GPIO_PDPU_SHIFT		13
#define	GPIO_PDPU_NONE		0x0
#define	GPIO_PDPU_PULLUP	0x2000
#define	GPIO_PDPU_PULLDOWN	0x4000
#define	GPIO_ENAQ			BIT(15)
#define	GPIO_ENAQ_OFF		0x0
#define	GPIO_ENAQ_ON		0x8000


// SCU [MOD_NUM=F040, MOD_REV=00, MOD_32BIT=C0]
// Looks like SCU module, registers collected using TC1766 official public datasheet and tests on real hardware (using "black box" method).
#define	SCU_BASE					0xF4400000
/* Clock Control Register */
#define	SCU_CLC						MMIO32(SCU_BASE + 0x00)

/* Module Identifier Register */
#define	SCU_ID						MMIO32(SCU_BASE + 0x08)

/* Reset Status Register */
#define	SCU_RST_SR					MMIO32(SCU_BASE + 0x10)
#define	SCU_RST_SR_RSSTM			BIT(0)									 // System Timer Reset Status
#define	SCU_RST_SR_RSEXT			BIT(1)									 // HDRST Line State during Last Reset
#define	SCU_RST_SR_HWCFG			GENMASK(18, 16)							 // Boot Configuration Selection Status
#define	SCU_RST_SR_HWCFG_SHIFT		16
#define	SCU_RST_SR_HWBRKIN			BIT(21)									 // Latched State of BRKIN Input
#define	SCU_RST_SR_TMPLS			BIT(22)									 // Latched State of TESTMODE Input
#define	SCU_RST_SR_PWORST			BIT(27)									 // The last reset was a power-on reset
#define	SCU_RST_SR_HDRST			BIT(28)									 // The last reset was a hardware reset.
#define	SCU_RST_SR_SFTRST			BIT(29)									 // The last reset was a software reset.
#define	SCU_RST_SR_WDTRST			BIT(30)									 // The last reset was a watchdog reset.
#define	SCU_RST_SR_PWDRST			BIT(31)									 // The last reset was a wake-up from power-down

/* Reset Request Register */
#define	SCU_RST_REQ					MMIO32(SCU_BASE + 0x18)
#define	SCU_RST_REQ_RRSTM			BIT(0)									 // Reset Request for the System Timer
#define	SCU_RST_REQ_RREXT			BIT(2)									 // Reset Request for External Devices
#define	SCU_RST_REQ_SWCFG			GENMASK(18, 16)							 // Software Boot Configuration
#define	SCU_RST_REQ_SWCFG_SHIFT		16
#define	SCU_RST_REQ_SWBRKIN			BIT(21)									 // Software Break Signal Boot Value
#define	SCU_RST_REQ_SWBOOT			BIT(24)									 // Software Boot Configuration Selection

#define	SCU_WDTCON0					MMIO32(SCU_BASE + 0x24)
#define	SCU_WDTCON0_ENDINIT			BIT(0)									 // End-of-Initialization Control Bit.
#define	SCU_WDTCON0_WDTLCK			BIT(1)									 // Lock bit to Control Access to WDT_CON0.
#define	SCU_WDTCON0_WDTHPW0			GENMASK(3, 2)							 // Hardware Password 0.
#define	SCU_WDTCON0_WDTHPW0_SHIFT	2
#define	SCU_WDTCON0_WDTHPW1			GENMASK(7, 4)							 // Hardware Password 1.
#define	SCU_WDTCON0_WDTHPW1_SHIFT	4
#define	SCU_WDTCON0_WDTPW			GENMASK(15, 8)							 // User-Definable Password Field for Access to WDT_CON0.
#define	SCU_WDTCON0_WDTPW_SHIFT		8
#define	SCU_WDTCON0_WDTREL			GENMASK(31, 16)							 // Reload Value for the Watchdog Timer.
#define	SCU_WDTCON0_WDTREL_SHIFT	16

#define	SCU_WDTCON1					MMIO32(SCU_BASE + 0x28)
#define	SCU_WDTCON1_WDTIR			BIT(2)									 // Watchdog Timer Input Frequency Request Control Bit.
#define	SCU_WDTCON1_WDTDR			BIT(3)									 // Watchdog Timer Disable Request Control Bit.

#define	SCU_WDT_SR					MMIO32(SCU_BASE + 0x2C)
#define	SCU_WDT_SR_WDTAE			BIT(0)									 // Watchdog Access Error Status Flag
#define	SCU_WDT_SR_WDTOE			BIT(1)									 // Watchdog Overflow Error Status Flag
#define	SCU_WDT_SR_WDTIS			BIT(2)									 // Watchdog Input Clock Status Flag
#define	SCU_WDT_SR_WDTDS			BIT(3)									 // Watchdog Enable/Disable Status Flag
#define	SCU_WDT_SR_WDTTO			BIT(4)									 // Watchdog Time-out Mode Flag
#define	SCU_WDT_SR_WDTPR			BIT(5)									 // Watchdog Prewarning Mode Flag
#define	SCU_WDT_SR_WDTTIM			GENMASK(31, 16)							 // Watchdog Timer Value
#define	SCU_WDT_SR_WDTTIM_SHIFT		16

#define	SCU_DSP_UNK0				MMIO32(SCU_BASE + 0x30)

#define	SCU_EXTI					MMIO32(SCU_BASE + 0x3C)
#define	SCU_EXTI_EXT0				GENMASK(1, 0)
#define	SCU_EXTI_EXT0_SHIFT			0
#define	SCU_EXTI_EXT0_OFF			0x0
#define	SCU_EXTI_EXT0_RISING		0x1
#define	SCU_EXTI_EXT0_FALLING		0x2
#define	SCU_EXTI_EXT0_ANY			0x3
#define	SCU_EXTI_EXT1				GENMASK(3, 2)
#define	SCU_EXTI_EXT1_SHIFT			2
#define	SCU_EXTI_EXT1_OFF			0x0
#define	SCU_EXTI_EXT1_RISING		0x4
#define	SCU_EXTI_EXT1_FALLING		0x8
#define	SCU_EXTI_EXT1_ANY			0xC
#define	SCU_EXTI_EXT2				GENMASK(5, 4)
#define	SCU_EXTI_EXT2_SHIFT			4
#define	SCU_EXTI_EXT2_OFF			0x0
#define	SCU_EXTI_EXT2_RISING		0x10
#define	SCU_EXTI_EXT2_FALLING		0x20
#define	SCU_EXTI_EXT2_ANY			0x30
#define	SCU_EXTI_EXT3				GENMASK(7, 6)
#define	SCU_EXTI_EXT3_SHIFT			6
#define	SCU_EXTI_EXT3_OFF			0x0
#define	SCU_EXTI_EXT3_RISING		0x40
#define	SCU_EXTI_EXT3_FALLING		0x80
#define	SCU_EXTI_EXT3_ANY			0xC0
#define	SCU_EXTI_EXT4				GENMASK(9, 8)
#define	SCU_EXTI_EXT4_SHIFT			8
#define	SCU_EXTI_EXT4_OFF			0x0
#define	SCU_EXTI_EXT4_RISING		0x100
#define	SCU_EXTI_EXT4_FALLING		0x200
#define	SCU_EXTI_EXT4_ANY			0x300
#define	SCU_EXTI_EXT5				GENMASK(11, 10)
#define	SCU_EXTI_EXT5_SHIFT			10
#define	SCU_EXTI_EXT5_OFF			0x0
#define	SCU_EXTI_EXT5_RISING		0x400
#define	SCU_EXTI_EXT5_FALLING		0x800
#define	SCU_EXTI_EXT5_ANY			0xC00
#define	SCU_EXTI_EXT6				GENMASK(13, 12)
#define	SCU_EXTI_EXT6_SHIFT			12
#define	SCU_EXTI_EXT6_OFF			0x0
#define	SCU_EXTI_EXT6_RISING		0x1000
#define	SCU_EXTI_EXT6_FALLING		0x2000
#define	SCU_EXTI_EXT6_ANY			0x3000
#define	SCU_EXTI_EXT7				GENMASK(15, 14)
#define	SCU_EXTI_EXT7_SHIFT			14
#define	SCU_EXTI_EXT7_OFF			0x0
#define	SCU_EXTI_EXT7_RISING		0x4000
#define	SCU_EXTI_EXT7_FALLING		0x8000
#define	SCU_EXTI_EXT7_ANY			0xC000

#define	SCU_EBUCLC1					MMIO32(SCU_BASE + 0x40)
#define	SCU_EBUCLC1_FLAG1			GENMASK(3, 0)
#define	SCU_EBUCLC1_FLAG1_SHIFT		0
#define	SCU_EBUCLC1_READY			GENMASK(7, 4)
#define	SCU_EBUCLC1_READY_SHIFT		4

#define	SCU_EBUCLC2					MMIO32(SCU_BASE + 0x44)
#define	SCU_EBUCLC2_FLAG1			GENMASK(3, 0)
#define	SCU_EBUCLC2_FLAG1_SHIFT		0
#define	SCU_EBUCLC2_READY			GENMASK(7, 4)
#define	SCU_EBUCLC2_READY_SHIFT		4

#define	SCU_EBUCLC					MMIO32(SCU_BASE + 0x48)
#define	SCU_EBUCLC_LOCK				BIT(0)
#define	SCU_EBUCLC_VCOBYP			BIT(8)

#define	SCU_MANID					MMIO32(SCU_BASE + 0x5C)
#define	SCU_MANID_DEPT				GENMASK(3, 0)
#define	SCU_MANID_DEPT_SHIFT		0
#define	SCU_MANID_MANUF				GENMASK(14, 4)
#define	SCU_MANID_MANUF_SHIFT		4

#define	SCU_CHIPID					MMIO32(SCU_BASE + 0x60)
#define	SCU_CHIPID_CHREV			GENMASK(7, 0)
#define	SCU_CHIPID_CHREV_SHIFT		0
#define	SCU_CHIPID_MANUF			GENMASK(15, 8)
#define	SCU_CHIPID_MANUF_SHIFT		8

#define	SCU_RTCIF					MMIO32(SCU_BASE + 0x64)

#define	SCU_ID0						MMIO32(SCU_BASE + 0x6C)

#define	SCU_ID1						MMIO32(SCU_BASE + 0x70)

#define	SCU_BOOT_CFG				MMIO32(SCU_BASE + 0x74)
#define	SCU_BOOT_CFG_USART1			BIT(28)									 // Allow boot from USART1
#define	SCU_BOOT_CFG_BYPASS_FW		BIT(29)									 // Force boot from 0x82000, bypass firmware
#define	SCU_BOOT_CFG_USB			BIT(30)									 // Allow boot from USB

#define	SCU_BOOT_FLAG				MMIO32(SCU_BASE + 0x78)
#define	SCU_BOOT_FLAG_BOOT_OK		BIT(0)

#define	SCU_ROMAMCR					MMIO32(SCU_BASE + 0x7C)
#define	SCU_ROMAMCR_MOUNT_BROM		BIT(0)

#define	SCU_RTID					MMIO32(SCU_BASE + 0x80)

/* DMA Request Select */
#define	SCU_DMARS					MMIO32(SCU_BASE + 0x84)
#define	SCU_DMARS_SEL0				BIT(0)
#define	SCU_DMARS_SEL1				BIT(1)
#define	SCU_DMARS_SEL2				BIT(2)
#define	SCU_DMARS_SEL3				BIT(3)
#define	SCU_DMARS_SEL4				BIT(4)
#define	SCU_DMARS_SEL5				BIT(5)
#define	SCU_DMARS_SEL6				BIT(6)
#define	SCU_DMARS_SEL7				BIT(7)
#define	SCU_DMARS_SEL8				BIT(8)
#define	SCU_DMARS_SEL9				BIT(9)
#define	SCU_DMARS_SEL10				BIT(10)
#define	SCU_DMARS_SEL11				BIT(11)
#define	SCU_DMARS_SEL12				BIT(12)
#define	SCU_DMARS_SEL13				BIT(13)
#define	SCU_DMARS_SEL14				BIT(14)
#define	SCU_DMARS_SEL15				BIT(15)

/* Service Routing Control Register */
#define	SCU_EXTI0_SRC				MMIO32(SCU_BASE + 0xB8)

/* Service Routing Control Register */
#define	SCU_EXTI1_SRC				MMIO32(SCU_BASE + 0xBC)

/* Service Routing Control Register */
#define	SCU_EXTI2_SRC				MMIO32(SCU_BASE + 0xC0)

/* Service Routing Control Register */
#define	SCU_EXTI3_SRC				MMIO32(SCU_BASE + 0xC4)

/* Service Routing Control Register */
#define	SCU_EXTI4_SRC				MMIO32(SCU_BASE + 0xC8)

/* Service Routing Control Register */
#define	SCU_DSP_SRC(n)				MMIO32(SCU_BASE + 0xCC + ((n) * 0x4))

/* Service Routing Control Register */
#define	SCU_UNK0_SRC				MMIO32(SCU_BASE + 0xE8)

/* Service Routing Control Register */
#define	SCU_UNK1_SRC				MMIO32(SCU_BASE + 0xEC)

/* Service Routing Control Register */
#define	SCU_UNK2_SRC				MMIO32(SCU_BASE + 0xF0)

/* Service Routing Control Register */
#define	SCU_EXTI5_SRC				MMIO32(SCU_BASE + 0xF4)

/* Service Routing Control Register */
#define	SCU_EXTI6_SRC				MMIO32(SCU_BASE + 0xF8)

/* Service Routing Control Register */
#define	SCU_EXTI7_SRC				MMIO32(SCU_BASE + 0xFC)


// PLL
// Looks like a CGU module, registers collected using tests on real hardware (using "black box" method).
#define	PLL_BASE						0xF4500000
#define	PLL_OSC							MMIO32(PLL_BASE + 0xA0)
#define	PLL_OSC_LOCK					BIT(0)
#define	PLL_OSC_NDIV					GENMASK(18, 16)			 // Feedback divider (multiply by N+1)
#define	PLL_OSC_NDIV_SHIFT				16

#define	PLL_CON0						MMIO32(PLL_BASE + 0xA4)
#define	PLL_CON0_PLL1_K2				GENMASK(2, 0)			 // div by (K1 * 6 + (K2 - 1))
#define	PLL_CON0_PLL1_K2_SHIFT			0
#define	PLL_CON0_PLL1_K1				GENMASK(6, 3)
#define	PLL_CON0_PLL1_K1_SHIFT			3
#define	PLL_CON0_PLL2_K2				GENMASK(10, 8)			 // div by (K1 * 6 + (K2 - 1))
#define	PLL_CON0_PLL2_K2_SHIFT			8
#define	PLL_CON0_PLL2_K1				GENMASK(14, 11)
#define	PLL_CON0_PLL2_K1_SHIFT			11
#define	PLL_CON0_PLL3_K2				GENMASK(18, 16)			 // div by (K1 * 6 + (K2 - 1))
#define	PLL_CON0_PLL3_K2_SHIFT			16
#define	PLL_CON0_PLL3_K1				GENMASK(22, 19)
#define	PLL_CON0_PLL3_K1_SHIFT			19
#define	PLL_CON0_PLL4_K2				GENMASK(26, 24)			 // div by (K1 * 6 + (K2 - 1))
#define	PLL_CON0_PLL4_K2_SHIFT			24
#define	PLL_CON0_PLL4_K1				GENMASK(30, 27)
#define	PLL_CON0_PLL4_K1_SHIFT			27

#define	PLL_CON1						MMIO32(PLL_BASE + 0xA8)
#define	PLL_CON1_FSYS_CLKSEL			GENMASK(17, 16)			 // Source clock for fSYS (BYPASS: fSYS=fOSC, PLL: fSYS=fPLL / 2)
#define	PLL_CON1_FSYS_CLKSEL_SHIFT		16
#define	PLL_CON1_FSYS_CLKSEL_BYPASS		0x0
#define	PLL_CON1_FSYS_CLKSEL_PLL		0x20000
#define	PLL_CON1_FSYS_CLKSEL_DISABLE	0x30000
#define	PLL_CON1_AHB_CLKSEL				GENMASK(22, 20)			 // Source clock for fPLL
#define	PLL_CON1_AHB_CLKSEL_SHIFT		20
#define	PLL_CON1_AHB_CLKSEL_BYPASS		0x0
#define	PLL_CON1_AHB_CLKSEL_PLL0		0x200000
#define	PLL_CON1_AHB_CLKSEL_PLL1		0x300000
#define	PLL_CON1_AHB_CLKSEL_PLL2		0x400000
#define	PLL_CON1_AHB_CLKSEL_PLL3		0x500000
#define	PLL_CON1_AHB_CLKSEL_PLL4		0x600000
#define	PLL_CON1_FSTM_DIV_EN			BIT(25)					 // Enable fSTM divider
#define	PLL_CON1_FSTM_DIV				GENMASK(29, 28)			 // fSTM divider value (n^2)
#define	PLL_CON1_FSTM_DIV_SHIFT			28
#define	PLL_CON1_FSTM_DIV_1				0x0
#define	PLL_CON1_FSTM_DIV_2				0x10000000
#define	PLL_CON1_FSTM_DIV_4				0x20000000
#define	PLL_CON1_FSTM_DIV_8				0x30000000

#define	PLL_CON2						MMIO32(PLL_BASE + 0xAC)
#define	PLL_CON2_CPU_DIV				GENMASK(9, 8)
#define	PLL_CON2_CPU_DIV_SHIFT			8
#define	PLL_CON2_CPU_DIV_EN				BIT(12)
#define	PLL_CON2_CLK32_EN				BIT(24)

#define	PLL_STAT						MMIO32(PLL_BASE + 0xB0)
#define	PLL_STAT_LOCK					BIT(13)

#define	PLL_CON3						MMIO32(PLL_BASE + 0xB4)

/* Service Routing Control Register */
#define	PLL_SRC							MMIO32(PLL_BASE + 0xCC)


// SCCU
// Controlling MCU sleep. Very similar to "SCCU" description in the Teltonika TM1Q user manual.
#define	SCCU_BASE					0xF4600000
#define	SCCU_CON0					MMIO32(SCCU_BASE + 0x10)

/* Sleep timer reload */
#define	SCCU_TIMER_REL				MMIO32(SCCU_BASE + 0x14)
#define	SCCU_TIMER_REL_VALUE		GENMASK(12, 0)
#define	SCCU_TIMER_REL_VALUE_SHIFT	0

/* Sleep timer counter */
#define	SCCU_TIMER_CNT				MMIO32(SCCU_BASE + 0x18)
#define	SCCU_TIMER_CNT_VALUE		GENMASK(12, 0)
#define	SCCU_TIMER_CNT_VALUE_SHIFT	0

#define	SCCU_CON1					MMIO32(SCCU_BASE + 0x1C)
#define	SCCU_CON1_CAL				BIT(0)						 // Calibration?
#define	SCCU_CON1_TIMER_START		BIT(1)						 // Start sleep timer
#define	SCCU_CON1_TIMER_RESET		BIT(2)						 // Reset sleep timer

#define	SCCU_CAL					MMIO32(SCCU_BASE + 0x24)
#define	SCCU_CAL_VALUE0				GENMASK(12, 0)
#define	SCCU_CAL_VALUE0_SHIFT		0
#define	SCCU_CAL_VALUE1				GENMASK(25, 13)
#define	SCCU_CAL_VALUE1_SHIFT		13

#define	SCCU_TIMER_DIV				MMIO32(SCCU_BASE + 0x28)
#define	SCCU_TIMER_DIV_VALUE		GENMASK(7, 0)
#define	SCCU_TIMER_DIV_VALUE_SHIFT	0

#define	SCCU_SLEEP_CTRL				MMIO32(SCCU_BASE + 0x2C)
#define	SCCU_SLEEP_CTRL_SLEEP		BIT(0)						 // Enter sleep
#define	SCCU_SLEEP_CTRL_WAKEUP		BIT(1)						 // Force exit sleep

#define	SCCU_CON2					MMIO32(SCCU_BASE + 0x30)
#define	SCCU_CON2_UNK				GENMASK(7, 0)
#define	SCCU_CON2_UNK_SHIFT			0
#define	SCCU_CON2_REL_SUB			GENMASK(17, 16)				 // Substract this value from TIMER_REL (???)
#define	SCCU_CON2_REL_SUB_SHIFT		16

#define	SCCU_CON3					MMIO32(SCCU_BASE + 0x34)

#define	SCCU_STAT					MMIO32(SCCU_BASE + 0x40)
#define	SCCU_STAT_CPU				BIT(0)						 // CPU sleep status
#define	SCCU_STAT_CPU_SLEEP			0x0
#define	SCCU_STAT_CPU_NORMAL		0x1
#define	SCCU_STAT_TPU				BIT(1)						 // TPU sleep status
#define	SCCU_STAT_TPU_SLEEP			0x0
#define	SCCU_STAT_TPU_NORMAL		0x2

/* Service Routing Control Register */
#define	SCCU_WAKE_SRC				MMIO32(SCCU_BASE + 0xA0)

/* Service Routing Control Register */
#define	SCCU_UNK_SRC				MMIO32(SCCU_BASE + 0xA8)


// RTC [MOD_NUM=F049, MOD_REV=00, MOD_32BIT=C0]
// RTC from XC27x5X official public datasheet
#define	RTC_BASE				0xF4700000
/* Clock Control Register */
#define	RTC_CLC					MMIO32(RTC_BASE + 0x00)

/* Module Identifier Register */
#define	RTC_ID					MMIO32(RTC_BASE + 0x08)

/* RTC Shell Control Register */
#define	RTC_CTRL				MMIO32(RTC_BASE + 0x10)
#define	RTC_CTRL_RTCOUTEN		BIT(0)					 // RTC External Interrupt Output Enable
#define	RTC_CTRL_RTCINT			BIT(1)					 // RTC Interrupt Status
#define	RTC_CTRL_CLK32KEN		BIT(2)					 // 32k Clock Enable
#define	RTC_CTRL_PU32K			BIT(3)					 // 32 kHz Oscillator Power Up
#define	RTC_CTRL_CLK_SEL		BIT(4)					 // RTC Logic Clock Select
#define	RTC_CTRL_CLR_RTCINT		BIT(8)					 // Clears RTCINT
#define	RTC_CTRL_RTCBAD			BIT(9)					 // RTC Content Inconsistent Due to Power Supply Drop Down
#define	RTC_CTRL_CLR_RTCBAD		BIT(10)					 // Clears RTCBAD

/* RTC Control Register */
#define	RTC_CON					MMIO32(RTC_BASE + 0x14)
#define	RTC_CON_RUN				BIT(0)					 // RTC Run Bit
#define	RTC_CON_PRE				BIT(1)					 // RTC Input Source Prescaler (8:1) Enable
#define	RTC_CON_T14DEC			BIT(2)					 // Decrement Timer T14 Value
#define	RTC_CON_T14INC			BIT(3)					 // Increment Timer T14 Value
#define	RTC_CON_REFCLK			BIT(4)					 // RTC Input Source Prescaler (32:1) Disable
#define	RTC_CON_ACCPOS			BIT(15)					 // RTC Register Access Possible

/* Timer T14 Count/Reload Register */
#define	RTC_T14					MMIO32(RTC_BASE + 0x18)
#define	RTC_T14_REL				GENMASK(15, 0)			 // Timer T14 Reload Value
#define	RTC_T14_REL_SHIFT		0
#define	RTC_T14_CNT				GENMASK(31, 16)			 // Timer T14 Count Value
#define	RTC_T14_CNT_SHIFT		16

/* RTC Count Register */
#define	RTC_CNT					MMIO32(RTC_BASE + 0x1C)
#define	RTC_CNT_CNT				GENMASK(31, 0)			 // RTC Timer Count Value
#define	RTC_CNT_CNT_SHIFT		0

/* RTC Reload Register */
#define	RTC_REL					MMIO32(RTC_BASE + 0x20)
#define	RTC_REL_REL				GENMASK(31, 0)			 // RTC Timer Reload Value
#define	RTC_REL_REL_SHIFT		0

/* Interrupt Sub-Node Control Register */
#define	RTC_ISNC				MMIO32(RTC_BASE + 0x24)
#define	RTC_ISNC_T14IE			BIT(0)					 // T14 Overflow Interrupt Enable Control Bit
#define	RTC_ISNC_T14IR			BIT(1)					 // T14 Overflow Interrupt Request Flag
#define	RTC_ISNC_RTC0IE			BIT(2)					 // Section CNTx Interrupt Enable Control Bit
#define	RTC_ISNC_RTC0IR			BIT(3)					 // Section CNTx Interrupt Request Flag
#define	RTC_ISNC_RTC1IE			BIT(4)					 // Section CNTx Interrupt Enable Control Bit
#define	RTC_ISNC_RTC1IR			BIT(5)					 // Section CNTx Interrupt Request Flag
#define	RTC_ISNC_RTC2IE			BIT(6)					 // Section CNTx Interrupt Enable Control Bit
#define	RTC_ISNC_RTC2IR			BIT(7)					 // Section CNTx Interrupt Request Flag
#define	RTC_ISNC_RTC3IE			BIT(8)					 // Section CNTx Interrupt Enable Control Bit
#define	RTC_ISNC_RTC3IR			BIT(9)					 // Section CNTx Interrupt Request Flag
#define	RTC_ISNC_ALARMIE		BIT(10)					 // Alarm Interrupt Enable Control Bit
#define	RTC_ISNC_ALARMIR		BIT(11)					 // Alarm Interrupt Request Flag

#define	RTC_UNK0				MMIO32(RTC_BASE + 0x28)

/* RTC Alarm Register */
#define	RTC_ALARM				MMIO32(RTC_BASE + 0x2C)
#define	RTC_ALARM_VALUE			GENMASK(31, 0)
#define	RTC_ALARM_VALUE_SHIFT	0

/* Service Routing Control Register */
#define	RTC_SRC					MMIO32(RTC_BASE + 0xF0)


// I2C [MOD_NUM=0046, MOD_REV=04, MOD_32BIT=00]
// I2C from TC1100 official public datasheet.
#define	I2C_BASE					0xF4800000
/* Clock Control Register */
#define	I2C_CLC						MMIO32(I2C_BASE + 0x00)

/* Port Input Select Register */
#define	I2C_PISEL					MMIO32(I2C_BASE + 0x04)
#define	I2C_PISEL_SCL_IS0			BIT(0)
#define	I2C_PISEL_SCL_IS1			BIT(1)
#define	I2C_PISEL_SDA_IS0			BIT(4)
#define	I2C_PISEL_SDA_IS1			BIT(5)

/* Module Identifier Register */
#define	I2C_ID						MMIO32(I2C_BASE + 0x08)

/* System Control Register */
#define	I2C_SYSCON					MMIO32(I2C_BASE + 0x10)
#define	I2C_SYSCON_ADR				BIT(0)					 // Bit ADR is set after a start condition in slave mode
#define	I2C_SYSCON_AL				BIT(1)					 // Arbitration Lost
#define	I2C_SYSCON_SLA				BIT(2)					 // The IIC module has been selected as a slave (device address received).
#define	I2C_SYSCON_LRB				BIT(3)					 // Last Received Bit
#define	I2C_SYSCON_BB				BIT(4)					 // Bus Busy
#define	I2C_SYSCON_IRQD				BIT(5)					 // IIC Interrupt Request Bit for Data Transfer Events
#define	I2C_SYSCON_IRQP				BIT(6)					 // IIC Interrupt Request Bit for Protocol Events
#define	I2C_SYSCON_IRQE				BIT(7)					 // IIC Interrupt Request Bit for Data Transmission End
#define	I2C_SYSCON_CO				GENMASK(10, 8)			 // Counter of Transmitted Bytes Since Last Data Interrupt.
#define	I2C_SYSCON_CO_SHIFT			8
#define	I2C_SYSCON_WM				GENMASK(15, 8)			 // Write Mirror (WMEN=1)
#define	I2C_SYSCON_WM_SHIFT			8
#define	I2C_SYSCON_RMEN				BIT(15)					 // Read Mirror Enable
#define	I2C_SYSCON_M10				BIT(16)					 // 10-bit address mode
#define	I2C_SYSCON_RSC				BIT(17)					 // Repeated Start Condition
#define	I2C_SYSCON_MOD				GENMASK(19, 18)			 // Basic Operating Mode
#define	I2C_SYSCON_MOD_SHIFT		18
#define	I2C_SYSCON_MOD_DISABLED		0x0
#define	I2C_SYSCON_MOD_SLAVE		0x40000
#define	I2C_SYSCON_MOD_MASTER		0x80000
#define	I2C_SYSCON_MOD_MULTI_MASTER	0xC0000
#define	I2C_SYSCON_BUM				BIT(20)					 // Busy Master
#define	I2C_SYSCON_ACKDIS			BIT(21)					 // Acknowledge Pulse Disable
#define	I2C_SYSCON_INT				BIT(22)					 // Interrupt Delete Select
#define	I2C_SYSCON_TRX				BIT(23)					 // Transmit Select
#define	I2C_SYSCON_IGE				BIT(24)					 // Ignore IRQE
#define	I2C_SYSCON_RM				GENMASK(31, 24)			 // Read Mirror (RMEN=1)
#define	I2C_SYSCON_RM_SHIFT			24
#define	I2C_SYSCON_STP				BIT(25)					 // Stop Master
#define	I2C_SYSCON_CI				GENMASK(27, 26)			 // Length of the Receive/Transmit Buffer
#define	I2C_SYSCON_CI_SHIFT			26
#define	I2C_SYSCON_CI_1				0x0
#define	I2C_SYSCON_CI_2				0x4000000
#define	I2C_SYSCON_CI_3				0x8000000
#define	I2C_SYSCON_CI_4				0xC000000
#define	I2C_SYSCON_WMEN				BIT(31)					 // Write Mirror Enable

/* Bus Control Register */
#define	I2C_BUSCON					MMIO32(I2C_BASE + 0x14)
#define	I2C_BUSCON_SDAEN0			BIT(0)					 // Enable Input for Data Pin 0
#define	I2C_BUSCON_SDAEN1			BIT(1)					 // Enable Input for Data Pin 1
#define	I2C_BUSCON_SCLEN0			BIT(4)					 // Enable Input for Clock Pin 0
#define	I2C_BUSCON_SCLEN1			BIT(5)					 // Enable Input for Clock Pin 1
#define	I2C_BUSCON_BRP				GENMASK(15, 8)			 // Baud Rate Prescaler
#define	I2C_BUSCON_BRP_SHIFT		8
#define	I2C_BUSCON_ICA				GENMASK(25, 16)			 // Node Address
#define	I2C_BUSCON_ICA_SHIFT		16
#define	I2C_BUSCON_PREDIV			GENMASK(30, 29)			 // Pre Divider for Baud Rate Generation
#define	I2C_BUSCON_PREDIV_SHIFT		29
#define	I2C_BUSCON_PREDIV_1			0x0
#define	I2C_BUSCON_PREDIV_8			0x20000000
#define	I2C_BUSCON_PREDIV_64		0x40000000
#define	I2C_BUSCON_BRPMOD			BIT(31)					 // Baud Rate Prescaler Mode
#define	I2C_BUSCON_BRPMOD_MODE0		0x0
#define	I2C_BUSCON_BRPMOD_MODE1		0x80000000

/* Receive Transmit Buffer */
#define	I2C_RTB						MMIO32(I2C_BASE + 0x18)
#define	I2C_RTB_BYTE0				GENMASK(7, 0)
#define	I2C_RTB_BYTE0_SHIFT			0
#define	I2C_RTB_BYTE1				GENMASK(15, 8)
#define	I2C_RTB_BYTE1_SHIFT			8
#define	I2C_RTB_BYTE2				GENMASK(23, 16)
#define	I2C_RTB_BYTE2_SHIFT			16
#define	I2C_RTB_BYTE3				GENMASK(31, 24)
#define	I2C_RTB_BYTE3_SHIFT			24

/* Write Hardware Bits Control Register */
#define	I2C_WHBSYSCON				MMIO32(I2C_BASE + 0x20)
#define	I2C_WHBSYSCON_CLRAL			BIT(1)					 // Clear Arbitration Lost Bit
#define	I2C_WHBSYSCON_SETAL			BIT(2)					 // Set Arbitration Lost Bit
#define	I2C_WHBSYSCON_CLRIRQD		BIT(5)					 // Clear IIC Interrupt Request Bit for Data Transfer Events Bit
#define	I2C_WHBSYSCON_CLRIRQP		BIT(6)					 // Clear IIC Interrupt Request Bit for Protocol Events Bit
#define	I2C_WHBSYSCON_CLRIRQE		BIT(7)					 // Clear IIC Interrupt Request Bit for Data Transmission End Bit
#define	I2C_WHBSYSCON_SETIRQD		BIT(8)					 // Set IIC Interrupt Request Bit for Data Transfer Events Bit
#define	I2C_WHBSYSCON_SETIRQP		BIT(9)					 // Set IIC Interrupt Request Bit for Protocol Events Bit
#define	I2C_WHBSYSCON_SETIRQE		BIT(10)					 // Set IIC Interrupt Request Bit for Data Transmission End Bit
#define	I2C_WHBSYSCON_CLRRMEN		BIT(14)					 // Clear Read Mirror Enable Bit
#define	I2C_WHBSYSCON_SETRMEN		BIT(15)					 // Set Read Mirror Enable Bit
#define	I2C_WHBSYSCON_CLRRSC		BIT(16)					 // Clear Repeated Start Condition Bit
#define	I2C_WHBSYSCON_SETRSC		BIT(17)					 // Set Repeated Start Condition Bit
#define	I2C_WHBSYSCON_CLRBUM		BIT(19)					 // Clear Busy Master Bit
#define	I2C_WHBSYSCON_SETBUM		BIT(20)					 // Set Busy Master Bit
#define	I2C_WHBSYSCON_CLRACKDIS		BIT(21)					 // Clear Acknowledge Pulse Disable Bit
#define	I2C_WHBSYSCON_SETACKDIS		BIT(22)					 // Set Acknowledge Pulse Disable Bit
#define	I2C_WHBSYSCON_CLRTRX		BIT(23)					 // Clear Transmit Select Bit
#define	I2C_WHBSYSCON_SETTRX		BIT(24)					 // Set Transmit Select Bit
#define	I2C_WHBSYSCON_CLRSTP		BIT(25)					 // Clear Stop Master Bit
#define	I2C_WHBSYSCON_SETSTP		BIT(26)					 // Set Stop Master Bit
#define	I2C_WHBSYSCON_CLRWMEN		BIT(30)					 // Set Write Mirror Enable Bit
#define	I2C_WHBSYSCON_SETWMEN		BIT(31)					 // Clear Write Mirror Enable Bit

/* Service Routing Control Register */
#define	I2C_END_SRC					MMIO32(I2C_BASE + 0xF4)

/* Service Routing Control Register */
#define	I2C_PROTO_SRC				MMIO32(I2C_BASE + 0xF8)

/* Service Routing Control Register */
#define	I2C_DATA_SRC				MMIO32(I2C_BASE + 0xFC)


// GPTU0 [MOD_NUM=0001, MOD_REV=02, MOD_32BIT=C0]
// GPTU from Tricore TC1765 official public datasheet
#define	GPTU0_BASE							0xF4900000
#define	GPTU0								0xF4900000

#define	GPTU1_BASE							0xF4A00000
#define	GPTU1								0xF4A00000

/* Clock Control Register */
#define	GPTU_CLC(base)						MMIO32((base) + 0x00)

/* Module Identifier Register */
#define	GPTU_ID(base)						MMIO32((base) + 0x08)

#define	GPTU_T01IRS(base)					MMIO32((base) + 0x10)
#define	GPTU_T01IRS_T0AINS					GENMASK(1, 0)						 // T0A Input Selection
#define	GPTU_T01IRS_T0AINS_SHIFT			0
#define	GPTU_T01IRS_T0AINS_BYPASS			0x0
#define	GPTU_T01IRS_T0AINS_CNT0				0x1
#define	GPTU_T01IRS_T0AINS_CNT1				0x2
#define	GPTU_T01IRS_T0AINS_CONCAT			0x3
#define	GPTU_T01IRS_T0BINS					GENMASK(3, 2)						 // T0B Input Selection
#define	GPTU_T01IRS_T0BINS_SHIFT			2
#define	GPTU_T01IRS_T0BINS_BYPASS			0x0
#define	GPTU_T01IRS_T0BINS_CNT0				0x4
#define	GPTU_T01IRS_T0BINS_CNT1				0x8
#define	GPTU_T01IRS_T0BINS_CONCAT			0xC
#define	GPTU_T01IRS_T0CINS					GENMASK(5, 4)						 // T0C Input Selection
#define	GPTU_T01IRS_T0CINS_SHIFT			4
#define	GPTU_T01IRS_T0CINS_BYPASS			0x0
#define	GPTU_T01IRS_T0CINS_CNT0				0x10
#define	GPTU_T01IRS_T0CINS_CNT1				0x20
#define	GPTU_T01IRS_T0CINS_CONCAT			0x30
#define	GPTU_T01IRS_T0DINS					GENMASK(7, 6)						 // T0D Input Selection
#define	GPTU_T01IRS_T0DINS_SHIFT			6
#define	GPTU_T01IRS_T0DINS_BYPASS			0x0
#define	GPTU_T01IRS_T0DINS_CNT0				0x40
#define	GPTU_T01IRS_T0DINS_CNT1				0x80
#define	GPTU_T01IRS_T0DINS_CONCAT			0xC0
#define	GPTU_T01IRS_T1AINS					GENMASK(9, 8)						 // T1A Input Selection
#define	GPTU_T01IRS_T1AINS_SHIFT			8
#define	GPTU_T01IRS_T1AINS_BYPASS			0x0
#define	GPTU_T01IRS_T1AINS_CNT0				0x100
#define	GPTU_T01IRS_T1AINS_CNT1				0x200
#define	GPTU_T01IRS_T1AINS_CONCAT			0x300
#define	GPTU_T01IRS_T1BINS					GENMASK(11, 10)						 // T1B Input Selection
#define	GPTU_T01IRS_T1BINS_SHIFT			10
#define	GPTU_T01IRS_T1BINS_BYPASS			0x0
#define	GPTU_T01IRS_T1BINS_CNT0				0x400
#define	GPTU_T01IRS_T1BINS_CNT1				0x800
#define	GPTU_T01IRS_T1BINS_CONCAT			0xC00
#define	GPTU_T01IRS_T1CINS					GENMASK(13, 12)						 // T1C Input Selection
#define	GPTU_T01IRS_T1CINS_SHIFT			12
#define	GPTU_T01IRS_T1CINS_BYPASS			0x0
#define	GPTU_T01IRS_T1CINS_CNT0				0x1000
#define	GPTU_T01IRS_T1CINS_CNT1				0x2000
#define	GPTU_T01IRS_T1CINS_CONCAT			0x3000
#define	GPTU_T01IRS_T1DINS					GENMASK(15, 14)						 // T1D Input Selection
#define	GPTU_T01IRS_T1DINS_SHIFT			14
#define	GPTU_T01IRS_T1DINS_BYPASS			0x0
#define	GPTU_T01IRS_T1DINS_CNT0				0x4000
#define	GPTU_T01IRS_T1DINS_CNT1				0x8000
#define	GPTU_T01IRS_T1DINS_CONCAT			0xC000
#define	GPTU_T01IRS_T0AREL					BIT(16)								 // T0A Reload Source Selection
#define	GPTU_T01IRS_T0BREL					BIT(17)								 // T0B Reload Source Selection
#define	GPTU_T01IRS_T0CREL					BIT(18)								 // T0C Reload Source Selection
#define	GPTU_T01IRS_T0DREL					BIT(19)								 // T0D Reload Source Selection
#define	GPTU_T01IRS_T1AREL					BIT(20)								 // T1A Reload Source Selection
#define	GPTU_T01IRS_T1BREL					BIT(21)								 // T1B Reload Source Selection
#define	GPTU_T01IRS_T1CREL					BIT(22)								 // T1C Reload Source Selection
#define	GPTU_T01IRS_T1DREL					BIT(23)								 // T1D Reload Source Selection
#define	GPTU_T01IRS_T0INC					BIT(24)								 // T0 Carry Input Selection
#define	GPTU_T01IRS_T1INC					BIT(25)								 // T1 Carry Input Selection
#define	GPTU_T01IRS_T01IN0					GENMASK(29, 28)						 // T0 and T1 Global Input CNT0 Selection
#define	GPTU_T01IRS_T01IN0_SHIFT			28
#define	GPTU_T01IRS_T01IN0_OUV_T2A			0x0
#define	GPTU_T01IRS_T01IN0_POS_IN0			0x10000000
#define	GPTU_T01IRS_T01IN0_NEG_IN0			0x20000000
#define	GPTU_T01IRS_T01IN0_BOTH_IN0			0x30000000
#define	GPTU_T01IRS_T01IN1					GENMASK(31, 30)						 // T0 and T1 Global Input CNT1 Selection
#define	GPTU_T01IRS_T01IN1_SHIFT			30
#define	GPTU_T01IRS_T01IN1_OUV_T2B			0x0
#define	GPTU_T01IRS_T01IN1_POS_IN1			0x40000000
#define	GPTU_T01IRS_T01IN1_NEG_IN1			0x80000000
#define	GPTU_T01IRS_T01IN1_BOTH_IN1			0xC0000000

#define	GPTU_T01OTS(base)					MMIO32((base) + 0x14)
#define	GPTU_T01OTS_SOUT00					GENMASK(1, 0)						 // T0 Output 0 Source Selection
#define	GPTU_T01OTS_SOUT00_SHIFT			0
#define	GPTU_T01OTS_SOUT00_A				0x0
#define	GPTU_T01OTS_SOUT00_B				0x1
#define	GPTU_T01OTS_SOUT00_C				0x2
#define	GPTU_T01OTS_SOUT00_D				0x3
#define	GPTU_T01OTS_SOUT01					GENMASK(3, 2)						 // T0 Output 1 Source Selection
#define	GPTU_T01OTS_SOUT01_SHIFT			2
#define	GPTU_T01OTS_SOUT01_A				0x0
#define	GPTU_T01OTS_SOUT01_B				0x4
#define	GPTU_T01OTS_SOUT01_C				0x8
#define	GPTU_T01OTS_SOUT01_D				0xC
#define	GPTU_T01OTS_STRG00					GENMASK(5, 4)						 // T0 Trigger Output 0 Source Selection
#define	GPTU_T01OTS_STRG00_SHIFT			4
#define	GPTU_T01OTS_STRG00_A				0x0
#define	GPTU_T01OTS_STRG00_B				0x10
#define	GPTU_T01OTS_STRG00_C				0x20
#define	GPTU_T01OTS_STRG00_D				0x30
#define	GPTU_T01OTS_STRG01					GENMASK(7, 6)						 // T0 Trigger Output 1 Source Selection
#define	GPTU_T01OTS_STRG01_SHIFT			6
#define	GPTU_T01OTS_STRG01_A				0x0
#define	GPTU_T01OTS_STRG01_B				0x40
#define	GPTU_T01OTS_STRG01_C				0x80
#define	GPTU_T01OTS_STRG01_D				0xC0
#define	GPTU_T01OTS_SSR00					GENMASK(9, 8)						 // T0 Service Request 0 Source Selection
#define	GPTU_T01OTS_SSR00_SHIFT				8
#define	GPTU_T01OTS_SSR00_A					0x0
#define	GPTU_T01OTS_SSR00_B					0x100
#define	GPTU_T01OTS_SSR00_C					0x200
#define	GPTU_T01OTS_SSR00_D					0x300
#define	GPTU_T01OTS_SSR01					GENMASK(11, 10)						 // T0 Service Request 1 Source Selection
#define	GPTU_T01OTS_SSR01_SHIFT				10
#define	GPTU_T01OTS_SSR01_A					0x0
#define	GPTU_T01OTS_SSR01_B					0x400
#define	GPTU_T01OTS_SSR01_C					0x800
#define	GPTU_T01OTS_SSR01_D					0xC00
#define	GPTU_T01OTS_SOUT10					GENMASK(17, 16)						 // T1 Output 0 Source Selection
#define	GPTU_T01OTS_SOUT10_SHIFT			16
#define	GPTU_T01OTS_SOUT10_A				0x0
#define	GPTU_T01OTS_SOUT10_B				0x10000
#define	GPTU_T01OTS_SOUT10_C				0x20000
#define	GPTU_T01OTS_SOUT10_D				0x30000
#define	GPTU_T01OTS_SOUT11					GENMASK(19, 18)						 // T1 Output 1 Source Selection
#define	GPTU_T01OTS_SOUT11_SHIFT			18
#define	GPTU_T01OTS_SOUT11_A				0x0
#define	GPTU_T01OTS_SOUT11_B				0x40000
#define	GPTU_T01OTS_SOUT11_C				0x80000
#define	GPTU_T01OTS_SOUT11_D				0xC0000
#define	GPTU_T01OTS_STRG10					GENMASK(21, 20)						 // T1 Trigger Output 0 Source Selection
#define	GPTU_T01OTS_STRG10_SHIFT			20
#define	GPTU_T01OTS_STRG10_A				0x0
#define	GPTU_T01OTS_STRG10_B				0x100000
#define	GPTU_T01OTS_STRG10_C				0x200000
#define	GPTU_T01OTS_STRG10_D				0x300000
#define	GPTU_T01OTS_STRG11					GENMASK(23, 22)						 // T1 Trigger Output 1 Source Selection
#define	GPTU_T01OTS_STRG11_SHIFT			22
#define	GPTU_T01OTS_STRG11_A				0x0
#define	GPTU_T01OTS_STRG11_B				0x400000
#define	GPTU_T01OTS_STRG11_C				0x800000
#define	GPTU_T01OTS_STRG11_D				0xC00000
#define	GPTU_T01OTS_SSR10					GENMASK(25, 24)						 // T1 Service Request 0 Source Selection
#define	GPTU_T01OTS_SSR10_SHIFT				24
#define	GPTU_T01OTS_SSR10_A					0x0
#define	GPTU_T01OTS_SSR10_B					0x1000000
#define	GPTU_T01OTS_SSR10_C					0x2000000
#define	GPTU_T01OTS_SSR10_D					0x3000000
#define	GPTU_T01OTS_SSR11					GENMASK(27, 26)						 // T1 Service Request 1 Source Selection.
#define	GPTU_T01OTS_SSR11_SHIFT				26
#define	GPTU_T01OTS_SSR11_A					0x0
#define	GPTU_T01OTS_SSR11_B					0x4000000
#define	GPTU_T01OTS_SSR11_C					0x8000000
#define	GPTU_T01OTS_SSR11_D					0xC000000

#define	GPTU_T2CON(base)					MMIO32((base) + 0x18)
#define	GPTU_T2CON_T2ACSRC					GENMASK(1, 0)						 // Timer T2A Count Input Source Control
#define	GPTU_T2CON_T2ACSRC_SHIFT			0
#define	GPTU_T2CON_T2ACSRC_BYPASS			0x0
#define	GPTU_T2CON_T2ACSRC_EXT_COUNT		0x1
#define	GPTU_T2CON_T2ACSRC_QUADRATURE		0x2
#define	GPTU_T2CON_T2ACDIR					GENMASK(3, 2)						 // Timer T2A Direction Control
#define	GPTU_T2CON_T2ACDIR_SHIFT			2
#define	GPTU_T2CON_T2ACDIR_COUNT_UP			0x0
#define	GPTU_T2CON_T2ACDIR_COUNT_DOWN		0x4
#define	GPTU_T2CON_T2ACDIR_EXT_CONT_UP		0x8
#define	GPTU_T2CON_T2ACDIR_EXT_COUNT_DOWN	0xC
#define	GPTU_T2CON_T2ACCLR					GENMASK(5, 4)						 // Timer T2A Clear Control
#define	GPTU_T2CON_T2ACCLR_SHIFT			4
#define	GPTU_T2CON_T2ACCLR_EXT				0x0
#define	GPTU_T2CON_T2ACCLR_CP0_T2			0x10
#define	GPTU_T2CON_T2ACCLR_CP1_T2			0x20
#define	GPTU_T2CON_T2ACOV					GENMASK(7, 6)						 // Timer T2A Overflow/Underflow Generation Control
#define	GPTU_T2CON_T2ACOV_SHIFT				6
#define	GPTU_T2CON_T2ACOV_MODE0				0x0
#define	GPTU_T2CON_T2ACOV_MODE1				0x40
#define	GPTU_T2CON_T2ACOV_MODE2				0x80
#define	GPTU_T2CON_T2ACOV_MODE3				0xC0
#define	GPTU_T2CON_T2ACOS					BIT(8)								 // Timer T2A One-Shot Control.
#define	GPTU_T2CON_T2ADIR					BIT(12)								 // Timer T2A Direction Status Bit.
#define	GPTU_T2CON_T2ADIR_COUNT_UP			0x0
#define	GPTU_T2CON_T2ADIR_COUNT_DOWN		0x1000
#define	GPTU_T2CON_T2SPLIT					BIT(15)								 // Timer T2 Split Control.
#define	GPTU_T2CON_T2BCSRC					GENMASK(17, 16)						 // Timer T2B Count Input Source Control.
#define	GPTU_T2CON_T2BCSRC_SHIFT			16
#define	GPTU_T2CON_T2BCSRC_BYPASS			0x0
#define	GPTU_T2CON_T2BCSRC_EXT_COUNT		0x10000
#define	GPTU_T2CON_T2BCSRC_QUADRATURE		0x20000
#define	GPTU_T2CON_T2BCDIR					GENMASK(19, 18)						 // Timer T2B Direction Control.
#define	GPTU_T2CON_T2BCDIR_SHIFT			18
#define	GPTU_T2CON_T2BCDIR_COUNT_UP			0x0
#define	GPTU_T2CON_T2BCDIR_COUNT_DOWN		0x40000
#define	GPTU_T2CON_T2BCDIR_EXT_CONT_UP		0x80000
#define	GPTU_T2CON_T2BCDIR_EXT_COUNT_DOWN	0xC0000
#define	GPTU_T2CON_T2BCCLR					GENMASK(21, 20)						 // Timer T2B Clear Control.
#define	GPTU_T2CON_T2BCCLR_SHIFT			20
#define	GPTU_T2CON_T2BCCLR_EXT				0x0
#define	GPTU_T2CON_T2BCCLR_CP0_T2			0x100000
#define	GPTU_T2CON_T2BCCLR_CP1_T2			0x200000
#define	GPTU_T2CON_T2BCOV					GENMASK(23, 22)						 // Timer T2B Overflow/Underflow Generation Control.
#define	GPTU_T2CON_T2BCOV_SHIFT				22
#define	GPTU_T2CON_T2BCOV_MODE0				0x0
#define	GPTU_T2CON_T2BCOV_MODE1				0x400000
#define	GPTU_T2CON_T2BCOV_MODE2				0x800000
#define	GPTU_T2CON_T2BCOV_MODE3				0xC00000
#define	GPTU_T2CON_T2BCOS					BIT(24)								 // Timer T2B One-Shot Control.
#define	GPTU_T2CON_T2BDIR					BIT(28)								 // Timer T2B Direction Status Bit.
#define	GPTU_T2CON_T2BDIR_COUNT_UP			0x0
#define	GPTU_T2CON_T2BDIR_COUNT_DOWN		0x10000000

#define	GPTU_T2RCCON(base)					MMIO32((base) + 0x1C)
#define	GPTU_T2RCCON_T2AMRC0				GENMASK(2, 0)						 // Timer T2A Reload/Capture 0 Mode Control
#define	GPTU_T2RCCON_T2AMRC0_SHIFT			0
#define	GPTU_T2RCCON_T2AMRC1				GENMASK(6, 4)						 // Timer T2A Reload/Capture 1 Mode Control
#define	GPTU_T2RCCON_T2AMRC1_SHIFT			4
#define	GPTU_T2RCCON_T2BMRC0				GENMASK(18, 16)						 // Timer T2B Reload/Capture 0 Mode Control
#define	GPTU_T2RCCON_T2BMRC0_SHIFT			16
#define	GPTU_T2RCCON_T2BMRC1				GENMASK(22, 20)						 // Timer T2B Reload/Capture 1 Mode Control
#define	GPTU_T2RCCON_T2BMRC1_SHIFT			20

#define	GPTU_T2AIS(base)					MMIO32((base) + 0x20)
#define	GPTU_T2AIS_T2AICNT					GENMASK(2, 0)						 // Timer T2A External Count Input Selection
#define	GPTU_T2AIS_T2AICNT_SHIFT			0
#define	GPTU_T2AIS_T2AISTR					GENMASK(6, 4)						 // Timer T2A External Start Input Selection
#define	GPTU_T2AIS_T2AISTR_SHIFT			4
#define	GPTU_T2AIS_T2AISTP					GENMASK(10, 8)						 // Timer T2A External Stop Input Selection
#define	GPTU_T2AIS_T2AISTP_SHIFT			8
#define	GPTU_T2AIS_T2AIUD					GENMASK(14, 12)						 // Timer T2A External Up/Down Input Selection
#define	GPTU_T2AIS_T2AIUD_SHIFT				12
#define	GPTU_T2AIS_T2AICLR					GENMASK(18, 16)						 // Timer T2A External Clear Input Selection
#define	GPTU_T2AIS_T2AICLR_SHIFT			16
#define	GPTU_T2AIS_T2AIRC0					GENMASK(22, 20)						 // Timer T2A External Reload/Capture 0 Input Selection
#define	GPTU_T2AIS_T2AIRC0_SHIFT			20
#define	GPTU_T2AIS_T2AIRC1					GENMASK(26, 24)						 // Timer T2A External Reload/Capture 1 Input Selection
#define	GPTU_T2AIS_T2AIRC1_SHIFT			24

#define	GPTU_T2BIS(base)					MMIO32((base) + 0x24)
#define	GPTU_T2BIS_T2BICNT					GENMASK(2, 0)						 // Timer T2B External Count Input Selection
#define	GPTU_T2BIS_T2BICNT_SHIFT			0
#define	GPTU_T2BIS_T2BISTR					GENMASK(6, 4)						 // Timer T2B External Start Input Selection
#define	GPTU_T2BIS_T2BISTR_SHIFT			4
#define	GPTU_T2BIS_T2BISTP					GENMASK(10, 8)						 // Timer T2B External Stop Input Selection
#define	GPTU_T2BIS_T2BISTP_SHIFT			8
#define	GPTU_T2BIS_T2BIUD					GENMASK(14, 12)						 // Timer T2B External Up/Down Input Selection
#define	GPTU_T2BIS_T2BIUD_SHIFT				12
#define	GPTU_T2BIS_T2BICLR					GENMASK(18, 16)						 // Timer T2B External Clear Input Selection
#define	GPTU_T2BIS_T2BICLR_SHIFT			16
#define	GPTU_T2BIS_T2BIRC0					GENMASK(22, 20)						 // Timer T2B External Reload/Capture 0 Input Selection
#define	GPTU_T2BIS_T2BIRC0_SHIFT			20
#define	GPTU_T2BIS_T2BIRC1					GENMASK(26, 24)						 // Timer T2B External Reload/Capture 1 Input Selection
#define	GPTU_T2BIS_T2BIRC1_SHIFT			24

#define	GPTU_T2ES(base)						MMIO32((base) + 0x28)
#define	GPTU_T2ES_T2AECNT					GENMASK(1, 0)						 // Timer T2A External Count Input Active Edge Selection
#define	GPTU_T2ES_T2AECNT_SHIFT				0
#define	GPTU_T2ES_T2AESTR					GENMASK(3, 2)						 // Timer T2A External Start Input Active Edge Selection
#define	GPTU_T2ES_T2AESTR_SHIFT				2
#define	GPTU_T2ES_T2AESTP					GENMASK(5, 4)						 // Timer T2A External Stop Input Active Edge Selection
#define	GPTU_T2ES_T2AESTP_SHIFT				4
#define	GPTU_T2ES_T2AEUD					GENMASK(7, 6)						 // Timer T2A External Up/Down Input Active Edge Selection
#define	GPTU_T2ES_T2AEUD_SHIFT				6
#define	GPTU_T2ES_T2AECLR					GENMASK(9, 8)						 // Timer T2A External Clear Input Active Edge Selection
#define	GPTU_T2ES_T2AECLR_SHIFT				8
#define	GPTU_T2ES_T2AERC0					GENMASK(11, 10)						 // Timer T2A External Reload/Capture 0 Input Active Edge Selection
#define	GPTU_T2ES_T2AERC0_SHIFT				10
#define	GPTU_T2ES_T2AERC1					GENMASK(13, 12)						 // Timer T2A External Reload/Capture 1 Input Active Edge Selection
#define	GPTU_T2ES_T2AERC1_SHIFT				12
#define	GPTU_T2ES_T2BECNT					GENMASK(17, 16)						 // Timer T2B External Count Input Active Edge Selection
#define	GPTU_T2ES_T2BECNT_SHIFT				16
#define	GPTU_T2ES_T2BESTR					GENMASK(19, 18)						 // Timer T2B External Start Input Active Edge Selection
#define	GPTU_T2ES_T2BESTR_SHIFT				18
#define	GPTU_T2ES_T2BESTP					GENMASK(21, 20)						 // Timer T2B External Stop Input Active Edge Selection
#define	GPTU_T2ES_T2BESTP_SHIFT				20
#define	GPTU_T2ES_T2BEUD					GENMASK(23, 22)						 // Timer T2B External Up/Down Input Active Edge Selection
#define	GPTU_T2ES_T2BEUD_SHIFT				22
#define	GPTU_T2ES_T2BECLR					GENMASK(25, 24)						 // Timer T2B External Clear Input Active Edge Selection
#define	GPTU_T2ES_T2BECLR_SHIFT				24
#define	GPTU_T2ES_T2BERC0					GENMASK(27, 26)						 // Timer T2B External Reload/Capture 0 Input Active Edge Selection
#define	GPTU_T2ES_T2BERC0_SHIFT				26
#define	GPTU_T2ES_T2BERC1					GENMASK(29, 28)						 // Timer T2B External Reload/Capture 1 Input Active Edge Selection
#define	GPTU_T2ES_T2BERC1_SHIFT				28

#define	GPTU_OSEL(base)						MMIO32((base) + 0x2C)
#define	GPTU_OSEL_SO0						GENMASK(2, 0)						 // GPTU Output 0 Source Selection
#define	GPTU_OSEL_SO0_SHIFT					0
#define	GPTU_OSEL_SO0_OUT00					0x0
#define	GPTU_OSEL_SO0_OUT01					0x1
#define	GPTU_OSEL_SO0_OUT10					0x2
#define	GPTU_OSEL_SO0_OUT11					0x3
#define	GPTU_OSEL_SO0_OUV_T2A				0x4
#define	GPTU_OSEL_SO0_OUV_T2B				0x5
#define	GPTU_OSEL_SO0_UNK0					0x6
#define	GPTU_OSEL_SO0_UNK1					0x7
#define	GPTU_OSEL_SO1						GENMASK(6, 4)						 // GPTU Output 1 Source Selection
#define	GPTU_OSEL_SO1_SHIFT					4
#define	GPTU_OSEL_SO1_OUT00					0x0
#define	GPTU_OSEL_SO1_OUT01					0x10
#define	GPTU_OSEL_SO1_OUT10					0x20
#define	GPTU_OSEL_SO1_OUT11					0x30
#define	GPTU_OSEL_SO1_OUV_T2A				0x40
#define	GPTU_OSEL_SO1_OUV_T2B				0x50
#define	GPTU_OSEL_SO1_UNK0					0x60
#define	GPTU_OSEL_SO1_UNK1					0x70
#define	GPTU_OSEL_SO2						GENMASK(10, 8)						 // GPTU Output 2 Source Selection
#define	GPTU_OSEL_SO2_SHIFT					8
#define	GPTU_OSEL_SO2_OUT00					0x0
#define	GPTU_OSEL_SO2_OUT01					0x100
#define	GPTU_OSEL_SO2_OUT10					0x200
#define	GPTU_OSEL_SO2_OUT11					0x300
#define	GPTU_OSEL_SO2_OUV_T2A				0x400
#define	GPTU_OSEL_SO2_OUV_T2B				0x500
#define	GPTU_OSEL_SO2_UNK0					0x600
#define	GPTU_OSEL_SO2_UNK1					0x700
#define	GPTU_OSEL_SO3						GENMASK(14, 12)						 // GPTU Output 3 Source Selection
#define	GPTU_OSEL_SO3_SHIFT					12
#define	GPTU_OSEL_SO3_OUT00					0x0
#define	GPTU_OSEL_SO3_OUT01					0x1000
#define	GPTU_OSEL_SO3_OUT10					0x2000
#define	GPTU_OSEL_SO3_OUT11					0x3000
#define	GPTU_OSEL_SO3_OUV_T2A				0x4000
#define	GPTU_OSEL_SO3_OUV_T2B				0x5000
#define	GPTU_OSEL_SO3_UNK0					0x6000
#define	GPTU_OSEL_SO3_UNK1					0x7000
#define	GPTU_OSEL_SO4						GENMASK(18, 16)						 // GPTU Output 4 Source Selection
#define	GPTU_OSEL_SO4_SHIFT					16
#define	GPTU_OSEL_SO4_OUT00					0x0
#define	GPTU_OSEL_SO4_OUT01					0x10000
#define	GPTU_OSEL_SO4_OUT10					0x20000
#define	GPTU_OSEL_SO4_OUT11					0x30000
#define	GPTU_OSEL_SO4_OUV_T2A				0x40000
#define	GPTU_OSEL_SO4_OUV_T2B				0x50000
#define	GPTU_OSEL_SO4_UNK0					0x60000
#define	GPTU_OSEL_SO4_UNK1					0x70000
#define	GPTU_OSEL_SO5						GENMASK(22, 20)						 // GPTU Output 5 Source Selection
#define	GPTU_OSEL_SO5_SHIFT					20
#define	GPTU_OSEL_SO5_OUT00					0x0
#define	GPTU_OSEL_SO5_OUT01					0x100000
#define	GPTU_OSEL_SO5_OUT10					0x200000
#define	GPTU_OSEL_SO5_OUT11					0x300000
#define	GPTU_OSEL_SO5_OUV_T2A				0x400000
#define	GPTU_OSEL_SO5_OUV_T2B				0x500000
#define	GPTU_OSEL_SO5_UNK0					0x600000
#define	GPTU_OSEL_SO5_UNK1					0x700000
#define	GPTU_OSEL_SO6						GENMASK(26, 24)						 // GPTU Output 6 Source Selection
#define	GPTU_OSEL_SO6_SHIFT					24
#define	GPTU_OSEL_SO6_OUT00					0x0
#define	GPTU_OSEL_SO6_OUT01					0x1000000
#define	GPTU_OSEL_SO6_OUT10					0x2000000
#define	GPTU_OSEL_SO6_OUT11					0x3000000
#define	GPTU_OSEL_SO6_OUV_T2A				0x4000000
#define	GPTU_OSEL_SO6_OUV_T2B				0x5000000
#define	GPTU_OSEL_SO6_UNK0					0x6000000
#define	GPTU_OSEL_SO6_UNK1					0x7000000
#define	GPTU_OSEL_SO7						GENMASK(30, 28)						 // GPTU Output 7 Source Selection
#define	GPTU_OSEL_SO7_SHIFT					28
#define	GPTU_OSEL_SO7_OUT00					0x0
#define	GPTU_OSEL_SO7_OUT01					0x10000000
#define	GPTU_OSEL_SO7_OUT10					0x20000000
#define	GPTU_OSEL_SO7_OUT11					0x30000000
#define	GPTU_OSEL_SO7_OUV_T2A				0x40000000
#define	GPTU_OSEL_SO7_OUV_T2B				0x50000000
#define	GPTU_OSEL_SO7_UNK0					0x60000000
#define	GPTU_OSEL_SO7_UNK1					0x70000000

#define	GPTU_OUT(base)						MMIO32((base) + 0x30)
#define	GPTU_OUT_OUT0						BIT(0)								 // GPTU Output State Bit 0
#define	GPTU_OUT_OUT1						BIT(1)								 // GPTU Output State Bit 1
#define	GPTU_OUT_OUT2						BIT(2)								 // GPTU Output State Bit 2
#define	GPTU_OUT_OUT3						BIT(3)								 // GPTU Output State Bit 3
#define	GPTU_OUT_OUT4						BIT(4)								 // GPTU Output State Bit 4
#define	GPTU_OUT_OUT5						BIT(5)								 // GPTU Output State Bit 5
#define	GPTU_OUT_OUT6						BIT(6)								 // GPTU Output State Bit 6
#define	GPTU_OUT_OUT7						BIT(7)								 // GPTU Output State Bit 7
#define	GPTU_OUT_CLRO0						BIT(8)								 // GPTU Output 0 Clear Bit
#define	GPTU_OUT_CLRO1						BIT(9)								 // GPTU Output 1 Clear Bit
#define	GPTU_OUT_CLRO2						BIT(10)								 // GPTU Output 2 Clear Bit
#define	GPTU_OUT_CLRO3						BIT(11)								 // GPTU Output 3 Clear Bit
#define	GPTU_OUT_CLRO4						BIT(12)								 // GPTU Output 4 Clear Bit
#define	GPTU_OUT_CLRO5						BIT(13)								 // GPTU Output 5 Clear Bit
#define	GPTU_OUT_CLRO6						BIT(14)								 // GPTU Output 6 Clear Bit
#define	GPTU_OUT_CLRO7						BIT(15)								 // GPTU Output 7 Clear Bit
#define	GPTU_OUT_SETO0						BIT(16)								 // GPTU Output 0 Set Bit
#define	GPTU_OUT_SETO1						BIT(17)								 // GPTU Output 1 Set Bit
#define	GPTU_OUT_SETO2						BIT(18)								 // GPTU Output 2 Set Bit
#define	GPTU_OUT_SETO3						BIT(19)								 // GPTU Output 3 Set Bit
#define	GPTU_OUT_SETO4						BIT(20)								 // GPTU Output 4 Set Bit
#define	GPTU_OUT_SETO5						BIT(21)								 // GPTU Output 5 Set Bit
#define	GPTU_OUT_SETO6						BIT(22)								 // GPTU Output 6 Set Bit
#define	GPTU_OUT_SETO7						BIT(23)								 // GPTU Output 7 Set Bit

/* T0 Count register (32 bit) */
#define	GPTU_T0DCBA(base)					MMIO32((base) + 0x34)
#define	GPTU_T0DCBA_T0A						GENMASK(7, 0)
#define	GPTU_T0DCBA_T0A_SHIFT				0
#define	GPTU_T0DCBA_T0B						GENMASK(15, 8)
#define	GPTU_T0DCBA_T0B_SHIFT				8
#define	GPTU_T0DCBA_T0C						GENMASK(23, 16)
#define	GPTU_T0DCBA_T0C_SHIFT				16
#define	GPTU_T0DCBA_T0D						GENMASK(31, 24)
#define	GPTU_T0DCBA_T0D_SHIFT				24

/* T0 Count register (24 bit) */
#define	GPTU_T0CBA(base)					MMIO32((base) + 0x38)
#define	GPTU_T0CBA_T0A						GENMASK(7, 0)
#define	GPTU_T0CBA_T0A_SHIFT				0
#define	GPTU_T0CBA_T0B						GENMASK(15, 8)
#define	GPTU_T0CBA_T0B_SHIFT				8
#define	GPTU_T0CBA_T0C						GENMASK(23, 16)
#define	GPTU_T0CBA_T0C_SHIFT				16

/* T0 Reload register (32 bit) */
#define	GPTU_T0RDCBA(base)					MMIO32((base) + 0x3C)
#define	GPTU_T0RDCBA_T0RA					GENMASK(7, 0)
#define	GPTU_T0RDCBA_T0RA_SHIFT				0
#define	GPTU_T0RDCBA_T0RB					GENMASK(15, 8)
#define	GPTU_T0RDCBA_T0RB_SHIFT				8
#define	GPTU_T0RDCBA_T0RC					GENMASK(23, 16)
#define	GPTU_T0RDCBA_T0RC_SHIFT				16
#define	GPTU_T0RDCBA_T0RD					GENMASK(31, 24)
#define	GPTU_T0RDCBA_T0RD_SHIFT				24

/* T0 Reload register (24 bit) */
#define	GPTU_T0RCBA(base)					MMIO32((base) + 0x40)
#define	GPTU_T0RCBA_T0RA					GENMASK(7, 0)
#define	GPTU_T0RCBA_T0RA_SHIFT				0
#define	GPTU_T0RCBA_T0RB					GENMASK(15, 8)
#define	GPTU_T0RCBA_T0RB_SHIFT				8
#define	GPTU_T0RCBA_T0RC					GENMASK(23, 16)
#define	GPTU_T0RCBA_T0RC_SHIFT				16

/* T1 Count register (32 bit) */
#define	GPTU_T1DCBA(base)					MMIO32((base) + 0x44)
#define	GPTU_T1DCBA_T1A						GENMASK(7, 0)
#define	GPTU_T1DCBA_T1A_SHIFT				0
#define	GPTU_T1DCBA_T1B						GENMASK(15, 8)
#define	GPTU_T1DCBA_T1B_SHIFT				8
#define	GPTU_T1DCBA_T1C						GENMASK(23, 16)
#define	GPTU_T1DCBA_T1C_SHIFT				16
#define	GPTU_T1DCBA_T1D						GENMASK(31, 24)
#define	GPTU_T1DCBA_T1D_SHIFT				24

/* T1 Count register (24 bit) */
#define	GPTU_T1CBA(base)					MMIO32((base) + 0x48)
#define	GPTU_T1CBA_T1A						GENMASK(7, 0)
#define	GPTU_T1CBA_T1A_SHIFT				0
#define	GPTU_T1CBA_T1B						GENMASK(15, 8)
#define	GPTU_T1CBA_T1B_SHIFT				8
#define	GPTU_T1CBA_T1C						GENMASK(23, 16)
#define	GPTU_T1CBA_T1C_SHIFT				16

/* T1 Reload register (32 bit) */
#define	GPTU_T1RDCBA(base)					MMIO32((base) + 0x4C)
#define	GPTU_T1RDCBA_T1RA					GENMASK(7, 0)
#define	GPTU_T1RDCBA_T1RA_SHIFT				0
#define	GPTU_T1RDCBA_T1RB					GENMASK(15, 8)
#define	GPTU_T1RDCBA_T1RB_SHIFT				8
#define	GPTU_T1RDCBA_T1RC					GENMASK(23, 16)
#define	GPTU_T1RDCBA_T1RC_SHIFT				16
#define	GPTU_T1RDCBA_T1RD					GENMASK(31, 24)
#define	GPTU_T1RDCBA_T1RD_SHIFT				24

/* T1 Reload register (24 bit) */
#define	GPTU_T1RCBA(base)					MMIO32((base) + 0x50)
#define	GPTU_T1RCBA_T1RA					GENMASK(7, 0)
#define	GPTU_T1RCBA_T1RA_SHIFT				0
#define	GPTU_T1RCBA_T1RB					GENMASK(15, 8)
#define	GPTU_T1RCBA_T1RB_SHIFT				8
#define	GPTU_T1RCBA_T1RC					GENMASK(23, 16)
#define	GPTU_T1RCBA_T1RC_SHIFT				16

#define	GPTU_T2(base)						MMIO32((base) + 0x54)
#define	GPTU_T2_T2A							GENMASK(15, 0)						 // T2A Contents
#define	GPTU_T2_T2A_SHIFT					0
#define	GPTU_T2_T2B							GENMASK(31, 16)						 // T2B Contents
#define	GPTU_T2_T2B_SHIFT					16

#define	GPTU_T2RC0(base)					MMIO32((base) + 0x58)
#define	GPTU_T2RC0_T2ARC0					GENMASK(15, 0)						 // T2A Reload/Capture Value
#define	GPTU_T2RC0_T2ARC0_SHIFT				0
#define	GPTU_T2RC0_T2BRC0					GENMASK(31, 16)						 // T2B Reload/Capture Value
#define	GPTU_T2RC0_T2BRC0_SHIFT				16

#define	GPTU_T2RC1(base)					MMIO32((base) + 0x5C)
#define	GPTU_T2RC1_T2ARC1					GENMASK(15, 0)						 // T2A Reload/Capture Value
#define	GPTU_T2RC1_T2ARC1_SHIFT				0
#define	GPTU_T2RC1_T2BRC1					GENMASK(31, 16)						 // T2B Reload/Capture Value
#define	GPTU_T2RC1_T2BRC1_SHIFT				16

#define	GPTU_T012RUN(base)					MMIO32((base) + 0x60)
#define	GPTU_T012RUN_T0ARUN					BIT(0)								 // Timer T0A Run Control.
#define	GPTU_T012RUN_T0BRUN					BIT(1)								 // Timer T0B Run Control.
#define	GPTU_T012RUN_T0CRUN					BIT(2)								 // Timer T0C Run Control.
#define	GPTU_T012RUN_T0DRUN					BIT(3)								 // Timer T0D Run Control.
#define	GPTU_T012RUN_T1ARUN					BIT(4)								 // Timer T1A Run Control.
#define	GPTU_T012RUN_T1BRUN					BIT(5)								 // Timer T1B Run Control.
#define	GPTU_T012RUN_T1CRUN					BIT(6)								 // Timer T1C Run Control.
#define	GPTU_T012RUN_T1DRUN					BIT(7)								 // Timer T1D Run Control.
#define	GPTU_T012RUN_T2ARUN					BIT(8)								 // Timer T2A Run Status Bit.
#define	GPTU_T012RUN_T2ASETR				BIT(9)								 // Timer T2A Run Set Bit.
#define	GPTU_T012RUN_T2ACLRR				BIT(10)								 // Timer T2A Run Clear Bit.
#define	GPTU_T012RUN_T2BRUN					BIT(12)								 // Timer T2B Run Status Bit.
#define	GPTU_T012RUN_T2BSETR				BIT(13)								 // Timer T2B Run Set Bit.
#define	GPTU_T012RUN_T2BCLRR				BIT(14)								 // Timer T2B Run Clear Bit.

/* Service Request Source Selection Register */
#define	GPTU_SRSEL(base)					MMIO32((base) + 0xDC)
#define	GPTU_SRSEL_SSR7						GENMASK(3, 0)						 // GPTU IRQ 7 Source Selection
#define	GPTU_SRSEL_SSR7_SHIFT				0
#define	GPTU_SRSEL_SSR7_START_A				0x0
#define	GPTU_SRSEL_SSR7_STOP_A				0x1
#define	GPTU_SRSEL_SSR7_UPDOWN_A			0x2
#define	GPTU_SRSEL_SSR7_CLEAR_A				0x3
#define	GPTU_SRSEL_SSR7_RLCP0_A				0x4
#define	GPTU_SRSEL_SSR7_RLCP1_A				0x5
#define	GPTU_SRSEL_SSR7_OUV_T2A				0x6
#define	GPTU_SRSEL_SSR7_OUV_T2B				0x7
#define	GPTU_SRSEL_SSR7_START_B				0x8
#define	GPTU_SRSEL_SSR7_STOP_B				0x9
#define	GPTU_SRSEL_SSR7_RLCP0_B				0xA
#define	GPTU_SRSEL_SSR7_RLCP1_B				0xB
#define	GPTU_SRSEL_SSR7_SR00				0xC
#define	GPTU_SRSEL_SSR7_SR01				0xD
#define	GPTU_SRSEL_SSR7_SR10				0xE
#define	GPTU_SRSEL_SSR7_SR11				0xF
#define	GPTU_SRSEL_SSR6						GENMASK(7, 4)						 // GPTU IRQ 6 Source Selection
#define	GPTU_SRSEL_SSR6_SHIFT				4
#define	GPTU_SRSEL_SSR6_START_A				0x0
#define	GPTU_SRSEL_SSR6_STOP_A				0x10
#define	GPTU_SRSEL_SSR6_UPDOWN_A			0x20
#define	GPTU_SRSEL_SSR6_CLEAR_A				0x30
#define	GPTU_SRSEL_SSR6_RLCP0_A				0x40
#define	GPTU_SRSEL_SSR6_RLCP1_A				0x50
#define	GPTU_SRSEL_SSR6_OUV_T2A				0x60
#define	GPTU_SRSEL_SSR6_OUV_T2B				0x70
#define	GPTU_SRSEL_SSR6_START_B				0x80
#define	GPTU_SRSEL_SSR6_STOP_B				0x90
#define	GPTU_SRSEL_SSR6_RLCP0_B				0xA0
#define	GPTU_SRSEL_SSR6_RLCP1_B				0xB0
#define	GPTU_SRSEL_SSR6_SR00				0xC0
#define	GPTU_SRSEL_SSR6_SR01				0xD0
#define	GPTU_SRSEL_SSR6_SR10				0xE0
#define	GPTU_SRSEL_SSR6_SR11				0xF0
#define	GPTU_SRSEL_SSR5						GENMASK(11, 8)						 // GPTU IRQ 5 Source Selection
#define	GPTU_SRSEL_SSR5_SHIFT				8
#define	GPTU_SRSEL_SSR5_START_A				0x0
#define	GPTU_SRSEL_SSR5_STOP_A				0x100
#define	GPTU_SRSEL_SSR5_UPDOWN_A			0x200
#define	GPTU_SRSEL_SSR5_CLEAR_A				0x300
#define	GPTU_SRSEL_SSR5_RLCP0_A				0x400
#define	GPTU_SRSEL_SSR5_RLCP1_A				0x500
#define	GPTU_SRSEL_SSR5_OUV_T2A				0x600
#define	GPTU_SRSEL_SSR5_OUV_T2B				0x700
#define	GPTU_SRSEL_SSR5_START_B				0x800
#define	GPTU_SRSEL_SSR5_STOP_B				0x900
#define	GPTU_SRSEL_SSR5_RLCP0_B				0xA00
#define	GPTU_SRSEL_SSR5_RLCP1_B				0xB00
#define	GPTU_SRSEL_SSR5_SR00				0xC00
#define	GPTU_SRSEL_SSR5_SR01				0xD00
#define	GPTU_SRSEL_SSR5_SR10				0xE00
#define	GPTU_SRSEL_SSR5_SR11				0xF00
#define	GPTU_SRSEL_SSR4						GENMASK(15, 12)						 // GPTU IRQ 4 Source Selection
#define	GPTU_SRSEL_SSR4_SHIFT				12
#define	GPTU_SRSEL_SSR4_START_A				0x0
#define	GPTU_SRSEL_SSR4_STOP_A				0x1000
#define	GPTU_SRSEL_SSR4_UPDOWN_A			0x2000
#define	GPTU_SRSEL_SSR4_CLEAR_A				0x3000
#define	GPTU_SRSEL_SSR4_RLCP0_A				0x4000
#define	GPTU_SRSEL_SSR4_RLCP1_A				0x5000
#define	GPTU_SRSEL_SSR4_OUV_T2A				0x6000
#define	GPTU_SRSEL_SSR4_OUV_T2B				0x7000
#define	GPTU_SRSEL_SSR4_START_B				0x8000
#define	GPTU_SRSEL_SSR4_STOP_B				0x9000
#define	GPTU_SRSEL_SSR4_RLCP0_B				0xA000
#define	GPTU_SRSEL_SSR4_RLCP1_B				0xB000
#define	GPTU_SRSEL_SSR4_SR00				0xC000
#define	GPTU_SRSEL_SSR4_SR01				0xD000
#define	GPTU_SRSEL_SSR4_SR10				0xE000
#define	GPTU_SRSEL_SSR4_SR11				0xF000
#define	GPTU_SRSEL_SSR3						GENMASK(19, 16)						 // GPTU IRQ 3 Source Selection
#define	GPTU_SRSEL_SSR3_SHIFT				16
#define	GPTU_SRSEL_SSR3_START_A				0x0
#define	GPTU_SRSEL_SSR3_STOP_A				0x10000
#define	GPTU_SRSEL_SSR3_UPDOWN_A			0x20000
#define	GPTU_SRSEL_SSR3_CLEAR_A				0x30000
#define	GPTU_SRSEL_SSR3_RLCP0_A				0x40000
#define	GPTU_SRSEL_SSR3_RLCP1_A				0x50000
#define	GPTU_SRSEL_SSR3_OUV_T2A				0x60000
#define	GPTU_SRSEL_SSR3_OUV_T2B				0x70000
#define	GPTU_SRSEL_SSR3_START_B				0x80000
#define	GPTU_SRSEL_SSR3_STOP_B				0x90000
#define	GPTU_SRSEL_SSR3_RLCP0_B				0xA0000
#define	GPTU_SRSEL_SSR3_RLCP1_B				0xB0000
#define	GPTU_SRSEL_SSR3_SR00				0xC0000
#define	GPTU_SRSEL_SSR3_SR01				0xD0000
#define	GPTU_SRSEL_SSR3_SR10				0xE0000
#define	GPTU_SRSEL_SSR3_SR11				0xF0000
#define	GPTU_SRSEL_SSR2						GENMASK(23, 20)						 // GPTU IRQ 2 Source Selection
#define	GPTU_SRSEL_SSR2_SHIFT				20
#define	GPTU_SRSEL_SSR2_START_A				0x0
#define	GPTU_SRSEL_SSR2_STOP_A				0x100000
#define	GPTU_SRSEL_SSR2_UPDOWN_A			0x200000
#define	GPTU_SRSEL_SSR2_CLEAR_A				0x300000
#define	GPTU_SRSEL_SSR2_RLCP0_A				0x400000
#define	GPTU_SRSEL_SSR2_RLCP1_A				0x500000
#define	GPTU_SRSEL_SSR2_OUV_T2A				0x600000
#define	GPTU_SRSEL_SSR2_OUV_T2B				0x700000
#define	GPTU_SRSEL_SSR2_START_B				0x800000
#define	GPTU_SRSEL_SSR2_STOP_B				0x900000
#define	GPTU_SRSEL_SSR2_RLCP0_B				0xA00000
#define	GPTU_SRSEL_SSR2_RLCP1_B				0xB00000
#define	GPTU_SRSEL_SSR2_SR00				0xC00000
#define	GPTU_SRSEL_SSR2_SR01				0xD00000
#define	GPTU_SRSEL_SSR2_SR10				0xE00000
#define	GPTU_SRSEL_SSR2_SR11				0xF00000
#define	GPTU_SRSEL_SSR1						GENMASK(27, 24)						 // GPTU IRQ 1 Source Selection
#define	GPTU_SRSEL_SSR1_SHIFT				24
#define	GPTU_SRSEL_SSR1_START_A				0x0
#define	GPTU_SRSEL_SSR1_STOP_A				0x1000000
#define	GPTU_SRSEL_SSR1_UPDOWN_A			0x2000000
#define	GPTU_SRSEL_SSR1_CLEAR_A				0x3000000
#define	GPTU_SRSEL_SSR1_RLCP0_A				0x4000000
#define	GPTU_SRSEL_SSR1_RLCP1_A				0x5000000
#define	GPTU_SRSEL_SSR1_OUV_T2A				0x6000000
#define	GPTU_SRSEL_SSR1_OUV_T2B				0x7000000
#define	GPTU_SRSEL_SSR1_START_B				0x8000000
#define	GPTU_SRSEL_SSR1_STOP_B				0x9000000
#define	GPTU_SRSEL_SSR1_RLCP0_B				0xA000000
#define	GPTU_SRSEL_SSR1_RLCP1_B				0xB000000
#define	GPTU_SRSEL_SSR1_SR00				0xC000000
#define	GPTU_SRSEL_SSR1_SR01				0xD000000
#define	GPTU_SRSEL_SSR1_SR10				0xE000000
#define	GPTU_SRSEL_SSR1_SR11				0xF000000
#define	GPTU_SRSEL_SSR0						GENMASK(31, 28)						 // GPTU IRQ 0 Source Selection
#define	GPTU_SRSEL_SSR0_SHIFT				28
#define	GPTU_SRSEL_SSR0_START_A				0x0
#define	GPTU_SRSEL_SSR0_STOP_A				0x10000000
#define	GPTU_SRSEL_SSR0_UPDOWN_A			0x20000000
#define	GPTU_SRSEL_SSR0_CLEAR_A				0x30000000
#define	GPTU_SRSEL_SSR0_RLCP0_A				0x40000000
#define	GPTU_SRSEL_SSR0_RLCP1_A				0x50000000
#define	GPTU_SRSEL_SSR0_OUV_T2A				0x60000000
#define	GPTU_SRSEL_SSR0_OUV_T2B				0x70000000
#define	GPTU_SRSEL_SSR0_START_B				0x80000000
#define	GPTU_SRSEL_SSR0_STOP_B				0x90000000
#define	GPTU_SRSEL_SSR0_RLCP0_B				0xA0000000
#define	GPTU_SRSEL_SSR0_RLCP1_B				0xB0000000
#define	GPTU_SRSEL_SSR0_SR00				0xC0000000
#define	GPTU_SRSEL_SSR0_SR01				0xD0000000
#define	GPTU_SRSEL_SSR0_SR10				0xE0000000
#define	GPTU_SRSEL_SSR0_SR11				0xF0000000

/* Service Routing Control Register */
#define	GPTU_SRC(base, n)					MMIO32(base + 0xE0 + ((n) * 0x4))


// STM [MOD_NUM=0000, MOD_REV=02, MOD_32BIT=C0]
// STM from Tricore TC1766 official public datasheet
#define	STM_BASE	0xF4B00000
/* Clock Control Register */
#define	STM_CLC		MMIO32(STM_BASE + 0x00)

/* Module Identifier Register */
#define	STM_ID		MMIO32(STM_BASE + 0x08)

#define	STM_TIM0	MMIO32(STM_BASE + 0x10)

#define	STM_TIM1	MMIO32(STM_BASE + 0x14)

#define	STM_TIM2	MMIO32(STM_BASE + 0x18)

#define	STM_TIM3	MMIO32(STM_BASE + 0x1C)

#define	STM_TIM4	MMIO32(STM_BASE + 0x20)

#define	STM_TIM5	MMIO32(STM_BASE + 0x24)

#define	STM_TIM6	MMIO32(STM_BASE + 0x28)

#define	STM_CAP		MMIO32(STM_BASE + 0x2C)


// ADC [MOD_NUM=F024, MOD_REV=10, MOD_32BIT=C0]
// Measurement Interface
#define	ADC_BASE				0xF4C00000
/* Clock Control Register */
#define	ADC_CLC					MMIO32(ADC_BASE + 0x00)

/* Module Identifier Register */
#define	ADC_ID					MMIO32(ADC_BASE + 0x08)

#define	ADC_CON0				MMIO32(ADC_BASE + 0x14)
#define	ADC_CON0_EN_VREF		BIT(1)

#define	ADC_CON1				MMIO32(ADC_BASE + 0x18)
#define	ADC_CON1_CH				GENMASK(5, 0)
#define	ADC_CON1_CH_SHIFT		0
#define	ADC_CON1_CH_OFF			0x0
#define	ADC_CON1_CH_M0			0x1
#define	ADC_CON1_CH_M1			0x2
#define	ADC_CON1_CH_M2			0x3
#define	ADC_CON1_CH_M7			0x8
#define	ADC_CON1_CH_M8			0x9
#define	ADC_CON1_CH_M9			0xA
#define	ADC_CON1_CH_M10			0xB
#define	ADC_CON1_CH_M0_M9_A		0xC
#define	ADC_CON1_CH_M0_M9_B		0x12
#define	ADC_CON1_PREAMP_INV		BIT(6)
#define	ADC_CON1_PREAMP_FAST	BIT(11)
#define	ADC_CON1_MODE			GENMASK(14, 12)
#define	ADC_CON1_MODE_SHIFT		12
#define	ADC_CON1_MODE_V			0x0
#define	ADC_CON1_MODE_I_30		0x1000
#define	ADC_CON1_MODE_I_60		0x2000
#define	ADC_CON1_MODE_I_90		0x3000
#define	ADC_CON1_MODE_I_120		0x4000
#define	ADC_CON1_MODE_I_150		0x5000
#define	ADC_CON1_MODE_I_180		0x6000
#define	ADC_CON1_MODE_I_210		0x7000
#define	ADC_CON1_FREQ			GENMASK(18, 16)
#define	ADC_CON1_FREQ_SHIFT		16
#define	ADC_CON1_COUNT			GENMASK(21, 19)
#define	ADC_CON1_COUNT_SHIFT	19
#define	ADC_CON1_REF_CH			GENMASK(24, 22)
#define	ADC_CON1_REF_CH_SHIFT	22
#define	ADC_CON1_REF_CH_OFF		0x0
#define	ADC_CON1_REF_CH_M0		0x400000
#define	ADC_CON1_REF_CH_M1		0x800000
#define	ADC_CON1_REF_CH_M2		0xC00000
#define	ADC_CON1_REF_CH_M7		0x2000000
#define	ADC_CON1_REF_CH_M8		0x2400000
#define	ADC_CON1_REF_CH_M9		0x2800000
#define	ADC_CON1_REF_CH_M10		0x2C00000
#define	ADC_CON1_REF_CH_M0_M9_A	0x3000000
#define	ADC_CON1_REF_CH_M0_M9_B	0x4800000
#define	ADC_CON1_SINGLE			BIT(27)
#define	ADC_CON1_TRIG			BIT(28)
#define	ADC_CON1_ON				BIT(29)
#define	ADC_CON1_START			BIT(31)

#define	ADC_STAT				MMIO32(ADC_BASE + 0x1C)
#define	ADC_STAT_INDEX			GENMASK(2, 0)
#define	ADC_STAT_INDEX_SHIFT	0
#define	ADC_STAT_BUSY			BIT(30)
#define	ADC_STAT_READY			BIT(31)

#define	ADC_FIFO(n)				MMIO32(ADC_BASE + 0x20 + ((n) * 0x4))

#define	ADC_PLLCON				MMIO32(ADC_BASE + 0x40)
#define	ADC_PLLCON_K			GENMASK(7, 0)
#define	ADC_PLLCON_K_SHIFT		0
#define	ADC_PLLCON_L			GENMASK(15, 8)
#define	ADC_PLLCON_L_SHIFT		8

/* Service Routing Control Register */
#define	ADC_SRC(n)				MMIO32(ADC_BASE + 0xF0 + ((n) * 0x4))


// KEYPAD [MOD_NUM=F046, MOD_REV=00, MOD_32BIT=C0]
// Keypad scaner module, registers collected using tests on real hardware (using "black box" method).
#define	KEYPAD_BASE			0xF4D00000
/* Module Identifier Register */
#define	KEYPAD_ID			MMIO32(KEYPAD_BASE + 0x08)

#define	KEYPAD_CON			MMIO32(KEYPAD_BASE + 0x10)

#define	KEYPAD_PORT(n)		MMIO32(KEYPAD_BASE + 0x18 + ((n) * 0x4))

#define	KEYPAD_ISR			MMIO32(KEYPAD_BASE + 0x24)
#define	KEYPAD_ISR_PRESS	BIT(2)
#define	KEYPAD_ISR_RELEASE	BIT(3)

/* Service Routing Control Register */
#define	KEYPAD_PRESS_SRC	MMIO32(KEYPAD_BASE + 0xF0)

/* Service Routing Control Register */
#define	KEYPAD_UNK0_SRC		MMIO32(KEYPAD_BASE + 0xF4)

/* Service Routing Control Register */
#define	KEYPAD_UNK1_SRC		MMIO32(KEYPAD_BASE + 0xF8)

/* Service Routing Control Register */
#define	KEYPAD_RELEASE_SRC	MMIO32(KEYPAD_BASE + 0xFC)


// DSP [MOD_NUM=F022, MOD_REV=10, MOD_32BIT=C0]
// Looks like DSP module, but not sure.
#define	DSP_BASE	0xF6000000
/* Clock Control Register */
#define	DSP_CLC		MMIO32(DSP_BASE + 0x00)

/* Module Identifier Register */
#define	DSP_ID		MMIO32(DSP_BASE + 0x08)

#define	DSP_UNK0	MMIO32(DSP_BASE + 0x1C)

#define	DSP_UNK1	MMIO32(DSP_BASE + 0x24)

#define	DSP_RAM(n)	MMIO32(DSP_BASE + 0x1000 + ((n) * 0x4))


// GPRSCU [MOD_NUM=F003, MOD_REV=10, MOD_32BIT=C0]
// Looks like GPRS Cypher Uinit module, but not sure.
#define	GPRSCU_BASE		0xF6200000
/* Clock Control Register */
#define	GPRSCU_CLC		MMIO32(GPRSCU_BASE + 0x00)

/* Module Identifier Register */
#define	GPRSCU_ID		MMIO32(GPRSCU_BASE + 0x08)

/* Service Routing Control Register */
#define	GPRSCU_SRC(n)	MMIO32(GPRSCU_BASE + 0xF8 + ((n) * 0x4))


// AFC [MOD_NUM=F004, MOD_REV=00, MOD_32BIT=C0]
// Looks like AFC (Automatic Frequency Correction???) module, but not sure.
#define	AFC_BASE	0xF6300000
/* Clock Control Register */
#define	AFC_CLC		MMIO32(AFC_BASE + 0x00)

/* Module Identifier Register */
#define	AFC_ID		MMIO32(AFC_BASE + 0x08)


// TPU [MOD_NUM=F021, MOD_REV=00, MOD_32BIT=C0]
// Looks like TPU (time processing module) module, registers collected using tests on real hardware (using "black box" method).
#define	TPU_BASE					0xF6400000
/* Clock Control Register */
#define	TPU_CLC						MMIO32(TPU_BASE + 0x00)

/* Module Identifier Register */
#define	TPU_ID						MMIO32(TPU_BASE + 0x08)

#define	TPU_UNK0					MMIO32(TPU_BASE + 0x10)

#define	TPU_UNK1					MMIO32(TPU_BASE + 0x14)

#define	TPU_UNK2					MMIO32(TPU_BASE + 0x18)

#define	TPU_CORRECTION				MMIO32(TPU_BASE + 0x1C)
#define	TPU_CORRECTION_VALUE		GENMASK(14, 0)
#define	TPU_CORRECTION_VALUE_SHIFT	0
#define	TPU_CORRECTION_CTRL			BIT(16)

#define	TPU_OVERFLOW				MMIO32(TPU_BASE + 0x20)
#define	TPU_OVERFLOW_VALUE			GENMASK(14, 0)
#define	TPU_OVERFLOW_VALUE_SHIFT	0

#define	TPU_INT(n)					MMIO32(TPU_BASE + 0x24 + ((n) * 0x4))
#define	TPU_INT_VALUE				GENMASK(14, 0)
#define	TPU_INT_VALUE_SHIFT			0

#define	TPU_OFFSET					MMIO32(TPU_BASE + 0x2C)
#define	TPU_OFFSET_VALUE			GENMASK(14, 0)
#define	TPU_OFFSET_VALUE_SHIFT		0
#define	TPU_OFFSET_CTRL				BIT(16)

#define	TPU_SKIP					MMIO32(TPU_BASE + 0x30)
#define	TPU_SKIP_SKIPN				BIT(0)
#define	TPU_SKIP_SKIPC				BIT(1)

#define	TPU_COUNTER					MMIO32(TPU_BASE + 0x34)
#define	TPU_COUNTER_VALUE			GENMASK(14, 0)
#define	TPU_COUNTER_VALUE_SHIFT		0

#define	TPU_UNK3					MMIO32(TPU_BASE + 0x38)

#define	TPU_UNK4					MMIO32(TPU_BASE + 0x3C)

#define	TPU_UNK5					MMIO32(TPU_BASE + 0x40)

#define	TPU_UNK6					MMIO32(TPU_BASE + 0x44)

#define	TPU_PARAM					MMIO32(TPU_BASE + 0x5C)
#define	TPU_PARAM_TINI				BIT(0)
#define	TPU_PARAM_FDIS				BIT(1)

#define	TPU_UNK7					MMIO32(TPU_BASE + 0x60)

#define	TPU_PLLCON0					MMIO32(TPU_BASE + 0x68)
#define	TPU_PLLCON0_K_DIV			GENMASK(29, 0)
#define	TPU_PLLCON0_K_DIV_SHIFT		0

#define	TPU_PLLCON1					MMIO32(TPU_BASE + 0x6C)
#define	TPU_PLLCON1_L_DIV			GENMASK(29, 0)
#define	TPU_PLLCON1_L_DIV_SHIFT		0

#define	TPU_PLLCON2					MMIO32(TPU_BASE + 0x70)
#define	TPU_PLLCON2_LOAD			BIT(0)
#define	TPU_PLLCON2_INIT			BIT(1)

/* Service Routing Control Register */
#define	TPU_UNK_SRC(n)				MMIO32(TPU_BASE + 0xE0 + ((n) * 0x4))

/* Service Routing Control Register */
#define	TPU_SRC(n)					MMIO32(TPU_BASE + 0xF8 + ((n) * 0x4))

#define	TPU_RAM(n)					MMIO32(TPU_BASE + 0x1000 + ((n) * 0x4))


