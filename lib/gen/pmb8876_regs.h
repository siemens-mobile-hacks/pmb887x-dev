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
#define	NVIC_USART1_TX_IRQ		26
#define	NVIC_USART1_TBUF_IRQ	27
#define	NVIC_USART1_RX_IRQ		28
#define	NVIC_USART1_ERR_IRQ		29
#define	NVIC_USART1_CTS_IRQ		30
#define	NVIC_USART1_ABDET_IRQ	31
#define	NVIC_USART1_ABSTART_IRQ	32
#define	NVIC_USART1_TMO_IRQ		33
#define	NVIC_DMA_IRQ			35
#define	NVIC_DMA_CH0_IRQ		36
#define	NVIC_DMA_CH1_IRQ		37
#define	NVIC_DMA_CH2_IRQ		38
#define	NVIC_DMA_CH3_IRQ		39
#define	NVIC_DMA_CH4_IRQ		40
#define	NVIC_DMA_CH5_IRQ		41
#define	NVIC_DMA_CH6_IRQ		42
#define	NVIC_DMA_CH7_IRQ		43
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
#define	NVIC_GSM_TPU_INT0_IRQ	119
#define	NVIC_GSM_TPU_INT1_IRQ	120


// EBU [MOD_NUM=0014, MOD_REV=00, MOD_32BIT=C0]
#define	EBU_BASE					0xF0000000							
#define	EBU_CLC						MMIO32(EBU_BASE + 0x00)				
#define	EBU_CLC_DISR				BIT(0)									 // Module Disable Request Bit			
#define	EBU_CLC_DISS				BIT(1)									 // Module Disable Status Bit			
#define	EBU_CLC_SPEN				BIT(2)									 // Module Suspend Enable Bit			
#define	EBU_CLC_EDIS				BIT(3)									 // Module External Request Disable		
#define	EBU_CLC_SBWE				BIT(4)									 // Module Suspend Bit Write Enable		
#define	EBU_CLC_FSOE				BIT(5)									 // Module Fast Shut-Off Enable.		
#define	EBU_CLC_RMC					GENMASK(8, 8)							 // Module Clock Divider for Normal Mode
#define	EBU_CLC_RMC_SHIFT			8									

#define	EBU_ID						MMIO32(EBU_BASE + 0x08)				
#define	EBU_ID_MOD_REV				GENMASK(8, 0)																	
#define	EBU_ID_MOD_REV_SHIFT		0									
#define	EBU_ID_MOD_32B				GENMASK(8, 8)																	
#define	EBU_ID_MOD_32B_SHIFT		8									
#define	EBU_ID_MOD_NUMBER			GENMASK(16, 16)																	
#define	EBU_ID_MOD_NUMBER_SHIFT		16									

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

#define	EBU_BFCON					MMIO32(EBU_BASE + 0x20)				
#define	EBU_BFCON_FETBLEN			GENMASK(4, 0)							 // Fetch burst length					
#define	EBU_BFCON_FETBLEN_SHIFT		0									
#define	EBU_BFCON_FBBMSEL			BIT(4)									 // Flash burst buffer					
#define	EBU_BFCON_WAITFUNC			BIT(5)									 // Function of WAIT input				
#define	EBU_BFCON_EXTCLOCK			GENMASK(2, 6)							 // Frequency of external clock			
#define	EBU_BFCON_EXTCLOCK_SHIFT	6									

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
#define	USART0_BASE						0xF1000000			
#define	USART0							0xF1000000			

#define	USART1_BASE						0xF1800000			
#define	USART1							0xF1800000			

#define	USART_CLC(base)					MMIO32((base) + 0x00)
#define	USART_CLC_DISR					BIT(0)					 // Disable request bit (1: enable; 0: disable)				
#define	USART_CLC_DISS					BIT(1)					 // Disable status bit (1: disable; 0: enable)				
#define	USART_CLC_SPEN					BIT(2)					 // Suspend bit enable (1: enable; 0: disable)				
#define	USART_CLC_EDIS					BIT(3)					 // External request disable (1: disable; 0: enable)		
#define	USART_CLC_SBWE					BIT(4)					 // Suspend bit write enable (1: enable; 0: disable)		
#define	USART_CLC_FSOE					BIT(5)					 // Fast shut off enable (1: enable; 0: disable)			
#define	USART_CLC_RMC_CLK_DIV			GENMASK(8, 8)																		
#define	USART_CLC_RMC_CLK_DIV_SHIFT		8					
#define	USART_CLC_SMC_CLK_DIV			GENMASK(8, 16)																		
#define	USART_CLC_SMC_CLK_DIV_SHIFT		16					

#define	USART_PISEL(base)				MMIO32((base) + 0x04)

#define	USART_ID(base)					MMIO32((base) + 0x08)
#define	USART_ID_MOD_REV				GENMASK(8, 0)																		
#define	USART_ID_MOD_REV_SHIFT			0					
#define	USART_ID_MOD_32B				GENMASK(8, 8)																		
#define	USART_ID_MOD_32B_SHIFT			8					
#define	USART_ID_MOD_NUMBER				GENMASK(16, 16)																		
#define	USART_ID_MOD_NUMBER_SHIFT		16					

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


// USB [MOD_NUM=F047, MOD_REV=00, MOD_32BIT=C0]
#define	USB_BASE				0xF2200800			
#define	USB_CLC					MMIO32(USB_BASE + 0x00)
#define	USB_CLC_DISR			BIT(0)					 // Module Disable Request Bit			
#define	USB_CLC_DISS			BIT(1)					 // Module Disable Status Bit			
#define	USB_CLC_SPEN			BIT(2)					 // Module Suspend Enable Bit			
#define	USB_CLC_EDIS			BIT(3)					 // Module External Request Disable		
#define	USB_CLC_SBWE			BIT(4)					 // Module Suspend Bit Write Enable		
#define	USB_CLC_FSOE			BIT(5)					 // Module Fast Shut-Off Enable.		
#define	USB_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode
#define	USB_CLC_RMC_SHIFT		8					

#define	USB_ID					MMIO32(USB_BASE + 0x08)
#define	USB_ID_MOD_REV			GENMASK(8, 0)													
#define	USB_ID_MOD_REV_SHIFT	0					
#define	USB_ID_MOD_32B			GENMASK(8, 8)													
#define	USB_ID_MOD_32B_SHIFT	8					
#define	USB_ID_MOD_NUMBER		GENMASK(16, 16)													
#define	USB_ID_MOD_NUMBER_SHIFT	16					


// NVIC [MOD_NUM=0031, MOD_REV=00, MOD_32BIT=C0]
#define	NVIC_BASE					0xF2800000							
#define	NVIC_ID						MMIO32(NVIC_BASE + 0x00)			
#define	NVIC_ID_MOD_REV				GENMASK(8, 0)							
#define	NVIC_ID_MOD_REV_SHIFT		0									
#define	NVIC_ID_MOD_32B				GENMASK(8, 8)							
#define	NVIC_ID_MOD_32B_SHIFT		8									
#define	NVIC_ID_MOD_NUMBER			GENMASK(16, 16)							
#define	NVIC_ID_MOD_NUMBER_SHIFT	16									

#define	NVIC_FIQ_ACK				MMIO32(NVIC_BASE + 0x08)			

#define	NVIC_IRQ_ACK				MMIO32(NVIC_BASE + 0x14)			

#define	NVIC_CURRENT_FIQ			MMIO32(NVIC_BASE + 0x18)			

#define	NVIC_CURRENT_IRQ			MMIO32(NVIC_BASE + 0x1C)			

#define	NVIC_CON(n)					MMIO32(NVIC_BASE + 0x30 + ((n) * 0x4))
#define	NVIC_CON_PRIORITY			GENMASK(8, 0)							
#define	NVIC_CON_PRIORITY_SHIFT		0									
#define	NVIC_CON_FIQ				BIT(8)									


// DMA [AMBA PL080]
#define	DMA_BASE							0xF3000000							
/* Status of the DMA interrupts after masking */
#define	DMA_INT_STATUS						MMIO32(DMA_BASE + 0x00)				
#define	DMA_INT_STATUS_CH0					BIT(0)																				
#define	DMA_INT_STATUS_CH1					BIT(1)																				
#define	DMA_INT_STATUS_CH2					BIT(2)																				
#define	DMA_INT_STATUS_CH3					BIT(3)																				
#define	DMA_INT_STATUS_CH4					BIT(4)																				
#define	DMA_INT_STATUS_CH5					BIT(5)																				
#define	DMA_INT_STATUS_CH6					BIT(6)																				
#define	DMA_INT_STATUS_CH7					BIT(7)																				

/* Interrupt terminal count request status */
#define	DMA_TC_STATUS						MMIO32(DMA_BASE + 0x04)				
#define	DMA_TC_STATUS_CH0					BIT(0)																				
#define	DMA_TC_STATUS_CH1					BIT(1)																				
#define	DMA_TC_STATUS_CH2					BIT(2)																				
#define	DMA_TC_STATUS_CH3					BIT(3)																				
#define	DMA_TC_STATUS_CH4					BIT(4)																				
#define	DMA_TC_STATUS_CH5					BIT(5)																				
#define	DMA_TC_STATUS_CH6					BIT(6)																				
#define	DMA_TC_STATUS_CH7					BIT(7)																				

/* Terminal count request clear. */
#define	DMA_TC_CLEAR						MMIO32(DMA_BASE + 0x08)				
#define	DMA_TC_CLEAR_CH0					BIT(0)																				
#define	DMA_TC_CLEAR_CH1					BIT(1)																				
#define	DMA_TC_CLEAR_CH2					BIT(2)																				
#define	DMA_TC_CLEAR_CH3					BIT(3)																				
#define	DMA_TC_CLEAR_CH4					BIT(4)																				
#define	DMA_TC_CLEAR_CH5					BIT(5)																				
#define	DMA_TC_CLEAR_CH6					BIT(6)																				
#define	DMA_TC_CLEAR_CH7					BIT(7)																				

/* Interrupt error status */
#define	DMA_ERR_STATUS						MMIO32(DMA_BASE + 0x0C)				
#define	DMA_ERR_STATUS_CH0					BIT(0)																				
#define	DMA_ERR_STATUS_CH1					BIT(1)																				
#define	DMA_ERR_STATUS_CH2					BIT(2)																				
#define	DMA_ERR_STATUS_CH3					BIT(3)																				
#define	DMA_ERR_STATUS_CH4					BIT(4)																				
#define	DMA_ERR_STATUS_CH5					BIT(5)																				
#define	DMA_ERR_STATUS_CH6					BIT(6)																				
#define	DMA_ERR_STATUS_CH7					BIT(7)																				

