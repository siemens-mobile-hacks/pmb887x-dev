#pragma once
// IWYU pragma: private, include <pmb887x.h>
#include <pmb887x.h>

// GPIO numbers


// IRQ numbers
#define	VIC_IRQ_COUNT	170


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



// SSC [MOD_NUM=0045, MOD_REV=31, MOD_32BIT=00]
// SSC (see SSC0 in lantiq)
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
#define	SSC_CON_BC				GENMASK(3, 0)			 // Bit Count Status
#define	SSC_CON_BC_SHIFT		0
#define	SSC_CON_BM				GENMASK(3, 0)			 // Data Width Selection
#define	SSC_CON_BM_SHIFT		0
#define	SSC_CON_BM_1			0x0
#define	SSC_CON_BM_2			0x1
#define	SSC_CON_BM_3			0x2
#define	SSC_CON_BM_4			0x3
#define	SSC_CON_BM_5			0x4
#define	SSC_CON_BM_6			0x5
#define	SSC_CON_BM_7			0x6
#define	SSC_CON_BM_8			0x7
#define	SSC_CON_BM_9			0x8
#define	SSC_CON_BM_10			0x9
#define	SSC_CON_BM_11			0xA
#define	SSC_CON_BM_12			0xB
#define	SSC_CON_BM_13			0xC
#define	SSC_CON_BM_14			0xD
#define	SSC_CON_BM_15			0xE
#define	SSC_CON_BM_16			0xF
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
#define	SSC_IMSC_ERR			BIT(2)					 // Error interrupt mask

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


// USB [MOD_NUM=F047, MOD_REV=12, MOD_32BIT=C0]
// sci-worx USB device controller
#define	USB_BASE									0xF2200000
/* Endpoint Enable Register, endpoints 0-7 */
#define	USB_EP_ENABLE_LOW							MMIO32(USB_BASE + 0x00)
#define	USB_EP_ENABLE_LOW_ENDPOINTS					GENMASK(7, 0)
#define	USB_EP_ENABLE_LOW_ENDPOINTS_SHIFT			0

/* Endpoint Enable Register, endpoints 8-10 */
#define	USB_EP_ENABLE_HIGH							MMIO32(USB_BASE + 0x04)
#define	USB_EP_ENABLE_HIGH_ENDPOINTS				GENMASK(2, 0)
#define	USB_EP_ENABLE_HIGH_ENDPOINTS_SHIFT			0

/* USB Device Address Register */
#define	USB_DEVICE_ADDRESS							MMIO32(USB_BASE + 0x08)
#define	USB_DEVICE_ADDRESS_ADDRESS					GENMASK(6, 0)
#define	USB_DEVICE_ADDRESS_ADDRESS_SHIFT			0
#define	USB_DEVICE_ADDRESS_ENABLE					BIT(7)									 // Device address is active

/* USB Frame Number Low Register */
#define	USB_FRAME_NUMBER_LOW						MMIO32(USB_BASE + 0x0C)
#define	USB_FRAME_NUMBER_LOW_VALUE					GENMASK(7, 0)
#define	USB_FRAME_NUMBER_LOW_VALUE_SHIFT			0

/* USB Frame Number High Register */
#define	USB_FRAME_NUMBER_HIGH						MMIO32(USB_BASE + 0x10)
#define	USB_FRAME_NUMBER_HIGH_VALUE					GENMASK(2, 0)
#define	USB_FRAME_NUMBER_HIGH_VALUE_SHIFT			0

/* USB Core Control Register */
#define	USB_CONTROL									MMIO32(USB_BASE + 0x18)
#define	USB_CONTROL_ENABLE							BIT(0)									 // Enable USB core

/* Setup Packet Byte Register */
#define	USB_SETUP_PACKET(n)							MMIO32(USB_BASE + 0x1C + ((n) * 0x4))
#define	USB_SETUP_PACKET_DATA						GENMASK(7, 0)
#define	USB_SETUP_PACKET_DATA_SHIFT					0

/* Endpoint 0 Status Register */
#define	USB_EP0_STATUS								MMIO32(USB_BASE + 0x3C)

/* Endpoint Configuration Register */
#define	USB_EP_CONFIG(n)							MMIO32(USB_BASE + 0x40 + ((n) * 0x4))

/* Global Interrupt Status Register (write one to clear) */
#define	USB_GLOBAL_INT_STATUS						MMIO32(USB_BASE + 0x180)
#define	USB_GLOBAL_INT_STATUS_SOURCES				GENMASK(7, 0)
#define	USB_GLOBAL_INT_STATUS_SOURCES_SHIFT			0

/* Global Interrupt Enable Register */
#define	USB_GLOBAL_INT_ENABLE						MMIO32(USB_BASE + 0x184)
#define	USB_GLOBAL_INT_ENABLE_SOURCES				GENMASK(7, 0)
#define	USB_GLOBAL_INT_ENABLE_SOURCES_SHIFT			0

/* DMA Channel Group 0 Interrupt Status Register (write one to clear) */
#define	USB_DMA0_INT_STATUS							MMIO32(USB_BASE + 0x188)
#define	USB_DMA0_INT_STATUS_SOURCES					GENMASK(7, 0)
#define	USB_DMA0_INT_STATUS_SOURCES_SHIFT			0

/* DMA Channel Group 0 Interrupt Enable Register */
#define	USB_DMA0_INT_ENABLE							MMIO32(USB_BASE + 0x18C)
#define	USB_DMA0_INT_ENABLE_SOURCES					GENMASK(7, 0)
#define	USB_DMA0_INT_ENABLE_SOURCES_SHIFT			0

/* DMA Channel Group 1 Interrupt Status Register (write one to clear) */
#define	USB_DMA1_INT_STATUS							MMIO32(USB_BASE + 0x190)
#define	USB_DMA1_INT_STATUS_SOURCES					GENMASK(1, 0)
#define	USB_DMA1_INT_STATUS_SOURCES_SHIFT			0

/* DMA Channel Group 1 Interrupt Enable Register */
#define	USB_DMA1_INT_ENABLE							MMIO32(USB_BASE + 0x194)
#define	USB_DMA1_INT_ENABLE_SOURCES					GENMASK(1, 0)
#define	USB_DMA1_INT_ENABLE_SOURCES_SHIFT			0

/* USB Event Interrupt Status Register (write one to clear) */
#define	USB_EVENT_INT_STATUS						MMIO32(USB_BASE + 0x198)
#define	USB_EVENT_INT_STATUS_EVENTS					GENMASK(7, 0)
#define	USB_EVENT_INT_STATUS_EVENTS_SHIFT			0

/* USB Event Interrupt Enable Register */
#define	USB_EVENT_INT_ENABLE						MMIO32(USB_BASE + 0x19C)
#define	USB_EVENT_INT_ENABLE_EVENTS					GENMASK(7, 0)
#define	USB_EVENT_INT_ENABLE_EVENTS_SHIFT			0

/* Endpoint Interrupt Group A Status Register, endpoints 0-7 (write one to clear) */
#define	USB_EP_A_INT_STATUS_LOW						MMIO32(USB_BASE + 0x1A0)
#define	USB_EP_A_INT_STATUS_LOW_ENDPOINTS			GENMASK(7, 0)
#define	USB_EP_A_INT_STATUS_LOW_ENDPOINTS_SHIFT		0

/* Endpoint Interrupt Group A Enable Register, endpoints 0-7 */
#define	USB_EP_A_INT_ENABLE_LOW						MMIO32(USB_BASE + 0x1A4)
#define	USB_EP_A_INT_ENABLE_LOW_ENDPOINTS			GENMASK(7, 0)
#define	USB_EP_A_INT_ENABLE_LOW_ENDPOINTS_SHIFT		0

/* Endpoint Interrupt Group A Status Register, endpoints 8-10 (write one to clear) */
#define	USB_EP_A_INT_STATUS_HIGH					MMIO32(USB_BASE + 0x1A8)
#define	USB_EP_A_INT_STATUS_HIGH_ENDPOINTS			GENMASK(2, 0)
#define	USB_EP_A_INT_STATUS_HIGH_ENDPOINTS_SHIFT	0

/* Endpoint Interrupt Group A Enable Register, endpoints 8-10 */
#define	USB_EP_A_INT_ENABLE_HIGH					MMIO32(USB_BASE + 0x1AC)
#define	USB_EP_A_INT_ENABLE_HIGH_ENDPOINTS			GENMASK(2, 0)
#define	USB_EP_A_INT_ENABLE_HIGH_ENDPOINTS_SHIFT	0

/* Endpoint Interrupt Group B Status Register, endpoints 0-7 (write one to clear) */
#define	USB_EP_B_INT_STATUS_LOW						MMIO32(USB_BASE + 0x1B0)
#define	USB_EP_B_INT_STATUS_LOW_ENDPOINTS			GENMASK(7, 0)
#define	USB_EP_B_INT_STATUS_LOW_ENDPOINTS_SHIFT		0

/* Endpoint Interrupt Group B Enable Register, endpoints 0-7 */
#define	USB_EP_B_INT_ENABLE_LOW						MMIO32(USB_BASE + 0x1B4)
#define	USB_EP_B_INT_ENABLE_LOW_ENDPOINTS			GENMASK(7, 0)
#define	USB_EP_B_INT_ENABLE_LOW_ENDPOINTS_SHIFT		0

/* Endpoint Interrupt Group B Status Register, endpoints 8-10 (write one to clear) */
#define	USB_EP_B_INT_STATUS_HIGH					MMIO32(USB_BASE + 0x1B8)
#define	USB_EP_B_INT_STATUS_HIGH_ENDPOINTS			GENMASK(2, 0)
#define	USB_EP_B_INT_STATUS_HIGH_ENDPOINTS_SHIFT	0

/* Endpoint Interrupt Group B Enable Register, endpoints 8-10 */
#define	USB_EP_B_INT_ENABLE_HIGH					MMIO32(USB_BASE + 0x1BC)
#define	USB_EP_B_INT_ENABLE_HIGH_ENDPOINTS			GENMASK(2, 0)
#define	USB_EP_B_INT_ENABLE_HIGH_ENDPOINTS_SHIFT	0

