#pragma once
#include <pmb887x.h>

// IRQ numbers
#define	NVIC_USART0_TX_IRQ		4
#define	NVIC_USART0_TBUF_IRQ	5
#define	NVIC_USART0_RX_IRQ		6
#define	NVIC_USART0_ERR_IRQ		7
#define	NVIC_USART0_CTS_IRQ		8
#define	NVIC_USART0_ABDET_IRQ	9
#define	NVIC_USART0_ABSTART_IRQ	10
#define	NVIC_USART0_TMO_IRQ		11
#define	NVIC_USB_IRQ			25
#define	NVIC_USART1_TX_IRQ		26
#define	NVIC_USART1_TBUF_IRQ	27
#define	NVIC_USART1_RX_IRQ		28
#define	NVIC_USART1_ERR_IRQ		29
#define	NVIC_USART1_CTS_IRQ		30
#define	NVIC_USART1_ABDET_IRQ	31
#define	NVIC_USART1_ABSTART_IRQ	32
#define	NVIC_USART1_TMO_IRQ		33
#define	NVIC_DMAC_IRQ			35
#define	NVIC_DMAC_CH0_IRQ		36
#define	NVIC_DMAC_CH1_IRQ		37
#define	NVIC_DMAC_CH2_IRQ		38
#define	NVIC_DMAC_CH3_IRQ		39
#define	NVIC_DMAC_CH4_IRQ		40
#define	NVIC_DMAC_CH5_IRQ		41
#define	NVIC_DMAC_CH6_IRQ		42
#define	NVIC_DMAC_CH7_IRQ		43
#define	NVIC_RTC_IRQ			46
#define	NVIC_SCU_EXTI0_IRQ		48
#define	NVIC_SCU_EXTI1_IRQ		49
#define	NVIC_SCU_EXTI2_IRQ		50
#define	NVIC_SCU_EXTI3_IRQ		51
#define	NVIC_SCU_EXTI4_IRQ		52
#define	NVIC_SCU_DSP0_IRQ		53
#define	NVIC_SCU_DSP1_IRQ		54
#define	NVIC_SCU_DSP2_IRQ		55
#define	NVIC_SCU_DSP3_IRQ		56
#define	NVIC_SCU_DSP4_IRQ		57
#define	NVIC_SCU_UNK0_IRQ		58
#define	NVIC_SCU_UNK1_IRQ		59
#define	NVIC_SCU_UNK2_IRQ		60
#define	NVIC_SCU_EXTI5_IRQ		61
#define	NVIC_SCU_EXTI6_IRQ		62
#define	NVIC_SCU_EXTI7_IRQ		63
#define	NVIC_PLL_IRQ			65
#define	NVIC_I2C_INT0_IRQ		66
#define	NVIC_I2C_INT1_IRQ		67
#define	NVIC_I2C_INT2_IRQ		68
#define	NVIC_AMC_INT0_IRQ		70
#define	NVIC_AMC_INT1_IRQ		71
#define	NVIC_CAPCOM0_T0_IRQ		72
#define	NVIC_CAPCOM0_T1_IRQ		73
#define	NVIC_CAPCOM0_CC0_IRQ	74
#define	NVIC_CAPCOM0_CC1_IRQ	75
#define	NVIC_CAPCOM0_CC2_IRQ	76
#define	NVIC_CAPCOM0_CC3_IRQ	77
#define	NVIC_CAPCOM0_CC4_IRQ	78
#define	NVIC_CAPCOM0_CC5_IRQ	79
#define	NVIC_CAPCOM0_CC6_IRQ	80
#define	NVIC_CAPCOM0_CC7_IRQ	81
#define	NVIC_CAPCOM1_T0_IRQ		82
#define	NVIC_CAPCOM1_T1_IRQ		83
#define	NVIC_CAPCOM1_CC0_IRQ	84
#define	NVIC_CAPCOM1_CC1_IRQ	85
#define	NVIC_CAPCOM1_CC2_IRQ	86
#define	NVIC_CAPCOM1_CC3_IRQ	87
#define	NVIC_CAPCOM1_CC4_IRQ	88
#define	NVIC_CAPCOM1_CC5_IRQ	89
#define	NVIC_CAPCOM1_CC6_IRQ	90
#define	NVIC_CAPCOM1_CC7_IRQ	91
#define	NVIC_GPTU0_SRC0_IRQ		92
#define	NVIC_GPTU0_SRC1_IRQ		93
#define	NVIC_GPTU0_SRC2_IRQ		94
#define	NVIC_GPTU0_SRC3_IRQ		95
#define	NVIC_GPTU0_SRC4_IRQ		96
#define	NVIC_GPTU0_SRC5_IRQ		97
#define	NVIC_GPTU0_SRC6_IRQ		98
#define	NVIC_GPTU0_SRC7_IRQ		99
#define	NVIC_GPTU1_SRC0_IRQ		100
#define	NVIC_GPTU1_SRC1_IRQ		101
#define	NVIC_GPTU1_SRC2_IRQ		102
#define	NVIC_GPTU1_SRC3_IRQ		103
#define	NVIC_GPTU1_SRC4_IRQ		104
#define	NVIC_GPTU1_SRC5_IRQ		105
#define	NVIC_GPTU1_SRC6_IRQ		106
#define	NVIC_GPTU1_SRC7_IRQ		107
#define	NVIC_KEYPAD_PRESS_IRQ	108
#define	NVIC_KEYPAD_UNK0_IRQ	109
#define	NVIC_KEYPAD_UNK1_IRQ	110
#define	NVIC_KEYPAD_RELEASE_IRQ	111
#define	NVIC_TPU_INT_UNK0_IRQ	113
#define	NVIC_TPU_INT_UNK1_IRQ	114
#define	NVIC_TPU_INT_UNK2_IRQ	115
#define	NVIC_TPU_INT_UNK3_IRQ	116
#define	NVIC_TPU_INT_UNK4_IRQ	117
#define	NVIC_TPU_INT_UNK5_IRQ	118
#define	NVIC_TPU_INT0_IRQ		119
#define	NVIC_TPU_INT1_IRQ		120
#define	NVIC_GPRSCU_INT0_IRQ	121
#define	NVIC_GPRSCU_INT1_IRQ	122


// Common regs for all modules
/* Clock Control Register */
#define	MOD_CLC_DISR						BIT(0)			 // Module Disable Request Bit
#define	MOD_CLC_DISS						BIT(1)			 // Module Disable Status Bit
#define	MOD_CLC_SPEN						BIT(2)			 // Module Suspend Enable Bit
#define	MOD_CLC_EDIS						BIT(3)			 // Module External Request Disable
#define	MOD_CLC_SBWE						BIT(4)			 // Module Suspend Bit Write Enable
#define	MOD_CLC_FSOE						BIT(5)			 // Module Fast Shut-Off Enable.
#define	MOD_CLC_RMC							GENMASK(8, 8)	 // Module Clock Divider for Normal Mode
#define	MOD_CLC_RMC_SHIFT					8

/* Module Identifier Register */
#define	MOD_ID_REV							GENMASK(8, 0)
#define	MOD_ID_REV_SHIFT					0
#define	MOD_ID_32B							GENMASK(8, 8)
#define	MOD_ID_32B_SHIFT					8
#define	MOD_ID_NUMBER						GENMASK(16, 16)
#define	MOD_ID_NUMBER_SHIFT					16

/* Service Routing Control Register */
#define	MOD_SRC_SRPN						GENMASK(8, 0)	 // IRQ priority number
#define	MOD_SRC_SRPN_SHIFT					0
#define	MOD_SRC_TOS							GENMASK(2, 10)	 // Type of service for node
#define	MOD_SRC_TOS_SHIFT					10
#define	MOD_SRC_SRE							BIT(12)			 // IRQ enable
#define	MOD_SRC_SRR							BIT(13)			 // IRQ Service Request Bit
#define	MOD_SRC_CLRR						BIT(14)			 // IRQ Request Clear Bit
#define	MOD_SRC_SETR						BIT(15)			 // IRQ Request Set Bit

#define	AMBA_PERIPH_ID0_PARTNUMBER0			GENMASK(8, 0)
#define	AMBA_PERIPH_ID0_PARTNUMBER0_SHIFT	0

#define	AMBA_PERIPH_ID1_PARTNUMBER1			GENMASK(4, 0)
#define	AMBA_PERIPH_ID1_PARTNUMBER1_SHIFT	0
#define	AMBA_PERIPH_ID1_DESIGNER0			GENMASK(8, 4)
#define	AMBA_PERIPH_ID1_DESIGNER0_SHIFT		4

#define	AMBA_PERIPH_ID2_DESIGNER1			GENMASK(4, 0)
#define	AMBA_PERIPH_ID2_DESIGNER1_SHIFT		0
#define	AMBA_PERIPH_ID2_REVISION			GENMASK(8, 4)
#define	AMBA_PERIPH_ID2_REVISION_SHIFT		4

#define	AMBA_PERIPH_ID3_CONFIGURATION		GENMASK(8, 0)
#define	AMBA_PERIPH_ID3_CONFIGURATION_SHIFT	0



// EBU [MOD_NUM=0014, MOD_REV=00, MOD_32BIT=C0]
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
#define	EBU_CON_ARBMODE				GENMASK(2, 6)							 // Arbitration mode
#define	EBU_CON_ARBMODE_SHIFT		6
#define	EBU_CON_TOUTC				GENMASK(8, 8)							 // Time-out control
#define	EBU_CON_TOUTC_SHIFT			8
#define	EBU_CON_GLOBALCS			GENMASK(8, 16)							 // Global chip select signal
#define	EBU_CON_GLOBALCS_SHIFT		16
#define	EBU_CON_BUSCLK				GENMASK(2, 24)							 // Clock generation
#define	EBU_CON_BUSCLK_SHIFT		24
#define	EBU_CON_SDCMSEL				BIT(26)									 // SDRAM Clock Mode Select
#define	EBU_CON_CS0FAM				BIT(27)									 // CS0 Fills Address Map
#define	EBU_CON_EMUFAM				BIT(28)									 // CSEMU Fills Address Map
#define	EBU_CON_BFSSS				BIT(29)									 // Burst FLASH Single Stage Synchronization

#define	EBU_BFCON					MMIO32(EBU_BASE + 0x20)
#define	EBU_BFCON_FETBLEN0			GENMASK(4, 0)							 // Fetch Burst Length for Burst FLASH Type 0
#define	EBU_BFCON_FETBLEN0_SHIFT	0
#define	EBU_BFCON_FBBMSEL0			BIT(4)									 // FLASH Burst Buffer Mode Select for Burst FLASH Type 0
#define	EBU_BFCON_WAITFUNC0			BIT(5)									 // Function of WAIT Input for Burst FLASH Type 0
#define	EBU_BFCON_EXTCLOCK			GENMASK(2, 6)							 // Frequency of external clock
#define	EBU_BFCON_EXTCLOCK_SHIFT	6
#define	EBU_BFCON_BFCMSEL			BIT(8)									 // Burst FLASH Clock Mode Select
#define	EBU_BFCON_EBSE0				BIT(9)									 // Early Burst Signal Enable for Burst FLASH Type 0
#define	EBU_BFCON_DBA0				BIT(10)									 // Disable Burst Address Wrapping
#define	EBU_BFCON_FDBKEN			BIT(11)									 // Burst FLASH Clock Feedback Enable
#define	EBU_BFCON_DTALTNCY			GENMASK(4, 12)							 // Latency Cycle Control
#define	EBU_BFCON_DTALTNCY_SHIFT	12
#define	EBU_BFCON_FETBLEN1			GENMASK(4, 16)							 // Fetch Burst Length for Burst FLASH Type 1
#define	EBU_BFCON_FETBLEN1_SHIFT	16
#define	EBU_BFCON_FBBMSEL1			BIT(20)									 // FLASH Burst Buffer Mode Select for Burst FLASH Type 1
#define	EBU_BFCON_WAITFUNC1			BIT(21)									 // Function of WAIT Input for Burst FLASH Type 1
#define	EBU_BFCON_DBA1				BIT(23)									 // Disable Burst Address Wrapping
#define	EBU_BFCON_EBSE1				BIT(25)									 // Early Burst Signal Enable for Burst FLASH Type 1

#define	EBU_SDRMREF(n)				MMIO32(EBU_BASE + 0x40 + ((n) * 0x8))
#define	EBU_SDRMREF_REFRESHC		GENMASK(6, 0)							 // Refresh counter period
#define	EBU_SDRMREF_REFRESHC_SHIFT	0
#define	EBU_SDRMREF_REFRESHR		GENMASK(3, 6)							 // Number of refresh commands
#define	EBU_SDRMREF_REFRESHR_SHIFT	6
#define	EBU_SDRMREF_SELFREXST		BIT(9)									 // Self refresh exit status
#define	EBU_SDRMREF_SELFREX			BIT(10)									 // Self refresh exit
#define	EBU_SDRMREF_SELFRENST		BIT(11)									 // Self refresh entry status
#define	EBU_SDRMREF_SELFREN			BIT(12)									 // Self refresh entry
#define	EBU_SDRMREF_AUTOSELFR		BIT(13)									 // Automatic self refresh