/* Interrupt error clear. */
#define	DMA_ERR_CLEAR						MMIO32(DMA_BASE + 0x10)				
#define	DMA_ERR_CLEAR_CH0					BIT(0)																				
#define	DMA_ERR_CLEAR_CH1					BIT(1)																				
#define	DMA_ERR_CLEAR_CH2					BIT(2)																				
#define	DMA_ERR_CLEAR_CH3					BIT(3)																				
#define	DMA_ERR_CLEAR_CH4					BIT(4)																				
#define	DMA_ERR_CLEAR_CH5					BIT(5)																				
#define	DMA_ERR_CLEAR_CH6					BIT(6)																				
#define	DMA_ERR_CLEAR_CH7					BIT(7)																				

/* Status of the terminal count interrupt prior to masking */
#define	DMA_RAW_TC_STATUS					MMIO32(DMA_BASE + 0x14)				
#define	DMA_RAW_TC_STATUS_CH0				BIT(0)																				
#define	DMA_RAW_TC_STATUS_CH1				BIT(1)																				
#define	DMA_RAW_TC_STATUS_CH2				BIT(2)																				
#define	DMA_RAW_TC_STATUS_CH3				BIT(3)																				
#define	DMA_RAW_TC_STATUS_CH4				BIT(4)																				
#define	DMA_RAW_TC_STATUS_CH5				BIT(5)																				
#define	DMA_RAW_TC_STATUS_CH6				BIT(6)																				
#define	DMA_RAW_TC_STATUS_CH7				BIT(7)																				

/* Status of the error interrupt prior to masking */
#define	DMA_RAW_ERR_CLEAR					MMIO32(DMA_BASE + 0x18)				
#define	DMA_RAW_ERR_CLEAR_CH0				BIT(0)																				
#define	DMA_RAW_ERR_CLEAR_CH1				BIT(1)																				
#define	DMA_RAW_ERR_CLEAR_CH2				BIT(2)																				
#define	DMA_RAW_ERR_CLEAR_CH3				BIT(3)																				
#define	DMA_RAW_ERR_CLEAR_CH4				BIT(4)																				
#define	DMA_RAW_ERR_CLEAR_CH5				BIT(5)																				
#define	DMA_RAW_ERR_CLEAR_CH6				BIT(6)																				
#define	DMA_RAW_ERR_CLEAR_CH7				BIT(7)																				

/* Channel enable status */
#define	DMA_EN_CHAN							MMIO32(DMA_BASE + 0x1C)				
#define	DMA_EN_CHAN_CH0						BIT(0)																				
#define	DMA_EN_CHAN_CH1						BIT(1)																				
#define	DMA_EN_CHAN_CH2						BIT(2)																				
#define	DMA_EN_CHAN_CH3						BIT(3)																				
#define	DMA_EN_CHAN_CH4						BIT(4)																				
#define	DMA_EN_CHAN_CH5						BIT(5)																				
#define	DMA_EN_CHAN_CH6						BIT(6)																				
#define	DMA_EN_CHAN_CH7						BIT(7)																				

/* Software burst request. */
#define	DMA_SOFT_BREQ						MMIO32(DMA_BASE + 0x20)				
#define	DMA_SOFT_BREQ_CH0_0					BIT(0)																				
#define	DMA_SOFT_BREQ_CH0_1					BIT(1)																				
#define	DMA_SOFT_BREQ_CH1_0					BIT(2)																				
#define	DMA_SOFT_BREQ_CH1_1					BIT(3)																				
#define	DMA_SOFT_BREQ_CH2_0					BIT(4)																				
#define	DMA_SOFT_BREQ_CH2_1					BIT(5)																				
#define	DMA_SOFT_BREQ_CH3_0					BIT(6)																				
#define	DMA_SOFT_BREQ_CH3_1					BIT(7)																				
#define	DMA_SOFT_BREQ_CH4_0					BIT(8)																				
#define	DMA_SOFT_BREQ_CH4_1					BIT(9)																				
#define	DMA_SOFT_BREQ_CH5_0					BIT(10)																				
#define	DMA_SOFT_BREQ_CH5_1					BIT(11)																				
#define	DMA_SOFT_BREQ_CH6_0					BIT(12)																				
#define	DMA_SOFT_BREQ_CH6_1					BIT(13)																				
#define	DMA_SOFT_BREQ_CH7_0					BIT(14)																				
#define	DMA_SOFT_BREQ_CH7_1					BIT(15)																				

/* Software single request. */
#define	DMA_SOFT_SREQ						MMIO32(DMA_BASE + 0x24)				
#define	DMA_SOFT_SREQ_CH0_0					BIT(0)																				
#define	DMA_SOFT_SREQ_CH0_1					BIT(1)																				
#define	DMA_SOFT_SREQ_CH1_0					BIT(2)																				
#define	DMA_SOFT_SREQ_CH1_1					BIT(3)																				
#define	DMA_SOFT_SREQ_CH2_0					BIT(4)																				
#define	DMA_SOFT_SREQ_CH2_1					BIT(5)																				
#define	DMA_SOFT_SREQ_CH3_0					BIT(6)																				
#define	DMA_SOFT_SREQ_CH3_1					BIT(7)																				
#define	DMA_SOFT_SREQ_CH4_0					BIT(8)																				
#define	DMA_SOFT_SREQ_CH4_1					BIT(9)																				
#define	DMA_SOFT_SREQ_CH5_0					BIT(10)																				
#define	DMA_SOFT_SREQ_CH5_1					BIT(11)																				
#define	DMA_SOFT_SREQ_CH6_0					BIT(12)																				
#define	DMA_SOFT_SREQ_CH6_1					BIT(13)																				
#define	DMA_SOFT_SREQ_CH7_0					BIT(14)																				
#define	DMA_SOFT_SREQ_CH7_1					BIT(15)																				

/* Software last burst request. */
#define	DMA_SOFT_LBREQ						MMIO32(DMA_BASE + 0x28)				
#define	DMA_SOFT_LBREQ_CH0_0				BIT(0)																				
#define	DMA_SOFT_LBREQ_CH0_1				BIT(1)																				
#define	DMA_SOFT_LBREQ_CH1_0				BIT(2)																				
#define	DMA_SOFT_LBREQ_CH1_1				BIT(3)																				
#define	DMA_SOFT_LBREQ_CH2_0				BIT(4)																				
#define	DMA_SOFT_LBREQ_CH2_1				BIT(5)																				
#define	DMA_SOFT_LBREQ_CH3_0				BIT(6)																				
#define	DMA_SOFT_LBREQ_CH3_1				BIT(7)																				
#define	DMA_SOFT_LBREQ_CH4_0				BIT(8)																				
#define	DMA_SOFT_LBREQ_CH4_1				BIT(9)																				
#define	DMA_SOFT_LBREQ_CH5_0				BIT(10)																				
#define	DMA_SOFT_LBREQ_CH5_1				BIT(11)																				
#define	DMA_SOFT_LBREQ_CH6_0				BIT(12)																				
#define	DMA_SOFT_LBREQ_CH6_1				BIT(13)																				
#define	DMA_SOFT_LBREQ_CH7_0				BIT(14)																				
#define	DMA_SOFT_LBREQ_CH7_1				BIT(15)																				

/* Software last single request. */
#define	DMA_SOFT_LSREQ						MMIO32(DMA_BASE + 0x2C)				
#define	DMA_SOFT_LSREQ_CH0_0				BIT(0)																				
#define	DMA_SOFT_LSREQ_CH0_1				BIT(1)																				
#define	DMA_SOFT_LSREQ_CH1_0				BIT(2)																				
#define	DMA_SOFT_LSREQ_CH1_1				BIT(3)																				
#define	DMA_SOFT_LSREQ_CH2_0				BIT(4)																				
#define	DMA_SOFT_LSREQ_CH2_1				BIT(5)																				
#define	DMA_SOFT_LSREQ_CH3_0				BIT(6)																				
#define	DMA_SOFT_LSREQ_CH3_1				BIT(7)																				
#define	DMA_SOFT_LSREQ_CH4_0				BIT(8)																				
#define	DMA_SOFT_LSREQ_CH4_1				BIT(9)																				
#define	DMA_SOFT_LSREQ_CH5_0				BIT(10)																				
#define	DMA_SOFT_LSREQ_CH5_1				BIT(11)																				
#define	DMA_SOFT_LSREQ_CH6_0				BIT(12)																				
#define	DMA_SOFT_LSREQ_CH6_1				BIT(13)																				
#define	DMA_SOFT_LSREQ_CH7_0				BIT(14)																				
#define	DMA_SOFT_LSREQ_CH7_1				BIT(15)																				

/* Configuration Register */
#define	DMA_CONFIG							MMIO32(DMA_BASE + 0x30)				
#define	DMA_CONFIG_ENABLE					BIT(0)									 // DMAC Enable								
#define	DMA_CONFIG_M1						BIT(1)									 // AHB Master 1 endianness configuration	
#define	DMA_CONFIG_M1_LE					0x0									
#define	DMA_CONFIG_M1_BE					0x2									
#define	DMA_CONFIG_M2						BIT(2)									 // AHB Master 2 endianness configuration	
#define	DMA_CONFIG_M2_LE					0x0									
#define	DMA_CONFIG_M2_BE					0x4									

/* Synchronization Register */
#define	DMA_SYNC							MMIO32(DMA_BASE + 0x34)				
#define	DMA_SYNC_CH0_0						BIT(0)																				
#define	DMA_SYNC_CH0_1						BIT(1)																				
#define	DMA_SYNC_CH1_0						BIT(2)																				
#define	DMA_SYNC_CH1_1						BIT(3)																				
#define	DMA_SYNC_CH2_0						BIT(4)																				
#define	DMA_SYNC_CH2_1						BIT(5)																				
#define	DMA_SYNC_CH3_0						BIT(6)																				
#define	DMA_SYNC_CH3_1						BIT(7)																				
#define	DMA_SYNC_CH4_0						BIT(8)																				
#define	DMA_SYNC_CH4_1						BIT(9)																				
#define	DMA_SYNC_CH5_0						BIT(10)																				
#define	DMA_SYNC_CH5_1						BIT(11)																				
#define	DMA_SYNC_CH6_0						BIT(12)																				
#define	DMA_SYNC_CH6_1						BIT(13)																				
#define	DMA_SYNC_CH7_0						BIT(14)																				
#define	DMA_SYNC_CH7_1						BIT(15)																				