/* Endpoint FIFO Data Register */
#define	USB_EP_DATA(n)								MMIO32(USB_BASE + 0x1C0 + ((n) * 0x10))

/* Endpoint FIFO Control Register */
#define	USB_EP_CONTROL(n)							MMIO32(USB_BASE + 0x1C4 + ((n) * 0x10))

/* Endpoint FIFO Byte Count Low Register */
#define	USB_EP_COUNT_LOW(n)							MMIO32(USB_BASE + 0x1C8 + ((n) * 0x10))
#define	USB_EP_COUNT_LOW_VALUE						GENMASK(7, 0)
#define	USB_EP_COUNT_LOW_VALUE_SHIFT				0

/* Endpoint FIFO Byte Count High Register */
#define	USB_EP_COUNT_HIGH(n)						MMIO32(USB_BASE + 0x1CC + ((n) * 0x10))
#define	USB_EP_COUNT_HIGH_VALUE						GENMASK(2, 0)
#define	USB_EP_COUNT_HIGH_VALUE_SHIFT				0

/* USB PHY Control and Status Register */
#define	USB_PHY_CONTROL								MMIO32(USB_BASE + 0x2FC)

/* Clock Control Register */
#define	USB_CLC										MMIO32(USB_BASE + 0x800)

/* Wrapper Configuration Register */
#define	USB_CFG										MMIO32(USB_BASE + 0x804)

/* Module Identifier Register */
#define	USB_ID										MMIO32(USB_BASE + 0x808)


// VIC [MOD_NUM=0031, MOD_REV=11, MOD_32BIT=C0]
// Vectored Interrupt Controller
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
#define	DMAC_CH_LLI_ITEM						GENMASK(31, 2)								 // Linked list item
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

/* Peripheral identification register 0 */
#define	DMAC_PERIPH_ID0							MMIO32(DMAC_BASE + 0xFE0)

/* Peripheral identification register 1 */
#define	DMAC_PERIPH_ID1							MMIO32(DMAC_BASE + 0xFE4)

/* Peripheral identification register 2 */
#define	DMAC_PERIPH_ID2							MMIO32(DMAC_BASE + 0xFE8)

/* Peripheral identification register 3 */
#define	DMAC_PERIPH_ID3							MMIO32(DMAC_BASE + 0xFEC)

/* PrimeCell identification register 0 */
#define	DMAC_PCELL_ID0							MMIO32(DMAC_BASE + 0xFF0)

/* PrimeCell identification register 1 */
#define	DMAC_PCELL_ID1							MMIO32(DMAC_BASE + 0xFF4)

/* PrimeCell identification register 2 */
#define	DMAC_PCELL_ID2							MMIO32(DMAC_BASE + 0xFF8)

/* PrimeCell identification register 3 */
#define	DMAC_PCELL_ID3							MMIO32(DMAC_BASE + 0xFFC)


// SCU [MOD_NUM=F040, MOD_REV=12, MOD_32BIT=C0]
// System Control Unit (see SCU in TC1766 datasheet)
#define	SCU_BASE						0xF4400000
/* Clock Control Register */
#define	SCU_CLC							MMIO32(SCU_BASE + 0x00)

/* Module Identifier Register */
#define	SCU_ID							MMIO32(SCU_BASE + 0x08)

/* Reset Status Register */
#define	SCU_RST_SR						MMIO32(SCU_BASE + 0x10)
#define	SCU_RST_SR_RSSTM				BIT(0)									 // System Timer Reset Status
#define	SCU_RST_SR_RSEXT				BIT(1)									 // HDRST Line State during Last Reset
#define	SCU_RST_SR_HWCFG				GENMASK(18, 16)							 // Boot Configuration Selection Status
#define	SCU_RST_SR_HWCFG_SHIFT			16
#define	SCU_RST_SR_HWBRKIN				BIT(21)									 // Latched State of BRKIN Input
#define	SCU_RST_SR_TMPLS				BIT(22)									 // Latched State of TESTMODE Input
#define	SCU_RST_SR_PWORST				BIT(27)									 // The last reset was a power-on reset
#define	SCU_RST_SR_HDRST				BIT(28)									 // The last reset was a hardware reset.
#define	SCU_RST_SR_SFTRST				BIT(29)									 // The last reset was a software reset.
#define	SCU_RST_SR_WDTRST				BIT(30)									 // The last reset was a watchdog reset.
#define	SCU_RST_SR_PWDRST				BIT(31)									 // The last reset was a wake-up from power-down

/* Reset Control Register */
#define	SCU_RST_CON						MMIO32(SCU_BASE + 0x14)
#define	SCU_RST_CON_SWCFG				GENMASK(18, 16)							 // Software Boot Configuration
#define	SCU_RST_CON_SWCFG_SHIFT			16
#define	SCU_RST_CON_SWBRKIN				BIT(21)									 // Software Break Signal Boot Value
#define	SCU_RST_CON_SWBOOT				BIT(24)									 // Software Boot Configuration Selection

/* Peripheral Reset Request Register */
#define	SCU_RST_REQ						MMIO32(SCU_BASE + 0x18)
#define	SCU_RST_REQ_DSP					BIT(0)									 // DSP software reset request
#define	SCU_RST_REQ_RTC					BIT(1)									 // RTC software reset request
#define	SCU_RST_REQ_USART0				BIT(2)									 // USART0 software reset request
#define	SCU_RST_REQ_SSC0				BIT(3)									 // SSC0 software reset request
#define	SCU_RST_REQ_SIM					BIT(4)									 // SIM software reset request
#define	SCU_RST_REQ_USART1				BIT(5)									 // USART1 software reset request
#define	SCU_RST_REQ_SSC1				BIT(6)									 // SSC1 software reset request
#define	SCU_RST_REQ_MMCI				BIT(7)									 // MMCI software reset request
#define	SCU_RST_REQ_DISP				BIT(8)									 // Display software reset request
#define	SCU_RST_REQ_USB					BIT(9)									 // USB software reset request
#define	SCU_RST_REQ_DMA1				BIT(11)									 // DMA1 software reset request
#define	SCU_RST_REQ_DMA2				BIT(12)									 // DMA2 software reset request
#define	SCU_RST_REQ_DMA3				BIT(13)									 // DMA3 software reset request
#define	SCU_RST_REQ_FIRDA				BIT(14)									 // FIRDA software reset request
#define	SCU_RST_REQ_I2C					BIT(15)									 // I2C software reset request

/* Sleep Request Register */
#define	SCU_SLEEP_REQ					MMIO32(SCU_BASE + 0x20)
#define	SCU_SLEEP_REQ_REQ				BIT(0)									 // Sleep request asserted before WFI

#define	SCU_WDTCON0						MMIO32(SCU_BASE + 0x24)
#define	SCU_WDTCON0_ENDINIT				BIT(0)									 // End-of-Initialization Control Bit.
#define	SCU_WDTCON0_WDTLCK				BIT(1)									 // Lock bit to Control Access to WDT_CON0.
#define	SCU_WDTCON0_WDTHPW0				GENMASK(3, 2)							 // Hardware Password 0.
#define	SCU_WDTCON0_WDTHPW0_SHIFT		2
#define	SCU_WDTCON0_WDTHPW1				GENMASK(7, 4)							 // Hardware Password 1.
#define	SCU_WDTCON0_WDTHPW1_SHIFT		4
#define	SCU_WDTCON0_WDTPW				GENMASK(15, 8)							 // User-Definable Password Field for Access to WDT_CON0.
#define	SCU_WDTCON0_WDTPW_SHIFT			8
#define	SCU_WDTCON0_WDTREL				GENMASK(31, 16)							 // Reload Value for the Watchdog Timer.
#define	SCU_WDTCON0_WDTREL_SHIFT		16

#define	SCU_WDTCON1						MMIO32(SCU_BASE + 0x28)
#define	SCU_WDTCON1_WDTIR				BIT(2)									 // Watchdog Timer Input Frequency Request Control Bit.
#define	SCU_WDTCON1_WDTDR				BIT(3)									 // Watchdog Timer Disable Request Control Bit.

#define	SCU_WDT_SR						MMIO32(SCU_BASE + 0x2C)
#define	SCU_WDT_SR_WDTAE				BIT(0)									 // Watchdog Access Error Status Flag
#define	SCU_WDT_SR_WDTOE				BIT(1)									 // Watchdog Overflow Error Status Flag
#define	SCU_WDT_SR_WDTIS				BIT(2)									 // Watchdog Input Clock Status Flag
#define	SCU_WDT_SR_WDTDS				BIT(3)									 // Watchdog Enable/Disable Status Flag
#define	SCU_WDT_SR_WDTTO				BIT(4)									 // Watchdog Time-out Mode Flag
#define	SCU_WDT_SR_WDTPR				BIT(5)									 // Watchdog Prewarning Mode Flag
#define	SCU_WDT_SR_WDTTIM				GENMASK(31, 16)							 // Watchdog Timer Value
#define	SCU_WDT_SR_WDTTIM_SHIFT			16

/* MCU-to-DSP Interrupt Request Register */
#define	SCU_DSP_INT						MMIO32(SCU_BASE + 0x30)
#define	SCU_DSP_INT_REQ					GENMASK(2, 0)							 // MCU interrupt request lines to the DSP
#define	SCU_DSP_INT_REQ_SHIFT			0