#define	EBU_SDRMCON(n)				MMIO32(EBU_BASE + 0x50 + ((n) * 0x8))
#define	EBU_SDRMCON_CRAS			GENMASK(4, 0)							 // Row to precharge delay counter
#define	EBU_SDRMCON_CRAS_SHIFT		0
#define	EBU_SDRMCON_CRFSH			GENMASK(4, 4)							 // Refresh commands counter
#define	EBU_SDRMCON_CRFSH_SHIFT		4
#define	EBU_SDRMCON_CRSC			GENMASK(2, 8)							 // Mode register setup time
#define	EBU_SDRMCON_CRSC_SHIFT		8
#define	EBU_SDRMCON_CRP				GENMASK(2, 10)							 // Row precharge time counter
#define	EBU_SDRMCON_CRP_SHIFT		10
#define	EBU_SDRMCON_AWIDTH			GENMASK(2, 12)							 // Width of column address
#define	EBU_SDRMCON_AWIDTH_SHIFT	12
#define	EBU_SDRMCON_CRCD			GENMASK(2, 14)							 // Row to column delay counter
#define	EBU_SDRMCON_CRCD_SHIFT		14
#define	EBU_SDRMCON_CRC				GENMASK(3, 16)							 // Row cycle time counter
#define	EBU_SDRMCON_CRC_SHIFT		16
#define	EBU_SDRMCON_PAGEM			GENMASK(3, 19)							 // Mask for page tag
#define	EBU_SDRMCON_PAGEM_SHIFT		19
#define	EBU_SDRMCON_BANKM			GENMASK(3, 22)							 // Mask for bank tag
#define	EBU_SDRMCON_BANKM_SHIFT		22

#define	EBU_SDRMOD(n)				MMIO32(EBU_BASE + 0x60 + ((n) * 0x8))
#define	EBU_SDRMOD_BURSTL			GENMASK(3, 0)							 // Burst length
#define	EBU_SDRMOD_BURSTL_SHIFT		0
#define	EBU_SDRMOD_BTYP				BIT(3)									 // Burst type
#define	EBU_SDRMOD_CASLAT			GENMASK(3, 4)							 // CAS latency
#define	EBU_SDRMOD_CASLAT_SHIFT		4
#define	EBU_SDRMOD_OPMODE			GENMASK(7, 7)							 // Operation Mode
#define	EBU_SDRMOD_OPMODE_SHIFT		7

#define	EBU_SDRSTAT(n)				MMIO32(EBU_BASE + 0x70 + ((n) * 0x8))
#define	EBU_SDRSTAT_REFERR			BIT(0)									 // SDRAM Refresh Error
#define	EBU_SDRSTAT_SDRM_BUSY		BIT(1)									 // SDRAM Busy

#define	EBU_ADDRSEL(n)				MMIO32(EBU_BASE + 0x80 + ((n) * 0x8))
#define	EBU_ADDRSEL_REGENAB			BIT(0)									 // Memory Region
#define	EBU_ADDRSEL_ALTENAB			BIT(1)									 // Alternate Segment Comparison
#define	EBU_ADDRSEL_MASK			GENMASK(4, 4)							 // Address Mask
#define	EBU_ADDRSEL_MASK_SHIFT		4
#define	EBU_ADDRSEL_ALTSEG			GENMASK(4, 8)							 // Alternate Segment
#define	EBU_ADDRSEL_ALTSEG_SHIFT	8
#define	EBU_ADDRSEL_BASE			GENMASK(20, 12)							 // Base Address
#define	EBU_ADDRSEL_BASE_SHIFT		12

#define	EBU_BUSCON(n)				MMIO32(EBU_BASE + 0xC0 + ((n) * 0x8))
#define	EBU_BUSCON_MULTMAP			GENMASK(7, 0)							 // Multiplier map
#define	EBU_BUSCON_MULTMAP_SHIFT	0
#define	EBU_BUSCON_WPRE				BIT(8)									 // Weak prefetch
#define	EBU_BUSCON_AALIGN			BIT(9)									 // Address alignment
#define	EBU_BUSCON_CTYPE			GENMASK(2, 10)							 // Cycle Type
#define	EBU_BUSCON_CTYPE_SHIFT		10
#define	EBU_BUSCON_CMULT			GENMASK(3, 13)							 // Cycle multiplier
#define	EBU_BUSCON_CMULT_SHIFT		13
#define	EBU_BUSCON_ENDIAN			BIT(16)									 // Endian mode
#define	EBU_BUSCON_DLOAD			BIT(17)									 // Data upload
#define	EBU_BUSCON_PRE				BIT(18)									 // Prefetch mechanism
#define	EBU_BUSCON_WAITINV			BIT(19)									 // Reversed polarity at WAIT
#define	EBU_BUSCON_BCGEN			GENMASK(2, 20)							 // Signal timing mode
#define	EBU_BUSCON_BCGEN_SHIFT		20
#define	EBU_BUSCON_PORTW			GENMASK(2, 22)							 // Port width
#define	EBU_BUSCON_PORTW_SHIFT		22
#define	EBU_BUSCON_WAIT				GENMASK(2, 24)							 // External wait state
#define	EBU_BUSCON_WAIT_SHIFT		24
#define	EBU_BUSCON_XCMDDELAY		GENMASK(2, 26)							 // External command delay
#define	EBU_BUSCON_XCMDDELAY_SHIFT	26
#define	EBU_BUSCON_AGEN				GENMASK(3, 28)							 // Address generation
#define	EBU_BUSCON_AGEN_SHIFT		28
#define	EBU_BUSCON_WRITE			BIT(31)									 // Write protection

#define	EBU_BUSAP(n)				MMIO32(EBU_BASE + 0x100 + ((n) * 0x8))
#define	EBU_BUSAP_DTACS				GENMASK(4, 0)							 // Between different regions
#define	EBU_BUSAP_DTACS_SHIFT		0
#define	EBU_BUSAP_DTARDWR			GENMASK(4, 4)							 // Between read and write accesses
#define	EBU_BUSAP_DTARDWR_SHIFT		4
#define	EBU_BUSAP_WRRECOVC			GENMASK(3, 8)							 // After write accesses
#define	EBU_BUSAP_WRRECOVC_SHIFT	8
#define	EBU_BUSAP_RDRECOVC			GENMASK(3, 11)							 // After read accesses
#define	EBU_BUSAP_RDRECOVC_SHIFT	11
#define	EBU_BUSAP_DATAC				GENMASK(2, 14)							 // Write accesses
#define	EBU_BUSAP_DATAC_SHIFT		14
#define	EBU_BUSAP_BURSTC			GENMASK(3, 16)							 // During burst accesses
#define	EBU_BUSAP_BURSTC_SHIFT		16
#define	EBU_BUSAP_WAITWRC			GENMASK(3, 19)							 // Programmed for wait accesses
#define	EBU_BUSAP_WAITWRC_SHIFT		19
#define	EBU_BUSAP_WAITRDC			GENMASK(3, 22)							 // Programmed for read accesses
#define	EBU_BUSAP_WAITRDC_SHIFT		22
#define	EBU_BUSAP_CMDDELAY			GENMASK(3, 25)							 // Programmed command
#define	EBU_BUSAP_CMDDELAY_SHIFT	25
#define	EBU_BUSAP_AHOLDC			GENMASK(2, 28)							 // Multiplexed accesses
#define	EBU_BUSAP_AHOLDC_SHIFT		28
#define	EBU_BUSAP_ADDRC				GENMASK(2, 30)							 // Address Cycles
#define	EBU_BUSAP_ADDRC_SHIFT		30

#define	EBU_EMUAS					MMIO32(EBU_BASE + 0x160)
#define	EBU_EMUAS_REGENAB			BIT(0)									 // Memory region
#define	EBU_EMUAS_ALTENAB			BIT(1)									 // Alternate segment comparison
#define	EBU_EMUAS_MASK				GENMASK(4, 4)							 // Address mask
#define	EBU_EMUAS_MASK_SHIFT		4
#define	EBU_EMUAS_ALTSEG			GENMASK(4, 8)							 // Alternate segment
#define	EBU_EMUAS_ALTSEG_SHIFT		8
#define	EBU_EMUAS_BASE				GENMASK(20, 12)							 // Base address
#define	EBU_EMUAS_BASE_SHIFT		12

#define	EBU_EMUBC					MMIO32(EBU_BASE + 0x168)
#define	EBU_EMUBC_MULTMAP			GENMASK(7, 0)							 // Multiplier map
#define	EBU_EMUBC_MULTMAP_SHIFT		0
#define	EBU_EMUBC_WPRE				BIT(8)									 // Weak prefetch
#define	EBU_EMUBC_AALIGN			BIT(9)									 // Address alignment
#define	EBU_EMUBC_CTYPE				GENMASK(2, 10)							 // Cycle Type
#define	EBU_EMUBC_CTYPE_SHIFT		10
#define	EBU_EMUBC_CMULT				GENMASK(3, 13)							 // Cycle multiplier
#define	EBU_EMUBC_CMULT_SHIFT		13
#define	EBU_EMUBC_ENDIAN			BIT(16)									 // Endian mode
#define	EBU_EMUBC_DLOAD				BIT(17)									 // Data upload
#define	EBU_EMUBC_PRE				BIT(18)									 // Prefetch mechanism
#define	EBU_EMUBC_WAITINV			BIT(19)									 // Reversed polarity at WAIT
#define	EBU_EMUBC_BCGEN				GENMASK(2, 20)							 // Signal timing mode
#define	EBU_EMUBC_BCGEN_SHIFT		20
#define	EBU_EMUBC_PORTW				GENMASK(2, 22)							 // Port width
#define	EBU_EMUBC_PORTW_SHIFT		22
#define	EBU_EMUBC_WAIT				GENMASK(2, 24)							 // External wait state
#define	EBU_EMUBC_WAIT_SHIFT		24
#define	EBU_EMUBC_XCMDDELAY			GENMASK(2, 26)							 // External command delay
#define	EBU_EMUBC_XCMDDELAY_SHIFT	26
#define	EBU_EMUBC_AGEN				GENMASK(3, 28)							 // Address generation
#define	EBU_EMUBC_AGEN_SHIFT		28
#define	EBU_EMUBC_WRITE				BIT(31)									 // Write protection

#define	EBU_EMUBAP					MMIO32(EBU_BASE + 0x170)
#define	EBU_EMUBAP_DTACS			GENMASK(4, 0)							 // Between different regions
#define	EBU_EMUBAP_DTACS_SHIFT		0
#define	EBU_EMUBAP_DTARDWR			GENMASK(4, 4)							 // Between read and write accesses
#define	EBU_EMUBAP_DTARDWR_SHIFT	4
#define	EBU_EMUBAP_WRRECOVC			GENMASK(3, 8)							 // After write accesses
#define	EBU_EMUBAP_WRRECOVC_SHIFT	8
#define	EBU_EMUBAP_RDRECOVC			GENMASK(3, 11)							 // After read accesses
#define	EBU_EMUBAP_RDRECOVC_SHIFT	11
#define	EBU_EMUBAP_DATAC			GENMASK(2, 14)							 // Write accesses
#define	EBU_EMUBAP_DATAC_SHIFT		14
#define	EBU_EMUBAP_BURSTC			GENMASK(3, 16)							 // During burst accesses
#define	EBU_EMUBAP_BURSTC_SHIFT		16
#define	EBU_EMUBAP_WAITWRC			GENMASK(3, 19)							 // Programmed for wait accesses
#define	EBU_EMUBAP_WAITWRC_SHIFT	19
#define	EBU_EMUBAP_WAITRDC			GENMASK(3, 22)							 // Programmed for read accesses
#define	EBU_EMUBAP_WAITRDC_SHIFT	22
#define	EBU_EMUBAP_CMDDELAY			GENMASK(3, 25)							 // Programmed command
#define	EBU_EMUBAP_CMDDELAY_SHIFT	25
#define	EBU_EMUBAP_AHOLDC			GENMASK(2, 28)							 // Multiplexed accesses
#define	EBU_EMUBAP_AHOLDC_SHIFT		28
#define	EBU_EMUBAP_ADDRC			GENMASK(2, 30)							 // Address Cycles
#define	EBU_EMUBAP_ADDRC_SHIFT		30