#define	DMA_CH_SRC_ADDR(n)					MMIO32(DMA_BASE + 0x100 + ((n) * 0x20))

#define	DMA_CH_DST_ADDR						MMIO32(DMA_BASE + 0x104)			

#define	DMA_CH_LLI							MMIO32(DMA_BASE + 0x108)			
#define	DMA_CH_LLI_LM						BIT(0)									 // AHB master select for loading the next LLI
#define	DMA_CH_LLI_LM_AHB1					0x0									
#define	DMA_CH_LLI_LM_AHB2					0x1									
#define	DMA_CH_LLI_LLI						GENMASK(29, 2)							 // Linked list item						
#define	DMA_CH_LLI_LLI_SHIFT				2									

#define	DMA_CH_CONTROL(n)					MMIO32(DMA_BASE + 0x10C + ((n) * 0x20))
#define	DMA_CH_CONTROL_TRANSFER_SZIE		GENMASK(12, 0)							 // Transfer size.							
#define	DMA_CH_CONTROL_TRANSFER_SZIE_SHIFT	0									
#define	DMA_CH_CONTROL_SB_SIZE				GENMASK(3, 12)							 // Source burst size						
#define	DMA_CH_CONTROL_SB_SIZE_SHIFT		12									
#define	DMA_CH_CONTROL_SB_SIZE_SZ_1			0x0									
#define	DMA_CH_CONTROL_SB_SIZE_SZ_4			0x1000								
#define	DMA_CH_CONTROL_SB_SIZE_SZ_8			0x2000								
#define	DMA_CH_CONTROL_SB_SIZE_SZ_16		0x3000								
#define	DMA_CH_CONTROL_SB_SIZE_SZ_32		0x4000								
#define	DMA_CH_CONTROL_SB_SIZE_SZ_64		0x5000								
#define	DMA_CH_CONTROL_SB_SIZE_SZ_128		0x6000								
#define	DMA_CH_CONTROL_SB_SIZE_SZ_256		0x7000								
#define	DMA_CH_CONTROL_DB_SIZE				GENMASK(3, 15)							 // Destination burst size					
#define	DMA_CH_CONTROL_DB_SIZE_SHIFT		15									
#define	DMA_CH_CONTROL_DB_SIZE_SZ_1			0x0									
#define	DMA_CH_CONTROL_DB_SIZE_SZ_4			0x8000								
#define	DMA_CH_CONTROL_DB_SIZE_SZ_8			0x10000								
#define	DMA_CH_CONTROL_DB_SIZE_SZ_16		0x18000								
#define	DMA_CH_CONTROL_DB_SIZE_SZ_32		0x20000								
#define	DMA_CH_CONTROL_DB_SIZE_SZ_64		0x28000								
#define	DMA_CH_CONTROL_DB_SIZE_SZ_128		0x30000								
#define	DMA_CH_CONTROL_DB_SIZE_SZ_256		0x38000								
#define	DMA_CH_CONTROL_S_WIDTH				GENMASK(3, 18)							 // Source transfer width					
#define	DMA_CH_CONTROL_S_WIDTH_SHIFT		18									
#define	DMA_CH_CONTROL_S_WIDTH_BYTE			0x0									
#define	DMA_CH_CONTROL_S_WIDTH_WORD			0x40000								
#define	DMA_CH_CONTROL_S_WIDTH_DWORD		0x80000								
#define	DMA_CH_CONTROL_D_WIDTH				GENMASK(3, 21)							 // Destination transfer width				
#define	DMA_CH_CONTROL_D_WIDTH_SHIFT		21									
#define	DMA_CH_CONTROL_D_WIDTH_BYTE			0x0									
#define	DMA_CH_CONTROL_D_WIDTH_WORD			0x200000							
#define	DMA_CH_CONTROL_D_WIDTH_DWORD		0x400000							
#define	DMA_CH_CONTROL_S					BIT(24)									 // Source AHB master select				
#define	DMA_CH_CONTROL_S_AHB1				0x0									
#define	DMA_CH_CONTROL_S_AHB2				0x1000000							
#define	DMA_CH_CONTROL_D					BIT(25)									 // Destination AHB master select			
#define	DMA_CH_CONTROL_D_AHB1				0x0									
#define	DMA_CH_CONTROL_D_AHB2				0x2000000							
#define	DMA_CH_CONTROL_SI					BIT(26)									 // Source increment.						
#define	DMA_CH_CONTROL_DI					BIT(27)									 // Destination increment.					
#define	DMA_CH_CONTROL_PROTECTION			GENMASK(3, 28)							 // Protection.								
#define	DMA_CH_CONTROL_PROTECTION_SHIFT		28									
#define	DMA_CH_CONTROL_I					BIT(31)									 // Terminal count interrupt enable bit.	

#define	DMA_CH_CONFIG(n)					MMIO32(DMA_BASE + 0x110 + ((n) * 0x20))
#define	DMA_CH_CONFIG_ENABLE				BIT(0)									 // Channel enable.							
#define	DMA_CH_CONFIG_SRC_PERIPH			GENMASK(4, 1)							 // Source peripheral.						
#define	DMA_CH_CONFIG_SRC_PERIPH_SHIFT		1									
#define	DMA_CH_CONFIG_DST_PERIPH			GENMASK(6, 5)							 // Destination peripheral.					
#define	DMA_CH_CONFIG_DST_PERIPH_SHIFT		5									
#define	DMA_CH_CONFIG_FLOW_CTRL				GENMASK(3, 11)							 // Flow control and transfer type			
#define	DMA_CH_CONFIG_FLOW_CTRL_SHIFT		11									
#define	DMA_CH_CONFIG_FLOW_CTRL_MEM2MEM		0x0									
#define	DMA_CH_CONFIG_FLOW_CTRL_MEM2PER		0x800								
#define	DMA_CH_CONFIG_FLOW_CTRL_PER2MEM		0x1000								
#define	DMA_CH_CONFIG_FLOW_CTRL_SRC2DST		0x1800								
#define	DMA_CH_CONFIG_FLOW_CTRL_SRC2DST_DST	0x2000								
#define	DMA_CH_CONFIG_FLOW_CTRL_MEM2PER_PER	0x2800								
#define	DMA_CH_CONFIG_FLOW_CTRL_PER2MEM_PER	0x3000								
#define	DMA_CH_CONFIG_FLOW_CTRL_SRC2DST_SRC	0x3800								
#define	DMA_CH_CONFIG_INT_MASK_ERR			BIT(14)									 // Interrupt error mask.					
#define	DMA_CH_CONFIG_INT_MASK_TC			BIT(15)									 // Terminal count interrupt mask.			
#define	DMA_CH_CONFIG_LOCK					BIT(16)									 // Lock.									
#define	DMA_CH_CONFIG_ACTIVE				BIT(17)									 // Active.									
#define	DMA_CH_CONFIG_HALT					BIT(18)									 // Halt.									

#define	DMA_PERIPH_ID0						MMIO32(DMA_BASE + 0xFE0)			
#define	DMA_PERIPH_ID0_PARTNUMBER0			GENMASK(8, 0)																		
#define	DMA_PERIPH_ID0_PARTNUMBER0_SHIFT	0									

#define	DMA_PERIPH_ID1						MMIO32(DMA_BASE + 0xFE4)			
#define	DMA_PERIPH_ID1_PARTNUMBER1			GENMASK(4, 0)																		
#define	DMA_PERIPH_ID1_PARTNUMBER1_SHIFT	0									
#define	DMA_PERIPH_ID1_DESIGNER0			GENMASK(8, 4)																		
#define	DMA_PERIPH_ID1_DESIGNER0_SHIFT		4									

#define	DMA_PERIPH_ID2						MMIO32(DMA_BASE + 0xFE8)			
#define	DMA_PERIPH_ID2_DESIGNER1			GENMASK(4, 0)																		
#define	DMA_PERIPH_ID2_DESIGNER1_SHIFT		0									
#define	DMA_PERIPH_ID2_REVISION				GENMASK(8, 4)																		
#define	DMA_PERIPH_ID2_REVISION_SHIFT		4									

#define	DMA_PERIPH_ID3						MMIO32(DMA_BASE + 0xFEC)			
#define	DMA_PERIPH_ID3_CONFIGURATION		GENMASK(8, 0)																		
#define	DMA_PERIPH_ID3_CONFIGURATION_SHIFT	0									

#define	DMA_PCELL_ID0						MMIO32(DMA_BASE + 0xFF0)			

#define	DMA_PCELL_ID1						MMIO32(DMA_BASE + 0xFF4)			

#define	DMA_PCELL_ID2						MMIO32(DMA_BASE + 0xFF8)			

#define	DMA_PCELL_ID3						MMIO32(DMA_BASE + 0xFFC)			


// CAPCOM0 [MOD_NUM=0050, MOD_REV=00, MOD_32BIT=00]
#define	CAPCOM0_BASE				0xF4000000			
#define	CAPCOM0						0xF4000000			

#define	CAPCOM1_BASE				0xF4100000			
#define	CAPCOM1						0xF4100000			

#define	CAPCOM_CLC(base)			MMIO32((base) + 0x00)
#define	CAPCOM_CLC_DISR				BIT(0)					 // Module Disable Request Bit			
#define	CAPCOM_CLC_DISS				BIT(1)					 // Module Disable Status Bit			
#define	CAPCOM_CLC_SPEN				BIT(2)					 // Module Suspend Enable Bit			
#define	CAPCOM_CLC_EDIS				BIT(3)					 // Module External Request Disable		
#define	CAPCOM_CLC_SBWE				BIT(4)					 // Module Suspend Bit Write Enable		
#define	CAPCOM_CLC_FSOE				BIT(5)					 // Module Fast Shut-Off Enable.		
#define	CAPCOM_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode
#define	CAPCOM_CLC_RMC_SHIFT		8					

#define	CAPCOM_CCPISEL(base)		MMIO32((base) + 0x04)

#define	CAPCOM_ID(base)				MMIO32((base) + 0x08)
#define	CAPCOM_ID_MOD_REV			GENMASK(8, 0)													
#define	CAPCOM_ID_MOD_REV_SHIFT		0					
#define	CAPCOM_ID_MOD_32B			GENMASK(8, 8)													
#define	CAPCOM_ID_MOD_32B_SHIFT		8					
#define	CAPCOM_ID_MOD_NUMBER		GENMASK(16, 16)													
#define	CAPCOM_ID_MOD_NUMBER_SHIFT	16					

