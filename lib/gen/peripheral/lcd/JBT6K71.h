#pragma once

#include <bitops.h> // IWYU pragma: export

// JBT6K71
// Toshiba JBT6K71-AS(A) LCD controller
/* Internal oscillator startup and device-code access */
#define	JBT6K71_OSCILLATION							0x000
#define	JBT6K71_OSCILLATION_OSC						BIT(0)			 // Oscillator startup request

/* Source shift direction and driven LCD line count */
#define	JBT6K71_DRIVER_OUTPUT_CONTROL				0x001
#define	JBT6K71_DRIVER_OUTPUT_CONTROL_NL			GENMASK(5, 0)	 // Driven LCD line-count selection
#define	JBT6K71_DRIVER_OUTPUT_CONTROL_NL_SHIFT		0
#define	JBT6K71_DRIVER_OUTPUT_CONTROL_SS			BIT(8)			 // Source output shift direction

/* LCD AC-drive waveform selection */
#define	JBT6K71_LCD_DRIVING_SIGNAL					0x002
#define	JBT6K71_LCD_DRIVING_SIGNAL_B_C				BIT(9)			 // Frame or line AC-drive waveform

/* MPU format, color order, RAM write, and address update */
#define	JBT6K71_ENTRY_MODE							0x003
#define	JBT6K71_ENTRY_MODE_AM						BIT(3)			 // Address counter update axis
#define	JBT6K71_ENTRY_MODE_ID						GENMASK(5, 4)	 // Horizontal and vertical address directions
#define	JBT6K71_ENTRY_MODE_ID_SHIFT					4
#define	JBT6K71_ENTRY_MODE_HWM						BIT(8)			 // High-speed RAM write enable
#define	JBT6K71_ENTRY_MODE_IF18						BIT(11)			 // 16-bit MPU color-depth selection
#define	JBT6K71_ENTRY_MODE_BGR						BIT(12)			 // Source RGB component order
#define	JBT6K71_ENTRY_MODE_DFM						GENMASK(14, 13)	 // MPU display-data bit assignment
#define	JBT6K71_ENTRY_MODE_DFM_SHIFT				13
#define	JBT6K71_ENTRY_MODE_TRI						BIT(15)			 // MPU data-transfer rate selection

/* External-interface horizontal timing */
#define	JBT6K71_HORIZONTAL_VALID_WIDTH				0x006
#define	JBT6K71_HORIZONTAL_VALID_WIDTH_HBP			GENMASK(4, 0)	 // Horizontal back porch in DOTCLK cycles
#define	JBT6K71_HORIZONTAL_VALID_WIDTH_HBP_SHIFT	0
#define	JBT6K71_HORIZONTAL_VALID_WIDTH_HWS			GENMASK(15, 8)	 // Horizontal valid width in DOTCLK cycles
#define	JBT6K71_HORIZONTAL_VALID_WIDTH_HWS_SHIFT	8

/* Color, scroll, split-screen, partial-area, and reversal mode */
#define	JBT6K71_DISPLAY_MODE_1						0x007
#define	JBT6K71_DISPLAY_MODE_1_REV					BIT(2)			 // Display-data reversal enable
#define	JBT6K71_DISPLAY_MODE_1_NBW					BIT(4)			 // FR and REV display-polarity mapping
#define	JBT6K71_DISPLAY_MODE_1_PT					GENMASK(7, 6)	 // Partial-display inactive-source state
#define	JBT6K71_DISPLAY_MODE_1_PT_SHIFT				6
#define	JBT6K71_DISPLAY_MODE_1_SPT					BIT(8)			 // Split-screen drive enable
#define	JBT6K71_DISPLAY_MODE_1_VLE					GENMASK(10, 9)	 // Per-screen vertical-scroll enable
#define	JBT6K71_DISPLAY_MODE_1_VLE_SHIFT			9
#define	JBT6K71_DISPLAY_MODE_1_COL					GENMASK(15, 14)	 // Display color depth
#define	JBT6K71_DISPLAY_MODE_1_COL_SHIFT			14

