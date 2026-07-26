#include <pmb887x.h>

#include "host.h"
#include "test.h"

#if !defined(PMB8876)
#error MMICIF is available only on PMB8876
#endif

#if defined(BOARD_SIEMENS_C81)

static void enable_ati_endpoint(void) {
	static const uint32_t DATA_PINS[] = {
		GPIO_CIF_D0,
		GPIO_CIF_D1,
		GPIO_CIF_D2,
		GPIO_CIF_D3,
		GPIO_CIF_D4,
		GPIO_CIF_D5,
		GPIO_CIF_D6,
		GPIO_CIF_D7,
		GPIO_DIF_D0,
		GPIO_DIF_D1,
		GPIO_DIF_D2,
		GPIO_DIF_D3,
		GPIO_DIF_D4,
		GPIO_DIF_D5,
		GPIO_DIF_D6,
		GPIO_DIF_D7,
	};
	static const uint32_t CONTROL_PINS[] = {
		GPIO_DIF_CD,
		GPIO_DIF_CS1,
		GPIO_DIF_CS2,
		GPIO_DIF_WR,
		GPIO_DIF_RD,
	};

	GPIO_CLC = 1U << MOD_CLC_RMC_SHIFT;
	i2c_init();
	GPIO_PIN(GPIO_CLK32) = GPIO_OS_ALT1 | GPIO_PS_ALT;
	PLL_CON2 |= PLL_CON2_CLK32_EN;
	gpio_init_output(
		GPIO_MM_EN,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_CLKOUT2,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_DIF_BYPASS,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_CIF_PD,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	for (uint32_t i = 0; i < ARRAY_SIZE(DATA_PINS); i++)
		gpio_init_input(DATA_PINS[i], GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_PULLDOWN, GPIO_ENAQ_ON);
	for (uint32_t i = 0; i < ARRAY_SIZE(CONTROL_PINS); i++) {
		gpio_init_output(
			CONTROL_PINS[i],
			GPIO_OS_NONE,
			GPIO_PS_MANUAL,
			true,
			GPIO_PPEN_PUSHPULL,
			GPIO_PDPU_NONE,
			GPIO_ENAQ_OFF
		);
	}
	gpio_init_input(GPIO_CIF_PCLK, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_PULLDOWN, GPIO_ENAQ_ON);
	gpio_init_input(GPIO_CIF_VSYNC, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_PULLDOWN, GPIO_ENAQ_ON);

	i2c_smbus_write_byte(0x31, 0x10, 0x08);
	stopwatch_usleep_wd(10000);
	gpio_set(GPIO_MM_EN, true);
	stopwatch_usleep_wd(40000);
	gpio_set(GPIO_CLKOUT2, true);
	stopwatch_usleep_wd(40000);
	for (uint32_t i = 0; i < ARRAY_SIZE(DATA_PINS); i++)
		GPIO_PIN(DATA_PINS[i]) = GPIO_IS_ALT2 | GPIO_OS_ALT2 | GPIO_PS_ALT;
	for (uint32_t i = 0; i < ARRAY_SIZE(CONTROL_PINS); i++)
		GPIO_PIN(CONTROL_PINS[i]) = GPIO_OS_ALT2 | GPIO_PS_ALT;
	GPIO_PIN(GPIO_CIF_PCLK) = GPIO_IS_ALT2 | GPIO_PS_ALT | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
}

#endif

static void test_reset_values(void) {
	test_category("Reset values");
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, MMICIF_CLC);
	MMICIF_CLC = 1U << MOD_CLC_RMC_SHIFT;
	test_module_clock("module clock is enabled", MMICIF_CLC);
	test_module_id("module ID", 0xF053C000U, MMICIF_ID);
	test_eq_u32("CONFIG reset value", 0, MMICIF_CONFIG);
	test_eq_u32("UNK2C reset value", 0, MMICIF_UNK2C);
	test_eq_u32("UNK44 reset value", 0, MMICIF_UNK44);
	test_eq_u32("TRANSFER_CONFIG reset value", 0, MMICIF_TRANSFER_CONFIG);
	test_eq_u32("UNK4C reset value", 0, MMICIF_UNK4C);
	test_eq_u32("UNK50 reset value", 0, MMICIF_UNK50);
	test_eq_u32("UNK54 reset value", 0, MMICIF_UNK54);
	test_eq_u32("IRQSM reset value", 0, MMICIF_IRQSM);
	test_eq_u32("IRQSS reset value", 0, MMICIF_IRQSS);
	test_eq_u32("UNK80 reset value", 0, MMICIF_UNK80);
}