#define	CAPCOM_ZERO1(base)			MMIO32((base) + 0x0C)

#define	CAPCOM_T01CON(base)			MMIO32((base) + 0x10)

#define	CAPCOM_CCM0(base)			MMIO32((base) + 0x14)

#define	CAPCOM_CCM1(base)			MMIO32((base) + 0x18)

#define	CAPCOM_CCOUT(base)			MMIO32((base) + 0x24)

#define	CAPCOM_CCIOC(base)			MMIO32((base) + 0x28)

#define	CAPCOM_CCSEE(base)			MMIO32((base) + 0x2C)

#define	CAPCOM_CCSEM(base)			MMIO32((base) + 0x30)

#define	CAPCOM_CCDRM(base)			MMIO32((base) + 0x34)

#define	CAPCOM_T0(base)				MMIO32((base) + 0x40)

#define	CAPCOM_T0REL(base)			MMIO32((base) + 0x44)

#define	CAPCOM_T1(base)				MMIO32((base) + 0x48)

#define	CAPCOM_T1REL(base)			MMIO32((base) + 0x4C)

#define	CAPCOM_CC0(base)			MMIO32((base) + 0x50)

#define	CAPCOM_CC1(base)			MMIO32((base) + 0x54)

#define	CAPCOM_CC2(base)			MMIO32((base) + 0x58)

#define	CAPCOM_CC3(base)			MMIO32((base) + 0x5C)

#define	CAPCOM_CC4(base)			MMIO32((base) + 0x60)

#define	CAPCOM_CC5(base)			MMIO32((base) + 0x64)

#define	CAPCOM_CC6(base)			MMIO32((base) + 0x68)

#define	CAPCOM_CC7(base)			MMIO32((base) + 0x6C)

#define	CAPCOM_CC7IC(base)			MMIO32((base) + 0xD8)

#define	CAPCOM_CC6IC(base)			MMIO32((base) + 0xDC)

#define	CAPCOM_CC5IC(base)			MMIO32((base) + 0xE0)

#define	CAPCOM_CC4IC(base)			MMIO32((base) + 0xE4)

#define	CAPCOM_CC3IC(base)			MMIO32((base) + 0xE8)

#define	CAPCOM_CC2IC(base)			MMIO32((base) + 0xEC)

#define	CAPCOM_CC1IC(base)			MMIO32((base) + 0xF0)

#define	CAPCOM_CC0IC(base)			MMIO32((base) + 0xF4)

#define	CAPCOM_T1IC(base)			MMIO32((base) + 0xF8)

#define	CAPCOM_T0IC(base)			MMIO32((base) + 0xFC)


// GPIO [MOD_NUM=F023, MOD_REV=00, MOD_32BIT=C0]
#define	GPIO_BASE					0xF4300000							
#define	GPIO_CLC					MMIO32(GPIO_BASE + 0x00)			
#define	GPIO_CLC_DISR				BIT(0)									 // Module Disable Request Bit			
#define	GPIO_CLC_DISS				BIT(1)									 // Module Disable Status Bit			
#define	GPIO_CLC_SPEN				BIT(2)									 // Module Suspend Enable Bit			
#define	GPIO_CLC_EDIS				BIT(3)									 // Module External Request Disable		
#define	GPIO_CLC_SBWE				BIT(4)									 // Module Suspend Bit Write Enable		
#define	GPIO_CLC_FSOE				BIT(5)									 // Module Fast Shut-Off Enable			
#define	GPIO_CLC_RMC				GENMASK(8, 8)							 // Module Clock Divider for Normal Mode
#define	GPIO_CLC_RMC_SHIFT			8									

#define	GPIO_ID						MMIO32(GPIO_BASE + 0x08)			
#define	GPIO_ID_MOD_REV				GENMASK(8, 0)																	
#define	GPIO_ID_MOD_REV_SHIFT		0									
#define	GPIO_ID_MOD_32B				GENMASK(8, 8)																	
#define	GPIO_ID_MOD_32B_SHIFT		8									
#define	GPIO_ID_MOD_NUMBER			GENMASK(16, 16)																	
#define	GPIO_ID_MOD_NUMBER_SHIFT	16									

#define	GPIO_MON_CR1				MMIO32(GPIO_BASE + 0x10)			

#define	GPIO_MON_CR2				MMIO32(GPIO_BASE + 0x14)			

#define	GPIO_MON_CR3				MMIO32(GPIO_BASE + 0x18)			

#define	GPIO_MON_CR4				MMIO32(GPIO_BASE + 0x1C)			

#define	GPIO_PIN(n)					MMIO32(GPIO_BASE + 0x20 + ((n) * 0x4))
#define	GPIO_IS						GENMASK(3, 0)																	
#define	GPIO_IS_SHIFT				0									
#define	GPIO_IS_NONE				0x0									
#define	GPIO_IS_ALT0				0x1									
#define	GPIO_IS_ALT1				0x2									
#define	GPIO_IS_ALT2				0x3									
#define	GPIO_IS_ALT3				0x4									
#define	GPIO_IS_ALT4				0x5									
#define	GPIO_IS_ALT5				0x6									
#define	GPIO_IS_ALT6				0x7									
#define	GPIO_OS						GENMASK(3, 4)																	
#define	GPIO_OS_SHIFT				4									
#define	GPIO_OS_NONE				0x0									
#define	GPIO_OS_ALT0				0x10								
#define	GPIO_OS_ALT1				0x20								
#define	GPIO_OS_ALT2				0x30								
#define	GPIO_OS_ALT3				0x40								
#define	GPIO_OS_ALT4				0x50								
#define	GPIO_OS_ALT5				0x60								
#define	GPIO_OS_ALT6				0x70								
#define	GPIO_PS						BIT(8)																			
#define	GPIO_PS_ALT					0x0									
#define	GPIO_PS_MANUAL				0x100								
#define	GPIO_DATA					BIT(9)																			
#define	GPIO_DATA_LOW				0x0									
#define	GPIO_DATA_HIGH				0x200								
#define	GPIO_DIR					BIT(10)																			
#define	GPIO_DIR_IN					0x0									
#define	GPIO_DIR_OUT				0x400								
#define	GPIO_PPEN					BIT(12)																			
#define	GPIO_PPEN_PUSHPULL			0x0									
#define	GPIO_PPEN_OPENDRAIN			0x1000								
#define	GPIO_PDPU					GENMASK(2, 13)																	
#define	GPIO_PDPU_SHIFT				13									
#define	GPIO_PDPU_NONE				0x0									
#define	GPIO_PDPU_PULLUP			0x2000								
#define	GPIO_PDPU_PULLDOWN			0x4000								
#define	GPIO_ENAQ					BIT(15)																			
#define	GPIO_ENAQ_OFF				0x0									
#define	GPIO_ENAQ_ON				0x8000								


// SCU [MOD_NUM=F040, MOD_REV=00, MOD_32BIT=C0]
#define	SCU_BASE					0xF4400000			
#define	SCU_CLC						MMIO32(SCU_BASE + 0x00)
#define	SCU_CLC_DISR				BIT(0)					 // Module Disable Request Bit							
#define	SCU_CLC_DISS				BIT(1)					 // Module Disable Status Bit							
#define	SCU_CLC_SPEN				BIT(2)					 // Module Suspend Enable Bit							
#define	SCU_CLC_EDIS				BIT(3)					 // Module External Request Disable						
#define	SCU_CLC_SBWE				BIT(4)					 // Module Suspend Bit Write Enable						
#define	SCU_CLC_FSOE				BIT(5)					 // Module Fast Shut-Off Enable.						
#define	SCU_CLC_RMC					GENMASK(8, 8)			 // Module Clock Divider for Normal Mode				
#define	SCU_CLC_RMC_SHIFT			8					

#define	SCU_ID						MMIO32(SCU_BASE + 0x08)
#define	SCU_ID_REV_NUMBER			GENMASK(8, 0)			 // SCU Module Revision Number.							
#define	SCU_ID_REV_NUMBER_SHIFT		0					
#define	SCU_ID_MOD_32B				GENMASK(8, 8)			 // Indicates a 32-bit ID register.						
#define	SCU_ID_MOD_32B_SHIFT		8					
#define	SCU_ID_MOD_NUMBER			GENMASK(16, 16)			 // SCU Module Identification Number.					
#define	SCU_ID_MOD_NUMBER_SHIFT		16					

/* Reset Status Register */
#define	SCU_RST_SR					MMIO32(SCU_BASE + 0x10)
#define	SCU_RST_SR_RSSTM			BIT(0)					 // System Timer Reset Status							
#define	SCU_RST_SR_RSEXT			BIT(1)					 // HDRST Line State during Last Reset					
#define	SCU_RST_SR_HWCFG			GENMASK(3, 16)			 // Boot Configuration Selection Status					
#define	SCU_RST_SR_HWCFG_SHIFT		16					
#define	SCU_RST_SR_TESTMODE			BIT(20)					 // State of TESTMODE Pin								
#define	SCU_RST_SR_HWBRKIN			BIT(21)					 // State of BRKIN Pin									
#define	SCU_RST_SR_PWORST			BIT(27)					 // Power-On Reset Status Flag							
#define	SCU_RST_SR_HDRST			BIT(28)					 // Hardware Reset Status Flag							
#define	SCU_RST_SR_SFTRST			BIT(29)					 // Software Reset Status Flag							
#define	SCU_RST_SR_WDTRST			BIT(30)					 // Watchdog Reset Status Flag							
#define	SCU_RST_SR_PWDRST			BIT(31)					 // Power Down/Wake-Up Reset Flag						

/* Reset Request Register */
#define	SCU_RST_REQ					MMIO32(SCU_BASE + 0x18)
#define	SCU_RST_REQ_RRSTM			BIT(0)					 // Reset Request for the System Timer					
#define	SCU_RST_REQ_RREXT			BIT(2)					 // Reset Request for External Devices					
#define	SCU_RST_REQ_SWCFG			GENMASK(3, 16)			 // Software Boot Configuration							
#define	SCU_RST_REQ_SWCFG_SHIFT		16					
#define	SCU_RST_REQ_SWBRKIN			BIT(21)					 // Software Break Signal Boot Value					
#define	SCU_RST_REQ_SWBOOT			BIT(24)					 // Software Boot Configuration Selection				