/* Vertical front and back porch timing */
#define	JBT6K71_DISPLAY_MODE_2						0x008
#define	JBT6K71_DISPLAY_MODE_2_BP					GENMASK(3, 0)	 // Vertical back porch in lines
#define	JBT6K71_DISPLAY_MODE_2_BP_SHIFT				0
#define	JBT6K71_DISPLAY_MODE_2_FP					GENMASK(15, 8)	 // Vertical front porch in lines
#define	JBT6K71_DISPLAY_MODE_2_FP_SHIFT				8

/* Partial-display refresh interval control */
#define	JBT6K71_DISPLAY_MODE_3						0x009
#define	JBT6K71_DISPLAY_MODE_3_RSH					GENMASK(3, 0)	 // Base refresh-field count
#define	JBT6K71_DISPLAY_MODE_3_RSH_SHIFT			0
#define	JBT6K71_DISPLAY_MODE_3_RSB					GENMASK(5, 4)	 // Refresh-field count multiplier
#define	JBT6K71_DISPLAY_MODE_3_RSB_SHIFT			4
#define	JBT6K71_DISPLAY_MODE_3_RSE					BIT(7)			 // Partial-display refresh enable

/* RAM-write XY expansion and superimposition control */
#define	JBT6K71_DISPLAY_MODE_4						0x00B
#define	JBT6K71_DISPLAY_MODE_4_SIP					BIT(0)			 // Superimposition enable
#define	JBT6K71_DISPLAY_MODE_4_Y2					BIT(5)			 // Vertical doubling enable
#define	JBT6K71_DISPLAY_MODE_4_X2					BIT(6)			 // Horizontal doubling enable
#define	JBT6K71_DISPLAY_MODE_4_XYON					BIT(7)			 // XY expansion enable

/* RAM interface, display timing source, and RGB format */
#define	JBT6K71_EXTERNAL_DISPLAY_1					0x00C
#define	JBT6K71_EXTERNAL_DISPLAY_1_RIM				GENMASK(1, 0)	 // RGB interface width and transfer mode
#define	JBT6K71_EXTERNAL_DISPLAY_1_RIM_SHIFT		0
#define	JBT6K71_EXTERNAL_DISPLAY_1_DM				GENMASK(5, 4)	 // Display synchronization source
#define	JBT6K71_EXTERNAL_DISPLAY_1_DM_SHIFT			4
#define	JBT6K71_EXTERNAL_DISPLAY_1_RM				BIT(8)			 // RAM access interface selection

/* Internal-oscillator line timing */
#define	JBT6K71_FR_FREQUENCY_ADJUSTMENT				0x00D
#define	JBT6K71_FR_FREQUENCY_ADJUSTMENT_RTNI		GENMASK(4, 0)	 // Internal 1H clock-cycle count
#define	JBT6K71_FR_FREQUENCY_ADJUSTMENT_RTNI_SHIFT	0
#define	JBT6K71_FR_FREQUENCY_ADJUSTMENT_DIVI		GENMASK(9, 8)	 // Internal oscillator division ratio
#define	JBT6K71_FR_FREQUENCY_ADJUSTMENT_DIVI_SHIFT	8

/* External-DOTCLK line timing */
#define	JBT6K71_EXTERNAL_DISPLAY_2					0x00E
#define	JBT6K71_EXTERNAL_DISPLAY_2_RTNE				GENMASK(7, 0)	 // External 1H clock-cycle count
#define	JBT6K71_EXTERNAL_DISPLAY_2_RTNE_SHIFT		0
#define	JBT6K71_EXTERNAL_DISPLAY_2_DIVE				GENMASK(9, 8)	 // DOTCLK division ratio
#define	JBT6K71_EXTERNAL_DISPLAY_2_DIVE_SHIFT		8

/* External display-signal polarities */
#define	JBT6K71_EXTERNAL_DISPLAY_3					0x00F
#define	JBT6K71_EXTERNAL_DISPLAY_3_DPL				BIT(0)			 // DOTCLK sampling edge
#define	JBT6K71_EXTERNAL_DISPLAY_3_EPL				BIT(1)			 // ENABLE active polarity
#define	JBT6K71_EXTERNAL_DISPLAY_3_VPL				BIT(2)			 // VLD active polarity
#define	JBT6K71_EXTERNAL_DISPLAY_3_HSPL				BIT(3)			 // HSYNC active polarity
#define	JBT6K71_EXTERNAL_DISPLAY_3_VSPL				BIT(4)			 // VSYNC active polarity