/* External Interrupt Filter Select Register */
#define	SCU_EXTI_FILTER					MMIO32(SCU_BASE + 0x38)
#define	SCU_EXTI_FILTER_EXT0			GENMASK(1, 0)
#define	SCU_EXTI_FILTER_EXT0_SHIFT		0
#define	SCU_EXTI_FILTER_EXT0_OFF		0x0
#define	SCU_EXTI_FILTER_EXT0_CLOCK_1	0x1
#define	SCU_EXTI_FILTER_EXT0_CLOCK_2	0x2
#define	SCU_EXTI_FILTER_EXT0_CLOCK_3	0x3
#define	SCU_EXTI_FILTER_EXT1			GENMASK(3, 2)
#define	SCU_EXTI_FILTER_EXT1_SHIFT		2
#define	SCU_EXTI_FILTER_EXT1_OFF		0x0
#define	SCU_EXTI_FILTER_EXT1_CLOCK_1	0x4
#define	SCU_EXTI_FILTER_EXT1_CLOCK_2	0x8
#define	SCU_EXTI_FILTER_EXT1_CLOCK_3	0xC
#define	SCU_EXTI_FILTER_EXT2			GENMASK(5, 4)
#define	SCU_EXTI_FILTER_EXT2_SHIFT		4
#define	SCU_EXTI_FILTER_EXT2_OFF		0x0
#define	SCU_EXTI_FILTER_EXT2_CLOCK_1	0x10
#define	SCU_EXTI_FILTER_EXT2_CLOCK_2	0x20
#define	SCU_EXTI_FILTER_EXT2_CLOCK_3	0x30
#define	SCU_EXTI_FILTER_EXT3			GENMASK(7, 6)
#define	SCU_EXTI_FILTER_EXT3_SHIFT		6
#define	SCU_EXTI_FILTER_EXT3_OFF		0x0
#define	SCU_EXTI_FILTER_EXT3_CLOCK_1	0x40
#define	SCU_EXTI_FILTER_EXT3_CLOCK_2	0x80
#define	SCU_EXTI_FILTER_EXT3_CLOCK_3	0xC0
#define	SCU_EXTI_FILTER_EXT4			GENMASK(9, 8)
#define	SCU_EXTI_FILTER_EXT4_SHIFT		8
#define	SCU_EXTI_FILTER_EXT4_OFF		0x0
#define	SCU_EXTI_FILTER_EXT4_CLOCK_1	0x100
#define	SCU_EXTI_FILTER_EXT4_CLOCK_2	0x200
#define	SCU_EXTI_FILTER_EXT4_CLOCK_3	0x300
#define	SCU_EXTI_FILTER_EXT5			GENMASK(11, 10)
#define	SCU_EXTI_FILTER_EXT5_SHIFT		10
#define	SCU_EXTI_FILTER_EXT5_OFF		0x0
#define	SCU_EXTI_FILTER_EXT5_CLOCK_1	0x400
#define	SCU_EXTI_FILTER_EXT5_CLOCK_2	0x800
#define	SCU_EXTI_FILTER_EXT5_CLOCK_3	0xC00
#define	SCU_EXTI_FILTER_EXT6			GENMASK(13, 12)
#define	SCU_EXTI_FILTER_EXT6_SHIFT		12
#define	SCU_EXTI_FILTER_EXT6_OFF		0x0
#define	SCU_EXTI_FILTER_EXT6_CLOCK_1	0x1000
#define	SCU_EXTI_FILTER_EXT6_CLOCK_2	0x2000
#define	SCU_EXTI_FILTER_EXT6_CLOCK_3	0x3000
#define	SCU_EXTI_FILTER_EXT7			GENMASK(15, 14)
#define	SCU_EXTI_FILTER_EXT7_SHIFT		14
#define	SCU_EXTI_FILTER_EXT7_OFF		0x0
#define	SCU_EXTI_FILTER_EXT7_CLOCK_1	0x4000
#define	SCU_EXTI_FILTER_EXT7_CLOCK_2	0x8000
#define	SCU_EXTI_FILTER_EXT7_CLOCK_3	0xC000
#define	SCU_EXTI_FILTER_PM_INT			GENMASK(17, 16)
#define	SCU_EXTI_FILTER_PM_INT_SHIFT	16
#define	SCU_EXTI_FILTER_PM_INT_OFF		0x0
#define	SCU_EXTI_FILTER_PM_INT_CLOCK_1	0x10000
#define	SCU_EXTI_FILTER_PM_INT_CLOCK_2	0x20000
#define	SCU_EXTI_FILTER_PM_INT_CLOCK_3	0x30000

/* External Interrupt Edge Select Register */
#define	SCU_EXTI_EDGE					MMIO32(SCU_BASE + 0x3C)
#define	SCU_EXTI_EDGE_EXT0				GENMASK(1, 0)
#define	SCU_EXTI_EDGE_EXT0_SHIFT		0
#define	SCU_EXTI_EDGE_EXT0_OFF			0x0
#define	SCU_EXTI_EDGE_EXT0_RISING		0x1
#define	SCU_EXTI_EDGE_EXT0_FALLING		0x2
#define	SCU_EXTI_EDGE_EXT0_ANY			0x3
#define	SCU_EXTI_EDGE_EXT1				GENMASK(3, 2)
#define	SCU_EXTI_EDGE_EXT1_SHIFT		2
#define	SCU_EXTI_EDGE_EXT1_OFF			0x0
#define	SCU_EXTI_EDGE_EXT1_RISING		0x4
#define	SCU_EXTI_EDGE_EXT1_FALLING		0x8
#define	SCU_EXTI_EDGE_EXT1_ANY			0xC
#define	SCU_EXTI_EDGE_EXT2				GENMASK(5, 4)
#define	SCU_EXTI_EDGE_EXT2_SHIFT		4
#define	SCU_EXTI_EDGE_EXT2_OFF			0x0
#define	SCU_EXTI_EDGE_EXT2_RISING		0x10
#define	SCU_EXTI_EDGE_EXT2_FALLING		0x20
#define	SCU_EXTI_EDGE_EXT2_ANY			0x30
#define	SCU_EXTI_EDGE_EXT3				GENMASK(7, 6)
#define	SCU_EXTI_EDGE_EXT3_SHIFT		6
#define	SCU_EXTI_EDGE_EXT3_OFF			0x0
#define	SCU_EXTI_EDGE_EXT3_RISING		0x40
#define	SCU_EXTI_EDGE_EXT3_FALLING		0x80
#define	SCU_EXTI_EDGE_EXT3_ANY			0xC0
#define	SCU_EXTI_EDGE_EXT4				GENMASK(9, 8)
#define	SCU_EXTI_EDGE_EXT4_SHIFT		8
#define	SCU_EXTI_EDGE_EXT4_OFF			0x0
#define	SCU_EXTI_EDGE_EXT4_RISING		0x100
#define	SCU_EXTI_EDGE_EXT4_FALLING		0x200
#define	SCU_EXTI_EDGE_EXT4_ANY			0x300
#define	SCU_EXTI_EDGE_EXT5				GENMASK(11, 10)
#define	SCU_EXTI_EDGE_EXT5_SHIFT		10
#define	SCU_EXTI_EDGE_EXT5_OFF			0x0
#define	SCU_EXTI_EDGE_EXT5_RISING		0x400
#define	SCU_EXTI_EDGE_EXT5_FALLING		0x800
#define	SCU_EXTI_EDGE_EXT5_ANY			0xC00
#define	SCU_EXTI_EDGE_EXT6				GENMASK(13, 12)
#define	SCU_EXTI_EDGE_EXT6_SHIFT		12
#define	SCU_EXTI_EDGE_EXT6_OFF			0x0
#define	SCU_EXTI_EDGE_EXT6_RISING		0x1000
#define	SCU_EXTI_EDGE_EXT6_FALLING		0x2000
#define	SCU_EXTI_EDGE_EXT6_ANY			0x3000
#define	SCU_EXTI_EDGE_EXT7				GENMASK(15, 14)
#define	SCU_EXTI_EDGE_EXT7_SHIFT		14
#define	SCU_EXTI_EDGE_EXT7_OFF			0x0
#define	SCU_EXTI_EDGE_EXT7_RISING		0x4000
#define	SCU_EXTI_EDGE_EXT7_FALLING		0x8000
#define	SCU_EXTI_EDGE_EXT7_ANY			0xC000
#define	SCU_EXTI_EDGE_PM_INT			GENMASK(17, 16)
#define	SCU_EXTI_EDGE_PM_INT_SHIFT		16
#define	SCU_EXTI_EDGE_PM_INT_OFF		0x0
#define	SCU_EXTI_EDGE_PM_INT_RISING		0x10000
#define	SCU_EXTI_EDGE_PM_INT_FALLING	0x20000
#define	SCU_EXTI_EDGE_PM_INT_ANY		0x30000

#define	SCU_EBUCLC1						MMIO32(SCU_BASE + 0x40)
#define	SCU_EBUCLC1_FLAG1				GENMASK(3, 0)
#define	SCU_EBUCLC1_FLAG1_SHIFT			0
#define	SCU_EBUCLC1_READY				GENMASK(7, 4)
#define	SCU_EBUCLC1_READY_SHIFT			4

#define	SCU_EBUCLC2						MMIO32(SCU_BASE + 0x44)
#define	SCU_EBUCLC2_FLAG1				GENMASK(3, 0)
#define	SCU_EBUCLC2_FLAG1_SHIFT			0
#define	SCU_EBUCLC2_READY				GENMASK(7, 4)
#define	SCU_EBUCLC2_READY_SHIFT			4

#define	SCU_EBUCLC						MMIO32(SCU_BASE + 0x48)
#define	SCU_EBUCLC_LOCK					BIT(0)
#define	SCU_EBUCLC_VCOBYP				BIT(8)

/* Emulator identification register */
#define	SCU_EMU_ID						MMIO32(SCU_BASE + 0x4C)
#define	SCU_EMU_ID_VALUE				GENMASK(31, 0)
#define	SCU_EMU_ID_VALUE_SHIFT			0
#define	SCU_EMU_ID_VALUE_QEMU			0x51454D55

#define	SCU_MANID						MMIO32(SCU_BASE + 0x5C)
#define	SCU_MANID_DEPT					GENMASK(3, 0)
#define	SCU_MANID_DEPT_SHIFT			0
#define	SCU_MANID_MANUF					GENMASK(14, 4)
#define	SCU_MANID_MANUF_SHIFT			4

#define	SCU_CHIPID						MMIO32(SCU_BASE + 0x60)
#define	SCU_CHIPID_CHREV				GENMASK(7, 0)
#define	SCU_CHIPID_CHREV_SHIFT			0
#define	SCU_CHIPID_CHIPD				GENMASK(15, 8)
#define	SCU_CHIPID_CHIPD_SHIFT			8