#define	EBU_EMUOVL					MMIO32(EBU_BASE + 0x178)
#define	EBU_EMUOVL_OVERLAY			GENMASK(8, 0)							 // Overlay chip select
#define	EBU_EMUOVL_OVERLAY_SHIFT	0

#define	EBU_USERCON					MMIO32(EBU_BASE + 0x190)


// USART0 [MOD_NUM=0044, MOD_REV=00, MOD_32BIT=00]
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
#define	USART_CON_M						GENMASK(3, 0)			 // ASC Mode Control.
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
#define	USART_PMW_PW_VALUE				GENMASK(8, 0)			 // IrDA Pulse Width Value
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
#define	USART_RXFCON_RXFITL				GENMASK(4, 8)			 // Receive FIFO interrupt trigger level
#define	USART_RXFCON_RXFITL_SHIFT		8

#define	USART_TXFCON(base)				MMIO32((base) + 0x44)
#define	USART_TXFCON_TXFEN				BIT(0)					 // Transmit FIFO enable
#define	USART_TXFCON_TXFFLU				BIT(1)					 // Transmit FIFO flush
#define	USART_TXFCON_TXTMEN				BIT(2)					 // Transmit FIFO transparent mode enable
#define	USART_TXFCON_TXFITL				GENMASK(4, 8)			 // Transmit FIFO interrupt trigger level
#define	USART_TXFCON_TXFITL_SHIFT		8

#define	USART_FSTAT(base)				MMIO32((base) + 0x48)
#define	USART_FSTAT_RXFFL				GENMASK(4, 0)			 // Receive FIFO filling level mask
#define	USART_FSTAT_RXFFL_SHIFT			0
#define	USART_FSTAT_TXFFL				GENMASK(4, 8)			 // Transmit FIFO filling level mask
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
#define	USART_FCCON_RTS_TRIGGER			GENMASK(6, 8)			 // RTS receive FIFO trigger level
#define	USART_FCCON_RTS_TRIGGER_SHIFT	8

#define	USART_FCSTAT(base)				MMIO32((base) + 0x60)
#define	USART_FCSTAT_CTS				BIT(0)					 // CTS Status (0: inactive; 1: active)
#define	USART_FCSTAT_RTS				BIT(1)					 // RTS Status (0: inactive; 1: active)

#define	USART_IMSC(base)				MMIO32((base) + 0x64)
#define	USART_IMSC_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_IMSC_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_IMSC_RX					BIT(2)					 // Receive interrupt mask
#define	USART_IMSC_ERR					BIT(3)					 // Error interrupt mask
#define	USART_IMSC_ABSTART				BIT(4)					 // Autobaud start interrupt mask
#define	USART_IMSC_ABDET				BIT(5)					 // Autobaud detected interrupt mask
#define	USART_IMSC_CTS					BIT(6)					 // CTS interrupt mask
#define	USART_IMSC_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_RIS(base)					MMIO32((base) + 0x68)
#define	USART_RIS_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_RIS_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_RIS_RX					BIT(2)					 // Receive interrupt mask
#define	USART_RIS_ERR					BIT(3)					 // Error interrupt mask
#define	USART_RIS_ABSTART				BIT(4)					 // Autobaud start interrupt mask
#define	USART_RIS_ABDET					BIT(5)					 // Autobaud detected interrupt mask
#define	USART_RIS_CTS					BIT(6)					 // CTS interrupt mask
#define	USART_RIS_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_MIS(base)					MMIO32((base) + 0x6C)
#define	USART_MIS_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_MIS_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_MIS_RX					BIT(2)					 // Receive interrupt mask
#define	USART_MIS_ERR					BIT(3)					 // Error interrupt mask
#define	USART_MIS_ABSTART				BIT(4)					 // Autobaud start interrupt mask
#define	USART_MIS_ABDET					BIT(5)					 // Autobaud detected interrupt mask
#define	USART_MIS_CTS					BIT(6)					 // CTS interrupt mask
#define	USART_MIS_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_ICR(base)					MMIO32((base) + 0x70)
#define	USART_ICR_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_ICR_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_ICR_RX					BIT(2)					 // Receive interrupt mask
#define	USART_ICR_ERR					BIT(3)					 // Error interrupt mask
#define	USART_ICR_ABSTART				BIT(4)					 // Autobaud start interrupt mask
#define	USART_ICR_ABDET					BIT(5)					 // Autobaud detected interrupt mask
#define	USART_ICR_CTS					BIT(6)					 // CTS interrupt mask
#define	USART_ICR_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_ISR(base)					MMIO32((base) + 0x74)
#define	USART_ISR_TX					BIT(0)					 // Transmit interrupt mask
#define	USART_ISR_TB					BIT(1)					 // Transmit buffer interrupt mask
#define	USART_ISR_RX					BIT(2)					 // Receive interrupt mask
#define	USART_ISR_ERR					BIT(3)					 // Error interrupt mask
#define	USART_ISR_ABSTART				BIT(4)					 // Autobaud start interrupt mask
#define	USART_ISR_ABDET					BIT(5)					 // Autobaud detected interrupt mask
#define	USART_ISR_CTS					BIT(6)					 // CTS interrupt mask
#define	USART_ISR_TMO					BIT(7)					 // RX timeout interrupt mask

#define	USART_TMO(base)					MMIO32((base) + 0x7C)


// DIF [MOD_NUM=F043, MOD_REV=00, MOD_32BIT=C0]
// Looks like DIF (Display Interface) module, but not sure.
#define	DIF_BASE	0xF1B00000
/* Clock Control Register */
#define	DIF_CLC		MMIO32(DIF_BASE + 0x00)

/* Module Identifier Register */
#define	DIF_ID		MMIO32(DIF_BASE + 0x08)


// USB [MOD_NUM=F047, MOD_REV=00, MOD_32BIT=C0]
// Looks like USB module, but not sure.
#define	USB_BASE	0xF2200800
/* Clock Control Register */
#define	USB_CLC		MMIO32(USB_BASE + 0x00)

/* Module Identifier Register */
#define	USB_ID		MMIO32(USB_BASE + 0x08)


// NVIC [MOD_NUM=0031, MOD_REV=00, MOD_32BIT=C0]
// NVIC module, registers collected using tests on real hardware (using "black box" method).
#define	NVIC_BASE				0xF2800000
/* Module Identifier Register */
#define	NVIC_ID					MMIO32(NVIC_BASE + 0x00)

#define	NVIC_FIQ_ACK			MMIO32(NVIC_BASE + 0x08)

#define	NVIC_IRQ_ACK			MMIO32(NVIC_BASE + 0x14)

#define	NVIC_CURRENT_FIQ		MMIO32(NVIC_BASE + 0x18)

#define	NVIC_CURRENT_IRQ		MMIO32(NVIC_BASE + 0x1C)

#define	NVIC_CON(n)				MMIO32(NVIC_BASE + 0x30 + ((n) * 0x4))
#define	NVIC_CON_PRIORITY		GENMASK(8, 0)
#define	NVIC_CON_PRIORITY_SHIFT	0
#define	NVIC_CON_FIQ			BIT(8)


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
#define	DMAC_RAW_ERR_CLEAR						MMIO32(DMAC_BASE + 0x18)
#define	DMAC_RAW_ERR_CLEAR_CH0					BIT(0)
#define	DMAC_RAW_ERR_CLEAR_CH1					BIT(1)
#define	DMAC_RAW_ERR_CLEAR_CH2					BIT(2)
#define	DMAC_RAW_ERR_CLEAR_CH3					BIT(3)
#define	DMAC_RAW_ERR_CLEAR_CH4					BIT(4)
#define	DMAC_RAW_ERR_CLEAR_CH5					BIT(5)
#define	DMAC_RAW_ERR_CLEAR_CH6					BIT(6)
#define	DMAC_RAW_ERR_CLEAR_CH7					BIT(7)

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

#define	DMAC_CH_DST_ADDR						MMIO32(DMAC_BASE + 0x104)

#define	DMAC_CH_LLI								MMIO32(DMAC_BASE + 0x108)
#define	DMAC_CH_LLI_LM							BIT(0)										 // AHB master select for loading the next LLI
#define	DMAC_CH_LLI_LM_AHB1						0x0
#define	DMAC_CH_LLI_LM_AHB2						0x1
#define	DMAC_CH_LLI_LLI							GENMASK(29, 2)								 // Linked list item
#define	DMAC_CH_LLI_LLI_SHIFT					2

#define	DMAC_CH_CONTROL(n)						MMIO32(DMAC_BASE + 0x10C + ((n) * 0x20))
#define	DMAC_CH_CONTROL_TRANSFER_SZIE			GENMASK(12, 0)								 // Transfer size.
#define	DMAC_CH_CONTROL_TRANSFER_SZIE_SHIFT		0
#define	DMAC_CH_CONTROL_SB_SIZE					GENMASK(3, 12)								 // Source burst size
#define	DMAC_CH_CONTROL_SB_SIZE_SHIFT			12
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_1			0x0
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_4			0x1000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_8			0x2000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_16			0x3000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_32			0x4000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_64			0x5000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_128			0x6000
#define	DMAC_CH_CONTROL_SB_SIZE_SZ_256			0x7000
#define	DMAC_CH_CONTROL_DB_SIZE					GENMASK(3, 15)								 // Destination burst size
#define	DMAC_CH_CONTROL_DB_SIZE_SHIFT			15
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_1			0x0
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_4			0x8000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_8			0x10000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_16			0x18000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_32			0x20000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_64			0x28000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_128			0x30000
#define	DMAC_CH_CONTROL_DB_SIZE_SZ_256			0x38000
#define	DMAC_CH_CONTROL_S_WIDTH					GENMASK(3, 18)								 // Source transfer width
#define	DMAC_CH_CONTROL_S_WIDTH_SHIFT			18
#define	DMAC_CH_CONTROL_S_WIDTH_BYTE			0x0
#define	DMAC_CH_CONTROL_S_WIDTH_WORD			0x40000
#define	DMAC_CH_CONTROL_S_WIDTH_DWORD			0x80000
#define	DMAC_CH_CONTROL_D_WIDTH					GENMASK(3, 21)								 // Destination transfer width
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
#define	DMAC_CH_CONTROL_PROTECTION				GENMASK(3, 28)								 // Protection.
#define	DMAC_CH_CONTROL_PROTECTION_SHIFT		28
#define	DMAC_CH_CONTROL_I						BIT(31)										 // Terminal count interrupt enable bit.

#define	DMAC_CH_CONFIG(n)						MMIO32(DMAC_BASE + 0x110 + ((n) * 0x20))
#define	DMAC_CH_CONFIG_ENABLE					BIT(0)										 // Channel enable.
#define	DMAC_CH_CONFIG_SRC_PERIPH				GENMASK(4, 1)								 // Source peripheral.
#define	DMAC_CH_CONFIG_SRC_PERIPH_SHIFT			1
#define	DMAC_CH_CONFIG_DST_PERIPH				GENMASK(6, 5)								 // Destination peripheral.
#define	DMAC_CH_CONFIG_DST_PERIPH_SHIFT			5
#define	DMAC_CH_CONFIG_FLOW_CTRL				GENMASK(3, 11)								 // Flow control and transfer type
#define	DMAC_CH_CONFIG_FLOW_CTRL_SHIFT			11
#define	DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM		0x0
#define	DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER		0x800
#define	DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM		0x1000
#define	DMAC_CH_CONFIG_FLOW_CTRL_SRC2DST		0x1800
#define	DMAC_CH_CONFIG_FLOW_CTRL_SRC2DST_DST	0x2000
#define	DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER	0x2800
#define	DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER	0x3000
#define	DMAC_CH_CONFIG_FLOW_CTRL_SRC2DST_SRC	0x3800
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


// CAPCOM0 [MOD_NUM=0050, MOD_REV=00, MOD_32BIT=00]
// CAPCOM from drivers/clocksource/xgold_capcom_timer.c (GPL2)
#define	CAPCOM0_BASE				0xF4000000
#define	CAPCOM0						0xF4000000

#define	CAPCOM1_BASE				0xF4100000
#define	CAPCOM1						0xF4100000

/* Clock Control Register */
#define	CAPCOM_CLC(base)			MMIO32((base) + 0x00)

#define	CAPCOM_CPISEL(base)			MMIO32((base) + 0x04)
#define	CAPCOM_CPISEL_C1C0IS		BIT(0)
#define	CAPCOM_CPISEL_C3C2IS		BIT(1)
#define	CAPCOM_CPISEL_C5C4IS		BIT(2)
#define	CAPCOM_CPISEL_C7C6IS		BIT(3)
#define	CAPCOM_CPISEL_T0INIS		BIT(4)
#define	CAPCOM_CPISEL_T1INIS		BIT(5)

#define	CAPCOM_PISEL(base)			MMIO32((base) + 0x04)

