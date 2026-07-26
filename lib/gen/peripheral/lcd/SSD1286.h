#pragma once

#include <bitops.h> // IWYU pragma: export

// SSD1286
// Solomon Systech SSD1286 TFT LCD controller
/* Oscillator control and device code read */
#define	SSD1286_OSCILLATION							0x00
#define	SSD1286_OSCILLATION_OSCEN					BIT(0)			 // Internal oscillator enable

/* LCD source and gate output configuration */
#define	SSD1286_DRIVER_OUTPUT_CONTROL				0x01
#define	SSD1286_DRIVER_OUTPUT_CONTROL_MUX			GENMASK(7, 0)	 // LCD drive line count minus one
#define	SSD1286_DRIVER_OUTPUT_CONTROL_MUX_SHIFT		0
#define	SSD1286_DRIVER_OUTPUT_CONTROL_RL			BIT(8)			 // Source output shift direction
#define	SSD1286_DRIVER_OUTPUT_CONTROL_TB			BIT(9)			 // Gate output shift direction
#define	SSD1286_DRIVER_OUTPUT_CONTROL_SM			BIT(10)			 // Gate scan sequence
#define	SSD1286_DRIVER_OUTPUT_CONTROL_BGR			BIT(11)			 // Source RGB component order
#define	SSD1286_DRIVER_OUTPUT_CONTROL_CAD			BIT(12)			 // TFT storage capacitor configuration
#define	SSD1286_DRIVER_OUTPUT_CONTROL_REV			BIT(13)			 // Grayscale polarity reversal

/* LCD inversion and write synchronization control */
#define	SSD1286_LCD_DRIVE_AC_CONTROL				0x02
#define	SSD1286_LCD_DRIVE_AC_CONTROL_NW				GENMASK(6, 0)	 // N-line inversion interval minus one
#define	SSD1286_LCD_DRIVE_AC_CONTROL_NW_SHIFT		0
#define	SSD1286_LCD_DRIVE_AC_CONTROL_WSMD			BIT(7)			 // WSYNC output mode
#define	SSD1286_LCD_DRIVE_AC_CONTROL_EOR			BIT(8)			 // Frame and N-line inversion XOR enable
#define	SSD1286_LCD_DRIVE_AC_CONTROL_B_C			BIT(9)			 // Frame or N-line inversion selection
#define	SSD1286_LCD_DRIVE_AC_CONTROL_ENWS			BIT(10)			 // WSYNC output enable
#define	SSD1286_LCD_DRIVE_AC_CONTROL_FLD			BIT(11)			 // Three-field interlace enable

/* Display source, color format, and GRAM access mode */
#define	SSD1286_ENTRY_MODE							0x03
#define	SSD1286_ENTRY_MODE_LG						GENMASK(2, 0)	 // GRAM compare operation
#define	SSD1286_ENTRY_MODE_LG_SHIFT					0
#define	SSD1286_ENTRY_MODE_AM						BIT(3)			 // Address counter update axis
#define	SSD1286_ENTRY_MODE_ID						GENMASK(5, 4)	 // Horizontal and vertical address directions
#define	SSD1286_ENTRY_MODE_ID_SHIFT					4
#define	SSD1286_ENTRY_MODE_TY						GENMASK(7, 6)	 // 262k-color 16-bit write format
#define	SSD1286_ENTRY_MODE_TY_SHIFT					6
#define	SSD1286_ENTRY_MODE_DMODE					GENMASK(9, 8)	 // Display data source
#define	SSD1286_ENTRY_MODE_DMODE_SHIFT				8
#define	SSD1286_ENTRY_MODE_WMODE					BIT(10)			 // GRAM write data source
#define	SSD1286_ENTRY_MODE_OEDEF					BIT(11)			 // OE-defined display window enable
#define	SSD1286_ENTRY_MODE_TRANS					BIT(12)			 // Generic-input transparency enable
#define	SSD1286_ENTRY_MODE_DFM						GENMASK(14, 13)	 // Display color depth
#define	SSD1286_ENTRY_MODE_DFM_SHIFT				13
#define	SSD1286_ENTRY_MODE_VSMODE					BIT(15)			 // VSYNC-controlled frame frequency enable

/* Red and green GRAM compare values */
#define	SSD1286_COMPARE_1							0x04
#define	SSD1286_COMPARE_1_CPG						GENMASK(7, 2)	 // Green compare value
#define	SSD1286_COMPARE_1_CPG_SHIFT					2
#define	SSD1286_COMPARE_1_CPR						GENMASK(15, 10)	 // Red compare value
#define	SSD1286_COMPARE_1_CPR_SHIFT					10

