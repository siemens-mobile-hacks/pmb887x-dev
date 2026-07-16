#include <pmb887x.h>

#include "test.h"

static void test_reset_values(void) {
	test_eq_u32("clock control reset value", MOD_CLC_DISR | MOD_CLC_DISS, GPRSCU_CLC);
	GPRSCU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("module clock is enabled", GPRSCU_CLC);
	test_module_id("module ID", 0xF003C000, GPRSCU_ID);
	test_eq_u32("control reset value", 0, GPRSCU_CON);
	test_eq_u32("FIFO status reset value", 32 << GPRSCU_STAT_INPUT_FREE_SHIFT, GPRSCU_STAT);
	test_eq_u32("segment descriptor reset value", 0, GPRSCU_SEGMENT);
	for (uint32_t index = 0; index < 2; index++)
		test_eq_u32("cipher input reset value", 0, GPRSCU_INPUT(index));
	for (uint32_t index = 0; index < 4; index++)
		test_eq_u32("cipher key reset value", 0, GPRSCU_KEY(index));
	test_eq_u32("frame check sequence reset value", 0, GPRSCU_FCS);
	test_eq_u32("CRC polynomial reset value", 0, GPRSCU_POLYNOM);
	test_eq_u32("interrupt source 0 reset value", 0, GPRSCU_SRC(0));
	test_eq_u32("interrupt source 1 reset value", 0, GPRSCU_SRC(1));
}

static void test_register_layout(void) {
	test_eq_u32("initialization command is bit 0", BIT(0), GPRSCU_CON_INIT);
	test_eq_u32("direction is bit 1", BIT(1), GPRSCU_CON_DIRECTION);
	test_eq_u32("CRC control is bit 2", BIT(2), GPRSCU_CON_CRC_CTRL);
	test_eq_u32("cipher control is bit 3", BIT(3), GPRSCU_CON_CIPH_CTRL);
	test_eq_u32("GEA2 selection is bit 4", BIT(4), GPRSCU_CON_GEA2);
	test_eq_u32("segment mode is bit 5", BIT(5), GPRSCU_CON_SEGMENT_MODE);
	test_eq_u32("segment start is bit 6", BIT(6), GPRSCU_CON_SEGMENT_START);
	test_eq_u32("segment reset is bit 9", BIT(9), GPRSCU_CON_SEGMENT_RESET);
	test_eq_u32("initialization busy status is bit 16", BIT(16), GPRSCU_CON_BUSY);
	test_eq_u32("GEA3 selection is bit 20", BIT(20), GPRSCU_CON_GEA3);
	test_eq_u32("output FIFO count occupies bits 5:0", GENMASK(5, 0), GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("input FIFO free count occupies bits 13:8", GENMASK(13, 8), GPRSCU_STAT_INPUT_FREE);
	test_eq_u32("segment length occupies bits 10:0", GENMASK(10, 0), GPRSCU_SEGMENT_LENGTH);
	test_eq_u32("segment CRC control is bit 18", BIT(18), GPRSCU_SEGMENT_CRC_CTRL);
	test_eq_u32("segment cipher control is bit 19", BIT(19), GPRSCU_SEGMENT_CIPH_CTRL);
}

static void test_passthrough(void) {
	GPRSCU_CON = GPRSCU_CON_INIT;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_CON & GPRSCU_CON_BUSY) != 0; attempt++) {
		test_watchdog_serve();
	}
	test_eq_u32("initialization command finishes", 0, GPRSCU_CON & (GPRSCU_CON_BUSY | GPRSCU_CON_INIT));
	test_eq_u32(
		"input FIFO has 32 bytes free",
		32,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT
	);
	test_eq_u32("output FIFO starts empty", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);

	MMIO8(GPRSCU_BASE + 0x14) = 0xA5;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT) == 0; attempt++) {
		test_watchdog_serve();
	}
	test_eq_u32("one output byte is available", 1, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("disabled CRC and cipher pass data through", 0xA5, MMIO8(GPRSCU_BASE + 0x14));
	test_eq_u32("reading the byte empties the output FIFO", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("valid FIFO access does not underrun", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_UNDERRUN);

	GPRSCU_CON = 0;
}

int main(void) {
	test_start("GPRSCU peripheral test");

	test_category("Reset values");
	test_reset_values();
	test_category("Register layout");
	test_register_layout();
	test_category("Unmodified data path");
	test_passthrough();

	return test_finish();
}