/* Module Identifier Register */
#define	CAPCOM_ID(base)				MMIO32((base) + 0x08)

#define	CAPCOM_T01CON(base)			MMIO32((base) + 0x10)
#define	CAPCOM_T01CON_T0I			GENMASK(3, 0)
#define	CAPCOM_T01CON_T0I_SHIFT		0
#define	CAPCOM_T01CON_T0M			BIT(3)
#define	CAPCOM_T01CON_T0R			BIT(6)
#define	CAPCOM_T01CON_T1I			GENMASK(3, 8)
#define	CAPCOM_T01CON_T1I_SHIFT		8
#define	CAPCOM_T01CON_T1M			BIT(11)
#define	CAPCOM_T01CON_T1R			BIT(14)

#define	CAPCOM_CCM0(base)			MMIO32((base) + 0x14)
#define	CAPCOM_CCM0_MOD0			GENMASK(3, 0)
#define	CAPCOM_CCM0_MOD0_SHIFT		0
#define	CAPCOM_CCM0_ACC0			BIT(3)
#define	CAPCOM_CCM0_ACC0_TIM0		0x0
#define	CAPCOM_CCM0_ACC0_TIM1		0x8
#define	CAPCOM_CCM0_MOD1			GENMASK(3, 4)
#define	CAPCOM_CCM0_MOD1_SHIFT		4
#define	CAPCOM_CCM0_ACC1			BIT(7)
#define	CAPCOM_CCM0_ACC1_TIM0		0x0
#define	CAPCOM_CCM0_ACC1_TIM1		0x80
#define	CAPCOM_CCM0_MOD2			GENMASK(3, 8)
#define	CAPCOM_CCM0_MOD2_SHIFT		8
#define	CAPCOM_CCM0_ACC2			BIT(11)
#define	CAPCOM_CCM0_ACC2_TIM0		0x0
#define	CAPCOM_CCM0_ACC2_TIM1		0x800
#define	CAPCOM_CCM0_MOD3			GENMASK(3, 12)
#define	CAPCOM_CCM0_MOD3_SHIFT		12
#define	CAPCOM_CCM0_ACC3			BIT(15)
#define	CAPCOM_CCM0_ACC3_TIM0		0x0
#define	CAPCOM_CCM0_ACC3_TIM1		0x8000

#define	CAPCOM_CCM1(base)			MMIO32((base) + 0x18)
#define	CAPCOM_CCM1_MOD4			GENMASK(3, 0)
#define	CAPCOM_CCM1_MOD4_SHIFT		0
#define	CAPCOM_CCM1_ACC4			BIT(3)
#define	CAPCOM_CCM1_ACC4_TIM0		0x0
#define	CAPCOM_CCM1_ACC4_TIM1		0x8
#define	CAPCOM_CCM1_MOD5			GENMASK(3, 4)
#define	CAPCOM_CCM1_MOD5_SHIFT		4
#define	CAPCOM_CCM1_ACC5			BIT(7)
#define	CAPCOM_CCM1_ACC5_TIM0		0x0
#define	CAPCOM_CCM1_ACC5_TIM1		0x80
#define	CAPCOM_CCM1_MOD6			GENMASK(3, 8)
#define	CAPCOM_CCM1_MOD6_SHIFT		8
#define	CAPCOM_CCM1_ACC6			BIT(11)
#define	CAPCOM_CCM1_ACC6_TIM0		0x0
#define	CAPCOM_CCM1_ACC6_TIM1		0x800
#define	CAPCOM_CCM1_MOD7			GENMASK(3, 12)
#define	CAPCOM_CCM1_MOD7_SHIFT		12
#define	CAPCOM_CCM1_ACC7			BIT(15)
#define	CAPCOM_CCM1_ACC7_TIM0		0x0
#define	CAPCOM_CCM1_ACC7_TIM1		0x8000

#define	CAPCOM_OUT(base)			MMIO32((base) + 0x24)
#define	CAPCOM_OUT_O0				BIT(0)
#define	CAPCOM_OUT_O1				BIT(1)
#define	CAPCOM_OUT_O2				BIT(2)
#define	CAPCOM_OUT_O3				BIT(3)
#define	CAPCOM_OUT_O4				BIT(4)
#define	CAPCOM_OUT_O5				BIT(5)
#define	CAPCOM_OUT_O6				BIT(6)
#define	CAPCOM_OUT_O7				BIT(7)

#define	CAPCOM_IOC(base)			MMIO32((base) + 0x28)
#define	CAPCOM_IOC_PDS				BIT(3)
#define	CAPCOM_IOC_PDS_OUT			0x0
#define	CAPCOM_IOC_PDS_IN			0x8

#define	CAPCOM_SEM(base)			MMIO32((base) + 0x2C)
#define	CAPCOM_SEM_SEM0				BIT(0)
#define	CAPCOM_SEM_SEM1				BIT(1)
#define	CAPCOM_SEM_SEM2				BIT(2)
#define	CAPCOM_SEM_SEM3				BIT(3)
#define	CAPCOM_SEM_SEM4				BIT(4)
#define	CAPCOM_SEM_SEM5				BIT(5)
#define	CAPCOM_SEM_SEM6				BIT(6)
#define	CAPCOM_SEM_SEM7				BIT(7)

#define	CAPCOM_SEE(base)			MMIO32((base) + 0x30)
#define	CAPCOM_SEE_SEE0				BIT(0)
#define	CAPCOM_SEE_SEE1				BIT(1)
#define	CAPCOM_SEE_SEE2				BIT(2)
#define	CAPCOM_SEE_SEE3				BIT(3)
#define	CAPCOM_SEE_SEE4				BIT(4)
#define	CAPCOM_SEE_SEE5				BIT(5)
#define	CAPCOM_SEE_SEE6				BIT(6)
#define	CAPCOM_SEE_SEE7				BIT(7)

#define	CAPCOM_DRM(base)			MMIO32((base) + 0x34)
#define	CAPCOM_DRM_DR0M				GENMASK(2, 0)
#define	CAPCOM_DRM_DR0M_SHIFT		0
#define	CAPCOM_DRM_DR0M_CON			0x0
#define	CAPCOM_DRM_DR0M_DIS			0x1
#define	CAPCOM_DRM_DR0M_EN			0x2
#define	CAPCOM_DRM_DR0M_RES			0x3
#define	CAPCOM_DRM_DR1M				GENMASK(2, 2)
#define	CAPCOM_DRM_DR1M_SHIFT		2
#define	CAPCOM_DRM_DR1M_CON			0x0
#define	CAPCOM_DRM_DR1M_DIS			0x4
#define	CAPCOM_DRM_DR1M_EN			0x8
#define	CAPCOM_DRM_DR1M_RES			0xC
#define	CAPCOM_DRM_DR2M				GENMASK(2, 4)
#define	CAPCOM_DRM_DR2M_SHIFT		4
#define	CAPCOM_DRM_DR2M_CON			0x0
#define	CAPCOM_DRM_DR2M_DIS			0x10
#define	CAPCOM_DRM_DR2M_EN			0x20
#define	CAPCOM_DRM_DR2M_RES			0x30
#define	CAPCOM_DRM_DR3M				GENMASK(2, 6)
#define	CAPCOM_DRM_DR3M_SHIFT		6
#define	CAPCOM_DRM_DR3M_CON			0x0
#define	CAPCOM_DRM_DR3M_DIS			0x40
#define	CAPCOM_DRM_DR3M_EN			0x80
#define	CAPCOM_DRM_DR3M_RES			0xC0

#define	CAPCOM_WHBSSEE(base)		MMIO32((base) + 0x38)
#define	CAPCOM_WHBSSEE_SETSEE0		BIT(0)
#define	CAPCOM_WHBSSEE_SETSEE0_NOE	0x0
#define	CAPCOM_WHBSSEE_SETSEE0_SET	0x1
#define	CAPCOM_WHBSSEE_SETSEE1		BIT(1)
#define	CAPCOM_WHBSSEE_SETSEE1_NOE	0x0
#define	CAPCOM_WHBSSEE_SETSEE1_SET	0x2
#define	CAPCOM_WHBSSEE_SETSEE2		BIT(2)
#define	CAPCOM_WHBSSEE_SETSEE2_NOE	0x0
#define	CAPCOM_WHBSSEE_SETSEE2_SET	0x4
#define	CAPCOM_WHBSSEE_SETSEE3		BIT(3)
#define	CAPCOM_WHBSSEE_SETSEE3_NOE	0x0
#define	CAPCOM_WHBSSEE_SETSEE3_SET	0x8
#define	CAPCOM_WHBSSEE_SETSEE4		BIT(4)
#define	CAPCOM_WHBSSEE_SETSEE4_NOE	0x0
#define	CAPCOM_WHBSSEE_SETSEE4_SET	0x10
#define	CAPCOM_WHBSSEE_SETSEE5		BIT(5)
#define	CAPCOM_WHBSSEE_SETSEE5_NOE	0x0
#define	CAPCOM_WHBSSEE_SETSEE5_SET	0x20
#define	CAPCOM_WHBSSEE_SETSEE6		BIT(6)
#define	CAPCOM_WHBSSEE_SETSEE6_NOE	0x0
#define	CAPCOM_WHBSSEE_SETSEE6_SET	0x40
#define	CAPCOM_WHBSSEE_SETSEE7		BIT(7)
#define	CAPCOM_WHBSSEE_SETSEE7_NOE	0x0
#define	CAPCOM_WHBSSEE_SETSEE7_SET	0x80

#define	CAPCOM_WHBCSEE(base)		MMIO32((base) + 0x3C)
#define	CAPCOM_WHBCSEE_CLRSEE0		BIT(0)
#define	CAPCOM_WHBCSEE_CLRSEE0_NOE	0x0
#define	CAPCOM_WHBCSEE_CLRSEE0_CLR	0x1
#define	CAPCOM_WHBCSEE_CLRSEE1		BIT(1)
#define	CAPCOM_WHBCSEE_CLRSEE1_NOE	0x0
#define	CAPCOM_WHBCSEE_CLRSEE1_CLR	0x2
#define	CAPCOM_WHBCSEE_CLRSEE2		BIT(2)
#define	CAPCOM_WHBCSEE_CLRSEE2_NOE	0x0
#define	CAPCOM_WHBCSEE_CLRSEE2_CLR	0x4
#define	CAPCOM_WHBCSEE_CLRSEE3		BIT(3)
#define	CAPCOM_WHBCSEE_CLRSEE3_NOE	0x0
#define	CAPCOM_WHBCSEE_CLRSEE3_CLR	0x8
#define	CAPCOM_WHBCSEE_CLRSEE4		BIT(4)
#define	CAPCOM_WHBCSEE_CLRSEE4_NOE	0x0
#define	CAPCOM_WHBCSEE_CLRSEE4_CLR	0x10
#define	CAPCOM_WHBCSEE_CLRSEE5		BIT(5)
#define	CAPCOM_WHBCSEE_CLRSEE5_NOE	0x0
#define	CAPCOM_WHBCSEE_CLRSEE5_CLR	0x20
#define	CAPCOM_WHBCSEE_CLRSEE6		BIT(6)
#define	CAPCOM_WHBCSEE_CLRSEE6_NOE	0x0
#define	CAPCOM_WHBCSEE_CLRSEE6_CLR	0x40
#define	CAPCOM_WHBCSEE_CLRSEE7		BIT(7)
#define	CAPCOM_WHBCSEE_CLRSEE7_NOE	0x0
#define	CAPCOM_WHBCSEE_CLRSEE7_CLR	0x80

#define	CAPCOM_T0(base)				MMIO32((base) + 0x40)
#define	CAPCOM_T0_T0				GENMASK(31, 0)
#define	CAPCOM_T0_T0_SHIFT			0
#define	CAPCOM_T0_OVF0				BIT(31)
#define	CAPCOM_T0_OVF0_CLEARED		0x0
#define	CAPCOM_T0_OVF0_SET			0x80000000

#define	CAPCOM_T0REL(base)			MMIO32((base) + 0x44)
#define	CAPCOM_T0REL_T0REL			GENMASK(31, 0)
#define	CAPCOM_T0REL_T0REL_SHIFT	0

#define	CAPCOM_T1(base)				MMIO32((base) + 0x48)
#define	CAPCOM_T1_T1				GENMASK(31, 0)
#define	CAPCOM_T1_T1_SHIFT			0
#define	CAPCOM_T1_OVF1				BIT(31)
#define	CAPCOM_T1_OVF1_CLEARED		0x0
#define	CAPCOM_T1_OVF1_SET			0x80000000

#define	CAPCOM_T1REL(base)			MMIO32((base) + 0x4C)
#define	CAPCOM_T1REL_T1REL			GENMASK(31, 0)
#define	CAPCOM_T1REL_T1REL_SHIFT	0