/* Blue GRAM compare value */
#define	SSD1286_COMPARE_2							0x05
#define	SSD1286_COMPARE_2_CPB						GENMASK(7, 2)	 // Blue compare value
#define	SSD1286_COMPARE_2_CPB_SHIFT					2

/* Display, scrolling, split-screen, and gate control */
#define	SSD1286_DISPLAY_CONTROL						0x07
#define	SSD1286_DISPLAY_CONTROL_D					GENMASK(1, 0)	 // Display operating state
#define	SSD1286_DISPLAY_CONTROL_D_SHIFT				0
#define	SSD1286_DISPLAY_CONTROL_CM					BIT(3)			 // Eight-color display mode enable
#define	SSD1286_DISPLAY_CONTROL_DTE					BIT(4)			 // Display timing enable
#define	SSD1286_DISPLAY_CONTROL_GON					BIT(5)			 // Gate-off output level
#define	SSD1286_DISPLAY_CONTROL_SPT					BIT(8)			 // Second-screen drive enable
#define	SSD1286_DISPLAY_CONTROL_VLE					GENMASK(10, 9)	 // Per-screen vertical scroll enable
#define	SSD1286_DISPLAY_CONTROL_VLE_SHIFT			9
#define	SSD1286_DISPLAY_CONTROL_PT					GENMASK(12, 11)	 // Partial-display inactive-source level
#define	SSD1286_DISPLAY_CONTROL_PT_SHIFT			11

/* LCD line timing and internal clock control */
#define	SSD1286_FRAME_CYCLE_CONTROL					0x0B
#define	SSD1286_FRAME_CYCLE_CONTROL_RTN				GENMASK(3, 0)	 // Line clock count minus sixteen
#define	SSD1286_FRAME_CYCLE_CONTROL_RTN_SHIFT		0
#define	SSD1286_FRAME_CYCLE_CONTROL_SRTN			BIT(4)			 // Manual line-clock count enable
#define	SSD1286_FRAME_CYCLE_CONTROL_SDIV			BIT(5)			 // Manual clock divider enable
#define	SSD1286_FRAME_CYCLE_CONTROL_DIV				GENMASK(9, 8)	 // Internal clock division ratio
#define	SSD1286_FRAME_CYCLE_CONTROL_DIV_SHIFT		8
#define	SSD1286_FRAME_CYCLE_CONTROL_EQ				GENMASK(11, 10)	 // Equalizing period
#define	SSD1286_FRAME_CYCLE_CONTROL_EQ_SHIFT		10
#define	SSD1286_FRAME_CYCLE_CONTROL_SDT				GENMASK(13, 12)	 // Gate-to-source output delay
#define	SSD1286_FRAME_CYCLE_CONTROL_SDT_SHIFT		12
#define	SSD1286_FRAME_CYCLE_CONTROL_NO				GENMASK(15, 14)	 // Gate-output non-overlap period
#define	SSD1286_FRAME_CYCLE_CONTROL_NO_SHIFT		14

/* Step-up circuits, op-amp power, and sleep control */
#define	SSD1286_POWER_CONTROL_1						0x10
#define	SSD1286_POWER_CONTROL_1_SLP					BIT(0)			 // Sleep mode enable
#define	SSD1286_POWER_CONTROL_1_AP					GENMASK(3, 1)	 // Operational-amplifier drive current
#define	SSD1286_POWER_CONTROL_1_AP_SHIFT			1
#define	SSD1286_POWER_CONTROL_1_DC					GENMASK(5, 4)	 // VCIX2 step-up cycle
#define	SSD1286_POWER_CONTROL_1_DC_SHIFT			4
#define	SSD1286_POWER_CONTROL_1_BTL					GENMASK(8, 6)	 // VgoffL step-up output level
#define	SSD1286_POWER_CONTROL_1_BTL_SHIFT			6
#define	SSD1286_POWER_CONTROL_1_BTH					GENMASK(11, 9)	 // VGH step-up output level
#define	SSD1286_POWER_CONTROL_1_BTH_SHIFT			9
#define	SSD1286_POWER_CONTROL_1_DCY					GENMASK(14, 12)	 // High-voltage step-up cycle
#define	SSD1286_POWER_CONTROL_1_DCY_SHIFT			12