/* Internal-clock ASW pulse timing */
#define	JBT6K71_LTPS_CONTROL_1						0x012
#define	JBT6K71_LTPS_CONTROL_1_CLTI					GENMASK(1, 0)	 // ASW1 rising-edge position
#define	JBT6K71_LTPS_CONTROL_1_CLTI_SHIFT			0
#define	JBT6K71_LTPS_CONTROL_1_CLWI					GENMASK(11, 8)	 // ASW1/2/3 high-pulse width
#define	JBT6K71_LTPS_CONTROL_1_CLWI_SHIFT			8

/* Internal-clock OEV pulse timing */
#define	JBT6K71_LTPS_CONTROL_2						0x013
#define	JBT6K71_LTPS_CONTROL_2_OEVFI				GENMASK(1, 0)	 // OEV falling-edge position
#define	JBT6K71_LTPS_CONTROL_2_OEVFI_SHIFT			0
#define	JBT6K71_LTPS_CONTROL_2_OEVBI				GENMASK(9, 8)	 // OEV rising-edge position
#define	JBT6K71_LTPS_CONTROL_2_OEVBI_SHIFT			8

/* Internal-clock ASW hold timing */
#define	JBT6K71_LTPS_CONTROL_3						0x014
#define	JBT6K71_LTPS_CONTROL_3_SHI					GENMASK(1, 0)	 // ASW hold time after falling edge
#define	JBT6K71_LTPS_CONTROL_3_SHI_SHIFT			0

/* Internal-clock CKV1/2 edge timing */
#define	JBT6K71_LTPS_CONTROL_4						0x015
#define	JBT6K71_LTPS_CONTROL_4_CKFI					GENMASK(9, 8)	 // CKV1/2 leading-edge position
#define	JBT6K71_LTPS_CONTROL_4_CKFI_SHIFT			8
#define	JBT6K71_LTPS_CONTROL_4_CKBI					GENMASK(13, 12)	 // CKV1/2 trailing-edge position
#define	JBT6K71_LTPS_CONTROL_4_CKBI_SHIFT			12

/* External-clock ASW pulse timing */
#define	JBT6K71_LTPS_CONTROL_5						0x018
#define	JBT6K71_LTPS_CONTROL_5_CLTE					GENMASK(3, 0)	 // ASW1 rising-edge position
#define	JBT6K71_LTPS_CONTROL_5_CLTE_SHIFT			0
#define	JBT6K71_LTPS_CONTROL_5_CLWE					GENMASK(13, 8)	 // ASW1/2/3 high-pulse width
#define	JBT6K71_LTPS_CONTROL_5_CLWE_SHIFT			8

/* External-clock OEV pulse timing */
#define	JBT6K71_LTPS_CONTROL_6						0x019
#define	JBT6K71_LTPS_CONTROL_6_OEVFE				GENMASK(3, 0)	 // OEV falling-edge position
#define	JBT6K71_LTPS_CONTROL_6_OEVFE_SHIFT			0
#define	JBT6K71_LTPS_CONTROL_6_OEVBE				GENMASK(11, 8)	 // OEV rising-edge position
#define	JBT6K71_LTPS_CONTROL_6_OEVBE_SHIFT			8

/* External-clock ASW hold timing */
#define	JBT6K71_LTPS_CONTROL_7						0x01A
#define	JBT6K71_LTPS_CONTROL_7_SHE					GENMASK(3, 0)	 // ASW hold time after falling edge
#define	JBT6K71_LTPS_CONTROL_7_SHE_SHIFT			0

/* External-clock CKV1/2 edge timing */
#define	JBT6K71_LTPS_CONTROL_8						0x01B
#define	JBT6K71_LTPS_CONTROL_8_CKFE					GENMASK(11, 8)	 // CKV1/2 leading-edge position
#define	JBT6K71_LTPS_CONTROL_8_CKFE_SHIFT			8
#define	JBT6K71_LTPS_CONTROL_8_CKBE					GENMASK(15, 12)	 // CKV1/2 trailing-edge position
#define	JBT6K71_LTPS_CONTROL_8_CKBE_SHIFT			12