#define	CAPCOM_CC(base, n)			MMIO32(base + 0x50 + ((n) * 0x4))

#define	CAPCOM_T01OCR(base)			MMIO32((base) + 0x94)
#define	CAPCOM_T01OCR_CT0			BIT(0)
#define	CAPCOM_T01OCR_CT0_NOC		0x0
#define	CAPCOM_T01OCR_CT0_CSR		0x1
#define	CAPCOM_T01OCR_CT1			BIT(1)
#define	CAPCOM_T01OCR_CT1_NOC		0x0
#define	CAPCOM_T01OCR_CT1_CSR		0x2

#define	CAPCOM_WHBSOUT(base)		MMIO32((base) + 0x98)
#define	CAPCOM_WHBSOUT_SET0O		BIT(0)
#define	CAPCOM_WHBSOUT_SET1O		BIT(1)
#define	CAPCOM_WHBSOUT_SET2O		BIT(2)
#define	CAPCOM_WHBSOUT_SET3O		BIT(3)
#define	CAPCOM_WHBSOUT_SET4O		BIT(4)
#define	CAPCOM_WHBSOUT_SET5O		BIT(5)
#define	CAPCOM_WHBSOUT_SET6O		BIT(6)
#define	CAPCOM_WHBSOUT_SET7O		BIT(7)

#define	CAPCOM_WHBCOUT(base)		MMIO32((base) + 0x9C)
#define	CAPCOM_WHBCOUT_CLR0O		BIT(0)
#define	CAPCOM_WHBCOUT_CLR1O		BIT(1)
#define	CAPCOM_WHBCOUT_CLR2O		BIT(2)
#define	CAPCOM_WHBCOUT_CLR3O		BIT(3)
#define	CAPCOM_WHBCOUT_CLR4O		BIT(4)
#define	CAPCOM_WHBCOUT_CLR5O		BIT(5)
#define	CAPCOM_WHBCOUT_CLR6O		BIT(6)
#define	CAPCOM_WHBCOUT_CLR7O		BIT(7)

/* Service Routing Control Register */
#define	CAPCOM_CC7_SRC(base)		MMIO32((base) + 0xD8)

/* Service Routing Control Register */
#define	CAPCOM_CC6_SRC(base)		MMIO32((base) + 0xDC)

/* Service Routing Control Register */
#define	CAPCOM_CC5_SRC(base)		MMIO32((base) + 0xE0)

/* Service Routing Control Register */
#define	CAPCOM_CC4_SRC(base)		MMIO32((base) + 0xE4)

/* Service Routing Control Register */
#define	CAPCOM_CC3_SRC(base)		MMIO32((base) + 0xE8)

/* Service Routing Control Register */
#define	CAPCOM_CC2_SRC(base)		MMIO32((base) + 0xEC)

/* Service Routing Control Register */
#define	CAPCOM_CC1_SRC(base)		MMIO32((base) + 0xF0)

/* Service Routing Control Register */
#define	CAPCOM_CC0_SRC(base)		MMIO32((base) + 0xF4)

/* Service Routing Control Register */
#define	CAPCOM_T1_SRC(base)			MMIO32((base) + 0xF8)

/* Service Routing Control Register */
#define	CAPCOM_T0_SRC(base)			MMIO32((base) + 0xFC)


// GPIO [MOD_NUM=F023, MOD_REV=00, MOD_32BIT=C0]
// GPIO module, registers found on some "siemens club" forum, dont known source.
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
#define	GPIO_IS				GENMASK(3, 0)
#define	GPIO_IS_SHIFT		0
#define	GPIO_IS_NONE		0x0
#define	GPIO_IS_ALT0		0x1
#define	GPIO_IS_ALT1		0x2
#define	GPIO_IS_ALT2		0x3
#define	GPIO_IS_ALT3		0x4
#define	GPIO_IS_ALT4		0x5
#define	GPIO_IS_ALT5		0x6
#define	GPIO_IS_ALT6		0x7
#define	GPIO_OS				GENMASK(3, 4)
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
#define	GPIO_PDPU			GENMASK(2, 13)
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
#define	SCU_RST_SR_HWCFG			GENMASK(3, 16)							 // Boot Configuration Selection Status
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
#define	SCU_RST_REQ_SWCFG			GENMASK(3, 16)							 // Software Boot Configuration
#define	SCU_RST_REQ_SWCFG_SHIFT		16
#define	SCU_RST_REQ_SWBRKIN			BIT(21)									 // Software Break Signal Boot Value
#define	SCU_RST_REQ_SWBOOT			BIT(24)									 // Software Boot Configuration Selection

#define	SCU_WDTCON0					MMIO32(SCU_BASE + 0x24)
#define	SCU_WDTCON0_ENDINIT			BIT(0)									 // End-of-Initialization Control Bit.
#define	SCU_WDTCON0_WDTLCK			BIT(1)									 // Lock bit to Control Access to WDT_CON0.
#define	SCU_WDTCON0_WDTHPW0			GENMASK(2, 2)							 // Hardware Password 0.
#define	SCU_WDTCON0_WDTHPW0_SHIFT	2
#define	SCU_WDTCON0_WDTHPW1			GENMASK(4, 4)							 // Hardware Password 1.
#define	SCU_WDTCON0_WDTHPW1_SHIFT	4
#define	SCU_WDTCON0_WDTPW			GENMASK(8, 8)							 // User-Definable Password Field for Access to WDT_CON0.
#define	SCU_WDTCON0_WDTPW_SHIFT		8
#define	SCU_WDTCON0_WDTREL			GENMASK(16, 16)							 // Reload Value for the Watchdog Timer.
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
#define	SCU_WDT_SR_WDTTIM			GENMASK(16, 16)							 // Watchdog Timer Value
#define	SCU_WDT_SR_WDTTIM_SHIFT		16

#define	SCU_EXTI					MMIO32(SCU_BASE + 0x3C)
#define	SCU_EXTI_EXT0_FALLING		BIT(0)
#define	SCU_EXTI_EXT0_RISING		BIT(1)
#define	SCU_EXTI_EXT1_FALLING		BIT(2)
#define	SCU_EXTI_EXT1_RISING		BIT(3)
#define	SCU_EXTI_EXT2_FALLING		BIT(4)
#define	SCU_EXTI_EXT2_RISING		BIT(5)
#define	SCU_EXTI_EXT3_FALLING		BIT(6)
#define	SCU_EXTI_EXT3_RISING		BIT(7)
#define	SCU_EXTI_EXT4_FALLING		BIT(8)
#define	SCU_EXTI_EXT4_RISING		BIT(9)
#define	SCU_EXTI_EXT5_FALLING		BIT(10)
#define	SCU_EXTI_EXT5_RISING		BIT(11)
#define	SCU_EXTI_EXT6_FALLING		BIT(12)
#define	SCU_EXTI_EXT6_RISING		BIT(13)
#define	SCU_EXTI_EXT7_FALLING		BIT(14)
#define	SCU_EXTI_EXT7_RISING		BIT(15)

#define	SCU_EBUCLC1					MMIO32(SCU_BASE + 0x40)
#define	SCU_EBUCLC1_FLAG1			GENMASK(4, 0)
#define	SCU_EBUCLC1_FLAG1_SHIFT		0
#define	SCU_EBUCLC1_READY			GENMASK(4, 4)
#define	SCU_EBUCLC1_READY_SHIFT		4

#define	SCU_EBUCLC2					MMIO32(SCU_BASE + 0x44)
#define	SCU_EBUCLC2_FLAG1			GENMASK(4, 0)
#define	SCU_EBUCLC2_FLAG1_SHIFT		0
#define	SCU_EBUCLC2_READY			GENMASK(4, 4)
#define	SCU_EBUCLC2_READY_SHIFT		4

#define	SCU_EBUCLC					MMIO32(SCU_BASE + 0x48)
#define	SCU_EBUCLC_LOCK				BIT(0)
#define	SCU_EBUCLC_VCOBYP			BIT(8)

#define	SCU_MANID					MMIO32(SCU_BASE + 0x5C)
#define	SCU_MANID_DEPT				GENMASK(4, 0)
#define	SCU_MANID_DEPT_SHIFT		0
#define	SCU_MANID_MANUF				GENMASK(11, 4)
#define	SCU_MANID_MANUF_SHIFT		4

#define	SCU_CHIPID					MMIO32(SCU_BASE + 0x60)
#define	SCU_CHIPID_CHREV			GENMASK(8, 0)
#define	SCU_CHIPID_CHREV_SHIFT		0
#define	SCU_CHIPID_MANUF			GENMASK(8, 8)
#define	SCU_CHIPID_MANUF_SHIFT		8

#define	SCU_RTCIF					MMIO32(SCU_BASE + 0x64)

#define	SCU_BOOT_CFG				MMIO32(SCU_BASE + 0x74)
#define	SCU_BOOT_CFG_USART1			BIT(28)									 // Allow boot from USART1
#define	SCU_BOOT_CFG_BYPASS_FW		BIT(29)									 // Force boot from 0x82000, bypass firmware
#define	SCU_BOOT_CFG_USB			BIT(30)									 // Allow boot from USB

#define	SCU_BOOT_FLAG				MMIO32(SCU_BASE + 0x78)
#define	SCU_BOOT_FLAG_BOOT_OK		BIT(0)

#define	SCU_ROMAMCR					MMIO32(SCU_BASE + 0x7C)
#define	SCU_ROMAMCR_MOUNT_BROM		BIT(0)

#define	SCU_RTID					MMIO32(SCU_BASE + 0x80)

/* DMA Request Select Register */
#define	SCU_DMARS					MMIO32(SCU_BASE + 0x84)
#define	SCU_DMARS_SEL0				BIT(0)									 // Request Select Bit 0
#define	SCU_DMARS_SEL1				BIT(1)									 // Request Select Bit 1
#define	SCU_DMARS_SEL2				BIT(2)									 // Request Select Bit 2
#define	SCU_DMARS_SEL3				BIT(3)									 // Request Select Bit 3
#define	SCU_DMARS_SEL4				BIT(4)									 // Request Select Bit 4
#define	SCU_DMARS_SEL5				BIT(5)									 // Request Select Bit 5
#define	SCU_DMARS_SEL6				BIT(6)									 // Request Select Bit 6
#define	SCU_DMARS_SEL7				BIT(7)									 // Request Select Bit 7
#define	SCU_DMARS_SEL8				BIT(8)									 // Request Select Bit 8
#define	SCU_DMARS_SEL9				BIT(9)									 // Request Select Bit 9

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
// Looks like CGU module, registers collected using tests on real hardware (using "black box" method).
#define	PLL_BASE						0xF4500000
#define	PLL_OSC							MMIO32(PLL_BASE + 0xA0)
#define	PLL_OSC_LOCK					BIT(0)
#define	PLL_OSC_NDIV					GENMASK(3, 16)			 // Feedback divider (multiply by N+1)
#define	PLL_OSC_NDIV_SHIFT				16

#define	PLL_CON0						MMIO32(PLL_BASE + 0xA4)
#define	PLL_CON0_PLL1_K2				GENMASK(3, 0)			 // div by (K1 * 6 + (K2 - 1))
#define	PLL_CON0_PLL1_K2_SHIFT			0
#define	PLL_CON0_PLL1_K1				GENMASK(4, 3)
#define	PLL_CON0_PLL1_K1_SHIFT			3
#define	PLL_CON0_PLL2_K2				GENMASK(3, 8)			 // div by (K1 * 6 + (K2 - 1))
#define	PLL_CON0_PLL2_K2_SHIFT			8
#define	PLL_CON0_PLL2_K1				GENMASK(4, 11)
#define	PLL_CON0_PLL2_K1_SHIFT			11
#define	PLL_CON0_PLL3_K2				GENMASK(3, 16)			 // div by (K1 * 6 + (K2 - 1))
#define	PLL_CON0_PLL3_K2_SHIFT			16
#define	PLL_CON0_PLL3_K1				GENMASK(4, 19)
#define	PLL_CON0_PLL3_K1_SHIFT			19
#define	PLL_CON0_PLL4_K2				GENMASK(3, 24)			 // div by (K1 * 6 + (K2 - 1))
#define	PLL_CON0_PLL4_K2_SHIFT			24
#define	PLL_CON0_PLL4_K1				GENMASK(4, 27)
#define	PLL_CON0_PLL4_K1_SHIFT			27

