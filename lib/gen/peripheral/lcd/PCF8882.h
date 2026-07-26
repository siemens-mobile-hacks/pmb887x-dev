#pragma once

#include <bitops.h> // IWYU pragma: export

// PCF8882
// PCF8833-compatible 132x176 TFT LCD controller
/* No operation */
#define	PCF8882_NOP								0x00

/* Software reset */
#define	PCF8882_SWRESET							0x01

/* Read 24-bit display identification */
#define	PCF8882_RDDIDIF							0x04
#define	PCF8882_RDDIDIF_MODULE_CODE				GENMASK(7, 0)	 // Driver or module code
#define	PCF8882_RDDIDIF_MODULE_CODE_SHIFT		0
#define	PCF8882_RDDIDIF_VERSION					GENMASK(14, 8)	 // Driver or module version ID
#define	PCF8882_RDDIDIF_VERSION_SHIFT			8
#define	PCF8882_RDDIDIF_MODULE_TYPE				BIT(15)			 // Monochrome or color module type
#define	PCF8882_RDDIDIF_MODULE_TYPE_MONOCHROME	0x0
#define	PCF8882_RDDIDIF_MODULE_TYPE_COLOR		0x8000
#define	PCF8882_RDDIDIF_MANID					GENMASK(23, 16)	 // Manufacturer ID
#define	PCF8882_RDDIDIF_MANID_SHIFT				16

/* Read 32-bit display status */
#define	PCF8882_RDDST							0x09
#define	PCF8882_RDDST_TEARING					BIT(9)			 // Tearing-effect output enabled
#define	PCF8882_RDDST_DISPLAY_ON				BIT(10)			 // Display output enabled
#define	PCF8882_RDDST_ALL_OFF					BIT(11)			 // All-pixels-off mode active
#define	PCF8882_RDDST_ALL_ON					BIT(12)			 // All-pixels-on mode active
#define	PCF8882_RDDST_INVERSION					BIT(13)			 // Display inversion active
#define	PCF8882_RDDST_SCROLL					BIT(15)			 // Vertical scrolling active
#define	PCF8882_RDDST_NORMAL_DISPLAY			BIT(16)			 // Normal-display mode active
#define	PCF8882_RDDST_SLEEP_OUT					BIT(17)			 // Sleep-out state active
#define	PCF8882_RDDST_PARTIAL					BIT(18)			 // Partial-display mode active
#define	PCF8882_RDDST_IDLE						BIT(19)			 // Idle mode active
#define	PCF8882_RDDST_PIXEL_FORMAT				GENMASK(22, 20)	 // Interface pixel format
#define	PCF8882_RDDST_PIXEL_FORMAT_SHIFT		20
#define	PCF8882_RDDST_RGB						BIT(26)			 // BGR order active
#define	PCF8882_RDDST_LAO						BIT(27)			 // Bottom-to-top line order active
#define	PCF8882_RDDST_V							BIT(28)			 // Vertical RAM writes active
#define	PCF8882_RDDST_MX						BIT(29)			 // Horizontal mirroring active
#define	PCF8882_RDDST_MY						BIT(30)			 // Vertical mirroring active

/* Enter sleep mode */
#define	PCF8882_SLPIN							0x10

/* Leave sleep mode */
#define	PCF8882_SLPOUT							0x11

/* Enable partial-display mode */
#define	PCF8882_PTLON							0x12

/* Enable normal-display mode */
#define	PCF8882_NORON							0x13

/* Disable display inversion */
#define	PCF8882_INVOFF							0x20

/* Enable display inversion */
#define	PCF8882_INVON							0x21

/* Force all pixels off */
#define	PCF8882_DALO							0x22

/* Force all pixels on */
#define	PCF8882_DAL								0x23

/* Select gamma curve */
#define	PCF8882_GAMSET							0x26

/* Disable display output */
#define	PCF8882_DISPOFF							0x28

/* Enable display output */
#define	PCF8882_DISPON							0x29

/* Set column write range */
#define	PCF8882_CASET							0x2A
#define	PCF8882_CASET_XE						GENMASK(7, 0)	 // End column from 0 to 131
#define	PCF8882_CASET_XE_SHIFT					0
#define	PCF8882_CASET_XS						GENMASK(15, 8)	 // Start column from 0 to 131
#define	PCF8882_CASET_XS_SHIFT					8

/* Set page write range */
#define	PCF8882_PASET							0x2B
#define	PCF8882_PASET_YE						GENMASK(7, 0)	 // End page from 0 to 175
#define	PCF8882_PASET_YE_SHIFT					0
#define	PCF8882_PASET_YS						GENMASK(15, 8)	 // Start page from 0 to 175
#define	PCF8882_PASET_YS_SHIFT					8