#define	SCU_WDTCON0					MMIO32(SCU_BASE + 0x24)
#define	SCU_WDTCON0_ENDINIT			BIT(0)					 // End-of-Initialization Control Bit.					
#define	SCU_WDTCON0_WDTLCK			BIT(1)					 // Lock bit to Control Access to WDT_CON0.				
#define	SCU_WDTCON0_WDTHPW0			GENMASK(2, 2)			 // Hardware Password 0.								
#define	SCU_WDTCON0_WDTHPW0_SHIFT	2					
#define	SCU_WDTCON0_WDTHPW1			GENMASK(4, 4)			 // Hardware Password 1.								
#define	SCU_WDTCON0_WDTHPW1_SHIFT	4					
#define	SCU_WDTCON0_WDTPW			GENMASK(8, 8)			 // User-Definable Password Field for Access to WDT_CON0.
#define	SCU_WDTCON0_WDTPW_SHIFT		8					
#define	SCU_WDTCON0_WDTREL			GENMASK(16, 16)			 // Reload Value for the Watchdog Timer.				
#define	SCU_WDTCON0_WDTREL_SHIFT	16					

#define	SCU_WDTCON1					MMIO32(SCU_BASE + 0x28)
#define	SCU_WDTCON1_WDTIR			BIT(2)					 // Watchdog Timer Input Frequency Request Control Bit.	
#define	SCU_WDTCON1_WDTDR			BIT(3)					 // Watchdog Timer Disable Request Control Bit.			

#define	SCU_WDT_SR					MMIO32(SCU_BASE + 0x2C)
#define	SCU_WDT_SR_WDTAE			BIT(0)					 // Watchdog Access Error Status Flag					
#define	SCU_WDT_SR_WDTOE			BIT(1)					 // Watchdog Overflow Error Status Flag					
#define	SCU_WDT_SR_WDTIS			BIT(2)					 // Watchdog Input Clock Status Flag					
#define	SCU_WDT_SR_WDTDS			BIT(3)					 // Watchdog Enable/Disable Status Flag					
#define	SCU_WDT_SR_WDTTO			BIT(4)					 // Watchdog Time-out Mode Flag							
#define	SCU_WDT_SR_WDTPR			BIT(5)					 // Watchdog Prewarning Mode Flag						
#define	SCU_WDT_SR_WDTTIM			GENMASK(16, 16)			 // Watchdog Timer Value								
#define	SCU_WDT_SR_WDTTIM_SHIFT		16					

#define	SCU_EBUCLC0					MMIO32(SCU_BASE + 0x40)
#define	SCU_EBUCLC0_FLAG1			GENMASK(4, 0)																	
#define	SCU_EBUCLC0_FLAG1_SHIFT		0					
#define	SCU_EBUCLC0_FLAG2			GENMASK(4, 4)																	
#define	SCU_EBUCLC0_FLAG2_SHIFT		4					

#define	SCU_EBUCLC					MMIO32(SCU_BASE + 0x40)
#define	SCU_EBUCLC_LOCK				BIT(0)																			
#define	SCU_EBUCLC_VCOBYP			BIT(8)																			

#define	SCU_EBUCLC1					MMIO32(SCU_BASE + 0x44)
#define	SCU_EBUCLC1_FLAG1			GENMASK(4, 0)																	
#define	SCU_EBUCLC1_FLAG1_SHIFT		0					
#define	SCU_EBUCLC1_FLAG2			GENMASK(4, 4)																	
#define	SCU_EBUCLC1_FLAG2_SHIFT		4					

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

#define	SCU_BOOT_FLAG				MMIO32(SCU_BASE + 0x78)
#define	SCU_BOOT_FLAG_BOOT_OK		BIT(0)																			

#define	SCU_ROMAMCR					MMIO32(SCU_BASE + 0x7C)
#define	SCU_ROMAMCR_MOUNT_BROM		BIT(0)																			

#define	SCU_RTID					MMIO32(SCU_BASE + 0x80)

/* DMA Request Select Register */
#define	SCU_DMARS					MMIO32(SCU_BASE + 0x84)
#define	SCU_DMARS_SEL0				BIT(0)					 // Request Select Bit 0								
#define	SCU_DMARS_SEL1				BIT(1)					 // Request Select Bit 1								
#define	SCU_DMARS_SEL2				BIT(2)					 // Request Select Bit 2								
#define	SCU_DMARS_SEL3				BIT(3)					 // Request Select Bit 3								
#define	SCU_DMARS_SEL4				BIT(4)					 // Request Select Bit 4								
#define	SCU_DMARS_SEL5				BIT(5)					 // Request Select Bit 5								
#define	SCU_DMARS_SEL6				BIT(6)					 // Request Select Bit 6								
#define	SCU_DMARS_SEL7				BIT(7)					 // Request Select Bit 7								
#define	SCU_DMARS_SEL8				BIT(8)					 // Request Select Bit 8								
#define	SCU_DMARS_SEL9				BIT(9)					 // Request Select Bit 9								


// PLL
#define	PLL_BASE				0xF4500000			
#define	PLL_OSC					MMIO32(PLL_BASE + 0xA0)

#define	PLL_CON0				MMIO32(PLL_BASE + 0xA4)

#define	PLL_CON1				MMIO32(PLL_BASE + 0xA8)
#define	PLL_CON1_FSYS_DIV_EN	BIT(25)					 // Enable fsys divider and div fsys by 4
#define	PLL_CON1_FSYS_DIV		GENMASK(2, 28)			 // Fsys diviver (work if FSYS_DIV_EN=1)
#define	PLL_CON1_FSYS_DIV_SHIFT	28					

#define	PLL_CON2				MMIO32(PLL_BASE + 0xAC)

#define	PLL_STAT				MMIO32(PLL_BASE + 0xB0)

#define	PLL_CON3				MMIO32(PLL_BASE + 0xB4)

#define	PLL_CON4				MMIO32(PLL_BASE + 0xCC)


// RTC [MOD_NUM=F049, MOD_REV=00, MOD_32BIT=C0]
#define	RTC_BASE				0xF4700000			
#define	RTC_CLC					MMIO32(RTC_BASE + 0x00)
#define	RTC_CLC_DISR			BIT(0)					 // Module Disable Request Bit							
#define	RTC_CLC_DISS			BIT(1)					 // Module Disable Status Bit							
#define	RTC_CLC_SPEN			BIT(2)					 // Module Suspend Enable Bit							
#define	RTC_CLC_EDIS			BIT(3)					 // Module External Request Disable						
#define	RTC_CLC_SBWE			BIT(4)					 // Module Suspend Bit Write Enable						
#define	RTC_CLC_FSOE			BIT(5)					 // Module Fast Shut-Off Enable.						
#define	RTC_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode				
#define	RTC_CLC_RMC_SHIFT		8					

#define	RTC_ID					MMIO32(RTC_BASE + 0x08)
#define	RTC_ID_REV				GENMASK(8, 0)			 // Module Revision Number.								
#define	RTC_ID_REV_SHIFT		0					
#define	RTC_ID_MOD_32B			GENMASK(8, 8)			 // Indicates a 32-bit ID register.						
#define	RTC_ID_MOD_32B_SHIFT	8					
#define	RTC_ID_MOD				GENMASK(16, 16)			 // Module Identification Number.						
#define	RTC_ID_MOD_SHIFT		16					

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


// GPTU0 [MOD_NUM=0001, MOD_REV=00, MOD_32BIT=C0]
#define	GPTU0_BASE					0xF4900000			
#define	GPTU0						0xF4900000			

#define	GPTU1_BASE					0xF4A00000			
#define	GPTU1						0xF4A00000			

#define	GPTU_CLC(base)				MMIO32((base) + 0x00)
#define	GPTU_CLC_DISR				BIT(0)					 // Module Disable Request Bit									
#define	GPTU_CLC_DISS				BIT(1)					 // Module Disable Status Bit									
#define	GPTU_CLC_SPEN				BIT(2)					 // Module Suspend Enable Bit									
#define	GPTU_CLC_EDIS				BIT(3)					 // Module External Request Disable								
#define	GPTU_CLC_SBWE				BIT(4)					 // Module Suspend Bit Write Enable								
#define	GPTU_CLC_FSOE				BIT(5)					 // Module Fast Shut-Off Enable.								
#define	GPTU_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode						
#define	GPTU_CLC_RMC_SHIFT			8					

#define	GPTU_ID(base)				MMIO32((base) + 0x08)
#define	GPTU_ID_MOD_REV				GENMASK(8, 0)			 // GPTU Module Revision Number.								
#define	GPTU_ID_MOD_REV_SHIFT		0					
#define	GPTU_ID_MOD_32B				GENMASK(8, 8)			 // Indicates 32-bit Module GPTUx_ID format						
#define	GPTU_ID_MOD_32B_SHIFT		8					
#define	GPTU_ID_MOD_NUMBER			GENMASK(16, 16)			 // GPTU Module Identification Number.							
#define	GPTU_ID_MOD_NUMBER_SHIFT	16					

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
#define	GPTU_SRSEL_SSR7				GENMASK(4, 0)			 // GPTU Service Request Node 7 Source Selection				
#define	GPTU_SRSEL_SSR7_SHIFT		0					
#define	GPTU_SRSEL_SSR6				GENMASK(4, 4)			 // GPTU Service Request Node 6 Source Selection				
#define	GPTU_SRSEL_SSR6_SHIFT		4					
#define	GPTU_SRSEL_SSR5				GENMASK(4, 8)			 // GPTU Service Request Node 5 Source Selection				
#define	GPTU_SRSEL_SSR5_SHIFT		8					
#define	GPTU_SRSEL_SSR4				GENMASK(4, 12)			 // GPTU Service Request Node 4 Source Selection				
#define	GPTU_SRSEL_SSR4_SHIFT		12					
#define	GPTU_SRSEL_SSR3				GENMASK(4, 16)			 // GPTU Service Request Node 3 Source Selection				
#define	GPTU_SRSEL_SSR3_SHIFT		16					
#define	GPTU_SRSEL_SSR2				GENMASK(4, 20)			 // GPTU Service Request Node 2 Source Selection				
#define	GPTU_SRSEL_SSR2_SHIFT		20					
#define	GPTU_SRSEL_SSR1				GENMASK(4, 24)			 // GPTU Service Request Node 1 Source Selection				
#define	GPTU_SRSEL_SSR1_SHIFT		24					
#define	GPTU_SRSEL_SSR0				GENMASK(4, 28)			 // GPTU Service Request Node 0 Source Selection				
#define	GPTU_SRSEL_SSR0_SHIFT		28					