static void test_firmware_configuration(void) {
	test_category("Firmware configuration");
	MMICIF_CLC = (MMICIF_CLC & 0xFF0000C2U) | 0x00010100U;
	test_eq_u32("CLC accepts the firmware configuration", 0x00010100U, MMICIF_CLC);

	MMICIF_CONFIG = (MMICIF_CONFIG & ~0x3U) | 0x1U;
	test_eq_u32("CONFIG accepts interface selection 1", 1, MMICIF_CONFIG & 0x3U);

	MMICIF_UNK2C = 0x02000000U;
	test_eq_u32("UNK2C accepts the firmware value", 0x02000000U, MMICIF_UNK2C);

	MMICIF_UNK44 = (MMICIF_UNK44 & ~0x3D0U) | 0x40U;
	MMICIF_UNK44 = (MMICIF_UNK44 & ~0x3C0U) | 0x140U;
	test_eq_u32("UNK44 accepts the firmware value", 0x140U, MMICIF_UNK44 & 0x3F8U);

	MMICIF_TRANSFER_CONFIG = 0x0400090AU;
	test_eq_u32("TRANSFER_CONFIG accepts the firmware value", 0x0400090AU, MMICIF_TRANSFER_CONFIG);
	test_eq_u32(
		"TRANSFER_CONFIG selects read mode",
		MMICIF_TRANSFER_CONFIG_MODE_READ,
		MMICIF_TRANSFER_CONFIG & MMICIF_TRANSFER_CONFIG_MODE
	);
	MMICIF_TRANSFER_CONFIG =
		(MMICIF_TRANSFER_CONFIG & ~MMICIF_TRANSFER_CONFIG_MODE) |
		MMICIF_TRANSFER_CONFIG_MODE_WRITE;
	test_eq_u32(
		"TRANSFER_CONFIG switches to write mode",
		MMICIF_TRANSFER_CONFIG_MODE_WRITE,
		MMICIF_TRANSFER_CONFIG & MMICIF_TRANSFER_CONFIG_MODE
	);
	MMICIF_TRANSFER_CONFIG =
		(MMICIF_TRANSFER_CONFIG & ~MMICIF_TRANSFER_CONFIG_MODE) |
		MMICIF_TRANSFER_CONFIG_MODE_READ;
	test_eq_u32(
		"TRANSFER_CONFIG switches back to read mode",
		MMICIF_TRANSFER_CONFIG_MODE_READ,
		MMICIF_TRANSFER_CONFIG & MMICIF_TRANSFER_CONFIG_MODE
	);

	MMICIF_UNK4C = 0;
	MMICIF_UNK50 = 0x1F1F1F00U;
	MMICIF_UNK54 = 0x09000200U;
	test_eq_u32("UNK4C accepts the firmware value", 0, MMICIF_UNK4C);
	test_eq_u32("UNK50 accepts the firmware value", 0x1F1F1F00U, MMICIF_UNK50);
	test_eq_u32("UNK54 accepts the firmware value", 0x09000200U, MMICIF_UNK54);

	MMICIF_IRQSC = 0xFFU;
	MMICIF_IRQSM = 0xFFU;
	MMICIF_UNK80 = 0;
	test_eq_u32("IRQSM accepts the firmware mask", 0xFFU, MMICIF_IRQSM);
	test_eq_u32("IRQSC is write-only", 0, MMICIF_IRQSC);
	test_eq_u32("UNK80 accepts the firmware value", 0, MMICIF_UNK80);
}

static void test_unk80_write_mask(void) {
	test_category("UNK80 write mask");
	MMICIF_UNK80 = UINT32_MAX;
	test_eq_u32("only bits 0-3 are writable", 0x0FU, MMICIF_UNK80);
	MMICIF_UNK80 = 0;
	test_eq_u32("writable bits clear", 0, MMICIF_UNK80);
}

#if defined(BOARD_SIEMENS_C81)
static void test_ati_host_window(void) {
	test_category("ATI host window");
	enable_ati_endpoint();

	uint32_t chip_id = UINT32_MAX;
	bool success = mmicif_host_read32(0x00010000U, &chip_id);
	test_check("ATI chip ID read completes", success);
	if (!success)
		return;
	if (!test_id_u32("ATI chip ID", 0x574B1002U, chip_id))
		return;

	uint32_t original = UINT32_MAX;
	uint32_t actual = UINT32_MAX;
	success = mmicif_host_read32(0x00010058U, &original);
	test_check("ATI MMReg read completes", success);
	if (!success)
		return;
	uint32_t expected = (original & ~BIT(1)) | BIT(2);
	mmicif_host_write32(0x00010058U, expected);
	stopwatch_usleep_wd(1000);
	success = mmicif_host_read32(0x00010058U, &actual);
	test_check("ATI MMReg write/read completes", success);
	test_eq_u32("ATI MMReg write reaches the endpoint", expected, actual);
	mmicif_host_write32(0x00010058U, original);
}

#endif

int main(void) {
	test_start("MMICIF");
	test_reset_values();
	test_firmware_configuration();
	test_unk80_write_mask();
#if defined(BOARD_SIEMENS_C81)
	test_ati_host_window();
#endif

	return test_finish();
}