/* On-chip operational-amplifier drive control */
#define	JBT6K71_AMPLIFIER_CAPABILITY				0x01C
#define	JBT6K71_AMPLIFIER_CAPABILITY_ABSW			GENMASK(1, 0)	 // Operational-amplifier current capability
#define	JBT6K71_AMPLIFIER_CAPABILITY_ABSW_SHIFT		0

/* Standby and deep-standby control */
#define	JBT6K71_MODE								0x01D
#define	JBT6K71_MODE_STB							BIT(0)			 // Standby release control
#define	JBT6K71_MODE_DSTB							BIT(2)			 // Deep-standby control

/* VCS power-off transition timing */
#define	JBT6K71_POWER_OFF_LINE_COUNT				0x01E
#define	JBT6K71_POWER_OFF_LINE_COUNT_POFH			GENMASK(3, 0)	 // Lines before VCS high-to-low transition
#define	JBT6K71_POWER_OFF_LINE_COUNT_POFH_SHIFT		0

/* Power sequence, LTPS signals, and display-output control */
#define	JBT6K71_DISPLAY_CONTROL						0x100
#define	JBT6K71_DISPLAY_CONTROL_DCG					BIT(0)			 // DCG output enable
#define	JBT6K71_DISPLAY_CONTROL_VGAM				BIT(1)			 // Gamma power-supply enable
#define	JBT6K71_DISPLAY_CONTROL_FDON				BIT(2)			 // FDON output enable
#define	JBT6K71_DISPLAY_CONTROL_FR					BIT(3)			 // FR output enable
#define	JBT6K71_DISPLAY_CONTROL_D					GENMASK(5, 4)	 // Source output state
#define	JBT6K71_DISPLAY_CONTROL_D_SHIFT				4
#define	JBT6K71_DISPLAY_CONTROL_ASW					GENMASK(7, 6)	 // ASW output state
#define	JBT6K71_DISPLAY_CONTROL_ASW_SHIFT			6
#define	JBT6K71_DISPLAY_CONTROL_VCS					BIT(8)			 // VCS and VCOMD output enable
#define	JBT6K71_DISPLAY_CONTROL_OEV					BIT(9)			 // OEV output enable
#define	JBT6K71_DISPLAY_CONTROL_CON					BIT(10)			 // STV and CKV1/2 output enable
#define	JBT6K71_DISPLAY_CONTROL_UD					BIT(11)			 // Gate scan direction and U/D output level
#define	JBT6K71_DISPLAY_CONTROL_DCEV				BIT(12)			 // DCCLK, inverted DCCLK, and DCEV output enable
#define	JBT6K71_DISPLAY_CONTROL_PEV					BIT(13)			 // LCD voltage-booster and PEV output enable
#define	JBT6K71_DISPLAY_CONTROL_CONT				BIT(14)			 // Internal display-signal generation enable
#define	JBT6K71_DISPLAY_CONTROL_PO					BIT(15)			 // Sleep-mode control

/* Automatic power-to-display sequence control */
#define	JBT6K71_AUTO_MANAGEMENT_CONTROL				0x101
#define	JBT6K71_AUTO_MANAGEMENT_CONTROL_AUTO		BIT(0)			 // Automatic sequence control flag

/* VCOMD, VCS, and VGM output-voltage control */
#define	JBT6K71_POWER_SUPPLY_CONTROL_1				0x102
#define	JBT6K71_POWER_SUPPLY_CONTROL_1_VGM			GENMASK(3, 0)	 // VGM output voltage
#define	JBT6K71_POWER_SUPPLY_CONTROL_1_VGM_SHIFT	0
#define	JBT6K71_POWER_SUPPLY_CONTROL_1_VCS			GENMASK(7, 4)	 // VCS output voltage
#define	JBT6K71_POWER_SUPPLY_CONTROL_1_VCS_SHIFT	4
#define	JBT6K71_POWER_SUPPLY_CONTROL_1_VCD			GENMASK(11, 8)	 // VCOMD output voltage
#define	JBT6K71_POWER_SUPPLY_CONTROL_1_VCD_SHIFT	8

