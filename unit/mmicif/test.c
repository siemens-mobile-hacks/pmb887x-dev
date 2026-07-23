#include <pmb887x.h>

#include "test.h"

#if !defined(PMB8876)
#error MMICIF is available only on PMB8876
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

int main(void) {
	test_start("MMICIF");
	test_reset_values();
	test_firmware_configuration();
	test_unk80_write_mask();

	return test_finish();
}
