#pragma once

#include <bitops.h> // IWYU pragma: export

// PCF8833
// Philips PCF8833 132x132 STN RGB LCD controller
/* No operation */
#define	PCF8833_NOP								0x00

/* Software reset */
#define	PCF8833_SWRESET							0x01

/* Booster voltage off */
#define	PCF8833_BSTROFF							0x02

/* Booster voltage on */
#define	PCF8833_BSTRON							0x03

/* Read 24-bit display identification */
#define	PCF8833_RDDIDIF							0x04
#define	PCF8833_RDDIDIF_MODULE_CODE				GENMASK(7, 0)	 // Driver or module code
#define	PCF8833_RDDIDIF_MODULE_CODE_SHIFT		0
#define	PCF8833_RDDIDIF_VERSION					GENMASK(14, 8)	 // Driver or module version ID
#define	PCF8833_RDDIDIF_VERSION_SHIFT			8
#define	PCF8833_RDDIDIF_MODULE_TYPE				BIT(15)			 // Monochrome or color module type
#define	PCF8833_RDDIDIF_MODULE_TYPE_MONOCHROME	0x0
#define	PCF8833_RDDIDIF_MODULE_TYPE_COLOR		0x8000
#define	PCF8833_RDDIDIF_MANID					GENMASK(23, 16)	 // Manufacturer ID
#define	PCF8833_RDDIDIF_MANID_SHIFT				16

/* Read 32-bit display status */
#define	PCF8833_RDDST							0x09
#define	PCF8833_RDDST_TEARING					BIT(9)			 // Tearing-effect output enabled
#define	PCF8833_RDDST_DISPLAY_ON				BIT(10)			 // Display output enabled
#define	PCF8833_RDDST_ALL_OFF					BIT(11)			 // All-pixels-off mode active
#define	PCF8833_RDDST_ALL_ON					BIT(12)			 // All-pixels-on mode active
#define	PCF8833_RDDST_INVERSION					BIT(13)			 // Display inversion active
#define	PCF8833_RDDST_SCROLL					BIT(15)			 // Vertical scrolling active
#define	PCF8833_RDDST_NORMAL_DISPLAY			BIT(16)			 // Normal-display mode active
#define	PCF8833_RDDST_SLEEP_OUT					BIT(17)			 // Sleep-out state active
#define	PCF8833_RDDST_PARTIAL					BIT(18)			 // Partial-display mode active
#define	PCF8833_RDDST_IDLE						BIT(19)			 // Idle mode active
#define	PCF8833_RDDST_PIXEL_FORMAT				GENMASK(22, 20)	 // Interface pixel format
#define	PCF8833_RDDST_PIXEL_FORMAT_SHIFT		20
#define	PCF8833_RDDST_RGB						BIT(26)			 // BGR order active
#define	PCF8833_RDDST_LAO						BIT(27)			 // Bottom-to-top line order active
#define	PCF8833_RDDST_V							BIT(28)			 // Vertical RAM writes active
#define	PCF8833_RDDST_MX						BIT(29)			 // Horizontal mirroring active
#define	PCF8833_RDDST_MY						BIT(30)			 // Vertical mirroring active
#define	PCF8833_RDDST_BOOSTER_READY				BIT(31)			 // Booster enabled and ready

/* Enter sleep mode */
#define	PCF8833_SLPIN							0x10

/* Leave sleep mode */
#define	PCF8833_SLPOUT							0x11

/* Enable partial-display mode */
#define	PCF8833_PTLON							0x12

/* Enable normal-display mode */
#define	PCF8833_NORON							0x13

/* Disable display inversion */
#define	PCF8833_INVOFF							0x20

/* Enable display inversion */
#define	PCF8833_INVON							0x21

/* Force all pixels off */
#define	PCF8833_DALO							0x22

/* Force all pixels on */
#define	PCF8833_DAL								0x23