#define	GPTU_SRC7(base)				MMIO32((base) + 0xE0)
#define	GPTU_SRC7_SRPN				GENMASK(8, 0)			 // Service request node priority number						
#define	GPTU_SRC7_SRPN_SHIFT		0					
#define	GPTU_SRC7_TOS				GENMASK(2, 10)			 // Type of service for node									
#define	GPTU_SRC7_TOS_SHIFT			10					
#define	GPTU_SRC7_SRE				BIT(12)					 // Service request node enable									
#define	GPTU_SRC7_SRR				BIT(13)					 // Service Request Node Service Request Bit					
#define	GPTU_SRC7_CLRR				BIT(14)					 // Service Request Node Request Clear Bit						
#define	GPTU_SRC7_SETR				BIT(15)					 // Service Request Node Request Set Bit						

#define	GPTU_SRC6(base)				MMIO32((base) + 0xE4)
#define	GPTU_SRC6_SRPN				GENMASK(8, 0)			 // Service request node priority number						
#define	GPTU_SRC6_SRPN_SHIFT		0					
#define	GPTU_SRC6_TOS				GENMASK(2, 10)			 // Type of service for node									
#define	GPTU_SRC6_TOS_SHIFT			10					
#define	GPTU_SRC6_SRE				BIT(12)					 // Service request node enable									
#define	GPTU_SRC6_SRR				BIT(13)					 // Service Request Node Service Request Bit					
#define	GPTU_SRC6_CLRR				BIT(14)					 // Service Request Node Request Clear Bit						
#define	GPTU_SRC6_SETR				BIT(15)					 // Service Request Node Request Set Bit						

#define	GPTU_SRC5(base)				MMIO32((base) + 0xE8)
#define	GPTU_SRC5_SRPN				GENMASK(8, 0)			 // Service request node priority number						
#define	GPTU_SRC5_SRPN_SHIFT		0					
#define	GPTU_SRC5_TOS				GENMASK(2, 10)			 // Type of service for node									
#define	GPTU_SRC5_TOS_SHIFT			10					
#define	GPTU_SRC5_SRE				BIT(12)					 // Service request node enable									
#define	GPTU_SRC5_SRR				BIT(13)					 // Service Request Node Service Request Bit					
#define	GPTU_SRC5_CLRR				BIT(14)					 // Service Request Node Request Clear Bit						
#define	GPTU_SRC5_SETR				BIT(15)					 // Service Request Node Request Set Bit						

#define	GPTU_SRC4(base)				MMIO32((base) + 0xEC)
#define	GPTU_SRC4_SRPN				GENMASK(8, 0)			 // Service request node priority number						
#define	GPTU_SRC4_SRPN_SHIFT		0					
#define	GPTU_SRC4_TOS				GENMASK(2, 10)			 // Type of service for node									
#define	GPTU_SRC4_TOS_SHIFT			10					
#define	GPTU_SRC4_SRE				BIT(12)					 // Service request node enable									
#define	GPTU_SRC4_SRR				BIT(13)					 // Service Request Node Service Request Bit					
#define	GPTU_SRC4_CLRR				BIT(14)					 // Service Request Node Request Clear Bit						
#define	GPTU_SRC4_SETR				BIT(15)					 // Service Request Node Request Set Bit						

#define	GPTU_SRC3(base)				MMIO32((base) + 0xF0)
#define	GPTU_SRC3_SRPN				GENMASK(8, 0)			 // Service request node priority number						
#define	GPTU_SRC3_SRPN_SHIFT		0					
#define	GPTU_SRC3_TOS				GENMASK(2, 10)			 // Type of service for node									
#define	GPTU_SRC3_TOS_SHIFT			10					
#define	GPTU_SRC3_SRE				BIT(12)					 // Service request node enable									
#define	GPTU_SRC3_SRR				BIT(13)					 // Service Request Node Service Request Bit					
#define	GPTU_SRC3_CLRR				BIT(14)					 // Service Request Node Request Clear Bit						
#define	GPTU_SRC3_SETR				BIT(15)					 // Service Request Node Request Set Bit						

#define	GPTU_SRC2(base)				MMIO32((base) + 0xF4)
#define	GPTU_SRC2_SRPN				GENMASK(8, 0)			 // Service request node priority number						
#define	GPTU_SRC2_SRPN_SHIFT		0					
#define	GPTU_SRC2_TOS				GENMASK(2, 10)			 // Type of service for node									
#define	GPTU_SRC2_TOS_SHIFT			10					
#define	GPTU_SRC2_SRE				BIT(12)					 // Service request node enable									
#define	GPTU_SRC2_SRR				BIT(13)					 // Service Request Node Service Request Bit					
#define	GPTU_SRC2_CLRR				BIT(14)					 // Service Request Node Request Clear Bit						
#define	GPTU_SRC2_SETR				BIT(15)					 // Service Request Node Request Set Bit						

#define	GPTU_SRC1(base)				MMIO32((base) + 0xF8)
#define	GPTU_SRC1_SRPN				GENMASK(8, 0)			 // Service request node priority number						
#define	GPTU_SRC1_SRPN_SHIFT		0					
#define	GPTU_SRC1_TOS				GENMASK(2, 10)			 // Type of service for node									
#define	GPTU_SRC1_TOS_SHIFT			10					
#define	GPTU_SRC1_SRE				BIT(12)					 // Service request node enable									
#define	GPTU_SRC1_SRR				BIT(13)					 // Service Request Node Service Request Bit					
#define	GPTU_SRC1_CLRR				BIT(14)					 // Service Request Node Request Clear Bit						
#define	GPTU_SRC1_SETR				BIT(15)					 // Service Request Node Request Set Bit						

#define	GPTU_SRC0(base)				MMIO32((base) + 0xFC)
#define	GPTU_SRC0_SRPN				GENMASK(8, 0)			 // Service request node priority number						
#define	GPTU_SRC0_SRPN_SHIFT		0					
#define	GPTU_SRC0_TOS				GENMASK(2, 10)			 // Type of service for node									
#define	GPTU_SRC0_TOS_SHIFT			10					
#define	GPTU_SRC0_SRE				BIT(12)					 // Service request node enable									
#define	GPTU_SRC0_SRR				BIT(13)					 // Service Request Node Service Request Bit					
#define	GPTU_SRC0_CLRR				BIT(14)					 // Service Request Node Request Clear Bit						
#define	GPTU_SRC0_SETR				BIT(15)					 // Service Request Node Request Set Bit						


// STM [MOD_NUM=0000, MOD_REV=00, MOD_32BIT=C0]
#define	STM_BASE				0xF4B00000			
#define	STM_CLC					MMIO32(STM_BASE + 0x00)
#define	STM_CLC_DISR			BIT(0)					 // Module Disable Request Bit				
#define	STM_CLC_DISS			BIT(1)					 // Module Disable Status Bit				
#define	STM_CLC_SPEN			BIT(2)					 // Module Suspend Enable Bit				
#define	STM_CLC_EDIS			BIT(3)					 // Module External Request Disable			
#define	STM_CLC_SBWE			BIT(4)					 // Module Suspend Bit Write Enable			
#define	STM_CLC_FSOE			BIT(5)					 // Module Fast Shut-Off Enable.			
#define	STM_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode	
#define	STM_CLC_RMC_SHIFT		8					

#define	STM_ID					MMIO32(STM_BASE + 0x08)
#define	STM_ID_REV				GENMASK(8, 0)			 // System Timer Module Revision Number.	
#define	STM_ID_REV_SHIFT		0					
#define	STM_ID_MOD_32B			GENMASK(8, 8)			 // Indicates a 32-bit ID register.			
#define	STM_ID_MOD_32B_SHIFT	8					
#define	STM_ID_MOD				GENMASK(16, 16)			 // System Timer Module Identification Number.
#define	STM_ID_MOD_SHIFT		16					

#define	STM_TIM0				MMIO32(STM_BASE + 0x10)

#define	STM_TIM1				MMIO32(STM_BASE + 0x14)

#define	STM_TIM2				MMIO32(STM_BASE + 0x18)

#define	STM_TIM3				MMIO32(STM_BASE + 0x1C)

#define	STM_TIM4				MMIO32(STM_BASE + 0x20)

#define	STM_TIM5				MMIO32(STM_BASE + 0x24)

#define	STM_TIM6				MMIO32(STM_BASE + 0x28)


// AMC [MOD_NUM=F024, MOD_REV=00, MOD_32BIT=C0]
#define	AMC_BASE				0xF4C00000			
#define	AMC_CLC					MMIO32(AMC_BASE + 0x00)
#define	AMC_CLC_DISR			BIT(0)					 // Module Disable Request Bit				
#define	AMC_CLC_DISS			BIT(1)					 // Module Disable Status Bit				
#define	AMC_CLC_SPEN			BIT(2)					 // Module Suspend Enable Bit				
#define	AMC_CLC_EDIS			BIT(3)					 // Module External Request Disable			
#define	AMC_CLC_SBWE			BIT(4)					 // Module Suspend Bit Write Enable			
#define	AMC_CLC_FSOE			BIT(5)					 // Module Fast Shut-Off Enable.			
#define	AMC_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode	
#define	AMC_CLC_RMC_SHIFT		8					

#define	AMC_ID					MMIO32(AMC_BASE + 0x08)
#define	AMC_ID_REV				GENMASK(8, 0)			 // System Timer Module Revision Number.	
#define	AMC_ID_REV_SHIFT		0					
#define	AMC_ID_MOD_32B			GENMASK(8, 8)			 // Indicates a 32-bit ID register.			
#define	AMC_ID_MOD_32B_SHIFT	8					
#define	AMC_ID_MOD				GENMASK(16, 16)			 // System Timer Module Identification Number.
#define	AMC_ID_MOD_SHIFT		16					