/* VGH multiplier and VCIX2 voltage control */
#define	SSD1286_POWER_CONTROL_2						0x11
#define	SSD1286_POWER_CONTROL_2_VRC					GENMASK(2, 0)	 // VCIX2 output voltage
#define	SSD1286_POWER_CONTROL_2_VRC_SHIFT			0
#define	SSD1286_POWER_CONTROL_2_PU					GENMASK(4, 3)	 // VGH-to-VCI multiplication ratio
#define	SSD1286_POWER_CONTROL_2_PU_SHIFT			3

/* VLCD63 voltage control */
#define	SSD1286_POWER_CONTROL_3						0x12
#define	SSD1286_POWER_CONTROL_3_VRH					GENMASK(3, 0)	 // VLCD63 voltage amplification
#define	SSD1286_POWER_CONTROL_3_VRH_SHIFT			0

/* VCOM low-level generator control */
#define	SSD1286_POWER_CONTROL_4						0x13
#define	SSD1286_POWER_CONTROL_4_VDV					GENMASK(12, 8)	 // VCOM alternating amplitude
#define	SSD1286_POWER_CONTROL_4_VDV_SHIFT			8
#define	SSD1286_POWER_CONTROL_4_VCOMG				BIT(13)			 // VCOM low-level generator enable

/* External-interface line width and back porch */
#define	SSD1286_HORIZONTAL_PORCH					0x16
#define	SSD1286_HORIZONTAL_PORCH_HBP				GENMASK(5, 0)	 // Horizontal back porch clocks minus one
#define	SSD1286_HORIZONTAL_PORCH_HBP_SHIFT			0
#define	SSD1286_HORIZONTAL_PORCH_XL					GENMASK(15, 8)	 // Valid pixels per line
#define	SSD1286_HORIZONTAL_PORCH_XL_SHIFT			8

/* External-interface vertical front and back porches */
#define	SSD1286_VERTICAL_PORCH						0x17
#define	SSD1286_VERTICAL_PORCH_VBP					GENMASK(6, 0)	 // Vertical back porch lines minus one
#define	SSD1286_VERTICAL_PORCH_VBP_SHIFT			0
#define	SSD1286_VERTICAL_PORCH_VFP					GENMASK(14, 8)	 // Vertical front porch lines minus one
#define	SSD1286_VERTICAL_PORCH_VFP_SHIFT			8

/* VCOM high-level source and voltage control */
#define	SSD1286_POWER_CONTROL_5						0x1E
#define	SSD1286_POWER_CONTROL_5_VCM					GENMASK(5, 0)	 // VCOM high-level voltage ratio
#define	SSD1286_POWER_CONTROL_5_VCM_SHIFT			0
#define	SSD1286_POWER_CONTROL_5_NOTP				BIT(7)			 // Use software VCOM high-level setting

/* GRAM address counter initialization */
#define	SSD1286_RAM_ADDRESS							0x21
#define	SSD1286_RAM_ADDRESS_AD						GENMASK(15, 0)	 // Initial GRAM address
#define	SSD1286_RAM_ADDRESS_AD_SHIFT				0

/* GRAM pixel data access */
#define	SSD1286_GRAM_DATA							0x22
#define	SSD1286_GRAM_DATA_DATA						GENMASK(17, 0)	 // Interface-mapped 18-bit pixel data
#define	SSD1286_GRAM_DATA_DATA_SHIFT				0

/* Red and green GRAM write masks */
#define	SSD1286_RAM_WRITE_MASK_1					0x23
#define	SSD1286_RAM_WRITE_MASK_1_WMG				GENMASK(7, 2)	 // Green channel write mask
#define	SSD1286_RAM_WRITE_MASK_1_WMG_SHIFT			2
#define	SSD1286_RAM_WRITE_MASK_1_WMR				GENMASK(15, 10)	 // Red channel write mask
#define	SSD1286_RAM_WRITE_MASK_1_WMR_SHIFT			10

/* Blue GRAM write mask */
#define	SSD1286_RAM_WRITE_MASK_2					0x24
#define	SSD1286_RAM_WRITE_MASK_2_WMB				GENMASK(7, 2)	 // Blue channel write mask
#define	SSD1286_RAM_WRITE_MASK_2_WMB_SHIFT			2

/* VCOM OTP command register 1 */
#define	SSD1286_VCOM_OTP_1							0x28
#define	SSD1286_VCOM_OTP_1_VALUE					GENMASK(15, 0)	 // VCOM OTP command value
#define	SSD1286_VCOM_OTP_1_VALUE_SHIFT				0