/* Set LCD contrast offset */
#define	PCF8833_SETCON							0x25
#define	PCF8833_SETCON_VCON						GENMASK(6, 0)	 // Two's-complement VLCD offset
#define	PCF8833_SETCON_VCON_SHIFT				0

/* Disable display output */
#define	PCF8833_DISPOFF							0x28

/* Enable display output */
#define	PCF8833_DISPON							0x29

/* Set column write range */
#define	PCF8833_CASET							0x2A
#define	PCF8833_CASET_XE						GENMASK(7, 0)	 // End column from 0 to 131
#define	PCF8833_CASET_XE_SHIFT					0
#define	PCF8833_CASET_XS						GENMASK(15, 8)	 // Start column from 0 to 131
#define	PCF8833_CASET_XS_SHIFT					8

/* Set page write range */
#define	PCF8833_PASET							0x2B
#define	PCF8833_PASET_YE						GENMASK(7, 0)	 // End page from 0 to 131
#define	PCF8833_PASET_YE_SHIFT					0
#define	PCF8833_PASET_YS						GENMASK(15, 8)	 // Start page from 0 to 131
#define	PCF8833_PASET_YS_SHIFT					8

/* Start display RAM write */
#define	PCF8833_RAMWR							0x2C

/* Load the 256-color lookup table */
#define	PCF8833_RGBSET							0x2D

/* Set partial-display area */
#define	PCF8833_PTLAR							0x30
#define	PCF8833_PTLAR_AREA_END					GENMASK(7, 0)	 // Active-area end row
#define	PCF8833_PTLAR_AREA_END_SHIFT			0
#define	PCF8833_PTLAR_AREA_START				GENMASK(15, 8)	 // Active-area start row
#define	PCF8833_PTLAR_AREA_START_SHIFT			8

/* Set vertical-scroll areas */
#define	PCF8833_VSCRDEF							0x33
#define	PCF8833_VSCRDEF_BOTTOM_FIXED			GENMASK(7, 0)	 // Bottom fixed-area height
#define	PCF8833_VSCRDEF_BOTTOM_FIXED_SHIFT		0
#define	PCF8833_VSCRDEF_SCROLL_AREA				GENMASK(15, 8)	 // Scrollable-area height
#define	PCF8833_VSCRDEF_SCROLL_AREA_SHIFT		8
#define	PCF8833_VSCRDEF_TOP_FIXED				GENMASK(23, 16)	 // Top fixed-area height
#define	PCF8833_VSCRDEF_TOP_FIXED_SHIFT			16

/* Disable tearing-effect output */
#define	PCF8833_TEOFF							0x34

/* Enable tearing-effect output */
#define	PCF8833_TEON							0x35

/* Set RAM addressing and color order */
#define	PCF8833_MADCTL							0x36
#define	PCF8833_MADCTL_RGB						BIT(3)			 // Interface color order
#define	PCF8833_MADCTL_RGB_RGB_ORDER			0x0
#define	PCF8833_MADCTL_RGB_BGR_ORDER			0x8
#define	PCF8833_MADCTL_LAO						BIT(4)			 // Line-address order
#define	PCF8833_MADCTL_LAO_TOP_TO_BOTTOM		0x0
#define	PCF8833_MADCTL_LAO_BOTTOM_TO_TOP		0x10
#define	PCF8833_MADCTL_V						BIT(5)			 // RAM write direction
#define	PCF8833_MADCTL_V_WRITE_X				0x0
#define	PCF8833_MADCTL_V_WRITE_Y				0x20
#define	PCF8833_MADCTL_MX						BIT(6)			 // Horizontal mirroring
#define	PCF8833_MADCTL_MX_NORMAL				0x0
#define	PCF8833_MADCTL_MX_MIRRORED				0x40
#define	PCF8833_MADCTL_MY						BIT(7)			 // Vertical mirroring
#define	PCF8833_MADCTL_MY_NORMAL				0x0
#define	PCF8833_MADCTL_MY_MIRRORED				0x80