/* Real Time Clock Interface Enable Register */
#define	SCU_RTCIF						MMIO32(SCU_BASE + 0x64)
#define	SCU_RTCIF_RTCIFEN				GENMASK(7, 0)							 // RTC interface enable field; 0xAA enables access
#define	SCU_RTCIF_RTCIFEN_SHIFT			0

#define	SCU_UID0						MMIO32(SCU_BASE + 0x6C)

#define	SCU_UID1						MMIO32(SCU_BASE + 0x70)
#define	SCU_UID1_SECBOOT				BIT(24)									 // Secure boot
#define	SCU_UID1_PLATFORM				GENMASK(26, 25)
#define	SCU_UID1_PLATFORM_SHIFT			25
#define	SCU_UID1_PLATFORM_800			0x0
#define	SCU_UID1_PLATFORM_801			0x2000000
#define	SCU_UID1_PLATFORM_802			0x4000000
#define	SCU_UID1_PLATFORM_803			0x6000000

#define	SCU_UID2						MMIO32(SCU_BASE + 0x74)
#define	SCU_UID2_BOOT_USART1			BIT(28)									 // Allow boot from USART1
#define	SCU_UID2_BOOT_BSL				BIT(29)									 // Force boot from BSL, bypass firmware
#define	SCU_UID2_BOOT_USB				BIT(30)									 // Allow boot from USB

#define	SCU_BOOT_FLAG					MMIO32(SCU_BASE + 0x78)
#define	SCU_BOOT_FLAG_BOOT_OK			BIT(0)

#define	SCU_ROMAMCR						MMIO32(SCU_BASE + 0x7C)
#define	SCU_ROMAMCR_MOUNT_BROM			BIT(0)

/* Redesign Tracing Identification Register */
#define	SCU_RTID						MMIO32(SCU_BASE + 0x80)
#define	SCU_RTID_RT						GENMASK(15, 0)							 // Redesign tracing value
#define	SCU_RTID_RT_SHIFT				0

/* DMA Request Select */
#define	SCU_DMARS						MMIO32(SCU_BASE + 0x84)
#define	SCU_DMARS_SEL0					BIT(0)
#define	SCU_DMARS_SEL1					BIT(1)
#define	SCU_DMARS_SEL2					BIT(2)
#define	SCU_DMARS_SEL3					BIT(3)
#define	SCU_DMARS_SEL4					BIT(4)
#define	SCU_DMARS_SEL5					BIT(5)
#define	SCU_DMARS_SEL6					BIT(6)
#define	SCU_DMARS_SEL7					BIT(7)
#define	SCU_DMARS_SEL8					BIT(8)
#define	SCU_DMARS_SEL9					BIT(9)
#define	SCU_DMARS_SEL10					BIT(10)
#define	SCU_DMARS_SEL11					BIT(11)
#define	SCU_DMARS_SEL12					BIT(12)
#define	SCU_DMARS_SEL13					BIT(13)
#define	SCU_DMARS_SEL14					BIT(14)
#define	SCU_DMARS_SEL15					BIT(15)

/* Service Routing Control Register */
#define	SCU_EXTI0_SRC					MMIO32(SCU_BASE + 0xB8)

/* Service Routing Control Register */
#define	SCU_EXTI1_SRC					MMIO32(SCU_BASE + 0xBC)

/* Service Routing Control Register */
#define	SCU_EXTI2_SRC					MMIO32(SCU_BASE + 0xC0)

/* Service Routing Control Register */
#define	SCU_EXTI3_SRC					MMIO32(SCU_BASE + 0xC4)

/* Service Routing Control Register */
#define	SCU_EXTI4_SRC					MMIO32(SCU_BASE + 0xC8)

/* Power ASIC/PMIC Interrupt Service Request Register */
#define	SCU_PM_INT_SRC					MMIO32(SCU_BASE + 0xCC)

/* DSP-to-MCU Interrupt Service Request Registers; INT_TOMCU[3:0], D:E610 (8875) or D:DE10 (8876) */
#define	SCU_DSP_SRC(n)					MMIO32(SCU_BASE + 0xD0 + ((n) * 0x4))

/* Service Routing Control Register */
#define	SCU_UNK0_SRC					MMIO32(SCU_BASE + 0xE8)

/* Service Routing Control Register */
#define	SCU_UNK1_SRC					MMIO32(SCU_BASE + 0xEC)

/* Service Routing Control Register */
#define	SCU_UNK2_SRC					MMIO32(SCU_BASE + 0xF0)

/* Service Routing Control Register */
#define	SCU_EXTI5_SRC					MMIO32(SCU_BASE + 0xF4)

/* Service Routing Control Register */
#define	SCU_EXTI6_SRC					MMIO32(SCU_BASE + 0xF8)

/* Service Routing Control Register */
#define	SCU_EXTI7_SRC					MMIO32(SCU_BASE + 0xFC)


// CGU
// Clock Generation Unit
#define	CGU_BASE							0xF4500000
/* Clock Generation Unit Control Register 1 (CGU_CTL1) */
#define	CGU_OSC								MMIO32(CGU_BASE + 0xA0)
#define	CGU_OSC_PLL_POWER_UP				BIT(0)					 // Power up PLL
#define	CGU_OSC_PHASE1_POWER_UP				BIT(1)					 // Power up phase-shifter output 1
#define	CGU_OSC_PHASE2_POWER_UP				BIT(2)					 // Power up phase-shifter output 2
#define	CGU_OSC_PHASE3_POWER_UP				BIT(3)					 // Power up phase-shifter output 3
#define	CGU_OSC_PHASE4_POWER_UP				BIT(4)					 // Power up phase-shifter output 4
#define	CGU_OSC_PLL_BYPASS_N				BIT(8)					 // Disable PLL bypass
#define	CGU_OSC_PHASE1_BYPASS_N				BIT(9)					 // Disable bypass for phase-shifter output 1
#define	CGU_OSC_PHASE2_BYPASS_N				BIT(10)					 // Disable bypass for phase-shifter output 2
#define	CGU_OSC_PHASE3_BYPASS_N				BIT(11)					 // Disable bypass for phase-shifter output 3
#define	CGU_OSC_PHASE4_BYPASS_N				BIT(12)					 // Disable bypass for phase-shifter output 4
#define	CGU_OSC_NDIV						GENMASK(21, 16)			 // PLL feedback divider (multiply by N+1)
#define	CGU_OSC_NDIV_SHIFT					16
#define	CGU_OSC_MDIV						GENMASK(27, 24)			 // PLL input divider (divide by M+1)
#define	CGU_OSC_MDIV_SHIFT					24

/* Clock Generation Unit Control Register 3 (CGU_CTL3) */
#define	CGU_CON0							MMIO32(CGU_BASE + 0xA4)
#define	CGU_CON0_PHASE1_CONFIG				GENMASK(7, 0)			 // Complete K1/K2 configuration byte for phase-shifter output 1
#define	CGU_CON0_PHASE1_CONFIG_SHIFT		0
#define	CGU_CON0_PHASE1_K2					GENMASK(2, 0)			 // Phase 1 divider denominator term, valid values 0..5
#define	CGU_CON0_PHASE1_K2_SHIFT			0
#define	CGU_CON0_PHASE1_K1					GENMASK(6, 3)			 // Phase 1 divider: fPLL * 12 / (K1 * 6 + K2)
#define	CGU_CON0_PHASE1_K1_SHIFT			3
#define	CGU_CON0_PHASE2_CONFIG				GENMASK(15, 8)			 // Complete K1/K2 configuration byte for phase-shifter output 2
#define	CGU_CON0_PHASE2_CONFIG_SHIFT		8
#define	CGU_CON0_PHASE2_K2					GENMASK(10, 8)			 // Phase 2 divider denominator term, valid values 0..5
#define	CGU_CON0_PHASE2_K2_SHIFT			8
#define	CGU_CON0_PHASE2_K1					GENMASK(14, 11)			 // Phase 2 divider: fPLL * 12 / (K1 * 6 + K2)
#define	CGU_CON0_PHASE2_K1_SHIFT			11
#define	CGU_CON0_PHASE3_CONFIG				GENMASK(23, 16)			 // Complete K1/K2 configuration byte for phase-shifter output 3
#define	CGU_CON0_PHASE3_CONFIG_SHIFT		16
#define	CGU_CON0_PHASE3_K2					GENMASK(18, 16)			 // Phase 3 divider denominator term, valid values 0..5
#define	CGU_CON0_PHASE3_K2_SHIFT			16
#define	CGU_CON0_PHASE3_K1					GENMASK(22, 19)			 // Phase 3 divider: fPLL * 12 / (K1 * 6 + K2)
#define	CGU_CON0_PHASE3_K1_SHIFT			19
#define	CGU_CON0_PHASE4_CONFIG				GENMASK(31, 24)			 // Complete K1/K2 configuration byte for phase-shifter output 4
#define	CGU_CON0_PHASE4_CONFIG_SHIFT		24
#define	CGU_CON0_PHASE4_K2					GENMASK(26, 24)			 // Phase 4 divider denominator term, valid values 0..5
#define	CGU_CON0_PHASE4_K2_SHIFT			24
#define	CGU_CON0_PHASE4_K1					GENMASK(30, 27)			 // Phase 4 divider: fPLL * 12 / (K1 * 6 + K2)
#define	CGU_CON0_PHASE4_K1_SHIFT			27

