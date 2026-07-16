#include <pmb887x.h>

#include "test.h"

static uint32_t pulse_dsp_reset(void) {
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	/* Firmware uses the same readback before deasserting reset; back-to-back writes are too short. */
	uint32_t asserted_requests = SCU_RST_REQ;
	SCU_RST_REQ = 0;

	return asserted_requests;
}

static void test_reset_values(void) {
	test_module_id("module ID", 0xF022C000, DSP_ID);
	test_eq_u32("clock control reset value", MOD_CLC_DISR | MOD_CLC_DISS, DSP_CLC);
	test_eq_u32("communication flags reset value", 0, DSP_COM_STATUS);
	test_eq_u32("DSP interrupt requests reset value", 0, SCU_DSP_INT);
}

static void test_communication_flags(void) {
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("module clock is enabled", DSP_CLC);

	DSP_COM_CLEAR = UINT32_MAX;
	test_eq_u32("all communication flags can be cleared", 0, DSP_COM_STATUS);
	test_eq_u32("set register reads as zero when flags are clear", 0, DSP_COM_SET);
	test_eq_u32("clear register reads as zero when flags are clear", 0, DSP_COM_CLEAR);

	DSP_COM_SET = BIT(0) | BIT(15);
	test_eq_u32("set register has write-one-to-set semantics", BIT(0) | BIT(15), DSP_COM_STATUS);
	test_eq_u32("set register reads as zero when flags are set", 0, DSP_COM_SET);
	test_eq_u32("clear register reads as zero when flags are set", 0, DSP_COM_CLEAR);

	DSP_COM_CLEAR = BIT(0);
	test_eq_u32("clear register has write-one-to-clear semantics", BIT(15), DSP_COM_STATUS);

	DSP_COM_SET = BIT(0);
	test_eq_u32("cleared communication flag can be set again", BIT(0) | BIT(15), DSP_COM_STATUS);
	DSP_COM_CLEAR = UINT32_MAX;
}

static void test_interrupt_requests(void) {
	SCU_DSP_INT = 0;
	SCU_DSP_INT = BIT(0);
	test_eq_u32("DSP interrupt request can be asserted", BIT(0), SCU_DSP_INT & SCU_DSP_INT_REQ);
	SCU_DSP_INT = 0;
	test_eq_u32("DSP interrupt requests can be deasserted", 0, SCU_DSP_INT);
}

static void test_firmware_command_handshake(void) {
	DSP_COM_CLEAR = UINT32_MAX;
	SCU_DSP_INT = 0;

	/* This is the read-modify-write sequence used by phone firmware during DSP initialization. */
	DSP_COM_SET = DSP_COM_SET | BIT(0);
	SCU_DSP_INT = SCU_DSP_INT | BIT(0);
	SCU_DSP_INT = SCU_DSP_INT & ~BIT(0);

	for (unsigned int timeout = 1000; timeout != 0 && DSP_COM_STATUS != 0; timeout--) {
	}

	test_eq_u32("firmware command is acknowledged without stale flags", 0, DSP_COM_STATUS);
	DSP_COM_CLEAR = UINT32_MAX;
}

static void test_shared_ram(void) {
	volatile uint8_t *ram8 = (volatile uint8_t *) (DSP_BASE + 0x1800);
	volatile uint16_t *ram16 = (volatile uint16_t *) ram8;
	volatile uint32_t *ram32 = (volatile uint32_t *) ram8;

	ram32[0] = 0xA1B2C3D4;
	test_eq_u32("32-bit write is visible as lower 16-bit word", 0xC3D4, ram16[0]);
	test_eq_u32("32-bit write is visible as upper 16-bit word", 0xA1B2, ram16[1]);
	test_eq_u32("shared RAM is little-endian at byte width", 0xD4, ram8[0]);
	test_eq_u32("upper byte of a halfword is independently readable", 0xC3, ram8[1]);

	ram8[0] = 0x11;
	ram8[1] = 0x22;
	ram8[2] = 0x33;
	ram8[3] = 0x44;
	test_eq_u32("byte writes compose a 32-bit shared RAM word", 0x44332211, ram32[0]);

	ram16[0] = 0x5AA5;
	ram16[1] = 0xC33C;
	test_eq_u32("halfword writes compose a 32-bit shared RAM word", 0xC33C5AA5, ram32[0]);
}

static void test_dsp_reset(void) {
	DSP_COM_SET = BIT(15);
	test_eq_u32("communication flag is prepared set", BIT(15), DSP_COM_STATUS);

	test_eq_u32("DSP reset request asserts", SCU_RST_REQ_DSP, pulse_dsp_reset());
	test_eq_u32("DSP reset clears communication flags", 0, DSP_COM_STATUS);
	test_eq_u32("DSP reset preserves clock control", 1 << MOD_CLC_RMC_SHIFT, DSP_CLC);
	test_eq_u32("DSP reset request finishes deasserted", 0, SCU_RST_REQ & SCU_RST_REQ_DSP);
}

int main(void) {
	test_start("DSP peripheral test");

	test_category("Reset values");
	test_reset_values();
	test_category("Communication flags");
	test_communication_flags();
	test_category("Interrupt requests");
	test_interrupt_requests();
	test_category("Firmware command handshake");
	test_firmware_command_handshake();
	test_category("Shared RAM");
	test_shared_ram();
	test_category("DSP reset");
	test_dsp_reset();

	DSP_COM_CLEAR = UINT32_MAX;
	DSP_CLC = MOD_CLC_DISR;

	return test_finish();
}