/* Set vertical-scroll entry point */
#define	PCF8833_SEP								0x37
#define	PCF8833_SEP_VALUE						GENMASK(7, 0)	 // Scroll entry row
#define	PCF8833_SEP_VALUE_SHIFT					0

/* Disable 8-color idle mode */
#define	PCF8833_IDMOFF							0x38

/* Enable 8-color idle mode */
#define	PCF8833_IDMON							0x39

/* Set interface pixel format */
#define	PCF8833_COLMOD							0x3A
#define	PCF8833_COLMOD_P						GENMASK(2, 0)	 // Interface pixel format
#define	PCF8833_COLMOD_P_SHIFT					0
#define	PCF8833_COLMOD_P_PIXEL_8_BIT			0x2
#define	PCF8833_COLMOD_P_PIXEL_12_BIT			0x3
#define	PCF8833_COLMOD_P_PIXEL_16_BIT			0x5

/* Set the nine-bit VLCD operating point */
#define	PCF8833_SETVOP							0xB0
#define	PCF8833_SETVOP_VPR_04_00				GENMASK(4, 0)	 // VLCD operating-point low bits
#define	PCF8833_SETVOP_VPR_04_00_SHIFT			0
#define	PCF8833_SETVOP_VPR_08_05				GENMASK(11, 8)	 // VLCD operating-point high bits
#define	PCF8833_SETVOP_VPR_08_05_SHIFT			8

/* Use normal bottom-row order */
#define	PCF8833_BRS_NORMAL						0xB4

/* Mirror bottom rows */
#define	PCF8833_BRS_MIRRORED					0xB5

/* Use normal top-row order */
#define	PCF8833_TRS_NORMAL						0xB6

/* Mirror top rows */
#define	PCF8833_TRS_MIRRORED					0xB7

/* Disable super-frame inversion */
#define	PCF8833_FINV_OFF						0xB8

/* Enable super-frame inversion */
#define	PCF8833_FINV_ON							0xB9

/* Use normal display-RAM data order */
#define	PCF8833_DOR_NORMAL						0xBA

/* Swap display-RAM data MSB and LSB */
#define	PCF8833_DOR_SWAPPED						0xBB

/* Use non-segmented frame frequency */
#define	PCF8833_TCDFE_OFF						0xBC

/* Use temperature-segmented frame frequency */
#define	PCF8833_TCDFE_ON						0xBD

/* Disable temperature-compensated VLCD */
#define	PCF8833_TCVOPE_OFF						0xBE

/* Enable temperature-compensated VLCD */
#define	PCF8833_TCVOPE_ON						0xBF

/* Use the internal display oscillator */
#define	PCF8833_EC_INTERNAL						0xC0

/* Use the external display clock */
#define	PCF8833_EC_EXTERNAL						0xC1

/* Set voltage-multiplier factor */
#define	PCF8833_SETMUL							0xC2
#define	PCF8833_SETMUL_S						GENMASK(1, 0)	 // Voltage-multiplier factor
#define	PCF8833_SETMUL_S_SHIFT					0
#define	PCF8833_SETMUL_S_FACTOR_2				0x0
#define	PCF8833_SETMUL_S_FACTOR_3				0x1
#define	PCF8833_SETMUL_S_FACTOR_4				0x2
#define	PCF8833_SETMUL_S_FACTOR_5				0x3

/* Set VLCD temperature slopes A and B */
#define	PCF8833_TCVOPAB							0xC3
#define	PCF8833_TCVOPAB_SLA						GENMASK(2, 0)	 // Temperature slope A
#define	PCF8833_TCVOPAB_SLA_SHIFT				0
#define	PCF8833_TCVOPAB_SLB						GENMASK(6, 4)	 // Temperature slope B
#define	PCF8833_TCVOPAB_SLB_SHIFT				4