/* Booster clock modes and XVDD voltage control */
#define	JBT6K71_POWER_SUPPLY_CONTROL_2				0x103
#define	JBT6K71_POWER_SUPPLY_CONTROL_2_XVD			GENMASK(2, 0)	 // XVDD output voltage
#define	JBT6K71_POWER_SUPPLY_CONTROL_2_XVD_SHIFT	0
#define	JBT6K71_POWER_SUPPLY_CONTROL_2_WSEL1		BIT(12)			 // AVDD booster clock mode
#define	JBT6K71_POWER_SUPPLY_CONTROL_2_WSEL2		BIT(13)			 // XVDD booster clock mode

/* AVDD and XVDD boost-ratio control */
#define	JBT6K71_POWER_SUPPLY_CONTROL_3				0x104
#define	JBT6K71_POWER_SUPPLY_CONTROL_3_BAV			BIT(0)			 // AVDD booster multiplication ratio
#define	JBT6K71_POWER_SUPPLY_CONTROL_3_BXV			BIT(1)			 // XVDD booster multiplication ratio

/* Regulator clocks and DCCLK mask-period control */
#define	JBT6K71_POWER_SUPPLY_CONTROL_4				0x105
#define	JBT6K71_POWER_SUPPLY_CONTROL_4_CK1			GENMASK(1, 0)	 // AVDD-regulator clock division
#define	JBT6K71_POWER_SUPPLY_CONTROL_4_CK1_SHIFT	0
#define	JBT6K71_POWER_SUPPLY_CONTROL_4_CK2			GENMASK(3, 2)	 // XVDD-regulator clock division
#define	JBT6K71_POWER_SUPPLY_CONTROL_4_CK2_SHIFT	2
#define	JBT6K71_POWER_SUPPLY_CONTROL_4_CK3			GENMASK(5, 4)	 // External-regulator clock division
#define	JBT6K71_POWER_SUPPLY_CONTROL_4_CK3_SHIFT	4
#define	JBT6K71_POWER_SUPPLY_CONTROL_4_DCW			GENMASK(9, 8)	 // DCCLK mask period
#define	JBT6K71_POWER_SUPPLY_CONTROL_4_DCW_SHIFT	8

/* EXTOUT1/2 level-shifter polarity control */
#define	JBT6K71_EXTERNAL_POLARITY					0x108
#define	JBT6K71_EXTERNAL_POLARITY_EXTC1				BIT(0)			 // EXTOUT1 polarity inversion
#define	JBT6K71_EXTERNAL_POLARITY_EXTC2				BIT(1)			 // EXTOUT2 polarity inversion

/* Display and OSD RAM address low byte */
#define	JBT6K71_RAM_ADDRESS_LOW						0x200
#define	JBT6K71_RAM_ADDRESS_LOW_AD_07_00			GENMASK(7, 0)	 // RAM address bits 7 to 0
#define	JBT6K71_RAM_ADDRESS_LOW_AD_07_00_SHIFT		0

/* Display and OSD RAM address high bits */
#define	JBT6K71_RAM_ADDRESS_HIGH					0x201
#define	JBT6K71_RAM_ADDRESS_HIGH_AD_16_08			GENMASK(8, 0)	 // RAM address bits 16 to 8
#define	JBT6K71_RAM_ADDRESS_HIGH_AD_16_08_SHIFT		0

/* Interface-formatted display RAM pixel write */
#define	JBT6K71_GRAM_DATA							0x202
#define	JBT6K71_GRAM_DATA_DATA						GENMASK(17, 0)	 // Display pixel data for the selected interface
#define	JBT6K71_GRAM_DATA_DATA_SHIFT				0

/* Display RAM write-mask bits 0 to 11 */
#define	JBT6K71_GRAPHIC_OPERATION_1					0x203
#define	JBT6K71_GRAPHIC_OPERATION_1_WM_05_00		GENMASK(5, 0)	 // Pixel write-mask bits 5 to 0
#define	JBT6K71_GRAPHIC_OPERATION_1_WM_05_00_SHIFT	0
#define	JBT6K71_GRAPHIC_OPERATION_1_WM_11_06		GENMASK(13, 8)	 // Pixel write-mask bits 11 to 6
#define	JBT6K71_GRAPHIC_OPERATION_1_WM_11_06_SHIFT	8