/* VCOM OTP command register 2 */
#define	SSD1286_VCOM_OTP_2							0x29
#define	SSD1286_VCOM_OTP_2_VALUE					GENMASK(15, 0)	 // VCOM OTP command value
#define	SSD1286_VCOM_OTP_2_VALUE_SHIFT				0

/* Positive gamma micro-adjustments 0 and 1 */
#define	SSD1286_GAMMA_1								0x30
#define	SSD1286_GAMMA_1_PKP0						GENMASK(2, 0)	 // Positive gamma micro-adjustment 0
#define	SSD1286_GAMMA_1_PKP0_SHIFT					0
#define	SSD1286_GAMMA_1_PKP1						GENMASK(10, 8)	 // Positive gamma micro-adjustment 1
#define	SSD1286_GAMMA_1_PKP1_SHIFT					8

/* Positive gamma micro-adjustments 2 and 3 */
#define	SSD1286_GAMMA_2								0x31
#define	SSD1286_GAMMA_2_PKP2						GENMASK(2, 0)	 // Positive gamma micro-adjustment 2
#define	SSD1286_GAMMA_2_PKP2_SHIFT					0
#define	SSD1286_GAMMA_2_PKP3						GENMASK(10, 8)	 // Positive gamma micro-adjustment 3
#define	SSD1286_GAMMA_2_PKP3_SHIFT					8

/* Positive gamma micro-adjustments 4 and 5 */
#define	SSD1286_GAMMA_3								0x32
#define	SSD1286_GAMMA_3_PKP4						GENMASK(2, 0)	 // Positive gamma micro-adjustment 4
#define	SSD1286_GAMMA_3_PKP4_SHIFT					0
#define	SSD1286_GAMMA_3_PKP5						GENMASK(10, 8)	 // Positive gamma micro-adjustment 5
#define	SSD1286_GAMMA_3_PKP5_SHIFT					8

/* Positive gamma gradient adjustments 0 and 1 */
#define	SSD1286_GAMMA_4								0x33
#define	SSD1286_GAMMA_4_PRP0						GENMASK(2, 0)	 // Positive gamma gradient adjustment 0
#define	SSD1286_GAMMA_4_PRP0_SHIFT					0
#define	SSD1286_GAMMA_4_PRP1						GENMASK(10, 8)	 // Positive gamma gradient adjustment 1
#define	SSD1286_GAMMA_4_PRP1_SHIFT					8

/* Negative gamma micro-adjustments 0 and 1 */
#define	SSD1286_GAMMA_5								0x34
#define	SSD1286_GAMMA_5_PKN0						GENMASK(2, 0)	 // Negative gamma micro-adjustment 0
#define	SSD1286_GAMMA_5_PKN0_SHIFT					0
#define	SSD1286_GAMMA_5_PKN1						GENMASK(10, 8)	 // Negative gamma micro-adjustment 1
#define	SSD1286_GAMMA_5_PKN1_SHIFT					8

/* Negative gamma micro-adjustments 2 and 3 */
#define	SSD1286_GAMMA_6								0x35
#define	SSD1286_GAMMA_6_PKN2						GENMASK(2, 0)	 // Negative gamma micro-adjustment 2
#define	SSD1286_GAMMA_6_PKN2_SHIFT					0
#define	SSD1286_GAMMA_6_PKN3						GENMASK(10, 8)	 // Negative gamma micro-adjustment 3
#define	SSD1286_GAMMA_6_PKN3_SHIFT					8

/* Negative gamma micro-adjustments 4 and 5 */
#define	SSD1286_GAMMA_7								0x36
#define	SSD1286_GAMMA_7_PKN4						GENMASK(2, 0)	 // Negative gamma micro-adjustment 4
#define	SSD1286_GAMMA_7_PKN4_SHIFT					0
#define	SSD1286_GAMMA_7_PKN5						GENMASK(10, 8)	 // Negative gamma micro-adjustment 5
#define	SSD1286_GAMMA_7_PKN5_SHIFT					8

/* Negative gamma gradient adjustments 0 and 1 */
#define	SSD1286_GAMMA_8								0x37
#define	SSD1286_GAMMA_8_PRN0						GENMASK(2, 0)	 // Negative gamma gradient adjustment 0
#define	SSD1286_GAMMA_8_PRN0_SHIFT					0
#define	SSD1286_GAMMA_8_PRN1						GENMASK(10, 8)	 // Negative gamma gradient adjustment 1
#define	SSD1286_GAMMA_8_PRN1_SHIFT					8