/* Clock Generation Unit Control Register 4 (CGU_CTL4) */
#define	CGU_CON1							MMIO32(CGU_BASE + 0xA8)
#define	CGU_CON1_FPI1_CLKSEL				GENMASK(1, 0)			 // Source clock for FPI1
#define	CGU_CON1_FPI1_CLKSEL_SHIFT			0
#define	CGU_CON1_FPI1_CLKSEL_OSC			0x0
#define	CGU_CON1_FPI1_CLKSEL_CLK32K			0x1
#define	CGU_CON1_FPI1_CLKSEL_PLL_DIV_2		0x2
#define	CGU_CON1_FPI1_CLKSEL_DISABLE		0x3
#define	CGU_CON1_FPI1_CLKDIV				GENMASK(5, 4)			 // FPI1 clock divider; bypassed for CLK32K
#define	CGU_CON1_FPI1_CLKDIV_SHIFT			4
#define	CGU_CON1_FPI1_CLKDIV_DIV1			0x0
#define	CGU_CON1_FPI1_CLKDIV_DIV2			0x10
#define	CGU_CON1_FPI1_CLKDIV_DIV4			0x20
#define	CGU_CON1_FPI1_CLKDIV_DIV8			0x30
#define	CGU_CON1_FSYS_CLKSEL				GENMASK(17, 16)			 // Source clock for fSYS (BYPASS: fSYS=fOSC, PLL: fSYS=fPLL / 2)
#define	CGU_CON1_FSYS_CLKSEL_SHIFT			16
#define	CGU_CON1_FSYS_CLKSEL_BYPASS			0x0
#define	CGU_CON1_FSYS_CLKSEL_PLL			0x20000
#define	CGU_CON1_FSYS_CLKSEL_DISABLE		0x30000
#define	CGU_CON1_AHB_CLKSEL					GENMASK(22, 20)			 // Source clock for fAHB
#define	CGU_CON1_AHB_CLKSEL_SHIFT			20
#define	CGU_CON1_AHB_CLKSEL_BYPASS			0x0
#define	CGU_CON1_AHB_CLKSEL_DISABLE			0x100000
#define	CGU_CON1_AHB_CLKSEL_PLL				0x200000
#define	CGU_CON1_AHB_CLKSEL_PHASE1			0x300000
#define	CGU_CON1_AHB_CLKSEL_PHASE2			0x400000
#define	CGU_CON1_AHB_CLKSEL_PHASE3			0x500000
#define	CGU_CON1_AHB_CLKSEL_PHASE4			0x600000
#define	CGU_CON1_FSTM_DIV_EN				BIT(25)					 // Enable fSTM divider
#define	CGU_CON1_FSTM_DIV					GENMASK(29, 28)			 // fSTM divider: divide selected PLL output by 4 * 2^n
#define	CGU_CON1_FSTM_DIV_SHIFT				28
#define	CGU_CON1_FSTM_DIV_4					0x0
#define	CGU_CON1_FSTM_DIV_8					0x10000000
#define	CGU_CON1_FSTM_DIV_16				0x20000000
#define	CGU_CON1_FSTM_DIV_32				0x30000000

/* Clock Generation Unit Control Register 5 (CGU_CTL5) */
#define	CGU_CON2							MMIO32(CGU_BASE + 0xAC)
#define	CGU_CON2_DSP_CLKSEL					GENMASK(2, 0)			 // Source clock for DSP
#define	CGU_CON2_DSP_CLKSEL_SHIFT			0
#define	CGU_CON2_DSP_CLKSEL_PHASE1			0x3
#define	CGU_CON2_DSP_CLKSEL_DISABLE			0x7
#define	CGU_CON2_EBU_CLKSEL					GENMASK(6, 4)			 // Source clock for EBU
#define	CGU_CON2_EBU_CLKSEL_SHIFT			4
#define	CGU_CON2_EBU_CLKSEL_OSC				0x0
#define	CGU_CON2_EBU_CLKSEL_DISABLE			0x10
#define	CGU_CON2_EBU_CLKSEL_PLL				0x20
#define	CGU_CON2_EBU_CLKSEL_PHASE1			0x30
#define	CGU_CON2_EBU_CLKSEL_PHASE2			0x40
#define	CGU_CON2_EBU_CLKSEL_PHASE3			0x50
#define	CGU_CON2_EBU_CLKSEL_PHASE4			0x60
#define	CGU_CON2_EBU_CLKSEL_AHB				0x70
#define	CGU_CON2_CPU_DIV					GENMASK(9, 8)			 // ARM clock divider: divide fAHB by N+1
#define	CGU_CON2_CPU_DIV_SHIFT				8
#define	CGU_CON2_CPU_DIV_EN					BIT(12)					 // Enable ARM clock divider
#define	CGU_CON2_AFC32K_EN					BIT(13)					 // Enable the 32 kHz standby clock for AFC
#define	CGU_CON2_CLK48M_CLKSEL				GENMASK(15, 14)			 // Source clock for CLK48M
#define	CGU_CON2_CLK48M_CLKSEL_SHIFT		14
#define	CGU_CON2_CLK48M_CLKSEL_OSC			0x0
#define	CGU_CON2_CLK48M_CLKSEL_PHASE4		0x8000
#define	CGU_CON2_CLK48M_CLKSEL_DISABLE		0xC000
#define	CGU_CON2_CLKOUT0_EN					BIT(16)					 // Enable CLKOUT0
#define	CGU_CON2_CLKOUT0_CLKDIV				GENMASK(18, 17)			 // CLKOUT0 divider from the oscillator
#define	CGU_CON2_CLKOUT0_CLKDIV_SHIFT		17
#define	CGU_CON2_CLKOUT0_CLKDIV_DIV1		0x0
#define	CGU_CON2_CLKOUT0_CLKDIV_DIV2		0x20000
#define	CGU_CON2_CLKOUT0_CLKDIV_DIV4		0x40000
#define	CGU_CON2_CLKOUT0_CLKDIV_DIV8		0x60000
#define	CGU_CON2_CLKOUT1_EN					BIT(20)					 // Enable CLKOUT1
#define	CGU_CON2_CLKOUT1_CLKDIV				GENMASK(22, 21)			 // CLKOUT1 divider from the oscillator
#define	CGU_CON2_CLKOUT1_CLKDIV_SHIFT		21
#define	CGU_CON2_CLKOUT1_CLKDIV_DIV1		0x0
#define	CGU_CON2_CLKOUT1_CLKDIV_DIV2		0x200000
#define	CGU_CON2_CLKOUT1_CLKDIV_DIV4		0x400000
#define	CGU_CON2_CLKOUT1_CLKDIV_DIV8		0x600000
#define	CGU_CON2_CLK32K_EN					BIT(24)					 // Enable external 32 kHz clock output
#define	CGU_CON2_MS_CLKSEL					GENMASK(29, 28)			 // Source clock for the mixed-signal domain (CLK_MS_O)
#define	CGU_CON2_MS_CLKSEL_SHIFT			28
#define	CGU_CON2_MS_CLKSEL_OSC				0x0
#define	CGU_CON2_MS_CLKSEL_CLK32K			0x10000000
#define	CGU_CON2_MS_CLKSEL_OSC_DIV_64		0x20000000
#define	CGU_CON2_MS_CLKSEL_DISABLE			0x30000000

/* PLL Status Register */
#define	CGU_STAT							MMIO32(CGU_BASE + 0xB0)
#define	CGU_STAT_LOCK						BIT(13)					 // PLL lock status

/* Clock Generation Unit Control Register 6 (CGU_CTL6) */
#define	CGU_CON3							MMIO32(CGU_BASE + 0xB4)
#define	CGU_CON3_AHB_PER_CLKSEL				GENMASK(1, 0)			 // Source clock for AHB_PER
#define	CGU_CON3_AHB_PER_CLKSEL_SHIFT		0
#define	CGU_CON3_AHB_PER_CLKSEL_OSC			0x0
#define	CGU_CON3_AHB_PER_CLKSEL_CLK32K		0x1
#define	CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2	0x2
#define	CGU_CON3_AHB_PER_CLKSEL_DISABLE		0x3
#define	CGU_CON3_AHB_PER_CLKDIV				GENMASK(5, 4)			 // AHB_PER clock divider; bypassed for CLK32K
#define	CGU_CON3_AHB_PER_CLKDIV_SHIFT		4
#define	CGU_CON3_AHB_PER_CLKDIV_DIV1		0x0
#define	CGU_CON3_AHB_PER_CLKDIV_DIV2		0x10
#define	CGU_CON3_AHB_PER_CLKDIV_DIV4		0x20
#define	CGU_CON3_AHB_PER_CLKDIV_DIV8		0x30
#define	CGU_CON3_MMCI_CLKSEL				GENMASK(9, 8)			 // Source clock for MMCI
#define	CGU_CON3_MMCI_CLKSEL_SHIFT			8
#define	CGU_CON3_MMCI_CLKSEL_OSC			0x0
#define	CGU_CON3_MMCI_CLKSEL_CLK32K			0x100
#define	CGU_CON3_MMCI_CLKSEL_PHASE4			0x200
#define	CGU_CON3_MMCI_CLKSEL_DISABLE		0x300
#define	CGU_CON3_MMCI_CLKDIV				GENMASK(13, 12)			 // MMCI clock divider
#define	CGU_CON3_MMCI_CLKDIV_SHIFT			12
#define	CGU_CON3_MMCI_CLKDIV_DIV1			0x0
#define	CGU_CON3_MMCI_CLKDIV_DIV2			0x1000
#define	CGU_CON3_MMCI_CLKDIV_DIV4			0x2000
#define	CGU_CON3_MMCI_CLKDIV_DIV8			0x3000
#define	CGU_CON3_CLKOUT2_EN					BIT(16)					 // Enable CLKOUT2
#define	CGU_CON3_CLKOUT2_CLKDIV				GENMASK(21, 20)			 // CLKOUT2 divider from phase 4
#define	CGU_CON3_CLKOUT2_CLKDIV_SHIFT		20
#define	CGU_CON3_CLKOUT2_CLKDIV_DIV1		0x0
#define	CGU_CON3_CLKOUT2_CLKDIV_DIV2		0x100000
#define	CGU_CON3_CLKOUT2_CLKDIV_DIV4		0x200000
#define	CGU_CON3_CLKOUT2_CLKDIV_DIV8		0x300000
#define	CGU_CON3_CLK48M_CLKDIV				GENMASK(25, 24)			 // CLK48M divider
#define	CGU_CON3_CLK48M_CLKDIV_SHIFT		24
#define	CGU_CON3_CLK48M_CLKDIV_DIV1			0x0
#define	CGU_CON3_CLK48M_CLKDIV_DIV2			0x1000000
#define	CGU_CON3_CLK48M_CLKDIV_DIV4			0x2000000
#define	CGU_CON3_CLK48M_CLKDIV_DIV8			0x3000000
#define	CGU_CON3_DMA_CLK_DISABLE			BIT(28)					 // Disable the 104 MHz DMA clock when set