/* Start display RAM write */
#define	PCF8882_RAMWR							0x2C

/* Load the 256-color lookup table */
#define	PCF8882_RGBSET							0x2D

/* Set partial-display area */
#define	PCF8882_PTLAR							0x30
#define	PCF8882_PTLAR_AREA_END					GENMASK(7, 0)	 // Active-area end row
#define	PCF8882_PTLAR_AREA_END_SHIFT			0
#define	PCF8882_PTLAR_AREA_START				GENMASK(15, 8)	 // Active-area start row
#define	PCF8882_PTLAR_AREA_START_SHIFT			8

/* Set vertical-scroll areas */
#define	PCF8882_VSCRDEF							0x33
#define	PCF8882_VSCRDEF_BOTTOM_FIXED			GENMASK(7, 0)	 // Bottom fixed-area height
#define	PCF8882_VSCRDEF_BOTTOM_FIXED_SHIFT		0
#define	PCF8882_VSCRDEF_SCROLL_AREA				GENMASK(15, 8)	 // Scrollable-area height
#define	PCF8882_VSCRDEF_SCROLL_AREA_SHIFT		8
#define	PCF8882_VSCRDEF_TOP_FIXED				GENMASK(23, 16)	 // Top fixed-area height
#define	PCF8882_VSCRDEF_TOP_FIXED_SHIFT			16

/* Disable tearing-effect output */
#define	PCF8882_TEOFF							0x34

/* Enable tearing-effect output */
#define	PCF8882_TEON							0x35

/* Set RAM addressing and color order */
#define	PCF8882_MADCTL							0x36
#define	PCF8882_MADCTL_RGB						BIT(3)			 // Interface color order
#define	PCF8882_MADCTL_RGB_RGB_ORDER			0x0
#define	PCF8882_MADCTL_RGB_BGR_ORDER			0x8
#define	PCF8882_MADCTL_LAO						BIT(4)			 // Line-address order
#define	PCF8882_MADCTL_LAO_TOP_TO_BOTTOM		0x0
#define	PCF8882_MADCTL_LAO_BOTTOM_TO_TOP		0x10
#define	PCF8882_MADCTL_V						BIT(5)			 // RAM write direction
#define	PCF8882_MADCTL_V_WRITE_X				0x0
#define	PCF8882_MADCTL_V_WRITE_Y				0x20
#define	PCF8882_MADCTL_MX						BIT(6)			 // Horizontal mirroring
#define	PCF8882_MADCTL_MX_NORMAL				0x0
#define	PCF8882_MADCTL_MX_MIRRORED				0x40
#define	PCF8882_MADCTL_MY						BIT(7)			 // Vertical mirroring
#define	PCF8882_MADCTL_MY_NORMAL				0x0
#define	PCF8882_MADCTL_MY_MIRRORED				0x80

/* Set vertical-scroll entry point */
#define	PCF8882_SEP								0x37
#define	PCF8882_SEP_VALUE						GENMASK(7, 0)	 // Scroll entry row
#define	PCF8882_SEP_VALUE_SHIFT					0

/* Disable reduced-color idle mode */
#define	PCF8882_IDMOFF							0x38

/* Enable reduced-color idle mode */
#define	PCF8882_IDMON							0x39

/* Set interface pixel format */
#define	PCF8882_COLMOD							0x3A
#define	PCF8882_COLMOD_P						GENMASK(2, 0)	 // Interface pixel format
#define	PCF8882_COLMOD_P_SHIFT					0
#define	PCF8882_COLMOD_P_PIXEL_8_BIT			0x2
#define	PCF8882_COLMOD_P_PIXEL_12_BIT			0x3
#define	PCF8882_COLMOD_P_PIXEL_16_BIT			0x5

/* Read manufacturer ID */
#define	PCF8882_RDID1							0xDA
#define	PCF8882_RDID1_MANID						GENMASK(7, 0)	 // Manufacturer ID
#define	PCF8882_RDID1_MANID_SHIFT				0

/* Read module version ID */
#define	PCF8882_RDID2							0xDB
#define	PCF8882_RDID2_VERSION					GENMASK(7, 0)	 // Module version ID
#define	PCF8882_RDID2_VERSION_SHIFT				0

/* Read module ID */
#define	PCF8882_RDID3							0xDC
#define	PCF8882_RDID3_MODULE_ID					GENMASK(7, 0)	 // Module ID
#define	PCF8882_RDID3_MODULE_ID_SHIFT			0