/* Positive gamma amplification adjustment */
#define	SSD1286_GAMMA_9								0x3A
#define	SSD1286_GAMMA_9_VRP_03_00					GENMASK(3, 0)	 // Positive amplification low bits
#define	SSD1286_GAMMA_9_VRP_03_00_SHIFT				0
#define	SSD1286_GAMMA_9_VRP_14_10					GENMASK(12, 8)	 // Positive amplification high bits
#define	SSD1286_GAMMA_9_VRP_14_10_SHIFT				8

/* Negative gamma amplification adjustment */
#define	SSD1286_GAMMA_10							0x3B
#define	SSD1286_GAMMA_10_VRN_03_00					GENMASK(3, 0)	 // Negative amplification low bits
#define	SSD1286_GAMMA_10_VRN_03_00_SHIFT			0
#define	SSD1286_GAMMA_10_VRN_14_10					GENMASK(12, 8)	 // Negative amplification high bits
#define	SSD1286_GAMMA_10_VRN_14_10_SHIFT			8

/* Gate driver scan starting position */
#define	SSD1286_GATE_SCAN_START						0x40
#define	SSD1286_GATE_SCAN_START_SCN					GENMASK(7, 0)	 // Gate scan start line from 0 to 131
#define	SSD1286_GATE_SCAN_START_SCN_SHIFT			0

/* First- and second-screen vertical scroll offsets */
#define	SSD1286_VERTICAL_SCROLL						0x41
#define	SSD1286_VERTICAL_SCROLL_VL1					GENMASK(7, 0)	 // First-screen vertical scroll offset
#define	SSD1286_VERTICAL_SCROLL_VL1_SHIFT			0
#define	SSD1286_VERTICAL_SCROLL_VL2					GENMASK(15, 8)	 // Second-screen vertical scroll offset
#define	SSD1286_VERTICAL_SCROLL_VL2_SHIFT			8

/* First-screen gate drive range */
#define	SSD1286_FIRST_DISPLAY_DRIVE					0x42
#define	SSD1286_FIRST_DISPLAY_DRIVE_SS1				GENMASK(7, 0)	 // First-screen gate start minus one
#define	SSD1286_FIRST_DISPLAY_DRIVE_SS1_SHIFT		0
#define	SSD1286_FIRST_DISPLAY_DRIVE_SE1				GENMASK(15, 8)	 // First-screen gate end minus one
#define	SSD1286_FIRST_DISPLAY_DRIVE_SE1_SHIFT		8

/* Second-screen gate drive range */
#define	SSD1286_SECOND_DISPLAY_DRIVE				0x43
#define	SSD1286_SECOND_DISPLAY_DRIVE_SS2			GENMASK(7, 0)	 // Second-screen gate start minus one
#define	SSD1286_SECOND_DISPLAY_DRIVE_SS2_SHIFT		0
#define	SSD1286_SECOND_DISPLAY_DRIVE_SE2			GENMASK(15, 8)	 // Second-screen gate end minus one
#define	SSD1286_SECOND_DISPLAY_DRIVE_SE2_SHIFT		8

/* Horizontal GRAM access window */
#define	SSD1286_HORIZONTAL_RAM_ADDRESS				0x44
#define	SSD1286_HORIZONTAL_RAM_ADDRESS_HSA			GENMASK(7, 0)	 // Horizontal window start address
#define	SSD1286_HORIZONTAL_RAM_ADDRESS_HSA_SHIFT	0
#define	SSD1286_HORIZONTAL_RAM_ADDRESS_HEA			GENMASK(15, 8)	 // Horizontal window end address
#define	SSD1286_HORIZONTAL_RAM_ADDRESS_HEA_SHIFT	8

/* Vertical GRAM access window */
#define	SSD1286_VERTICAL_RAM_ADDRESS				0x45
#define	SSD1286_VERTICAL_RAM_ADDRESS_VSA			GENMASK(7, 0)	 // Vertical window start address
#define	SSD1286_VERTICAL_RAM_ADDRESS_VSA_SHIFT		0
#define	SSD1286_VERTICAL_RAM_ADDRESS_VEA			GENMASK(15, 8)	 // Vertical window end address
#define	SSD1286_VERTICAL_RAM_ADDRESS_VEA_SHIFT		8