/* Service Request Control Register */
#define	CGU_SRC								MMIO32(CGU_BASE + 0xCC)


// SCCU
// Standby Clock Control Unit
#define	SCCU_BASE					0xF4600000
/* Standby Power Control Register */
#define	SCCU_SPCR					MMIO32(SCCU_BASE + 0x10)
#define	SCCU_SPCR_DPDN				BIT(0)						 // VDD DSP Supply Domain Reset in Standby Mode
#define	SCCU_SPCR_APDN				BIT(1)						 // Analog Supply Domain Reset in Standby Mode
#define	SCCU_SPCR_DROFF				BIT(5)						 // Force DSP ROM Off Immediately
#define	SCCU_SPCR_DREN				BIT(6)						 // Enable DSP ROM Power-Off When VCXO and Shaper Power Are Off

/* Sleep Duration Value Register */
#define	SCCU_TDMINI					MMIO32(SCCU_BASE + 0x14)
#define	SCCU_TDMINI_TDMAIN			GENMASK(12, 0)				 // Duration (-1) of the Sleep Time in TDMA Frames
#define	SCCU_TDMINI_TDMAIN_SHIFT	0

/* Sleep Duration Status Register */
#define	SCCU_TDMOUT					MMIO32(SCCU_BASE + 0x18)
#define	SCCU_TDMOUT_TDMAOUT			GENMASK(12, 0)				 // Number of Frames Minus 1 During Which the GSM Timer Remained Frozen
#define	SCCU_TDMOUT_TDMAOUT_SHIFT	0

/* Sleep Control Register */
#define	SCCU_SLPCTRL				MMIO32(SCCU_BASE + 0x1C)
#define	SCCU_SLPCTRL_REFEN			BIT(0)						 // Reference Enable
#define	SCCU_SLPCTRL_SLPEN			BIT(1)						 // Sleep Enable
#define	SCCU_SLPCTRL_SLPRST			BIT(2)						 // Reset Sleep Counter
#define	SCCU_SLPCTRL_SLPSTP			BIT(3)						 // Sleep Stop
#define	SCCU_SLPCTRL_REFERR			BIT(4)						 // Reference Error Flag
#define	SCCU_SLPCTRL_HWACTDI		BIT(5)						 // Sleep Start Activation Disable

/* Standby Clock Reference Input Register */
#define	SCCU_REFIN					MMIO32(SCCU_BASE + 0x20)
#define	SCCU_REFIN_REFIN			GENMASK(12, 0)				 // MCU-Controlled Reference Value (1 LSB = 1.043 ppm)
#define	SCCU_REFIN_REFIN_SHIFT		0

/* Reference Calibration Output Values Register */
#define	SCCU_REF					MMIO32(SCCU_BASE + 0x24)
#define	SCCU_REF_REFOUT				GENMASK(12, 0)				 // Fine Reference Frequency Measurement (1 LSB = 1.043 ppm)
#define	SCCU_REF_REFOUT_SHIFT		0
#define	SCCU_REF_REFPOS				GENMASK(28, 16)				 // Coarse Slow Crystal Frequency Measurement (1 LSB = 1/8 TDMA Frame)
#define	SCCU_REF_REFPOS_SHIFT		16

/* Xtal Oscillator Number Register */
#define	SCCU_NQTZ					MMIO32(SCCU_BASE + 0x28)
#define	SCCU_NQTZ_NQTZ				GENMASK(7, 0)				 // Number of Slow Xtal Oscillator Periods in One TDMA Frame
#define	SCCU_NQTZ_NQTZ_SHIFT		0

/* Switch Control Register */
#define	SCCU_SCCTRL					MMIO32(SCCU_BASE + 0x2C)
#define	SCCU_SCCTRL_UCSLP			BIT(0)						 // Sleep Command
#define	SCCU_SCCTRL_UCWUP			BIT(1)						 // Wakeup Command
#define	SCCU_SCCTRL_SSCRST			BIT(2)						 // Reset Command

/* Wakeup Timing Register */
#define	SCCU_WAIT					MMIO32(SCCU_BASE + 0x30)
#define	SCCU_WAIT_PREWUP			GENMASK(1, 0)				 // Pre-Wakeup Time in TDMA Frames Minus 1
#define	SCCU_WAIT_PREWUP_SHIFT		0
#define	SCCU_WAIT_WAIT				GENMASK(17, 16)				 // VCXO Wait Loop Duration
#define	SCCU_WAIT_WAIT_SHIFT		16

/* Hardware Wakeup Control Register */
#define	SCCU_HWWAKEUP				MMIO32(SCCU_BASE + 0x34)
#define	SCCU_HWWAKEUP_RTC_EN		BIT(8)						 // Enable Sleep Mode Termination by RTC Block
#define	SCCU_HWWAKEUP_KPD_EN		BIT(9)						 // Enable Sleep Mode Termination by Keypad
#define	SCCU_HWWAKEUP_SIM_EN		BIT(10)						 // Enable Sleep Mode Termination by SIM Card Insertion/Removal
#define	SCCU_HWWAKEUP_EXT_EN		BIT(12)						 // Enable Wakeup by CAPCOM or External Interrupt Pins

/* Clock Status Register */
#define	SCCU_SCCUCLKSTA				MMIO32(SCCU_BASE + 0x40)
#define	SCCU_SCCUCLKSTA_CPUCLK		BIT(0)						 // Status of the MCU Clock
#define	SCCU_SCCUCLKSTA_GSMCLK		BIT(1)						 // Status of the System Interface Clock

/* State Machine Status Register */
#define	SCCU_SCCUMSTA				MMIO32(SCCU_BASE + 0x44)
#define	SCCU_SCCUMSTA_UC_ON			BIT(0)						 // SCCU State Machine Is in μC On State (S1)
#define	SCCU_SCCUMSTA_UC_OFF		BIT(1)						 // SCCU State Machine Is in μC Off State (S2)
#define	SCCU_SCCUMSTA_TCXO_OFF		BIT(2)						 // SCCU State Machine Is in TCXO Off State (S3)
#define	SCCU_SCCUMSTA_TCXO_ON		BIT(3)						 // SCCU State Machine Is in TCXO On State (S4)
#define	SCCU_SCCUMSTA_SHAP_ON		BIT(4)						 // SCCU State Machine Is in Shaper On State (S5)

/* Service Routing Control Register */
#define	SCCU_WAKE_SRC				MMIO32(SCCU_BASE + 0xA0)

/* Service Routing Control Register */
#define	SCCU_UNK_SRC				MMIO32(SCCU_BASE + 0xA8)


// RTC [MOD_NUM=F049, MOD_REV=11, MOD_32BIT=C0]
// Realtime Clock (see RTC in XC27x5X datasheet)
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

/* Interrupt Sub-Node Request Clear Register */
#define	RTC_ISNRC				MMIO32(RTC_BASE + 0x28)
#define	RTC_ISNRC_T14			BIT(0)					 // Clear T14 Interrupt Request Flag
#define	RTC_ISNRC_RTC0			BIT(2)					 // Clear RTC0 Interrupt Request Flag
#define	RTC_ISNRC_RTC1			BIT(4)					 // Clear RTC1 Interrupt Request Flag
#define	RTC_ISNRC_RTC2			BIT(6)					 // Clear RTC2 Interrupt Request Flag
#define	RTC_ISNRC_RTC3			BIT(8)					 // Clear RTC3 Interrupt Request Flag
#define	RTC_ISNRC_ALARM			BIT(10)					 // Clear Alarm Interrupt Request Flag

/* RTC Alarm Register */
#define	RTC_ALARM				MMIO32(RTC_BASE + 0x2C)
#define	RTC_ALARM_VALUE			GENMASK(31, 0)
#define	RTC_ALARM_VALUE_SHIFT	0

/* Service Routing Control Register */
#define	RTC_SRC					MMIO32(RTC_BASE + 0xF0)


// ADC [MOD_NUM=F024, MOD_REV=10, MOD_32BIT=C0]
// Measurement Interface and Analog Control
#define	ADC_BASE						0xF4C00000
/* Clock Control Register */
#define	ADC_CLC							MMIO32(ADC_BASE + 0x00)

/* Module Identifier Register */
#define	ADC_ID							MMIO32(ADC_BASE + 0x08)

/* Analog Control Register */
#define	ADC_ANA_CTRL					MMIO32(ADC_BASE + 0x14)
#define	ADC_ANA_CTRL_RXREF_PU			BIT(0)									 // RX Local Reference Buffer Switch
#define	ADC_ANA_CTRL_BG_PWUP			BIT(1)									 // Band-Gap Power Control
#define	ADC_ANA_CTRL_PA_OFF1			BIT(2)									 // PAOUT1 Analog Offset Correction
#define	ADC_ANA_CTRL_PA_OFF1_NONE		0x0
#define	ADC_ANA_CTRL_PA_OFF1_MINUS_50MV	0x4
#define	ADC_ANA_CTRL_TX_DIS				BIT(4)									 // Disable Analog Part of Modulator Unit
#define	ADC_ANA_CTRL_TXREF_PU			BIT(8)									 // TX Local Reference Buffer Switch
#define	ADC_ANA_CTRL_PAOPM1				BIT(14)									 // PAOUT1 Output Buffer Operation Mode
#define	ADC_ANA_CTRL_PAOPM1_STANDARD	0x0
#define	ADC_ANA_CTRL_PAOPM1_ENHANCED	0x4000
#define	ADC_ANA_CTRL_PA_CAL1			GENMASK(21, 16)							 // PAOUT1 Digital Offset Correction
#define	ADC_ANA_CTRL_PA_CAL1_SHIFT		16
#define	ADC_ANA_CTRL_TREF				GENMASK(31, 28)							 // Common Mode Voltage for Baseband TX Path
#define	ADC_ANA_CTRL_TREF_SHIFT			28
#define	ADC_ANA_CTRL_TREF_0_955V		0x0
#define	ADC_ANA_CTRL_TREF_1_010V		0x10000000
#define	ADC_ANA_CTRL_TREF_1_065V		0x20000000
#define	ADC_ANA_CTRL_TREF_1_120V		0x30000000
#define	ADC_ANA_CTRL_TREF_1_175V		0x40000000
#define	ADC_ANA_CTRL_TREF_1_230V		0x50000000
#define	ADC_ANA_CTRL_TREF_1_285V		0x60000000
#define	ADC_ANA_CTRL_TREF_1_340V		0x70000000
#define	ADC_ANA_CTRL_TREF_0_900V		0xF0000000