/* Display RAM write-mask bits 12 to 17 */
#define	JBT6K71_GRAPHIC_OPERATION_2					0x204
#define	JBT6K71_GRAPHIC_OPERATION_2_WM_17_12		GENMASK(5, 0)	 // Pixel write-mask bits 17 to 12
#define	JBT6K71_GRAPHIC_OPERATION_2_WM_17_12_SHIFT	0

/* Gamma fine adjustments 0 and 1 */
#define	JBT6K71_GRAY_SCALE_1						0x300
#define	JBT6K71_GRAY_SCALE_1_PK0					GENMASK(2, 0)	 // Gamma fine adjustment 0
#define	JBT6K71_GRAY_SCALE_1_PK0_SHIFT				0
#define	JBT6K71_GRAY_SCALE_1_PK1					GENMASK(10, 8)	 // Gamma fine adjustment 1
#define	JBT6K71_GRAY_SCALE_1_PK1_SHIFT				8

/* Gamma fine adjustments 2 and 3 */
#define	JBT6K71_GRAY_SCALE_2						0x301
#define	JBT6K71_GRAY_SCALE_2_PK2					GENMASK(2, 0)	 // Gamma fine adjustment 2
#define	JBT6K71_GRAY_SCALE_2_PK2_SHIFT				0
#define	JBT6K71_GRAY_SCALE_2_PK3					GENMASK(10, 8)	 // Gamma fine adjustment 3
#define	JBT6K71_GRAY_SCALE_2_PK3_SHIFT				8

/* Gamma fine adjustments 4 and 5 */
#define	JBT6K71_GRAY_SCALE_3						0x302
#define	JBT6K71_GRAY_SCALE_3_PK4					GENMASK(2, 0)	 // Gamma fine adjustment 4
#define	JBT6K71_GRAY_SCALE_3_PK4_SHIFT				0
#define	JBT6K71_GRAY_SCALE_3_PK5					GENMASK(10, 8)	 // Gamma fine adjustment 5
#define	JBT6K71_GRAY_SCALE_3_PK5_SHIFT				8

/* Gamma inclination adjustments 0 and 1 */
#define	JBT6K71_GRAY_SCALE_4						0x303
#define	JBT6K71_GRAY_SCALE_4_PR0					GENMASK(2, 0)	 // Gamma inclination adjustment 0
#define	JBT6K71_GRAY_SCALE_4_PR0_SHIFT				0
#define	JBT6K71_GRAY_SCALE_4_PR1					GENMASK(10, 8)	 // Gamma inclination adjustment 1
#define	JBT6K71_GRAY_SCALE_4_PR1_SHIFT				8

/* Gamma amplitude adjustments 0 and 1 */
#define	JBT6K71_GRAY_SCALE_5						0x304
#define	JBT6K71_GRAY_SCALE_5_VR0					GENMASK(2, 0)	 // Gamma amplitude adjustment 0
#define	JBT6K71_GRAY_SCALE_5_VR0_SHIFT				0
#define	JBT6K71_GRAY_SCALE_5_VR1					GENMASK(10, 8)	 // Gamma amplitude adjustment 1
#define	JBT6K71_GRAY_SCALE_5_VR1_SHIFT				8

/* Blue-channel gamma offset control */
#define	JBT6K71_BLUE_OFFSET							0x305
#define	JBT6K71_BLUE_OFFSET_BOFS					GENMASK(2, 0)	 // Blue gamma-curve voltage offset
#define	JBT6K71_BLUE_OFFSET_BOFS_SHIFT				0
#define	JBT6K71_BLUE_OFFSET_BUP						GENMASK(6, 4)	 // Blue extreme-level voltage adjustment
#define	JBT6K71_BLUE_OFFSET_BUP_SHIFT				4
#define	JBT6K71_BLUE_OFFSET_BLON					BIT(8)			 // Blue gamma-offset enable