#define	PLL_CON1						MMIO32(PLL_BASE + 0xA8)
#define	PLL_CON1_FSYS_CLKSEL			GENMASK(2, 16)			 // Source clock for fSYS (OSC_DIV1: fSYS=fOSC, PLL_DIV2: fSYS=fPLL / 2)
#define	PLL_CON1_FSYS_CLKSEL_SHIFT		16
#define	PLL_CON1_FSYS_CLKSEL_BYPASS		0x0
#define	PLL_CON1_FSYS_CLKSEL_PLL_DIV2	0x20000
#define	PLL_CON1_FSYS_CLKSEL_DISABLE	0x30000
#define	PLL_CON1_AHB_CLKSEL				GENMASK(3, 20)			 // Source clock for fPLL
#define	PLL_CON1_AHB_CLKSEL_SHIFT		20
#define	PLL_CON1_AHB_CLKSEL_BYPASS		0x0
#define	PLL_CON1_AHB_CLKSEL_PLL0		0x200000
#define	PLL_CON1_AHB_CLKSEL_PLL1		0x300000
#define	PLL_CON1_AHB_CLKSEL_PLL2		0x400000
#define	PLL_CON1_AHB_CLKSEL_PLL3		0x500000
#define	PLL_CON1_AHB_CLKSEL_PLL4		0x600000
#define	PLL_CON1_FSTM_DIV_EN			BIT(25)					 // Enable fSTM divider
#define	PLL_CON1_FSTM_DIV				GENMASK(2, 28)			 // fSTM divider value (n^2)
#define	PLL_CON1_FSTM_DIV_SHIFT			28
#define	PLL_CON1_FSTM_DIV_1				0x0
#define	PLL_CON1_FSTM_DIV_2				0x10000000
#define	PLL_CON1_FSTM_DIV_4				0x20000000
#define	PLL_CON1_FSTM_DIV_8				0x30000000

#define	PLL_CON2						MMIO32(PLL_BASE + 0xAC)
#define	PLL_CON2_CPU_DIV				GENMASK(2, 8)
#define	PLL_CON2_CPU_DIV_SHIFT			8
#define	PLL_CON2_CPU_DIV_EN				BIT(12)

#define	PLL_STAT						MMIO32(PLL_BASE + 0xB0)

#define	PLL_CON3						MMIO32(PLL_BASE + 0xB4)

/* Service Routing Control Register */
#define	PLL_SRC							MMIO32(PLL_BASE + 0xCC)


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
#define	RTC_CON_RUN				BIT(0)					 // RTC Enable
#define	RTC_CON_PRE				BIT(1)					 // RTC Input Source Pre-Scaler Enable
#define	RTC_CON_T14DEC			BIT(2)					 // Decrement T14 Timer Value
#define	RTC_CON_T14INC			BIT(3)					 // Increment T14 Timer Value
#define	RTC_CON_ACCPOS			BIT(4)					 // RTC Register Access Possible

/* Timer T14 Count/Reload Register */
#define	RTC_T14					MMIO32(RTC_BASE + 0x18)
#define	RTC_T14_REL				GENMASK(16, 0)			 // Timer T14 Reload Value
#define	RTC_T14_REL_SHIFT		0
#define	RTC_T14_CNT				GENMASK(16, 16)			 // Timer T14 Count Value
#define	RTC_T14_CNT_SHIFT		16

/* RTC Count Register */
#define	RTC_CNT					MMIO32(RTC_BASE + 0x1C)
#define	RTC_CNT_CNT				GENMASK(32, 0)			 // RTC Timer Count Value
#define	RTC_CNT_CNT_SHIFT		0

/* RTC Reload Register */
#define	RTC_REL					MMIO32(RTC_BASE + 0x20)
#define	RTC_REL_REL				GENMASK(32, 0)			 // RTC Timer Reload Value
#define	RTC_REL_REL_SHIFT		0

/* Interrupt Sub-Node Control Register */
#define	RTC_ISNC				MMIO32(RTC_BASE + 0x24)
#define	RTC_ISNC_T14IE			BIT(0)					 // T14 Overflow Interrupt Enable Control
#define	RTC_ISNC_T14IR			BIT(1)					 // T14 Overflow Interrupt Request
#define	RTC_ISNC_RTC0IE			BIT(2)					 // Interrupt Enable Control
#define	RTC_ISNC_RTC0IR			BIT(3)					 // Interrupt Request
#define	RTC_ISNC_RTC1IE			BIT(4)					 // Interrupt Enable Control
#define	RTC_ISNC_RTC1IR			BIT(5)					 // Interrupt Request
#define	RTC_ISNC_RTC2IE			BIT(6)					 // Interrupt Enable Control
#define	RTC_ISNC_RTC2IR			BIT(7)					 // Interrupt Request
#define	RTC_ISNC_RTC3IE			BIT(8)					 // Interrupt Enable Control
#define	RTC_ISNC_RTC3IR			BIT(9)					 // Interrupt Request
#define	RTC_ISNC_ALARMIE		BIT(10)					 // Alarm Interrupt Enable Control
#define	RTC_ISNC_ALARMIR		BIT(11)					 // Alarm Interrupt Request

/* RTC Alarm Register */
#define	RTC_ALARM				MMIO32(RTC_BASE + 0x2C)
#define	RTC_ALARM_VALUE			GENMASK(32, 0)
#define	RTC_ALARM_VALUE_SHIFT	0

/* Service Routing Control Register */
#define	RTC_SRC					MMIO32(RTC_BASE + 0xF0)


// I2C [MOD_NUM=0046, MOD_REV=00, MOD_32BIT=00]
// Looks like I2C module, but not sure.
#define	I2C_BASE	0xF4800000
/* Clock Control Register */
#define	I2C_CLC		MMIO32(I2C_BASE + 0x00)

/* Module Identifier Register */
#define	I2C_ID		MMIO32(I2C_BASE + 0x08)


// GPTU0 [MOD_NUM=0001, MOD_REV=00, MOD_32BIT=C0]
// GPTU from Tricore TC1765 official public datasheet
#define	GPTU0_BASE					0xF4900000
#define	GPTU0						0xF4900000

#define	GPTU1_BASE					0xF4A00000
#define	GPTU1						0xF4A00000

/* Clock Control Register */
#define	GPTU_CLC(base)				MMIO32((base) + 0x00)

/* Module Identifier Register */
#define	GPTU_ID(base)				MMIO32((base) + 0x08)

#define	GPTU_T01IRS(base)			MMIO32((base) + 0x10)
#define	GPTU_T01IRS_T0AINS			GENMASK(2, 0)			 // T0A Input Selection
#define	GPTU_T01IRS_T0AINS_SHIFT	0
#define	GPTU_T01IRS_T0BINS			GENMASK(2, 2)			 // T0B Input Selection
#define	GPTU_T01IRS_T0BINS_SHIFT	2
#define	GPTU_T01IRS_T0CINS			GENMASK(2, 4)			 // T0C Input Selection
#define	GPTU_T01IRS_T0CINS_SHIFT	4
#define	GPTU_T01IRS_T0DINS			GENMASK(2, 6)			 // T0D Input Selection
#define	GPTU_T01IRS_T0DINS_SHIFT	6
#define	GPTU_T01IRS_T1AINS			GENMASK(2, 8)			 // T1A Input Selection
#define	GPTU_T01IRS_T1AINS_SHIFT	8
#define	GPTU_T01IRS_T1BINS			GENMASK(2, 10)			 // T1B Input Selection
#define	GPTU_T01IRS_T1BINS_SHIFT	10
#define	GPTU_T01IRS_T1CINS			GENMASK(2, 12)			 // T1C Input Selection
#define	GPTU_T01IRS_T1CINS_SHIFT	12
#define	GPTU_T01IRS_T1DINS			GENMASK(2, 14)			 // T1D Input Selection
#define	GPTU_T01IRS_T1DINS_SHIFT	14
#define	GPTU_T01IRS_T0AREL			BIT(16)					 // T0A Reload Source Selection
#define	GPTU_T01IRS_T0BREL			BIT(17)					 // T0B Reload Source Selection
#define	GPTU_T01IRS_T0CREL			BIT(18)					 // T0C Reload Source Selection
#define	GPTU_T01IRS_T0DREL			BIT(19)					 // T0D Reload Source Selection
#define	GPTU_T01IRS_T1AREL			BIT(20)					 // T1A Reload Source Selection
#define	GPTU_T01IRS_T1BREL			BIT(21)					 // T1B Reload Source Selection
#define	GPTU_T01IRS_T1CREL			BIT(22)					 // T1C Reload Source Selection
#define	GPTU_T01IRS_T1DREL			BIT(23)					 // T1D Reload Source Selection
#define	GPTU_T01IRS_T0INC			BIT(24)					 // T0 Carry Input Selection
#define	GPTU_T01IRS_T1INC			BIT(25)					 // T1 Carry Input Selection
#define	GPTU_T01IRS_T01IN0			GENMASK(2, 28)			 // T0 and T1 Global Input CNT0 Selection
#define	GPTU_T01IRS_T01IN0_SHIFT	28
#define	GPTU_T01IRS_T01IN1			GENMASK(2, 30)			 // T0 and T1 Global Input CNT1 Selection
#define	GPTU_T01IRS_T01IN1_SHIFT	30

#define	GPTU_T01OTS(base)			MMIO32((base) + 0x14)
#define	GPTU_T01OTS_SOUT00			GENMASK(2, 0)			 // T0 Output 0 Source Selection
#define	GPTU_T01OTS_SOUT00_SHIFT	0
#define	GPTU_T01OTS_SOUT01			GENMASK(2, 2)			 // T0 Output 1 Source Selection
#define	GPTU_T01OTS_SOUT01_SHIFT	2
#define	GPTU_T01OTS_STRG00			GENMASK(2, 4)			 // T0 Trigger Output 0 Source Selection
#define	GPTU_T01OTS_STRG00_SHIFT	4
#define	GPTU_T01OTS_STRG01			GENMASK(2, 6)			 // T0 Trigger Output 1 Source Selection
#define	GPTU_T01OTS_STRG01_SHIFT	6
#define	GPTU_T01OTS_SSR00			GENMASK(2, 8)			 // T0 Service Request 0 Source Selection
#define	GPTU_T01OTS_SSR00_SHIFT		8
#define	GPTU_T01OTS_SSR01			GENMASK(2, 10)			 // T0 Service Request 1 Source Selection
#define	GPTU_T01OTS_SSR01_SHIFT		10
#define	GPTU_T01OTS_SOUT10			GENMASK(2, 16)			 // T1 Output 0 Source Selection
#define	GPTU_T01OTS_SOUT10_SHIFT	16
#define	GPTU_T01OTS_SOUT11			GENMASK(2, 18)			 // T1 Output 1 Source Selection
#define	GPTU_T01OTS_SOUT11_SHIFT	18
#define	GPTU_T01OTS_STRG10			GENMASK(2, 20)			 // T1 Trigger Output 0 Source Selection
#define	GPTU_T01OTS_STRG10_SHIFT	20
#define	GPTU_T01OTS_STRG11			GENMASK(2, 22)			 // T1 Trigger Output 1 Source Selection
#define	GPTU_T01OTS_STRG11_SHIFT	22
#define	GPTU_T01OTS_SSR10			GENMASK(2, 24)			 // T1 Service Request 0 Source Selection
#define	GPTU_T01OTS_SSR10_SHIFT		24
#define	GPTU_T01OTS_SSR11			GENMASK(2, 26)			 // T1 Service Request 1 Source Selection.
#define	GPTU_T01OTS_SSR11_SHIFT		26

#define	GPTU_T2CON(base)			MMIO32((base) + 0x18)
#define	GPTU_T2CON_T2ACSRC			GENMASK(2, 0)			 // Timer T2A Count Input Source Control
#define	GPTU_T2CON_T2ACSRC_SHIFT	0
#define	GPTU_T2CON_T2ACDIR			GENMASK(2, 2)			 // Timer T2A Direction Control
#define	GPTU_T2CON_T2ACDIR_SHIFT	2
#define	GPTU_T2CON_T2ACCLR			GENMASK(2, 4)			 // Timer T2A Clear Control
#define	GPTU_T2CON_T2ACCLR_SHIFT	4
#define	GPTU_T2CON_T2ACOV			GENMASK(2, 6)			 // Timer T2A Overflow/Underflow Generation Control
#define	GPTU_T2CON_T2ACOV_SHIFT		6
#define	GPTU_T2CON_T2ACOS			BIT(8)					 // Timer T2A One-Shot Control.
#define	GPTU_T2CON_T2ADIR			BIT(12)					 // Timer T2A Direction Status Bit.
#define	GPTU_T2CON_T2SPLIT			BIT(15)					 // Timer T2 Split Control.
#define	GPTU_T2CON_T2BCSRC			GENMASK(2, 16)			 // Timer T2B Count Input Source Control.
#define	GPTU_T2CON_T2BCSRC_SHIFT	16
#define	GPTU_T2CON_T2BCDIR			GENMASK(2, 18)			 // Timer T2B Direction Control.
#define	GPTU_T2CON_T2BCDIR_SHIFT	18
#define	GPTU_T2CON_T2BCCLR			GENMASK(2, 20)			 // Timer T2B Clear Control.
#define	GPTU_T2CON_T2BCCLR_SHIFT	20
#define	GPTU_T2CON_T2BCOV			GENMASK(2, 22)			 // Timer T2B Overflow/Underflow Generation Control.
#define	GPTU_T2CON_T2BCOV_SHIFT		22
#define	GPTU_T2CON_T2BCOS			BIT(24)					 // Timer T2B One-Shot Control.
#define	GPTU_T2CON_T2BDIR			BIT(28)					 // Timer T2B Direction Status Bit.