/* Measurement Control Register */
#define	ADC_CTRL						MMIO32(ADC_BASE + 0x18)
#define	ADC_CTRL_MX						GENMASK(5, 0)							 // Measurement mode
#define	ADC_CTRL_MX_SHIFT				0
#define	ADC_CTRL_MX_OFF					0x0
#define	ADC_CTRL_MX_M0					0x1
#define	ADC_CTRL_MX_M1					0x2
#define	ADC_CTRL_MX_M2					0x3
#define	ADC_CTRL_MX_M7					0x8
#define	ADC_CTRL_MX_M8					0x9
#define	ADC_CTRL_MX_M9					0xA
#define	ADC_CTRL_MX_M10					0xB
#define	ADC_CTRL_MX_M0_M9_A				0xC
#define	ADC_CTRL_MX_M0_M9_B				0x12
#define	ADC_CTRL_INV					BIT(6)									 // Inversion of Pre-Amplifier Input
#define	ADC_CTRL_CSEL					BIT(11)									 // Fast Settling of Pre-Amplifier
#define	ADC_CTRL_TC						GENMASK(14, 12)							 // Current Source for Current Measurement Mode
#define	ADC_CTRL_TC_SHIFT				12
#define	ADC_CTRL_TC_OFF					0x0
#define	ADC_CTRL_TC_I_30				0x1000
#define	ADC_CTRL_TC_I_60				0x2000
#define	ADC_CTRL_TC_I_90				0x3000
#define	ADC_CTRL_TC_I_120				0x4000
#define	ADC_CTRL_TC_I_150				0x5000
#define	ADC_CTRL_TC_I_180				0x6000
#define	ADC_CTRL_TC_I_210				0x7000
#define	ADC_CTRL_FREQ					GENMASK(18, 16)							 // ADC Sampling Rate Control
#define	ADC_CTRL_FREQ_SHIFT				16
#define	ADC_CTRL_FREQ_SINGLE_SHOT		0x0
#define	ADC_CTRL_FREQ_DIV_640			0x10000
#define	ADC_CTRL_FREQ_DIV_320			0x20000
#define	ADC_CTRL_FREQ_DIV_160			0x30000
#define	ADC_CTRL_FREQ_DIV_80			0x40000
#define	ADC_CTRL_FREQ_DIV_40			0x50000
#define	ADC_CTRL_FREQ_DIV_20			0x60000
#define	ADC_CTRL_FREQ_DIV_15			0x70000
#define	ADC_CTRL_BUFSIZE				GENMASK(21, 19)							 // Size of Output Buffer
#define	ADC_CTRL_BUFSIZE_SHIFT			19
#define	ADC_CTRL_BUFSIZE_1				0x0
#define	ADC_CTRL_BUFSIZE_2				0x80000
#define	ADC_CTRL_BUFSIZE_3				0x100000
#define	ADC_CTRL_BUFSIZE_4				0x180000
#define	ADC_CTRL_BUFSIZE_5				0x200000
#define	ADC_CTRL_BUFSIZE_6				0x280000
#define	ADC_CTRL_BUFSIZE_7				0x300000
#define	ADC_CTRL_BUFSIZE_8				0x380000
#define	ADC_CTRL_MXREF					GENMASK(25, 22)							 // Reference Measurement Mode
#define	ADC_CTRL_MXREF_SHIFT			22
#define	ADC_CTRL_MXREF_OFF				0x0
#define	ADC_CTRL_MXREF_M0				0x400000
#define	ADC_CTRL_MXREF_M1				0x800000
#define	ADC_CTRL_MXREF_M2				0xC00000
#define	ADC_CTRL_MXREF_M7				0x2000000
#define	ADC_CTRL_MXREF_M8				0x2400000
#define	ADC_CTRL_MXREF_M9				0x2800000
#define	ADC_CTRL_MXREF_M10				0x2C00000
#define	ADC_CTRL_MXREF_M0_M9_A			0x3000000
#define	ADC_CTRL_MXREF_M0_M9_B			0x4800000
#define	ADC_CTRL_ENSTOP					BIT(27)									 // Circular Output Buffer Disable
#define	ADC_CTRL_ENTRIG					BIT(28)									 // Triggered Mode Enable
#define	ADC_CTRL_ADCON					BIT(29)									 // ADC Power Save Control
#define	ADC_CTRL_START					BIT(31)									 // Start Conversion

/* Measurement Status Register */
#define	ADC_STAT						MMIO32(ADC_BASE + 0x1C)
#define	ADC_STAT_WPTR					GENMASK(2, 0)							 // Write Pointer – Index of latest valid MEAS_DATAx written
#define	ADC_STAT_WPTR_SHIFT				0
#define	ADC_STAT_BUSY					BIT(30)									 // ADC Busy Flag
#define	ADC_STAT_READY					BIT(31)									 // Data Ready Flag

/* Measurement Data Register */
#define	ADC_DATA(n)						MMIO32(ADC_BASE + 0x20 + ((n) * 0x4))

/* Measurement Peripheral Clock Control Register */
#define	ADC_CLK							MMIO32(ADC_BASE + 0x40)
#define	ADC_CLK_K						GENMASK(7, 0)							 // Numerator of Fractional Divider
#define	ADC_CLK_K_SHIFT					0
#define	ADC_CLK_L						GENMASK(15, 8)							 // Denominator of Fractional Divider
#define	ADC_CLK_L_SHIFT					8

/* Service Routing Control Register */
#define	ADC_SRC(n)						MMIO32(ADC_BASE + 0xF0 + ((n) * 0x4))


// DSP [MOD_NUM=F022, MOD_REV=10, MOD_32BIT=C0]
// Digital Signal Processor
#define	DSP_BASE					0xF6000000
#define	DSP_RAM_BASE				(DSP_BASE + 0x1000)
#define	DSP_RAM_SIZE				0x1800
#define	DSP_RAM(n)					MMIO32(DSP_RAM_BASE + ((n) * 0x4))

/* Clock Control Register */
#define	DSP_CLC						MMIO32(DSP_BASE + 0x00)

/* Module Identifier Register */
#define	DSP_ID						MMIO32(DSP_BASE + 0x08)

/* MCU Semaphore Set Register (write one to request, reads as zero) */
#define	DSP_SEM_SET					MMIO32(DSP_BASE + 0x10)
#define	DSP_SEM_SET_FLAGS			GENMASK(15, 0)						 // Semaphores requested by the MCU
#define	DSP_SEM_SET_FLAGS_SHIFT		0

/* MCU Semaphore Reset Register (write one to release, reads as zero) */
#define	DSP_SEM_CLEAR				MMIO32(DSP_BASE + 0x14)
#define	DSP_SEM_CLEAR_FLAGS			GENMASK(15, 0)						 // Semaphores released by the MCU
#define	DSP_SEM_CLEAR_FLAGS_SHIFT	0

/* MCU Semaphore Status Register (read-only) */
#define	DSP_SEM_STATUS				MMIO32(DSP_BASE + 0x18)
#define	DSP_SEM_STATUS_FLAGS		GENMASK(15, 0)						 // Zero for semaphores owned by the MCU, one for semaphores owned by the DSP or free
#define	DSP_SEM_STATUS_FLAGS_SHIFT	0

/* MCU Communication Flag Set Register (write one to set, reads as zero) */
#define	DSP_COM_SET					MMIO32(DSP_BASE + 0x1C)
#define	DSP_COM_SET_FLAGS			GENMASK(15, 0)						 // Communication flags to set; flag 0 handshakes boot commands on DSP interrupt 0
#define	DSP_COM_SET_FLAGS_SHIFT		0

/* MCU Communication Flag Reset Register (write one to clear, reads as zero) */
#define	DSP_COM_CLEAR				MMIO32(DSP_BASE + 0x20)
#define	DSP_COM_CLEAR_FLAGS			GENMASK(15, 0)						 // Communication flags to reset; DSP clears flag 0 after accepting a boot command
#define	DSP_COM_CLEAR_FLAGS_SHIFT	0

/* MCU Communication Flag Status Register (read-only) */
#define	DSP_COM_STATUS				MMIO32(DSP_BASE + 0x24)
#define	DSP_COM_STATUS_FLAGS		GENMASK(15, 0)						 // Shared MCU/DSP communication flag status
#define	DSP_COM_STATUS_FLAGS_SHIFT	0


// TPU [MOD_NUM=F021, MOD_REV=00, MOD_32BIT=C0]
// Time Processing Unit
#define	TPU_BASE									0xF6400000
#define	TPU_RAM_BASE								(TPU_BASE + 0x1000)
#define	TPU_RAM_SIZE								0x1000
#define	TPU_RAM(n)									MMIO32(TPU_RAM_BASE + ((n) * 0x4))

/* Clock Control Register */
#define	TPU_CLC										MMIO32(TPU_BASE + 0x00)

/* Module Identifier Register */
#define	TPU_ID										MMIO32(TPU_BASE + 0x08)

/* RF Control Register 1 */
#define	TPU_RFCON1									MMIO32(TPU_BASE + 0x10)
#define	TPU_RFCON1_STBSEL							GENMASK(3, 0)							 // Strobe Select. For each bit STBSEL[i]
#define	TPU_RFCON1_STBSEL_SHIFT						0
#define	TPU_RFCON1_STBSEL_ACTIVE_LOW				0x0
#define	TPU_RFCON1_STBSEL_ACTIVE_HIGH				0x1
#define	TPU_RFCON1_RFISSCP							BIT(8)									 // SSC Clock Polarity of RF Interface
#define	TPU_RFCON1_RFISSCP_IDLE_LOW_LEADING_RISE	0x0
#define	TPU_RFCON1_RFISSCP_IDLE_HIGH_LEADING_FALL	0x100
#define	TPU_RFCON1_RAMTYPE							BIT(9)									 // RF Control Unit RAM Partitioning Type
#define	TPU_RFCON1_RAMTYPE_1						0x0
#define	TPU_RFCON1_RAMTYPE_2						0x200