// KEYPAD [MOD_NUM=F046, MOD_REV=00, MOD_32BIT=C0]
#define	KEYPAD_BASE				0xF4D00000				
#define	KEYPAD_CLC				MMIO32(KEYPAD_BASE + 0x00)
#define	KEYPAD_CLC_DISR			BIT(0)						 // Module Disable Request Bit				
#define	KEYPAD_CLC_DISS			BIT(1)						 // Module Disable Status Bit				
#define	KEYPAD_CLC_SPEN			BIT(2)						 // Module Suspend Enable Bit				
#define	KEYPAD_CLC_EDIS			BIT(3)						 // Module External Request Disable			
#define	KEYPAD_CLC_SBWE			BIT(4)						 // Module Suspend Bit Write Enable			
#define	KEYPAD_CLC_FSOE			BIT(5)						 // Module Fast Shut-Off Enable.			
#define	KEYPAD_CLC_RMC			GENMASK(8, 8)				 // Module Clock Divider for Normal Mode	
#define	KEYPAD_CLC_RMC_SHIFT	8						

#define	KEYPAD_ID				MMIO32(KEYPAD_BASE + 0x08)
#define	KEYPAD_ID_REV			GENMASK(8, 0)				 // System Timer Module Revision Number.	
#define	KEYPAD_ID_REV_SHIFT		0						
#define	KEYPAD_ID_MOD_32B		GENMASK(8, 8)				 // Indicates a 32-bit ID register.			
#define	KEYPAD_ID_MOD_32B_SHIFT	8						
#define	KEYPAD_ID_MOD			GENMASK(16, 16)				 // System Timer Module Identification Number.
#define	KEYPAD_ID_MOD_SHIFT		16						


// GSM_DSP [MOD_NUM=F022, MOD_REV=00, MOD_32BIT=C0]
#define	GSM_DSP_BASE				0xF6000000				
#define	GSM_DSP_CLC					MMIO32(GSM_DSP_BASE + 0x00)
#define	GSM_DSP_CLC_DISR			BIT(0)						 // Module Disable Request Bit				
#define	GSM_DSP_CLC_DISS			BIT(1)						 // Module Disable Status Bit				
#define	GSM_DSP_CLC_SPEN			BIT(2)						 // Module Suspend Enable Bit				
#define	GSM_DSP_CLC_EDIS			BIT(3)						 // Module External Request Disable			
#define	GSM_DSP_CLC_SBWE			BIT(4)						 // Module Suspend Bit Write Enable			
#define	GSM_DSP_CLC_FSOE			BIT(5)						 // Module Fast Shut-Off Enable.			
#define	GSM_DSP_CLC_RMC				GENMASK(8, 8)				 // Module Clock Divider for Normal Mode	
#define	GSM_DSP_CLC_RMC_SHIFT		8						

#define	GSM_DSP_ID					MMIO32(GSM_DSP_BASE + 0x08)
#define	GSM_DSP_ID_REV				GENMASK(8, 0)				 // System Timer Module Revision Number.	
#define	GSM_DSP_ID_REV_SHIFT		0						
#define	GSM_DSP_ID_MOD_32B			GENMASK(8, 8)				 // Indicates a 32-bit ID register.			
#define	GSM_DSP_ID_MOD_32B_SHIFT	8						
#define	GSM_DSP_ID_MOD				GENMASK(16, 16)				 // System Timer Module Identification Number.
#define	GSM_DSP_ID_MOD_SHIFT		16						


// GSM_GPRS [MOD_NUM=F003, MOD_REV=00, MOD_32BIT=C0]
#define	GSM_GPRS_BASE				0xF6200000					
#define	GSM_GPRS_CLC				MMIO32(GSM_GPRS_BASE + 0x00)
#define	GSM_GPRS_CLC_DISR			BIT(0)							 // Module Disable Request Bit				
#define	GSM_GPRS_CLC_DISS			BIT(1)							 // Module Disable Status Bit				
#define	GSM_GPRS_CLC_SPEN			BIT(2)							 // Module Suspend Enable Bit				
#define	GSM_GPRS_CLC_EDIS			BIT(3)							 // Module External Request Disable			
#define	GSM_GPRS_CLC_SBWE			BIT(4)							 // Module Suspend Bit Write Enable			
#define	GSM_GPRS_CLC_FSOE			BIT(5)							 // Module Fast Shut-Off Enable.			
#define	GSM_GPRS_CLC_RMC			GENMASK(8, 8)					 // Module Clock Divider for Normal Mode	
#define	GSM_GPRS_CLC_RMC_SHIFT		8							

#define	GSM_GPRS_ID					MMIO32(GSM_GPRS_BASE + 0x08)
#define	GSM_GPRS_ID_REV				GENMASK(8, 0)					 // System Timer Module Revision Number.	
#define	GSM_GPRS_ID_REV_SHIFT		0							
#define	GSM_GPRS_ID_MOD_32B			GENMASK(8, 8)					 // Indicates a 32-bit ID register.			
#define	GSM_GPRS_ID_MOD_32B_SHIFT	8							
#define	GSM_GPRS_ID_MOD				GENMASK(16, 16)					 // System Timer Module Identification Number.
#define	GSM_GPRS_ID_MOD_SHIFT		16							


// GSM_AFC [MOD_NUM=F004, MOD_REV=00, MOD_32BIT=C0]
#define	GSM_AFC_BASE				0xF6300000				
#define	GSM_AFC_CLC					MMIO32(GSM_AFC_BASE + 0x00)
#define	GSM_AFC_CLC_DISR			BIT(0)						 // Module Disable Request Bit				
#define	GSM_AFC_CLC_DISS			BIT(1)						 // Module Disable Status Bit				
#define	GSM_AFC_CLC_SPEN			BIT(2)						 // Module Suspend Enable Bit				
#define	GSM_AFC_CLC_EDIS			BIT(3)						 // Module External Request Disable			
#define	GSM_AFC_CLC_SBWE			BIT(4)						 // Module Suspend Bit Write Enable			
#define	GSM_AFC_CLC_FSOE			BIT(5)						 // Module Fast Shut-Off Enable.			
#define	GSM_AFC_CLC_RMC				GENMASK(8, 8)				 // Module Clock Divider for Normal Mode	
#define	GSM_AFC_CLC_RMC_SHIFT		8						

#define	GSM_AFC_ID					MMIO32(GSM_AFC_BASE + 0x08)
#define	GSM_AFC_ID_REV				GENMASK(8, 0)				 // System Timer Module Revision Number.	
#define	GSM_AFC_ID_REV_SHIFT		0						
#define	GSM_AFC_ID_MOD_32B			GENMASK(8, 8)				 // Indicates a 32-bit ID register.			
#define	GSM_AFC_ID_MOD_32B_SHIFT	8						
#define	GSM_AFC_ID_MOD				GENMASK(16, 16)				 // System Timer Module Identification Number.
#define	GSM_AFC_ID_MOD_SHIFT		16						


// GSM_TPU [MOD_NUM=F021, MOD_REV=00, MOD_32BIT=C0]
#define	GSM_TPU_BASE					0xF6400000				
#define	GSM_TPU_CLC						MMIO32(GSM_TPU_BASE + 0x00)
#define	GSM_TPU_CLC_DISR				BIT(0)						 // Module Disable Request Bit				
#define	GSM_TPU_CLC_DISS				BIT(1)						 // Module Disable Status Bit				
#define	GSM_TPU_CLC_SPEN				BIT(2)						 // Module Suspend Enable Bit				
#define	GSM_TPU_CLC_EDIS				BIT(3)						 // Module External Request Disable			
#define	GSM_TPU_CLC_SBWE				BIT(4)						 // Module Suspend Bit Write Enable			
#define	GSM_TPU_CLC_FSOE				BIT(5)						 // Module Fast Shut-Off Enable.			
#define	GSM_TPU_CLC_RMC					GENMASK(8, 8)				 // Module Clock Divider for Normal Mode	
#define	GSM_TPU_CLC_RMC_SHIFT			8						

#define	GSM_TPU_ID						MMIO32(GSM_TPU_BASE + 0x08)
#define	GSM_TPU_ID_REV					GENMASK(8, 0)				 // System Timer Module Revision Number.	
#define	GSM_TPU_ID_REV_SHIFT			0						
#define	GSM_TPU_ID_MOD_32B				GENMASK(8, 8)				 // Indicates a 32-bit ID register.			
#define	GSM_TPU_ID_MOD_32B_SHIFT		8						
#define	GSM_TPU_ID_MOD					GENMASK(16, 16)				 // System Timer Module Identification Number.
#define	GSM_TPU_ID_MOD_SHIFT			16						

#define	GSM_TPU_UNK0					MMIO32(GSM_TPU_BASE + 0x10)

#define	GSM_TPU_UNK1					MMIO32(GSM_TPU_BASE + 0x14)

#define	GSM_TPU_UNK2					MMIO32(GSM_TPU_BASE + 0x18)

#define	GSM_TPU_CORRECTION				MMIO32(GSM_TPU_BASE + 0x1C)
#define	GSM_TPU_CORRECTION_VALUE		GENMASK(15, 0)															
#define	GSM_TPU_CORRECTION_VALUE_SHIFT	0						
#define	GSM_TPU_CORRECTION_CTRL			BIT(16)																	

#define	GSM_TPU_OVERFLOW				MMIO32(GSM_TPU_BASE + 0x20)
#define	GSM_TPU_OVERFLOW_VALUE			GENMASK(15, 0)															
#define	GSM_TPU_OVERFLOW_VALUE_SHIFT	0						

#define	GSM_TPU_INT0					MMIO32(GSM_TPU_BASE + 0x24)
#define	GSM_TPU_INT0_VALUE				GENMASK(15, 0)															
#define	GSM_TPU_INT0_VALUE_SHIFT		0						

#define	GSM_TPU_INT1					MMIO32(GSM_TPU_BASE + 0x28)
#define	GSM_TPU_INT1_VALUE				GENMASK(15, 0)															
#define	GSM_TPU_INT1_VALUE_SHIFT		0						

#define	GSM_TPU_OFFSET					MMIO32(GSM_TPU_BASE + 0x2C)
#define	GSM_TPU_OFFSET_VALUE			GENMASK(15, 0)															
#define	GSM_TPU_OFFSET_VALUE_SHIFT		0						
#define	GSM_TPU_OFFSET_CTRL				BIT(16)																	

#define	GSM_TPU_SKIP					MMIO32(GSM_TPU_BASE + 0x30)
#define	GSM_TPU_SKIP_SKIPN				BIT(0)																	
#define	GSM_TPU_SKIP_SKIPC				BIT(1)																	

#define	GSM_TPU_COUNTER					MMIO32(GSM_TPU_BASE + 0x34)
#define	GSM_TPU_COUNTER_VALUE			GENMASK(15, 0)															
#define	GSM_TPU_COUNTER_VALUE_SHIFT		0						