/* Set VLCD temperature slopes C and D */
#define	PCF8833_TCVOPCD							0xC4
#define	PCF8833_TCVOPCD_SLC						GENMASK(2, 0)	 // Temperature slope C
#define	PCF8833_TCVOPCD_SLC_SHIFT				0
#define	PCF8833_TCVOPCD_SLD						GENMASK(6, 4)	 // Temperature slope D
#define	PCF8833_TCVOPCD_SLD_SHIFT				4

/* Set segmented frame-frequency dividers */
#define	PCF8833_TCDF							0xC5
#define	PCF8833_TCDF_DFD						GENMASK(6, 0)	 // Temperature-region D divider
#define	PCF8833_TCDF_DFD_SHIFT					0
#define	PCF8833_TCDF_DFC						GENMASK(14, 8)	 // Temperature-region C divider
#define	PCF8833_TCDF_DFC_SHIFT					8
#define	PCF8833_TCDF_DFB						GENMASK(22, 16)	 // Temperature-region B divider
#define	PCF8833_TCDF_DFB_SHIFT					16
#define	PCF8833_TCDF_DFA						GENMASK(30, 24)	 // Temperature-region A divider
#define	PCF8833_TCDF_DFA_SHIFT					24

/* Set 8-color frame-frequency divider */
#define	PCF8833_DF8COLOR						0xC6
#define	PCF8833_DF8COLOR_DF8					GENMASK(6, 0)	 // 8-color frame-frequency divider
#define	PCF8833_DF8COLOR_DF8_SHIFT				0

/* Set LCD bias system */
#define	PCF8833_SETBS							0xC7
#define	PCF8833_SETBS_VB						GENMASK(3, 0)	 // LCD bias-system ratio
#define	PCF8833_SETBS_VB_SHIFT					0

/* Read measured temperature code */
#define	PCF8833_RDTEMP							0xC8
#define	PCF8833_RDTEMP_TD						GENMASK(7, 0)	 // Temperature code; T = 0.9375 x TD - 40 C
#define	PCF8833_RDTEMP_TD_SHIFT					0

/* Set N-line inversion interval */
#define	PCF8833_NLI								0xC9
#define	PCF8833_NLI_NLI							GENMASK(7, 0)	 // Inversion interval counter
#define	PCF8833_NLI_NLI_SHIFT					0

/* Read manufacturer ID */
#define	PCF8833_RDID1							0xDA
#define	PCF8833_RDID1_MANID						GENMASK(7, 0)	 // Manufacturer ID
#define	PCF8833_RDID1_MANID_SHIFT				0

/* Read module version ID */
#define	PCF8833_RDID2							0xDB
#define	PCF8833_RDID2_VERSION					GENMASK(7, 0)	 // Module version ID
#define	PCF8833_RDID2_VERSION_SHIFT				0

/* Read module ID */
#define	PCF8833_RDID3							0xDC
#define	PCF8833_RDID3_MODULE_ID					GENMASK(7, 0)	 // Module ID
#define	PCF8833_RDID3_MODULE_ID_SHIFT			0

/* Use settings written through the interface */
#define	PCF8833_SFD_INTERFACE					0xEE

/* Use OTP-programmed factory settings */
#define	PCF8833_SFD_OTP							0xEF

/* Enter OTP calibration mode */
#define	PCF8833_CALMODE							0xF0
#define	PCF8833_CALMODE_CALMM					BIT(0)			 // Calibration-mode control
#define	PCF8833_CALMODE_OPE						BIT(1)			 // OTP programming enable
#define	PCF8833_CALMODE_ORA						GENMASK(5, 3)	 // OTP register address
#define	PCF8833_CALMODE_ORA_SHIFT				3

/* Shift data into OTP registers */
#define	PCF8833_OTPSHTIN						0xF1
#define	PCF8833_OTPSHTIN_DATA					GENMASK(7, 0)	 // Arbitrary-length OTP data byte
#define	PCF8833_OTPSHTIN_DATA_SHIFT				0