#define	GPTU_T2RCCON(base)			MMIO32((base) + 0x1C)
#define	GPTU_T2RCCON_T2AMRC0		GENMASK(3, 0)			 // Timer T2A Reload/Capture 0 Mode Control
#define	GPTU_T2RCCON_T2AMRC0_SHIFT	0
#define	GPTU_T2RCCON_T2AMRC1		GENMASK(3, 4)			 // Timer T2A Reload/Capture 1 Mode Control
#define	GPTU_T2RCCON_T2AMRC1_SHIFT	4
#define	GPTU_T2RCCON_T2BMRC0		GENMASK(3, 16)			 // Timer T2B Reload/Capture 0 Mode Control
#define	GPTU_T2RCCON_T2BMRC0_SHIFT	16
#define	GPTU_T2RCCON_T2BMRC1		GENMASK(3, 20)			 // Timer T2B Reload/Capture 1 Mode Control
#define	GPTU_T2RCCON_T2BMRC1_SHIFT	20

#define	GPTU_T2AIS(base)			MMIO32((base) + 0x20)
#define	GPTU_T2AIS_T2AICNT			GENMASK(3, 0)			 // Timer T2A External Count Input Selection
#define	GPTU_T2AIS_T2AICNT_SHIFT	0
#define	GPTU_T2AIS_T2AISTR			GENMASK(3, 4)			 // Timer T2A External Start Input Selection
#define	GPTU_T2AIS_T2AISTR_SHIFT	4
#define	GPTU_T2AIS_T2AISTP			GENMASK(3, 8)			 // Timer T2A External Stop Input Selection
#define	GPTU_T2AIS_T2AISTP_SHIFT	8
#define	GPTU_T2AIS_T2AIUD			GENMASK(3, 12)			 // Timer T2A External Up/Down Input Selection
#define	GPTU_T2AIS_T2AIUD_SHIFT		12
#define	GPTU_T2AIS_T2AICLR			GENMASK(3, 16)			 // Timer T2A External Clear Input Selection
#define	GPTU_T2AIS_T2AICLR_SHIFT	16
#define	GPTU_T2AIS_T2AIRC0			GENMASK(3, 20)			 // Timer T2A External Reload/Capture 0 Input Selection
#define	GPTU_T2AIS_T2AIRC0_SHIFT	20
#define	GPTU_T2AIS_T2AIRC1			GENMASK(3, 24)			 // Timer T2A External Reload/Capture 1 Input Selection
#define	GPTU_T2AIS_T2AIRC1_SHIFT	24

#define	GPTU_T2BIS(base)			MMIO32((base) + 0x24)
#define	GPTU_T2BIS_T2BICNT			GENMASK(3, 0)			 // Timer T2B External Count Input Selection
#define	GPTU_T2BIS_T2BICNT_SHIFT	0
#define	GPTU_T2BIS_T2BISTR			GENMASK(3, 4)			 // Timer T2B External Start Input Selection
#define	GPTU_T2BIS_T2BISTR_SHIFT	4
#define	GPTU_T2BIS_T2BISTP			GENMASK(3, 8)			 // Timer T2B External Stop Input Selection
#define	GPTU_T2BIS_T2BISTP_SHIFT	8
#define	GPTU_T2BIS_T2BIUD			GENMASK(3, 12)			 // Timer T2B External Up/Down Input Selection
#define	GPTU_T2BIS_T2BIUD_SHIFT		12
#define	GPTU_T2BIS_T2BICLR			GENMASK(3, 16)			 // Timer T2B External Clear Input Selection
#define	GPTU_T2BIS_T2BICLR_SHIFT	16
#define	GPTU_T2BIS_T2BIRC0			GENMASK(3, 20)			 // Timer T2B External Reload/Capture 0 Input Selection
#define	GPTU_T2BIS_T2BIRC0_SHIFT	20
#define	GPTU_T2BIS_T2BIRC1			GENMASK(3, 24)			 // Timer T2B External Reload/Capture 1 Input Selection
#define	GPTU_T2BIS_T2BIRC1_SHIFT	24

#define	GPTU_T2ES(base)				MMIO32((base) + 0x28)
#define	GPTU_T2ES_T2AECNT			GENMASK(2, 0)			 // Timer T2A External Count Input Active Edge Selection
#define	GPTU_T2ES_T2AECNT_SHIFT		0
#define	GPTU_T2ES_T2AESTR			GENMASK(2, 2)			 // Timer T2A External Start Input Active Edge Selection
#define	GPTU_T2ES_T2AESTR_SHIFT		2
#define	GPTU_T2ES_T2AESTP			GENMASK(2, 4)			 // Timer T2A External Stop Input Active Edge Selection
#define	GPTU_T2ES_T2AESTP_SHIFT		4
#define	GPTU_T2ES_T2AEUD			GENMASK(2, 6)			 // Timer T2A External Up/Down Input Active Edge Selection
#define	GPTU_T2ES_T2AEUD_SHIFT		6
#define	GPTU_T2ES_T2AECLR			GENMASK(2, 8)			 // Timer T2A External Clear Input Active Edge Selection
#define	GPTU_T2ES_T2AECLR_SHIFT		8
#define	GPTU_T2ES_T2AERC0			GENMASK(2, 10)			 // Timer T2A External Reload/Capture 0 Input Active Edge Selection
#define	GPTU_T2ES_T2AERC0_SHIFT		10
#define	GPTU_T2ES_T2AERC1			GENMASK(2, 12)			 // Timer T2A External Reload/Capture 1 Input Active Edge Selection
#define	GPTU_T2ES_T2AERC1_SHIFT		12
#define	GPTU_T2ES_T2BECNT			GENMASK(2, 16)			 // Timer T2B External Count Input Active Edge Selection
#define	GPTU_T2ES_T2BECNT_SHIFT		16
#define	GPTU_T2ES_T2BESTR			GENMASK(2, 18)			 // Timer T2B External Start Input Active Edge Selection
#define	GPTU_T2ES_T2BESTR_SHIFT		18
#define	GPTU_T2ES_T2BESTP			GENMASK(2, 20)			 // Timer T2B External Stop Input Active Edge Selection
#define	GPTU_T2ES_T2BESTP_SHIFT		20
#define	GPTU_T2ES_T2BEUD			GENMASK(2, 22)			 // Timer T2B External Up/Down Input Active Edge Selection
#define	GPTU_T2ES_T2BEUD_SHIFT		22
#define	GPTU_T2ES_T2BECLR			GENMASK(2, 24)			 // Timer T2B External Clear Input Active Edge Selection
#define	GPTU_T2ES_T2BECLR_SHIFT		24
#define	GPTU_T2ES_T2BERC0			GENMASK(2, 26)			 // Timer T2B External Reload/Capture 0 Input Active Edge Selection
#define	GPTU_T2ES_T2BERC0_SHIFT		26
#define	GPTU_T2ES_T2BERC1			GENMASK(2, 28)			 // Timer T2B External Reload/Capture 1 Input Active Edge Selection
#define	GPTU_T2ES_T2BERC1_SHIFT		28

#define	GPTU_OSEL(base)				MMIO32((base) + 0x2C)
#define	GPTU_OSEL_GTSO0				GENMASK(3, 0)			 // GPTU Output 0 Source Selection
#define	GPTU_OSEL_GTSO0_SHIFT		0
#define	GPTU_OSEL_GTSO1				GENMASK(3, 4)			 // GPTU Output 1 Source Selection
#define	GPTU_OSEL_GTSO1_SHIFT		4
#define	GPTU_OSEL_GTSO2				GENMASK(3, 8)			 // GPTU Output 2 Source Selection
#define	GPTU_OSEL_GTSO2_SHIFT		8
#define	GPTU_OSEL_GTSO3				GENMASK(3, 12)			 // GPTU Output 3 Source Selection
#define	GPTU_OSEL_GTSO3_SHIFT		12
#define	GPTU_OSEL_GTSO4				GENMASK(3, 16)			 // GPTU Output 4 Source Selection
#define	GPTU_OSEL_GTSO4_SHIFT		16
#define	GPTU_OSEL_GTSO5				GENMASK(3, 20)			 // GPTU Output 5 Source Selection
#define	GPTU_OSEL_GTSO5_SHIFT		20
#define	GPTU_OSEL_GTSO6				GENMASK(3, 24)			 // GPTU Output 6 Source Selection
#define	GPTU_OSEL_GTSO6_SHIFT		24
#define	GPTU_OSEL_GTSO7				GENMASK(3, 28)			 // GPTU Output 7 Source Selection
#define	GPTU_OSEL_GTSO7_SHIFT		28

#define	GPTU_OUT(base)				MMIO32((base) + 0x30)
#define	GPTU_OUT_OUT0				BIT(0)					 // GPTU Output State Bit 0
#define	GPTU_OUT_OUT1				BIT(1)					 // GPTU Output State Bit 1
#define	GPTU_OUT_OUT2				BIT(2)					 // GPTU Output State Bit 2
#define	GPTU_OUT_OUT3				BIT(3)					 // GPTU Output State Bit 3
#define	GPTU_OUT_OUT4				BIT(4)					 // GPTU Output State Bit 4
#define	GPTU_OUT_OUT5				BIT(5)					 // GPTU Output State Bit 5
#define	GPTU_OUT_OUT6				BIT(6)					 // GPTU Output State Bit 6
#define	GPTU_OUT_OUT7				BIT(7)					 // GPTU Output State Bit 7
#define	GPTU_OUT_CLRO0				BIT(8)					 // GPTU Output 0 Clear Bit
#define	GPTU_OUT_CLRO1				BIT(9)					 // GPTU Output 1 Clear Bit
#define	GPTU_OUT_CLRO2				BIT(10)					 // GPTU Output 2 Clear Bit
#define	GPTU_OUT_CLRO3				BIT(11)					 // GPTU Output 3 Clear Bit
#define	GPTU_OUT_CLRO4				BIT(12)					 // GPTU Output 4 Clear Bit
#define	GPTU_OUT_CLRO5				BIT(13)					 // GPTU Output 5 Clear Bit
#define	GPTU_OUT_CLRO6				BIT(14)					 // GPTU Output 6 Clear Bit
#define	GPTU_OUT_CLRO7				BIT(15)					 // GPTU Output 7 Clear Bit
#define	GPTU_OUT_SETO0				BIT(16)					 // GPTU Output 0 Set Bit
#define	GPTU_OUT_SETO1				BIT(17)					 // GPTU Output 1 Set Bit
#define	GPTU_OUT_SETO2				BIT(18)					 // GPTU Output 2 Set Bit
#define	GPTU_OUT_SETO3				BIT(19)					 // GPTU Output 3 Set Bit
#define	GPTU_OUT_SETO4				BIT(20)					 // GPTU Output 4 Set Bit
#define	GPTU_OUT_SETO5				BIT(21)					 // GPTU Output 5 Set Bit
#define	GPTU_OUT_SETO6				BIT(22)					 // GPTU Output 6 Set Bit
#define	GPTU_OUT_SETO7				BIT(23)					 // GPTU Output 7 Set Bit

#define	GPTU_T0DCBA(base)			MMIO32((base) + 0x34)
#define	GPTU_T0DCBA_T0A				GENMASK(8, 0)
#define	GPTU_T0DCBA_T0A_SHIFT		0
#define	GPTU_T0DCBA_T0B				GENMASK(8, 8)
#define	GPTU_T0DCBA_T0B_SHIFT		8
#define	GPTU_T0DCBA_T0C				GENMASK(8, 16)
#define	GPTU_T0DCBA_T0C_SHIFT		16
#define	GPTU_T0DCBA_T0D				GENMASK(8, 24)
#define	GPTU_T0DCBA_T0D_SHIFT		24

#define	GPTU_T0CBA(base)			MMIO32((base) + 0x38)
#define	GPTU_T0CBA_T0A				GENMASK(8, 0)
#define	GPTU_T0CBA_T0A_SHIFT		0
#define	GPTU_T0CBA_T0B				GENMASK(8, 8)
#define	GPTU_T0CBA_T0B_SHIFT		8
#define	GPTU_T0CBA_T0C				GENMASK(8, 16)
#define	GPTU_T0CBA_T0C_SHIFT		16