#define	GSM_TPU_UNK3					MMIO32(GSM_TPU_BASE + 0x38)

#define	GSM_TPU_UNK4					MMIO32(GSM_TPU_BASE + 0x3C)

#define	GSM_TPU_UNK5					MMIO32(GSM_TPU_BASE + 0x40)

#define	GSM_TPU_UNK6					MMIO32(GSM_TPU_BASE + 0x44)

#define	GSM_TPU_PARAM					MMIO32(GSM_TPU_BASE + 0x5C)
#define	GSM_TPU_PARAM_TINI				BIT(0)																	
#define	GSM_TPU_PARAM_FDIS				BIT(1)																	

#define	GSM_TPU_UNK7					MMIO32(GSM_TPU_BASE + 0x60)

#define	GSM_TPU_PLLCON0					MMIO32(GSM_TPU_BASE + 0x68)
#define	GSM_TPU_PLLCON0_K_DIV			GENMASK(30, 0)															
#define	GSM_TPU_PLLCON0_K_DIV_SHIFT		0						

#define	GSM_TPU_PLLCON1					MMIO32(GSM_TPU_BASE + 0x6C)
#define	GSM_TPU_PLLCON1_L_DIV			GENMASK(30, 0)															
#define	GSM_TPU_PLLCON1_L_DIV_SHIFT		0						

#define	GSM_TPU_PLLCON2					MMIO32(GSM_TPU_BASE + 0x70)
#define	GSM_TPU_PLLCON2_LOAD			BIT(0)																	
#define	GSM_TPU_PLLCON2_INIT			BIT(1)																	

#define	GSM_TPU_ICR0					MMIO32(GSM_TPU_BASE + 0xF8)
#define	GSM_TPU_ICR0_ENABLE				BIT(12)																	
#define	GSM_TPU_ICR0_RESET				BIT(14)																	

#define	GSM_TPU_ICR1					MMIO32(GSM_TPU_BASE + 0xFC)
#define	GSM_TPU_ICR1_ENABLE				BIT(12)																	
#define	GSM_TPU_ICR1_RESET				BIT(14)																	


// CIF [MOD_NUM=F052, MOD_REV=00, MOD_32BIT=C0]
#define	CIF_BASE				0xF7000000			
#define	CIF_CLC					MMIO32(CIF_BASE + 0x00)
#define	CIF_CLC_DISR			BIT(0)					 // Module Disable Request Bit			
#define	CIF_CLC_DISS			BIT(1)					 // Module Disable Status Bit			
#define	CIF_CLC_SPEN			BIT(2)					 // Module Suspend Enable Bit			
#define	CIF_CLC_EDIS			BIT(3)					 // Module External Request Disable		
#define	CIF_CLC_SBWE			BIT(4)					 // Module Suspend Bit Write Enable		
#define	CIF_CLC_FSOE			BIT(5)					 // Module Fast Shut-Off Enable.		
#define	CIF_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode
#define	CIF_CLC_RMC_SHIFT		8					

#define	CIF_ID					MMIO32(CIF_BASE + 0x08)
#define	CIF_ID_MOD_REV			GENMASK(8, 0)													
#define	CIF_ID_MOD_REV_SHIFT	0					
#define	CIF_ID_MOD_32B			GENMASK(8, 8)													
#define	CIF_ID_MOD_32B_SHIFT	8					
#define	CIF_ID_MOD_NUMBER		GENMASK(16, 16)													
#define	CIF_ID_MOD_NUMBER_SHIFT	16					


// DIF [MOD_NUM=F043, MOD_REV=00, MOD_32BIT=C0]
#define	DIF_BASE				0xF7100000			
#define	DIF_CLC					MMIO32(DIF_BASE + 0x00)
#define	DIF_CLC_DISR			BIT(0)					 // Module Disable Request Bit			
#define	DIF_CLC_DISS			BIT(1)					 // Module Disable Status Bit			
#define	DIF_CLC_SPEN			BIT(2)					 // Module Suspend Enable Bit			
#define	DIF_CLC_EDIS			BIT(3)					 // Module External Request Disable		
#define	DIF_CLC_SBWE			BIT(4)					 // Module Suspend Bit Write Enable		
#define	DIF_CLC_FSOE			BIT(5)					 // Module Fast Shut-Off Enable.		
#define	DIF_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode
#define	DIF_CLC_RMC_SHIFT		8					

#define	DIF_ID					MMIO32(DIF_BASE + 0x08)
#define	DIF_ID_MOD_REV			GENMASK(8, 0)													
#define	DIF_ID_MOD_REV_SHIFT	0					
#define	DIF_ID_MOD_32B			GENMASK(8, 8)													
#define	DIF_ID_MOD_32B_SHIFT	8					
#define	DIF_ID_MOD_NUMBER		GENMASK(16, 16)													
#define	DIF_ID_MOD_NUMBER_SHIFT	16					


// MCI [MOD_NUM=F00F, MOD_REV=00, MOD_32BIT=C0]
#define	MCI_BASE				0xF7200000			
#define	MCI_CLC					MMIO32(MCI_BASE + 0x00)
#define	MCI_CLC_DISR			BIT(0)					 // Module Disable Request Bit			
#define	MCI_CLC_DISS			BIT(1)					 // Module Disable Status Bit			
#define	MCI_CLC_SPEN			BIT(2)					 // Module Suspend Enable Bit			
#define	MCI_CLC_EDIS			BIT(3)					 // Module External Request Disable		
#define	MCI_CLC_SBWE			BIT(4)					 // Module Suspend Bit Write Enable		
#define	MCI_CLC_FSOE			BIT(5)					 // Module Fast Shut-Off Enable.		
#define	MCI_CLC_RMC				GENMASK(8, 8)			 // Module Clock Divider for Normal Mode
#define	MCI_CLC_RMC_SHIFT		8					

#define	MCI_ID					MMIO32(MCI_BASE + 0x08)
#define	MCI_ID_MOD_REV			GENMASK(8, 0)													
#define	MCI_ID_MOD_REV_SHIFT	0					
#define	MCI_ID_MOD_32B			GENMASK(8, 8)													
#define	MCI_ID_MOD_32B_SHIFT	8					
#define	MCI_ID_MOD_NUMBER		GENMASK(16, 16)													
#define	MCI_ID_MOD_NUMBER_SHIFT	16					


// I2C [MOD_NUM=F057, MOD_REV=00, MOD_32BIT=C0]
#define	I2C_BASE				0xF7600000				
#define	I2C_CLC					MMIO32(I2C_BASE + 0x00)	
#define	I2C_CLC_DISR			BIT(0)						 // Module Disable Request Bit				
#define	I2C_CLC_DISS			BIT(1)						 // Module Disable Status Bit				
#define	I2C_CLC_SPEN			BIT(2)						 // Module Suspend Enable Bit				
#define	I2C_CLC_EDIS			BIT(3)						 // Module External Request Disable			
#define	I2C_CLC_SBWE			BIT(4)						 // Module Suspend Bit Write Enable			
#define	I2C_CLC_FSOE			BIT(5)						 // Module Fast Shut-Off Enable.			
#define	I2C_CLC_RMC				GENMASK(8, 8)				 // Module Clock Divider for Normal Mode	
#define	I2C_CLC_RMC_SHIFT		8						

#define	I2C_ID					MMIO32(I2C_BASE + 0x08)	
#define	I2C_ID_REV				GENMASK(8, 0)				 // System Timer Module Revision Number.	
#define	I2C_ID_REV_SHIFT		0						
#define	I2C_ID_MOD_32B			GENMASK(8, 8)				 // Indicates a 32-bit ID register.			
#define	I2C_ID_MOD_32B_SHIFT	8						
#define	I2C_ID_MOD				GENMASK(16, 16)				 // System Timer Module Identification Number.
#define	I2C_ID_MOD_SHIFT		16						

/* Transmission Data Register */
#define	I2C_TXD					MMIO32(I2C_BASE + 0x8000)
#define	I2C_TXD_READ			BIT(0)																	
#define	I2C_TXD_ADDR			GENMASK(7, 1)															
#define	I2C_TXD_ADDR_SHIFT		1						
#define	I2C_TXD_REG				GENMASK(8, 8)															
#define	I2C_TXD_REG_SHIFT		8						
#define	I2C_TXD_VAL				GENMASK(8, 16)															
#define	I2C_TXD_VAL_SHIFT		16						

/* Reception Data Register */
#define	I2C_RXD					MMIO32(I2C_BASE + 0xC000)
#define	I2C_RXD_READ			BIT(0)																	
#define	I2C_RXD_ADDR			GENMASK(7, 1)															
#define	I2C_RXD_ADDR_SHIFT		1						
#define	I2C_RXD_REG				GENMASK(8, 8)															
#define	I2C_RXD_REG_SHIFT		8						
#define	I2C_RXD_VAL				GENMASK(8, 16)															
#define	I2C_RXD_VAL_SHIFT		16						


// MMICIF [MOD_NUM=F053, MOD_REV=00, MOD_32BIT=C0]
#define	MMICIF_BASE					0xF8000000				
#define	MMICIF_CLC					MMIO32(MMICIF_BASE + 0x00)
#define	MMICIF_CLC_DISR				BIT(0)						 // Module Disable Request Bit			
#define	MMICIF_CLC_DISS				BIT(1)						 // Module Disable Status Bit			
#define	MMICIF_CLC_SPEN				BIT(2)						 // Module Suspend Enable Bit			
#define	MMICIF_CLC_EDIS				BIT(3)						 // Module External Request Disable		
#define	MMICIF_CLC_SBWE				BIT(4)						 // Module Suspend Bit Write Enable		
#define	MMICIF_CLC_FSOE				BIT(5)						 // Module Fast Shut-Off Enable.		
#define	MMICIF_CLC_RMC				GENMASK(8, 8)				 // Module Clock Divider for Normal Mode
#define	MMICIF_CLC_RMC_SHIFT		8						

#define	MMICIF_ID					MMIO32(MMICIF_BASE + 0x08)
#define	MMICIF_ID_MOD_REV			GENMASK(8, 0)														
#define	MMICIF_ID_MOD_REV_SHIFT		0						
#define	MMICIF_ID_MOD_32B			GENMASK(8, 8)														
#define	MMICIF_ID_MOD_32B_SHIFT		8						
#define	MMICIF_ID_MOD_NUMBER		GENMASK(16, 16)														
#define	MMICIF_ID_MOD_NUMBER_SHIFT	16						