/* RF Control Register 2 */
#define	TPU_RFCON2									MMIO32(TPU_BASE + 0x14)
#define	TPU_RFCON2_SSCBM							GENMASK(3, 0)							 // RF SSC Unit Telegram Length Control (telegram length = SSCBM + 1 bits)
#define	TPU_RFCON2_SSCBM_SHIFT						0
#define	TPU_RFCON2_SSCBM_2							0x1
#define	TPU_RFCON2_SSCBM_3							0x2
#define	TPU_RFCON2_SSCBM_4							0x3
#define	TPU_RFCON2_SSCBM_5							0x4
#define	TPU_RFCON2_SSCBM_6							0x5
#define	TPU_RFCON2_SSCBM_7							0x6
#define	TPU_RFCON2_SSCBM_8							0x7
#define	TPU_RFCON2_SSCBM_9							0x8
#define	TPU_RFCON2_SSCBM_10							0x9
#define	TPU_RFCON2_SSCBM_11							0xA
#define	TPU_RFCON2_SSCBM_12							0xB
#define	TPU_RFCON2_SSCBM_13							0xC
#define	TPU_RFCON2_SSCBM_14							0xD
#define	TPU_RFCON2_SSCBM_15							0xE
#define	TPU_RFCON2_SSCBM_16							0xF
#define	TPU_RFCON2_SSCHB							BIT(4)									 // RF SSC Unit Heading Control
#define	TPU_RFCON2_SSCHB_LSB						0x0
#define	TPU_RFCON2_SSCHB_MSB						0x10
#define	TPU_RFCON2_SSCPB							BIT(5)									 // RF SSC Unit Clock Phase Control
#define	TPU_RFCON2_SSCPB_LEADING_EDGE				0x0
#define	TPU_RFCON2_SSCPB_TRAILING_EDGE				0x20
#define	TPU_RFCON2_SSCSB							GENMASK(10, 8)							 // RF SSC Unit Strobe Select (select active RFSTR[i] during transmission)
#define	TPU_RFCON2_SSCSB_SHIFT						8
#define	TPU_RFCON2_SSCSB_RFSTR0						0x0
#define	TPU_RFCON2_SSCSB_RFSTR1						0x100
#define	TPU_RFCON2_SSCSB_RFSTR2						0x200
#define	TPU_RFCON2_SSCSB_RFSTR3						0x300
#define	TPU_RFCON2_SSCFB							BIT(13)									 // RF SSC Unit Clock Frequency
#define	TPU_RFCON2_SSCFB_6_50MHZ					0x0
#define	TPU_RFCON2_SSCFB_3_25MHZ					0x2000
#define	TPU_RFCON2_SSCEN							BIT(15)									 // RF SSC Unit Enable

/* RF SSC Trasmit Buffer */
#define	TPU_RFSSCTB									MMIO32(TPU_BASE + 0x18)
#define	TPU_RFSSCTB_VALUE							GENMASK(15, 0)
#define	TPU_RFSSCTB_VALUE_SHIFT						0

/* RTDMA Counter Correction Register */
#define	TPU_CORRECTION								MMIO32(TPU_BASE + 0x1C)
#define	TPU_CORRECTION_VALUE						GENMASK(14, 0)
#define	TPU_CORRECTION_VALUE_SHIFT					0
#define	TPU_CORRECTION_CTRL							BIT(15)

/* RTDMA Counter Overflow Register */
#define	TPU_OVERFLOW								MMIO32(TPU_BASE + 0x20)
#define	TPU_OVERFLOW_VALUE							GENMASK(14, 0)
#define	TPU_OVERFLOW_VALUE_SHIFT					0

/* RTDMA Timer Interrupt Register */
#define	TPU_INT(n)									MMIO32(TPU_BASE + 0x24 + ((n) * 0x4))
#define	TPU_INT_VALUE								GENMASK(14, 0)
#define	TPU_INT_VALUE_SHIFT							0

/* RTDMA Counter Offset Register */
#define	TPU_OFFSET									MMIO32(TPU_BASE + 0x2C)
#define	TPU_OFFSET_VALUE							GENMASK(14, 0)
#define	TPU_OFFSET_VALUE_SHIFT						0
#define	TPU_OFFSET_CTRL								BIT(15)

/* RTDMA Frame Skip Register */
#define	TPU_SKIP									MMIO32(TPU_BASE + 0x30)
#define	TPU_SKIP_SKIPN								BIT(0)									 // Skip next CTDMA counter reset
#define	TPU_SKIP_SKIPC								BIT(1)									 // Skip current CTDMA counter reset

/* Counter Latch Register */
#define	TPU_COUNTER									MMIO32(TPU_BASE + 0x34)
#define	TPU_COUNTER_VALUE							GENMASK(14, 0)
#define	TPU_COUNTER_VALUE_SHIFT						0

/* Current Timer Event Address Pointer */
#define	TPU_CEAP									MMIO32(TPU_BASE + 0x38)
#define	TPU_CEAP_VALUE								GENMASK(8, 0)
#define	TPU_CEAP_VALUE_SHIFT						0

/* Timer Event Top Address Pointer */
#define	TPU_EAPT									MMIO32(TPU_BASE + 0x3C)
#define	TPU_EAPT_VALUE								GENMASK(8, 0)
#define	TPU_EAPT_VALUE_SHIFT						0

/* Timer Event Bottom Address Pointer */
#define	TPU_EAPB									MMIO32(TPU_BASE + 0x40)
#define	TPU_EAPB_VALUE								GENMASK(8, 0)
#define	TPU_EAPB_VALUE_SHIFT						0

/* Time Group Enable Register */
#define	TPU_TGER									MMIO32(TPU_BASE + 0x44)

/* Timer Parameter Register */
#define	TPU_PARAM									MMIO32(TPU_BASE + 0x5C)
#define	TPU_PARAM_TINI								BIT(0)									 // Timer Init
#define	TPU_PARAM_FDIS								BIT(1)									 // Fade Out Unit Disable

/* Timer Fade Out Regiser */
#define	TPU_FADE									MMIO32(TPU_BASE + 0x60)

/* GSM System Interface Clock Control Register 1 */
#define	TPU_GSMCLK1									MMIO32(TPU_BASE + 0x68)
#define	TPU_GSMCLK1_K								GENMASK(29, 0)							 // Numerator of the fractional divider
#define	TPU_GSMCLK1_K_SHIFT							0

/* GSM System Interface Clock Control Register 1 */
#define	TPU_GSMCLK2									MMIO32(TPU_BASE + 0x6C)
#define	TPU_GSMCLK2_L								GENMASK(29, 0)							 // Denominator of the fractional divider
#define	TPU_GSMCLK2_L_SHIFT							0

/* GSM System Interface Clock Control Register 1 */
#define	TPU_GSMCLK3									MMIO32(TPU_BASE + 0x70)
#define	TPU_GSMCLK3_LOAD							BIT(0)									 // Load K and L value
#define	TPU_GSMCLK3_INIT							BIT(1)									 // Init K and L value

#define	TPU_UNK										MMIO32(TPU_BASE + 0xD8)

/* Service Routing Control Register */
#define	TPU_RFSSC_SRC								MMIO32(TPU_BASE + 0xE0)

/* Service Routing Control Register */
#define	TPU_GP_SRC(n)								MMIO32(TPU_BASE + 0xE4 + ((n) * 0x4))

/* Service Routing Control Register */
#define	TPU_SRC(n)									MMIO32(TPU_BASE + 0xF8 + ((n) * 0x4))


// MMICIF [MOD_NUM=F053, MOD_REV=12, MOD_32BIT=C0]
// Multi Media Controller Interface
#define	MMICIF_BASE							0xF8000000
#define	MMICIF_MMAP_BASE					(MMICIF_BASE + 0x2000000)
#define	MMICIF_MMAP_SIZE					0x1000000

/* Clock Control Register */
#define	MMICIF_CLC							MMIO32(MMICIF_BASE + 0x00)

/* Identification Register */
#define	MMICIF_ID							MMIO32(MMICIF_BASE + 0x04)

/* Interface Configuration Register */
#define	MMICIF_CONFIG						MMIO32(MMICIF_BASE + 0x08)

#define	MMICIF_UNK2C						MMIO32(MMICIF_BASE + 0x2C)

#define	MMICIF_UNK44						MMIO32(MMICIF_BASE + 0x44)

/* Transfer Configuration Register */
#define	MMICIF_TRANSFER_CONFIG				MMIO32(MMICIF_BASE + 0x48)
#define	MMICIF_TRANSFER_CONFIG_MODE			GENMASK(4, 0)
#define	MMICIF_TRANSFER_CONFIG_MODE_SHIFT	0
#define	MMICIF_TRANSFER_CONFIG_MODE_WRITE	0x3
#define	MMICIF_TRANSFER_CONFIG_MODE_READ	0xA

#define	MMICIF_UNK4C						MMIO32(MMICIF_BASE + 0x4C)

#define	MMICIF_UNK50						MMIO32(MMICIF_BASE + 0x50)

#define	MMICIF_UNK54						MMIO32(MMICIF_BASE + 0x54)

/* Interrupt Request Source Mask Register */
#define	MMICIF_IRQSM						MMIO32(MMICIF_BASE + 0x70)
#define	MMICIF_IRQSM_EVENT0					BIT(0)

/* Interrupt Request Source Status Register */
#define	MMICIF_IRQSS						MMIO32(MMICIF_BASE + 0x74)
#define	MMICIF_IRQSS_EVENT0					BIT(0)

/* Interrupt Request Source Clear Register */
#define	MMICIF_IRQSC						MMIO32(MMICIF_BASE + 0x78)
#define	MMICIF_IRQSC_EVENT0					BIT(0)

#define	MMICIF_UNK80						MMIO32(MMICIF_BASE + 0x80)