#define	GPTU_T0RDCBA(base)			MMIO32((base) + 0x3C)
#define	GPTU_T0RDCBA_T0RA			GENMASK(8, 0)
#define	GPTU_T0RDCBA_T0RA_SHIFT		0
#define	GPTU_T0RDCBA_T0RB			GENMASK(8, 8)
#define	GPTU_T0RDCBA_T0RB_SHIFT		8
#define	GPTU_T0RDCBA_T0RC			GENMASK(8, 16)
#define	GPTU_T0RDCBA_T0RC_SHIFT		16
#define	GPTU_T0RDCBA_T0RD			GENMASK(8, 24)
#define	GPTU_T0RDCBA_T0RD_SHIFT		24

#define	GPTU_T0RCBA(base)			MMIO32((base) + 0x40)
#define	GPTU_T0RCBA_T0RA			GENMASK(8, 0)
#define	GPTU_T0RCBA_T0RA_SHIFT		0
#define	GPTU_T0RCBA_T0RB			GENMASK(8, 8)
#define	GPTU_T0RCBA_T0RB_SHIFT		8
#define	GPTU_T0RCBA_T0RC			GENMASK(8, 16)
#define	GPTU_T0RCBA_T0RC_SHIFT		16

#define	GPTU_T1DCBA(base)			MMIO32((base) + 0x44)
#define	GPTU_T1DCBA_T1A				GENMASK(8, 0)
#define	GPTU_T1DCBA_T1A_SHIFT		0
#define	GPTU_T1DCBA_T1B				GENMASK(8, 8)
#define	GPTU_T1DCBA_T1B_SHIFT		8
#define	GPTU_T1DCBA_T1C				GENMASK(8, 16)
#define	GPTU_T1DCBA_T1C_SHIFT		16
#define	GPTU_T1DCBA_T1D				GENMASK(8, 24)
#define	GPTU_T1DCBA_T1D_SHIFT		24

#define	GPTU_T1CBA(base)			MMIO32((base) + 0x48)
#define	GPTU_T1CBA_T1A				GENMASK(8, 0)
#define	GPTU_T1CBA_T1A_SHIFT		0
#define	GPTU_T1CBA_T1B				GENMASK(8, 8)
#define	GPTU_T1CBA_T1B_SHIFT		8
#define	GPTU_T1CBA_T1C				GENMASK(8, 16)
#define	GPTU_T1CBA_T1C_SHIFT		16

#define	GPTU_T1RDCBA(base)			MMIO32((base) + 0x4C)
#define	GPTU_T1RDCBA_T1RA			GENMASK(8, 0)
#define	GPTU_T1RDCBA_T1RA_SHIFT		0
#define	GPTU_T1RDCBA_T1RB			GENMASK(8, 8)
#define	GPTU_T1RDCBA_T1RB_SHIFT		8
#define	GPTU_T1RDCBA_T1RC			GENMASK(8, 16)
#define	GPTU_T1RDCBA_T1RC_SHIFT		16
#define	GPTU_T1RDCBA_T1RD			GENMASK(8, 24)
#define	GPTU_T1RDCBA_T1RD_SHIFT		24

#define	GPTU_T1RCBA(base)			MMIO32((base) + 0x50)
#define	GPTU_T1RCBA_T1RA			GENMASK(8, 0)
#define	GPTU_T1RCBA_T1RA_SHIFT		0
#define	GPTU_T1RCBA_T1RB			GENMASK(8, 8)
#define	GPTU_T1RCBA_T1RB_SHIFT		8
#define	GPTU_T1RCBA_T1RC			GENMASK(8, 16)
#define	GPTU_T1RCBA_T1RC_SHIFT		16

#define	GPTU_T2(base)				MMIO32((base) + 0x54)
#define	GPTU_T2_T2A					GENMASK(16, 0)			 // T2A Contents
#define	GPTU_T2_T2A_SHIFT			0
#define	GPTU_T2_T2B					GENMASK(16, 16)			 // T2B Contents
#define	GPTU_T2_T2B_SHIFT			16

#define	GPTU_T2RC0(base)			MMIO32((base) + 0x58)
#define	GPTU_T2RC0_T2ARC0			GENMASK(16, 0)			 // T2A Reload/Capture Value
#define	GPTU_T2RC0_T2ARC0_SHIFT		0
#define	GPTU_T2RC0_T2BRC0			GENMASK(16, 16)			 // T2B Reload/Capture Value
#define	GPTU_T2RC0_T2BRC0_SHIFT		16

#define	GPTU_T2RC1(base)			MMIO32((base) + 0x5C)
#define	GPTU_T2RC1_T2ARC1			GENMASK(16, 0)			 // T2A Reload/Capture Value
#define	GPTU_T2RC1_T2ARC1_SHIFT		0
#define	GPTU_T2RC1_T2BRC1			GENMASK(16, 16)			 // T2B Reload/Capture Value
#define	GPTU_T2RC1_T2BRC1_SHIFT		16

#define	GPTU_T012RUN(base)			MMIO32((base) + 0x60)
#define	GPTU_T012RUN_T0ARUN			BIT(0)					 // Timer T0D Run Control.
#define	GPTU_T012RUN_T0BRUN			BIT(1)					 // Timer T0B Run Control.
#define	GPTU_T012RUN_T0CRUN			BIT(2)					 // Timer T0C Run Control.
#define	GPTU_T012RUN_T0DRUN			BIT(3)					 // Timer T0D Run Control.
#define	GPTU_T012RUN_T1ARUN			BIT(4)					 // Timer T1A Run Control.
#define	GPTU_T012RUN_T1BRUN			BIT(5)					 // Timer T1B Run Control.
#define	GPTU_T012RUN_T1CRUN			BIT(6)					 // Timer T1C Run Control.
#define	GPTU_T012RUN_T1DRUN			BIT(7)					 // Timer T1D Run Control.
#define	GPTU_T012RUN_T2ARUN			BIT(8)					 // Timer T2A Run Status Bit.
#define	GPTU_T012RUN_T2ASETR		BIT(9)					 // Timer T2A Run Set Bit.
#define	GPTU_T012RUN_T2ACLRR		BIT(10)					 // Timer T2A Run Clear Bit.
#define	GPTU_T012RUN_T2BRUN			BIT(12)					 // Timer T2B Run Status Bit.
#define	GPTU_T012RUN_T2BSETR		BIT(13)					 // Timer T2B Run Set Bit.
#define	GPTU_T012RUN_T2BCLRR		BIT(14)					 // Timer T2B Run Clear Bit.

#define	GPTU_SRSEL(base)			MMIO32((base) + 0xDC)
#define	GPTU_SRSEL_SSR7				GENMASK(4, 0)			 // GPTU IRQ 7 Source Selection
#define	GPTU_SRSEL_SSR7_SHIFT		0
#define	GPTU_SRSEL_SSR6				GENMASK(4, 4)			 // GPTU IRQ 6 Source Selection
#define	GPTU_SRSEL_SSR6_SHIFT		4
#define	GPTU_SRSEL_SSR5				GENMASK(4, 8)			 // GPTU IRQ 5 Source Selection
#define	GPTU_SRSEL_SSR5_SHIFT		8
#define	GPTU_SRSEL_SSR4				GENMASK(4, 12)			 // GPTU IRQ 4 Source Selection
#define	GPTU_SRSEL_SSR4_SHIFT		12
#define	GPTU_SRSEL_SSR3				GENMASK(4, 16)			 // GPTU IRQ 3 Source Selection
#define	GPTU_SRSEL_SSR3_SHIFT		16
#define	GPTU_SRSEL_SSR2				GENMASK(4, 20)			 // GPTU IRQ 2 Source Selection
#define	GPTU_SRSEL_SSR2_SHIFT		20
#define	GPTU_SRSEL_SSR1				GENMASK(4, 24)			 // GPTU IRQ 1 Source Selection
#define	GPTU_SRSEL_SSR1_SHIFT		24
#define	GPTU_SRSEL_SSR0				GENMASK(4, 28)			 // GPTU IRQ 0 Source Selection
#define	GPTU_SRSEL_SSR0_SHIFT		28

/* Service Routing Control Register */
#define	GPTU_SRC7(base)				MMIO32((base) + 0xE0)

/* Service Routing Control Register */
#define	GPTU_SRC6(base)				MMIO32((base) + 0xE4)

/* Service Routing Control Register */
#define	GPTU_SRC5(base)				MMIO32((base) + 0xE8)

/* Service Routing Control Register */
#define	GPTU_SRC4(base)				MMIO32((base) + 0xEC)

/* Service Routing Control Register */
#define	GPTU_SRC3(base)				MMIO32((base) + 0xF0)

/* Service Routing Control Register */
#define	GPTU_SRC2(base)				MMIO32((base) + 0xF4)

/* Service Routing Control Register */
#define	GPTU_SRC1(base)				MMIO32((base) + 0xF8)

/* Service Routing Control Register */
#define	GPTU_SRC0(base)				MMIO32((base) + 0xFC)


// STM [MOD_NUM=0000, MOD_REV=00, MOD_32BIT=C0]
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


// AMC [MOD_NUM=F024, MOD_REV=00, MOD_32BIT=C0]
// Looks like AMC (Analog Measurement Controller? ADC?) module, but not sure.
#define	AMC_BASE	0xF4C00000
/* Clock Control Register */
#define	AMC_CLC		MMIO32(AMC_BASE + 0x00)

/* Module Identifier Register */
#define	AMC_ID		MMIO32(AMC_BASE + 0x08)

/* Service Routing Control Register */
#define	AMC_SRC(n)	MMIO32(AMC_BASE + 0xF0 + ((n) * 0x4))


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


// DSP [MOD_NUM=F022, MOD_REV=00, MOD_32BIT=C0]
// Looks like DSP module, but not sure.
#define	DSP_BASE	0xF6000000
/* Clock Control Register */
#define	DSP_CLC		MMIO32(DSP_BASE + 0x00)

/* Module Identifier Register */
#define	DSP_ID		MMIO32(DSP_BASE + 0x08)

#define	DSP_UNK0	MMIO32(DSP_BASE + 0x1C)

#define	DSP_UNK1	MMIO32(DSP_BASE + 0x24)

#define	DSP_RAM(n)	MMIO32(DSP_BASE + 0x1000 + ((n) * 0x4))


// GPRSCU [MOD_NUM=F003, MOD_REV=00, MOD_32BIT=C0]
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
#define	TPU_CORRECTION_VALUE		GENMASK(15, 0)
#define	TPU_CORRECTION_VALUE_SHIFT	0
#define	TPU_CORRECTION_CTRL			BIT(16)

#define	TPU_OVERFLOW				MMIO32(TPU_BASE + 0x20)
#define	TPU_OVERFLOW_VALUE			GENMASK(15, 0)
#define	TPU_OVERFLOW_VALUE_SHIFT	0

#define	TPU_INT(n)					MMIO32(TPU_BASE + 0x24 + ((n) * 0x4))
#define	TPU_INT_VALUE				GENMASK(15, 0)
#define	TPU_INT_VALUE_SHIFT			0

#define	TPU_OFFSET					MMIO32(TPU_BASE + 0x2C)
#define	TPU_OFFSET_VALUE			GENMASK(15, 0)
#define	TPU_OFFSET_VALUE_SHIFT		0
#define	TPU_OFFSET_CTRL				BIT(16)

#define	TPU_SKIP					MMIO32(TPU_BASE + 0x30)
#define	TPU_SKIP_SKIPN				BIT(0)
#define	TPU_SKIP_SKIPC				BIT(1)

#define	TPU_COUNTER					MMIO32(TPU_BASE + 0x34)
#define	TPU_COUNTER_VALUE			GENMASK(15, 0)
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
#define	TPU_PLLCON0_K_DIV			GENMASK(30, 0)
#define	TPU_PLLCON0_K_DIV_SHIFT		0

#define	TPU_PLLCON1					MMIO32(TPU_BASE + 0x6C)
#define	TPU_PLLCON1_L_DIV			GENMASK(30, 0)
#define	TPU_PLLCON1_L_DIV_SHIFT		0

#define	TPU_PLLCON2					MMIO32(TPU_BASE + 0x70)
#define	TPU_PLLCON2_LOAD			BIT(0)
#define	TPU_PLLCON2_INIT			BIT(1)

/* Service Routing Control Register */
#define	TPU_UNK_SRC(n)				MMIO32(TPU_BASE + 0xE0 + ((n) * 0x4))

/* Service Routing Control Register */
#define	TPU_SRC(n)					MMIO32(TPU_BASE + 0xF8 + ((n) * 0x4))

#define	TPU_RAM(n)					MMIO32(TPU_BASE + 0x1000 + ((n) * 0x4))