/* First-screen vertical scroll offset */
#define	JBT6K71_VERTICAL_SCROLL_1					0x400
#define	JBT6K71_VERTICAL_SCROLL_1_VL1				GENMASK(8, 0)	 // First-screen scroll amount in lines
#define	JBT6K71_VERTICAL_SCROLL_1_VL1_SHIFT			0

/* Second-screen vertical scroll offset */
#define	JBT6K71_VERTICAL_SCROLL_2					0x401
#define	JBT6K71_VERTICAL_SCROLL_2_VL2				GENMASK(8, 0)	 // Second-screen scroll amount in lines
#define	JBT6K71_VERTICAL_SCROLL_2_VL2_SHIFT			0

/* First-screen gate-drive start position */
#define	JBT6K71_SCREEN1_DRIVE_START					0x402
#define	JBT6K71_SCREEN1_DRIVE_START_SS1				GENMASK(8, 0)	 // First-screen start line minus one
#define	JBT6K71_SCREEN1_DRIVE_START_SS1_SHIFT		0

/* First-screen gate-drive end position */
#define	JBT6K71_SCREEN1_DRIVE_END					0x403
#define	JBT6K71_SCREEN1_DRIVE_END_SE1				GENMASK(8, 0)	 // First-screen end line minus one
#define	JBT6K71_SCREEN1_DRIVE_END_SE1_SHIFT			0

/* Second-screen gate-drive start position */
#define	JBT6K71_SCREEN2_DRIVE_START					0x404
#define	JBT6K71_SCREEN2_DRIVE_START_SS2				GENMASK(8, 0)	 // Second-screen start line minus one
#define	JBT6K71_SCREEN2_DRIVE_START_SS2_SHIFT		0

/* Second-screen gate-drive end position */
#define	JBT6K71_SCREEN2_DRIVE_END					0x405
#define	JBT6K71_SCREEN2_DRIVE_END_SE2				GENMASK(8, 0)	 // Second-screen end line minus one
#define	JBT6K71_SCREEN2_DRIVE_END_SE2_SHIFT			0

/* Horizontal RAM window start */
#define	JBT6K71_HORIZONTAL_RAM_START				0x406
#define	JBT6K71_HORIZONTAL_RAM_START_HSA			GENMASK(7, 0)	 // Horizontal window start address
#define	JBT6K71_HORIZONTAL_RAM_START_HSA_SHIFT		0

/* Horizontal RAM window end */
#define	JBT6K71_HORIZONTAL_RAM_END					0x407
#define	JBT6K71_HORIZONTAL_RAM_END_HEA				GENMASK(7, 0)	 // Horizontal window end address
#define	JBT6K71_HORIZONTAL_RAM_END_HEA_SHIFT		0

/* Vertical RAM window start */
#define	JBT6K71_VERTICAL_RAM_START					0x408
#define	JBT6K71_VERTICAL_RAM_START_VSA				GENMASK(8, 0)	 // Vertical window start address
#define	JBT6K71_VERTICAL_RAM_START_VSA_SHIFT		0

/* Vertical RAM window end */
#define	JBT6K71_VERTICAL_RAM_END					0x409
#define	JBT6K71_VERTICAL_RAM_END_VEA				GENMASK(8, 0)	 // Vertical window end address
#define	JBT6K71_VERTICAL_RAM_END_VEA_SHIFT			0

/* OSD enable and RAM addressing control */
#define	JBT6K71_OSD_CONTROL							0x500
#define	JBT6K71_OSD_CONTROL_OSDON					BIT(0)			 // OSD feature enable
#define	JBT6K71_OSD_CONTROL_OSDW					BIT(8)			 // OSD RAM addressing mode

/* First OSD screen start position */
#define	JBT6K71_OSD_SCREEN1_START					0x504
#define	JBT6K71_OSD_SCREEN1_START_ST1				GENMASK(8, 0)	 // First OSD screen start line
#define	JBT6K71_OSD_SCREEN1_START_ST1_SHIFT			0

/* Second OSD screen start position */
#define	JBT6K71_OSD_SCREEN2_START					0x505
#define	JBT6K71_OSD_SCREEN2_START_ST2				GENMASK(8, 0)	 // Second OSD screen start line
#define	JBT6K71_OSD_SCREEN2_START_ST2_SHIFT			0
