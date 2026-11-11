#include <pmb887x.h>

#include "test.h"

#define GPRSCU_FIFO_ENTRIES 32
#define GPRSCU_LLC_POLYNOM 0x00DDD0DA
#define GPRSCU_LLC_FCS_INIT 0xFFFFFF00
#define GPRSCU_DATA_BYTE MMIO8(&GPRSCU_DATA)
#define GPRSCU_DATA_HALFWORD MMIO16(&GPRSCU_DATA)

struct gprscu_pattern_result {
	uint32_t fcs;
	uint32_t source_before_start;
	uint32_t source_after_start;
};

struct gea3_case {
	const char *name;
	uint32_t input;
	uint32_t input_suffix;
	uint32_t control;
	uint32_t key[2];
	uint32_t segment_control;
	uint32_t expected_fcs;
	uint8_t expected[59];
};

/* 3GPP TS 55.217 V6.1.0 and TS 55.218 V6.1.0 GEA3 known-answer vectors. */
static const struct gea3_case GEA3_CASES[] = {
	{
		.name = "GEA3 uplink vector 1",
		.input = 0x8E9421A3,
		.input_suffix = 0x00FF0000,
		.control = GPRSCU_CON_GEA3 | GPRSCU_CON_SEGMENT_MODE |
			GPRSCU_CON_CIPH_CTRL | GPRSCU_CON_CRC_CTRL,
		.key = { 0x82C5BC00, 0x2BD6459F },
		.segment_control = GPRSCU_SEGMENT_CIPH_CTRL | GPRSCU_SEGMENT_CRC_CTRL,
		.expected_fcs = 0x1B547300,
		.expected = {
			0x5F, 0x35, 0x97, 0x09, 0xDE, 0x95, 0x0D, 0x01, 0x05, 0xB1, 0x7B, 0x6C,
			0x90, 0x19, 0x42, 0x80, 0xF8, 0x80, 0xB4, 0x8D, 0xCC, 0xDC, 0x2A, 0xFE,
			0xED, 0x41, 0x5D, 0xBE, 0xF4, 0x35, 0x4E, 0xEB, 0xB2, 0x1D, 0x07, 0x3C,
			0xCB, 0xBF, 0xB2, 0xD7, 0x06, 0xBD, 0x7A, 0xFF, 0xD3, 0x71, 0xFC, 0x96,
			0xE3, 0x97, 0x0D, 0x14, 0x3D, 0xCB, 0x26, 0x24, 0x05, 0x48, 0x26,
		},
	},
	{
		.name = "GEA3 downlink vector 3",
		.input = 0x4BDBD5E5,
		.input_suffix = 0x04FF0000,
		.control = GPRSCU_CON_GEA3 | GPRSCU_CON_SEGMENT_MODE |
			GPRSCU_CON_CIPH_CTRL | GPRSCU_CON_DIRECTION,
		.key = { 0x9E720C2A, 0xEFA8B222 },
		.segment_control = GPRSCU_SEGMENT_CIPH_CTRL,
		.expected = {
			0x47, 0x18, 0xA2, 0xAD, 0xFC, 0x90, 0x59, 0x09, 0x49, 0xDD, 0xAD, 0xAB,
			0x40, 0x6E, 0xC3, 0xB9, 0x25, 0xF1, 0xAF, 0x12, 0x14, 0x67, 0x39, 0x09,
			0xDA, 0xAB, 0x96, 0xBB, 0x4C, 0x18, 0xB1, 0x37, 0x4B, 0xB1, 0xE9, 0x94,
			0x45, 0xA8, 0x1C, 0xC8, 0x56, 0xE4, 0x7C, 0x6E, 0x49, 0xE9, 0xDB, 0xB9,
			0x87, 0x3D, 0x08, 0x31, 0xB2, 0x17, 0x5C, 0xA1, 0xE1, 0x09, 0xBA,
		},
	},
};

#define GEA12_VECTOR_BYTES 18

struct gea12_case {
	const char *name;
	uint32_t control;
	uint8_t key[8];
	uint32_t input;
	uint8_t plaintext[GEA12_VECTOR_BYTES];
	uint8_t expected[GEA12_VECTOR_BYTES];
};

/* GEA-1 and GEA-2 known-answer vectors published with github.com/P1sec/gea-implementation. */
static const struct gea12_case GEA12_CASES[] = {
	{
		.name = "GEA1 zero key vector",
		.control = 0,
		.key = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.input = 0x00000000,
		.expected = {
			0x1F, 0xA1, 0x98, 0xAB, 0x21, 0x14, 0xC3, 0x8A, 0x9E, 0xBC, 0xCB, 0x63, 0xAD, 0x48, 0x13, 0xA7,
			0x40, 0xC1,
		},
	},
	{
		.name = "GEA1 downlink vector",
		.control = GPRSCU_CON_DIRECTION,
		.key = { 0x55, 0xE3, 0x03, 0xEB, 0x7D, 0x55, 0xB6, 0x85 },
		.input = 0xDA637A83,
		.plaintext = {
			0x6E, 0x00, 0xCF, 0xE7, 0xB7, 0xFB, 0x97, 0x48, 0x92, 0xB8, 0xCD, 0xE5, 0xE4, 0x33, 0x63, 0x39,
			0x7D, 0x85,
		},
		.expected = {
			0x58, 0xDA, 0xD0, 0x64, 0x57, 0xB9, 0xFE, 0x10, 0x15, 0xDA, 0x07, 0x76, 0xED, 0x19, 0x90, 0x7B,
			0x78, 0x88,
		},
	},
	{
		.name = "GEA1 uplink vector",
		.control = 0,
		.key = { 0xA7, 0x26, 0x5D, 0x19, 0x32, 0xA0, 0xD6, 0x18 },
		.input = 0x0E9B8ADF,
		.plaintext = {
			0x96, 0xE7, 0xB1, 0xD9, 0x2B, 0x1E, 0xA8, 0xFC, 0xDD, 0xA4, 0x12, 0x33, 0xC6, 0x32, 0x94, 0x05,
			0x53, 0x83,
		},
		.expected = {
			0xD7, 0x21, 0x97, 0xF6, 0x5D, 0x4D, 0x67, 0xB1, 0x4D, 0x2C, 0xEE, 0x81, 0x2C, 0xB0, 0xB9, 0xBE,
			0xA0, 0xC9,
		},
	},
	{
		.name = "GEA2 zero key vector",
		.control = GPRSCU_CON_GEA2,
		.key = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		.input = 0x00000000,
		.expected = {
			0x04, 0x51, 0x15, 0xD5, 0xE5, 0xA2, 0xD6, 0x25, 0x41, 0xDA, 0x07, 0x8B, 0x18, 0xBA, 0xA5, 0x3F,
			0xFE, 0x14,
		},
	},
	{
		.name = "GEA2 downlink vector",
		.control = GPRSCU_CON_GEA2 | GPRSCU_CON_DIRECTION,
		.key = { 0xB1, 0x0F, 0x38, 0x9B, 0x78, 0xA6, 0x16, 0x48 },
		.input = 0x24C05B01,
		.plaintext = {
			0xEA, 0xBF, 0x6D, 0x3C, 0x6B, 0xA5, 0xDB, 0xF7, 0x6E, 0xBB, 0x3C, 0x4C, 0x0A, 0xC0, 0x24, 0x0C,
			0xB0, 0xAB,
		},
		.expected = {
			0x51, 0x56, 0x56, 0x9D, 0x2A, 0xB9, 0x82, 0x57, 0xBE, 0x1A, 0x37, 0xD6, 0x0D, 0xDF, 0x07, 0xAE,
			0x90, 0x75,
		},
	},
	{
		.name = "GEA2 uplink vector",
		.control = GPRSCU_CON_GEA2,
		.key = { 0x0C, 0x34, 0xB2, 0x94, 0x0A, 0x97, 0x07, 0xFD },
		.input = 0xF59CC96A,
		.plaintext = {
			0xF9, 0x37, 0x3D, 0xE5, 0x2E, 0xA6, 0x2C, 0x49, 0x06, 0x97, 0x11, 0xE8, 0x33, 0x89, 0xD0, 0x37,
			0xFC, 0x17,
		},
		.expected = {
			0x50, 0x9C, 0x19, 0xB7, 0x8D, 0x1D, 0x4C, 0xEB, 0x49, 0xC3, 0xB1, 0xF4, 0x3D, 0xF0, 0x14, 0xF7,
			0x4C, 0xDA,
		},
	},
};

static void gprscu_initialize(uint32_t control) {
	GPRSCU_CON = control | GPRSCU_CON_INIT;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_CON & GPRSCU_CON_BUSY) != 0; attempt++)
		test_watchdog_serve();
}

static void gprscu_wait_output_count(uint32_t count) {
	for (uint32_t attempt = 0;
		attempt < 10000 && (GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT) != count;
		attempt++)
	{
		test_watchdog_serve();
	}
}

static bool gprscu_wait_output_available(void) {
	for (uint32_t attempt = 0; attempt < 100000; attempt++) {
		if ((GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT) != 0)
			return true;
		test_watchdog_serve();
	}
	return (GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT) != 0;
}

static void gprscu_segment_completion_clear(void) {
	GPRSCU_SRC(1) = MOD_SRC_CLRR;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_SRC(1) & MOD_SRC_SRR) != 0; attempt++)
		test_watchdog_serve();
}

static bool gprscu_segment_completion_wait(void) {
	for (uint32_t attempt = 0; attempt < 10000; attempt++) {
		if ((GPRSCU_SRC(1) & MOD_SRC_SRR) != 0)
			return true;
		test_watchdog_serve();
	}
	return (GPRSCU_SRC(1) & MOD_SRC_SRR) != 0;
}

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
	gprscu_initialize(0);
	test_eq_u32("initialization command finishes", 0, GPRSCU_CON & (GPRSCU_CON_BUSY | GPRSCU_CON_INIT));
	test_eq_u32(
		"input FIFO has 32 entries free",
		32,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT
	);
	test_eq_u32("output FIFO starts empty", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);

	GPRSCU_DATA_BYTE = 0xA5;
	gprscu_wait_output_count(1);
	test_eq_u32("one output byte is available", 1, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("disabled CRC and cipher pass data through", 0xA5, GPRSCU_DATA_BYTE);
	test_eq_u32("reading the byte empties the output FIFO", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("valid FIFO access does not underrun", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_UNDERRUN);

	GPRSCU_CON = 0;
}

static void test_access_widths(void) {
	gprscu_initialize(0);
	GPRSCU_DATA_BYTE = 0xA5;
	gprscu_wait_output_count(1);
	test_eq_u32("byte write occupies one FIFO entry", 1, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("byte access preserves data", 0xA5, GPRSCU_DATA_BYTE);

	gprscu_initialize(0);
	GPRSCU_DATA_HALFWORD = 0xC39A;
	gprscu_wait_output_count(1);
	test_eq_u32("halfword write occupies one FIFO entry", 1, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("halfword access preserves byte order", 0xC39A, GPRSCU_DATA_HALFWORD);

	gprscu_initialize(0);
	GPRSCU_DATA = 0x78563412;
	gprscu_wait_output_count(1);
	test_eq_u32("word write occupies one FIFO entry", 1, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("word access preserves byte order", 0x78563412, GPRSCU_DATA);
}

static void test_fifo_boundaries(void) {
	uint32_t expected[GPRSCU_FIFO_ENTRIES];
	uint32_t actual[GPRSCU_FIFO_ENTRIES];

	gprscu_initialize(0);
	for (size_t index = 0; index < ARRAY_SIZE(expected); index++) {
		expected[index] = 0xA5000000 | index;
		GPRSCU_DATA = expected[index];
	}
	gprscu_wait_output_count(GPRSCU_FIFO_ENTRIES);
	test_eq_u32("output FIFO contains 32 entries", GPRSCU_FIFO_ENTRIES,
		GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("all 32 input slots are occupied", 0,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);

	GPRSCU_DATA = 0xDEADBEEF;
	test_eq_u32("write past the full FIFO is ignored", 0,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);

	for (size_t index = 0; index < ARRAY_SIZE(actual); index++)
		actual[index] = GPRSCU_DATA;
	test_eq_memory("FIFO preserves 32-entry order and rejects overflow", expected, actual, sizeof(expected));
	test_eq_u32("output FIFO is empty after drain", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("input FIFO has 32 entries free after drain", GPRSCU_FIFO_ENTRIES,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);

	test_eq_u32("empty FIFO read returns the last output word", expected[ARRAY_SIZE(expected) - 1], GPRSCU_DATA);
	test_eq_u32("empty FIFO read sets underrun", GPRSCU_STAT_OUTPUT_UNDERRUN,
		GPRSCU_STAT & GPRSCU_STAT_OUTPUT_UNDERRUN);
	GPRSCU_STAT = GPRSCU_STAT_OUTPUT_UNDERRUN;
	test_eq_u32("underrun is write-one-to-clear", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_UNDERRUN);
}

static void test_fifo_wraparound(void) {
	uint32_t expected[GPRSCU_FIFO_ENTRIES * 3];
	uint32_t actual[GPRSCU_FIFO_ENTRIES * 3];
	size_t write_index = 0;
	size_t read_index = 0;

	gprscu_initialize(0);
	for (uint32_t round = 0; round < 2; round++) {
		for (uint32_t index = 0; index < 24; index++) {
			expected[write_index] = 0x5A000000 | write_index;
			GPRSCU_DATA = expected[write_index++];
		}
		gprscu_wait_output_count(24);
		for (uint32_t index = 0; index < 16; index++)
			actual[read_index++] = GPRSCU_DATA;

		if (round == 1) {
			test_eq_u32("interleaved read after wrapping leaves eight output entries", 8,
				GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
			test_eq_u32("interleaved read after wrapping frees 24 input entries", 24,
				(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);
		}

		for (uint32_t index = 0; index < 24; index++) {
			expected[write_index] = 0x5A000000 | write_index;
			GPRSCU_DATA = expected[write_index++];
		}
		gprscu_wait_output_count(GPRSCU_FIFO_ENTRIES);
		for (uint32_t index = 0; index < GPRSCU_FIFO_ENTRIES; index++)
			actual[read_index++] = GPRSCU_DATA;
	}

	test_eq_memory("FIFO preserves order through three pointer wraps", expected, actual, sizeof(expected));
	test_eq_u32("wrapped output FIFO finishes empty", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("wrapped input FIFO finishes fully free", GPRSCU_FIFO_ENTRIES,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);
}

static void test_initialize_pending_fifo(void) {
	static const uint32_t EXPECTED[] = { 0xBAD00000, 0xBAD00001, 0xBAD00002, 0x13579BDF };
	uint32_t actual[ARRAY_SIZE(EXPECTED)];

	gprscu_initialize(0);
	(void) GPRSCU_DATA;
	test_eq_u32("empty read sets underrun before initialization", GPRSCU_STAT_OUTPUT_UNDERRUN,
		GPRSCU_STAT & GPRSCU_STAT_OUTPUT_UNDERRUN);
	for (uint32_t index = 0; index < 3; index++)
		GPRSCU_DATA = 0xBAD00000 | index;
	gprscu_wait_output_count(3);
	test_eq_u32("pending FIFO has three output entries", 3, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("pending FIFO has 29 input entries free", 29,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);

	gprscu_initialize(0);
	test_eq_u32("initialization preserves pending output", 3, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("initialization preserves occupied input entries", 29,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);
	test_eq_u32("initialization preserves prior underrun", GPRSCU_STAT_OUTPUT_UNDERRUN,
		GPRSCU_STAT & GPRSCU_STAT_OUTPUT_UNDERRUN);

	GPRSCU_DATA = EXPECTED[3];
	gprscu_wait_output_count(ARRAY_SIZE(EXPECTED));
	for (size_t index = 0; index < ARRAY_SIZE(actual); index++)
		actual[index] = GPRSCU_DATA;
	test_eq_memory("initialization preserves pending FIFO order", EXPECTED, actual, sizeof(EXPECTED));
	test_eq_u32("preserved output FIFO drains completely", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("preserved input FIFO becomes fully free", GPRSCU_FIFO_ENTRIES,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);
}

static uint32_t gprscu_crc24(
	const uint8_t *data,
	size_t size,
	const uint16_t *segments,
	size_t segment_count
) {
	gprscu_initialize(GPRSCU_CON_SEGMENT_MODE);
	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	for (size_t index = 0; index < segment_count; index++)
		GPRSCU_SEGMENT = segments[index] | GPRSCU_SEGMENT_CRC_CTRL;
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	gprscu_segment_completion_clear();

	for (size_t index = 0; index < size; index++)
		GPRSCU_DATA_BYTE = data[index];
	test_eq_u32("buffered segment data leaves SRC1 clear before start", 0,
		GPRSCU_SRC(1) & MOD_SRC_SRR);

	GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_SRC(1) & MOD_SRC_SRR) == 0; attempt++)
		test_watchdog_serve();
	test_eq_u32("buffered segment start raises SRC1", MOD_SRC_SRR, GPRSCU_SRC(1) & MOD_SRC_SRR);
	test_eq_u32("segment start clears on completion", 0, GPRSCU_CON & GPRSCU_CON_SEGMENT_START);
	for (size_t index = 0; index < size; index++)
		(void) GPRSCU_DATA_BYTE;

	return GPRSCU_FCS;
}

static struct gprscu_pattern_result gprscu_crc24_pattern(size_t size, const char *data_name) {
	bool data_matches = true;

	gprscu_initialize(GPRSCU_CON_SEGMENT_MODE);
	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_SEGMENT = size | GPRSCU_SEGMENT_CRC_CTRL;
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	gprscu_segment_completion_clear();

	for (size_t index = 0; index < size; index++) {
		uint8_t value = (index * 73U + 41U) & 0xFFU;
		GPRSCU_DATA_BYTE = value;
		gprscu_wait_output_count(1);
		if (GPRSCU_DATA_BYTE != value)
			data_matches = false;
		if ((index & 0xFFU) == 0)
			test_watchdog_serve();
	}
	test_check(data_name, data_matches);
	uint32_t source_before_start = GPRSCU_SRC(1) & MOD_SRC_SRR;

	GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_CON & GPRSCU_CON_SEGMENT_START) != 0; attempt++)
		test_watchdog_serve();
	test_eq_u32("streamed segment start clears on completion", 0, GPRSCU_CON & GPRSCU_CON_SEGMENT_START);

	return (struct gprscu_pattern_result) {
		.fcs = GPRSCU_FCS,
		.source_before_start = source_before_start,
		.source_after_start = GPRSCU_SRC(1) & MOD_SRC_SRR,
	};
}

static void test_llc_fcs(void) {
	static const uint8_t ONE_BYTE[] = { 0xA5 };
	static const uint8_t ODD_LENGTH[] = { 0x01, 0x02, 0x03 };
	static const uint8_t CHECK_STRING[] = "123456789";
	static const uint8_t MULTI_SEGMENT[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
	static const uint16_t ONE_BYTE_SEGMENTS[] = { 1 };
	static const uint16_t ODD_LENGTH_SEGMENTS[] = { 3 };
	static const uint16_t CHECK_STRING_SEGMENTS[] = { 9 };
	static const uint16_t SPLIT_CHECK_STRING_SEGMENTS[] = { 4, 5 };
	static const uint16_t MULTI_SEGMENTS[] = { 2, 3, 5 };

	/* FCS[31:8] is CRC-24/LLC: init=FFFFFF, reflected polynomial=AD85DD, no final XOR. */
	test_eq_u32("LLC CRC-24 one-byte result", 0x51E728A5,
		gprscu_crc24(ONE_BYTE, sizeof(ONE_BYTE), ONE_BYTE_SEGMENTS, ARRAY_SIZE(ONE_BYTE_SEGMENTS)));
	test_eq_u32("LLC CRC-24 odd-length result", 0x3D002C03,
		gprscu_crc24(ODD_LENGTH, sizeof(ODD_LENGTH), ODD_LENGTH_SEGMENTS, ARRAY_SIZE(ODD_LENGTH_SEGMENTS)));
	test_eq_u32("LLC CRC-24 check string result", 0xB1793439,
		gprscu_crc24(CHECK_STRING, sizeof(CHECK_STRING) - 1, CHECK_STRING_SEGMENTS,
			ARRAY_SIZE(CHECK_STRING_SEGMENTS)));
	test_eq_u32("LLC CRC-24 continues across segment boundary", 0xB1793439,
		gprscu_crc24(CHECK_STRING, sizeof(CHECK_STRING) - 1, SPLIT_CHECK_STRING_SEGMENTS,
			ARRAY_SIZE(SPLIT_CHECK_STRING_SEGMENTS)));
	test_eq_u32("LLC CRC-24 continues across three unequal segments", 0x835D4409,
		gprscu_crc24(MULTI_SEGMENT, sizeof(MULTI_SEGMENT), MULTI_SEGMENTS, ARRAY_SIZE(MULTI_SEGMENTS)));
}

static void test_control_crc(void) {
	/* "12345678" as halfwords with the first byte in the low half, then "9" as a byte. */
	static const uint16_t CHECK_STRING_HALFWORDS[] = { 0x3231, 0x3433, 0x3635, 0x3837 };
	static const uint8_t CHECK_STRING_TAIL = '9';
	size_t entries = ARRAY_SIZE(CHECK_STRING_HALFWORDS) + 1;

	gprscu_initialize(0);
	GPRSCU_CON = GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	GPRSCU_CON = GPRSCU_CON_CRC_CTRL;
	for (size_t index = 0; index < ARRAY_SIZE(CHECK_STRING_HALFWORDS); index++)
		GPRSCU_DATA_HALFWORD = CHECK_STRING_HALFWORDS[index];
	GPRSCU_DATA_BYTE = CHECK_STRING_TAIL;
	gprscu_wait_output_count(entries);
	GPRSCU_CON = 0;

	test_eq_u32("control CRC covers every byte of halfword writes without descriptors", 0xB1793439, GPRSCU_FCS);
	for (size_t index = 0; index < entries; index++)
		(void) GPRSCU_DATA;
}

static void test_segment_lengths(void) {
	static const uint16_t FULL_FIFO_SEGMENT[] = { GPRSCU_FIFO_ENTRIES };
	uint8_t buffered[GPRSCU_FIFO_ENTRIES];
	for (size_t index = 0; index < ARRAY_SIZE(buffered); index++)
		buffered[index] = (index * 73U + 41U) & 0xFFU;
	test_eq_u32("LLC CRC-24 buffered 32-byte segment result", 0x7D6BC700,
		gprscu_crc24(buffered, sizeof(buffered), FULL_FIFO_SEGMENT, ARRAY_SIZE(FULL_FIFO_SEGMENT)));

	struct gprscu_pattern_result result = gprscu_crc24_pattern(
		31, "31-byte segment streams without FIFO backpressure");
	test_eq_u32("31 streamed bytes leave SRC1 clear", 0, result.source_before_start);
	test_eq_u32("drained 31-byte segment start leaves SRC1 clear", 0, result.source_after_start);
	test_eq_u32("LLC CRC-24 31-byte segment result", 0xC85EA0B7, result.fcs);

	result = gprscu_crc24_pattern(32, "32-byte segment streams without FIFO backpressure");
	test_eq_u32("32nd streamed byte raises SRC1", MOD_SRC_SRR, result.source_before_start);
	test_eq_u32("LLC CRC-24 32-byte segment result", 0x7D6BC700, result.fcs);

	result = gprscu_crc24_pattern(33, "33-byte segment streams across the FIFO boundary");
	test_eq_u32("SRC1 remains raised after 33 streamed bytes", MOD_SRC_SRR, result.source_before_start);
	test_eq_u32("LLC CRC-24 33-byte segment result", 0xD8356549, result.fcs);

	result = gprscu_crc24_pattern(
		GPRSCU_SEGMENT_LENGTH, "2047-byte maximum segment streams completely");
	test_eq_u32("maximum streamed segment raises SRC1", MOD_SRC_SRR, result.source_before_start);
	test_eq_u32("LLC CRC-24 maximum-length segment result", 0x351C6997, result.fcs);
}

static void test_segment_reset(void) {
	static const uint8_t ACTIVE_PREFIX[] = { 0xD1, 0xE2 };
	static const uint8_t ACTIVE_TAIL[] = { 0x01, 0x02, 0x03 };
	static const uint8_t RESTARTED[] = { 0x11, 0x22, 0x33 };

	gprscu_initialize(GPRSCU_CON_SEGMENT_MODE);
	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_SEGMENT = 5;
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	gprscu_segment_completion_clear();
	for (size_t index = 0; index < ARRAY_SIZE(ACTIVE_PREFIX); index++) {
		GPRSCU_DATA_BYTE = ACTIVE_PREFIX[index];
		gprscu_wait_output_count(1);
		test_eq_u32("partial segment passes data through", ACTIVE_PREFIX[index], GPRSCU_DATA_BYTE);
	}

	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_SEGMENT = ARRAY_SIZE(RESTARTED) | GPRSCU_SEGMENT_CRC_CTRL;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	for (size_t index = 0; index < ARRAY_SIZE(ACTIVE_TAIL); index++) {
		GPRSCU_DATA_BYTE = ACTIVE_TAIL[index];
		gprscu_wait_output_count(1);
		test_eq_u32("active segment tail passes data through", ACTIVE_TAIL[index], GPRSCU_DATA_BYTE);
	}
	test_eq_u32("segment reset preserves the active descriptor", GPRSCU_LLC_FCS_INIT, GPRSCU_FCS);

	for (size_t index = 0; index < ARRAY_SIZE(RESTARTED); index++) {
		GPRSCU_DATA_BYTE = RESTARTED[index];
		gprscu_wait_output_count(1);
		test_eq_u32("restarted segment passes data through", RESTARTED[index], GPRSCU_DATA_BYTE);
	}
	GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_CON & GPRSCU_CON_SEGMENT_START) != 0; attempt++)
		test_watchdog_serve();
	test_eq_u32("restarted segment completes", 0, GPRSCU_CON & GPRSCU_CON_SEGMENT_START);
	test_eq_u32("descriptor after reset starts with clean CRC state", 0xC9054133, GPRSCU_FCS);
}

static void test_segment_reset_pending_fifo(void) {
	static const uint8_t PREFIX[] = { 0xD1, 0xE2 };
	static const uint8_t TAIL[] = { 0x01, 0x02 };
	uint8_t actual_prefix[ARRAY_SIZE(PREFIX)];
	uint8_t actual_tail[ARRAY_SIZE(TAIL)];

	gprscu_initialize(GPRSCU_CON_SEGMENT_MODE);
	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_SEGMENT = (ARRAY_SIZE(PREFIX) + ARRAY_SIZE(TAIL)) | GPRSCU_SEGMENT_CRC_CTRL;
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	for (size_t index = 0; index < ARRAY_SIZE(PREFIX); index++)
		GPRSCU_DATA_BYTE = PREFIX[index];
	gprscu_wait_output_count(ARRAY_SIZE(PREFIX));

	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	test_eq_u32("segment reset preserves pending output entries", ARRAY_SIZE(PREFIX),
		GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("segment reset preserves occupied input entries", GPRSCU_FIFO_ENTRIES - ARRAY_SIZE(PREFIX),
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);
	for (size_t index = 0; index < ARRAY_SIZE(PREFIX); index++)
		actual_prefix[index] = GPRSCU_DATA_BYTE;
	test_eq_memory("pending prefix survives segment reset", PREFIX, actual_prefix, sizeof(PREFIX));

	for (size_t index = 0; index < ARRAY_SIZE(TAIL); index++) {
		GPRSCU_DATA_BYTE = TAIL[index];
		gprscu_wait_output_count(1);
		actual_tail[index] = GPRSCU_DATA_BYTE;
	}
	test_eq_memory("active descriptor tail survives segment reset", TAIL, actual_tail, sizeof(TAIL));
	test_eq_u32("active descriptor CRC continues through pending FIFO reset", 0x40EAEB02, GPRSCU_FCS);
	GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_CON & GPRSCU_CON_SEGMENT_START) != 0; attempt++)
		test_watchdog_serve();
	test_eq_u32("reset active descriptor completes after its pending data drains", 0,
		GPRSCU_CON & GPRSCU_CON_SEGMENT_START);
}

static void test_segment_completion_interrupt(void) {
	static const uint8_t DATA[] = { 0x11, 0x22, 0x33 };

	gprscu_initialize(GPRSCU_CON_SEGMENT_MODE);
	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_SEGMENT = sizeof(DATA) | GPRSCU_SEGMENT_CRC_CTRL;
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	GPRSCU_SRC(0) = MOD_SRC_CLRR;
	gprscu_segment_completion_clear();

	for (size_t index = 0; index < ARRAY_SIZE(DATA); index++)
		GPRSCU_DATA_BYTE = DATA[index];
	test_eq_u32("buffering a complete descriptor leaves SRC0 clear", 0, GPRSCU_SRC(0) & MOD_SRC_SRR);
	test_eq_u32("buffering the final descriptor byte raises SRC1", MOD_SRC_SRR,
		GPRSCU_SRC(1) & MOD_SRC_SRR);

	GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;
	test_check("segment completion raises SRC1", gprscu_segment_completion_wait());
	test_eq_u32("segment completion leaves SRC0 clear", 0, GPRSCU_SRC(0) & MOD_SRC_SRR);
	test_eq_u32("segment completion clears the start command", 0, GPRSCU_CON & GPRSCU_CON_SEGMENT_START);

	gprscu_segment_completion_clear();
	test_eq_u32("SRC1 completion request acknowledges with CLRR", 0, GPRSCU_SRC(1) & MOD_SRC_SRR);
	for (uint32_t attempt = 0; attempt < 1000; attempt++)
		test_watchdog_serve();
	test_eq_u32("acknowledged completion request remains clear", 0, GPRSCU_SRC(1) & MOD_SRC_SRR);

	for (size_t index = 0; index < ARRAY_SIZE(DATA); index++)
		(void) GPRSCU_DATA_BYTE;
}

static void test_descriptor_transition(void) {
	static const uint8_t FIRST[] = { 0x11, 0x22 };
	static const uint8_t SECOND[] = { 0x33, 0x44, 0x55 };

	gprscu_initialize(GPRSCU_CON_SEGMENT_MODE);
	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_SEGMENT = sizeof(FIRST) | GPRSCU_SEGMENT_CRC_CTRL;
	GPRSCU_SEGMENT = sizeof(SECOND) | GPRSCU_SEGMENT_CRC_CTRL;
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	gprscu_segment_completion_clear();

	for (size_t index = 0; index < ARRAY_SIZE(FIRST); index++) {
		GPRSCU_DATA_BYTE = FIRST[index];
		gprscu_wait_output_count(1);
		(void) GPRSCU_DATA_BYTE;
	}
	test_eq_u32("first descriptor boundary preserves its CRC accumulator", 0xCAD15A22, GPRSCU_FCS);
	test_eq_u32("intermediate descriptor boundary leaves SRC1 clear", 0, GPRSCU_SRC(1) & MOD_SRC_SRR);

	for (size_t index = 0; index < ARRAY_SIZE(SECOND); index++) {
		GPRSCU_DATA_BYTE = SECOND[index];
		gprscu_wait_output_count(1);
		(void) GPRSCU_DATA_BYTE;
	}
	test_eq_u32("second descriptor continues the first CRC accumulator", 0x98B6A855, GPRSCU_FCS);
	test_eq_u32("drained descriptor list leaves SRC1 clear before start", 0, GPRSCU_SRC(1) & MOD_SRC_SRR);

	GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;
	for (uint32_t attempt = 0; attempt < 10000 && (GPRSCU_CON & GPRSCU_CON_SEGMENT_START) != 0; attempt++)
		test_watchdog_serve();
	test_eq_u32("drained descriptor list clears the start command", 0,
		GPRSCU_CON & GPRSCU_CON_SEGMENT_START);
	test_eq_u32("drained descriptor list does not raise SRC1", 0, GPRSCU_SRC(1) & MOD_SRC_SRR);
}

static void test_gea3_case(const struct gea3_case *test_case) {
	uint8_t actual[ARRAY_SIZE(test_case->expected)];
	size_t written = 0;
	size_t received = 0;

	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_SEGMENT = sizeof(actual) | test_case->segment_control;
	GPRSCU_INPUT(0) = test_case->input;
	GPRSCU_INPUT(1) = test_case->input_suffix;
	GPRSCU_KEY(0) = test_case->key[0];
	GPRSCU_KEY(1) = test_case->key[1];
	GPRSCU_KEY(2) = test_case->key[0];
	GPRSCU_KEY(3) = test_case->key[1];
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	gprscu_initialize(test_case->control);
	test_eq_u32("GEA3 initialization completes", 0, GPRSCU_CON & (GPRSCU_CON_BUSY | GPRSCU_CON_INIT));

	while (written < GPRSCU_FIFO_ENTRIES) {
		GPRSCU_DATA_BYTE = 0;
		written++;
	}
	test_eq_u32("GEA3 preload fills the input FIFO", 0,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);
	GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;

	while (received < ARRAY_SIZE(actual) && gprscu_wait_output_available()) {
		actual[received] = GPRSCU_DATA_BYTE;
		if (received == 0) {
			test_eq_u32("GEA3 start command self-clears after acceptance", 0,
				GPRSCU_CON & GPRSCU_CON_SEGMENT_START);
			GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;
		}
		received++;
		if (written < ARRAY_SIZE(actual)) {
			GPRSCU_DATA_BYTE = 0;
			written++;
		}
	}

	if (!test_check("GEA3 produces every output byte after a repeated start", received == sizeof(actual)))
		return;
	test_eq_memory(test_case->name, test_case->expected, actual, sizeof(actual));
	if ((test_case->segment_control & GPRSCU_SEGMENT_CRC_CTRL) != 0)
		test_eq_u32("GEA3 cipher and CRC share the descriptor data stream", test_case->expected_fcs, GPRSCU_FCS);
	test_eq_u32("completed GEA3 descriptor clears the start command", 0,
		GPRSCU_CON & GPRSCU_CON_SEGMENT_START);
	test_eq_u32("GEA3 transfer drains the output FIFO", 0, GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT);
	test_eq_u32("GEA3 transfer releases every input FIFO entry", GPRSCU_FIFO_ENTRIES,
		(GPRSCU_STAT & GPRSCU_STAT_INPUT_FREE) >> GPRSCU_STAT_INPUT_FREE_SHIFT);
}

static uint32_t gprscu_key_word(const uint8_t *key) {
	return key[0] | (key[1] << 8) | (key[2] << 16) | ((uint32_t) key[3] << 24);
}

static void test_gea12_case(const struct gea12_case *test_case) {
	uint8_t actual[GEA12_VECTOR_BYTES];
	size_t received = 0;

	GPRSCU_CON = GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_SEGMENT = GEA12_VECTOR_BYTES | GPRSCU_SEGMENT_CIPH_CTRL;
	GPRSCU_INPUT(0) = test_case->input;
	GPRSCU_KEY(0) = gprscu_key_word(&test_case->key[0]);
	GPRSCU_KEY(1) = gprscu_key_word(&test_case->key[4]);
	gprscu_initialize(test_case->control | GPRSCU_CON_SEGMENT_MODE | GPRSCU_CON_CIPH_CTRL);

	for (size_t index = 0; index < GEA12_VECTOR_BYTES; index++)
		GPRSCU_DATA_BYTE = test_case->plaintext[index];
	GPRSCU_CON |= GPRSCU_CON_SEGMENT_START;
	while (received < GEA12_VECTOR_BYTES && gprscu_wait_output_available())
		actual[received++] = GPRSCU_DATA_BYTE;

	if (!test_check("GEA1/2 produces every output byte", received == GEA12_VECTOR_BYTES))
		return;
	test_eq_memory(test_case->name, test_case->expected, actual, sizeof(actual));
}

static uint32_t gprscu_lockstep_status_reads;

static uint16_t gprscu_lockstep(uint16_t value, bool halfword) {
	uint32_t status_reads = 1;

	if (halfword) {
		GPRSCU_DATA_HALFWORD = value;
	} else {
		GPRSCU_DATA_BYTE = value;
	}
	while ((GPRSCU_STAT & GPRSCU_STAT_OUTPUT_COUNT) == 0 && status_reads < 100000)
		status_reads++;
	gprscu_lockstep_status_reads = MAX(gprscu_lockstep_status_reads, status_reads);
	return halfword ? GPRSCU_DATA_HALFWORD : GPRSCU_DATA_BYTE;
}

static void gprscu_lockstep_bytes(const uint8_t *input, uint8_t *output, size_t size) {
	size_t index = 0;

	for (; index + 1 < size; index += 2) {
		uint16_t value = gprscu_lockstep(input[index] | (input[index + 1] << 8), true);
		output[index] = value;
		output[index + 1] = value >> 8;
	}
	if (index < size)
		output[index] = gprscu_lockstep(input[index], false);
}

static void gprscu_configure_control(const uint8_t *key, uint32_t input, uint32_t control) {
	GPRSCU_CON = GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_KEY(0) = gprscu_key_word(&key[0]);
	GPRSCU_KEY(1) = gprscu_key_word(&key[4]);
	GPRSCU_INPUT(0) = input;
	gprscu_initialize(control);
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
}

static void test_control_cipher_frame(void) {
	/* LLC UI header, the first 15 bytes of the GEA1 uplink vector, then its complemented FCS. */
	static const uint8_t HEADER[] = { 0x01, 0xC0 };
	static const uint8_t EXPECTED_FCS_OUTPUT[] = { 0xD4, 0xA5, 0xC1 };
	const struct gea12_case *vector = &GEA12_CASES[2];
	size_t body_size = GEA12_VECTOR_BYTES - ARRAY_SIZE(EXPECTED_FCS_OUTPUT);
	uint8_t header[ARRAY_SIZE(HEADER)];
	uint8_t body[GEA12_VECTOR_BYTES];
	uint8_t fcs_output[ARRAY_SIZE(EXPECTED_FCS_OUTPUT)];

	gprscu_lockstep_status_reads = 0;
	gprscu_configure_control(vector->key, vector->input, 0);
	GPRSCU_CON |= GPRSCU_CON_CRC_CTRL;
	gprscu_lockstep_bytes(HEADER, header, sizeof(header));
	GPRSCU_CON |= GPRSCU_CON_CIPH_CTRL;
	gprscu_lockstep_bytes(vector->plaintext, body, body_size);
	uint32_t fcs = GPRSCU_FCS;
	GPRSCU_CON &= ~GPRSCU_CON_CRC_CTRL;
	for (size_t index = 0; index < ARRAY_SIZE(fcs_output); index++)
		fcs_output[index] = gprscu_lockstep((~fcs >> (8 * (index + 1))) & 0xFF, false);
	GPRSCU_CON = 0;

	test_eq_memory("CRC-only header passes unciphered", HEADER, header, sizeof(header));
	test_eq_memory("control cipher starts the keystream at the first ciphered byte", vector->expected, body,
		body_size);
	test_eq_u32("control CRC covers the plaintext header and body", 0x74A99094, fcs);
	test_eq_memory("FCS bytes continue the keystream with CRC disabled", EXPECTED_FCS_OUTPUT, fcs_output,
		sizeof(fcs_output));
	test_check("each processed entry is readable within 10 status reads", gprscu_lockstep_status_reads <= 10);
	printf("# GPRSCU lockstep: max status reads %lu\n", gprscu_lockstep_status_reads);
}

static void test_control_downlink_residue(void) {
	/* The frame of test_control_cipher_frame() in plaintext with its FCS. */
	static const uint8_t FRAME[] = {
		0x01, 0xC0, 0x96, 0xE7, 0xB1, 0xD9, 0x2B, 0x1E, 0xA8, 0xFC, 0xDD, 0xA4, 0x12, 0x33, 0xC6, 0x32,
		0x94, 0x6F, 0x56, 0x8B,
	};
	uint8_t output[ARRAY_SIZE(FRAME)];

	gprscu_configure_control(GEA12_CASES[0].key, 0, GPRSCU_CON_DIRECTION);
	GPRSCU_CON = GPRSCU_CON_DIRECTION | GPRSCU_CON_CRC_CTRL;
	gprscu_lockstep_bytes(FRAME, output, sizeof(output));
	GPRSCU_CON = GPRSCU_CON_DIRECTION;

	test_eq_u32("a frame with a valid FCS leaves the CRC-24/LLC residue", 0x0C91B600, GPRSCU_FCS & 0xFFFFFF00);
}

static void test_control_downlink_cipher(void) {
	/* LLC UI header, the first 15 bytes of the GEA1 downlink vector and its FCS, as received and in plaintext. */
	static const uint8_t RECEIVED[] = {
		0x01, 0xC0, 0x58, 0xDA, 0xD0, 0x64, 0x57, 0xB9, 0xFE, 0x10, 0x15, 0xDA, 0x07, 0x76, 0xED, 0x19,
		0x90, 0xEC, 0x97, 0x55,
	};
	static const uint8_t PLAINTEXT[] = {
		0x01, 0xC0, 0x6E, 0x00, 0xCF, 0xE7, 0xB7, 0xFB, 0x97, 0x48, 0x92, 0xB8, 0xCD, 0xE5, 0xE4, 0x33,
		0x63, 0xAE, 0x92, 0x58,
	};
	static const size_t HEADER_SIZE = 2;
	const struct gea12_case *vector = &GEA12_CASES[1];
	uint8_t output[ARRAY_SIZE(RECEIVED)];

	gprscu_configure_control(vector->key, vector->input, GPRSCU_CON_DIRECTION);
	GPRSCU_CON = GPRSCU_CON_DIRECTION | GPRSCU_CON_CRC_CTRL;
	gprscu_lockstep_bytes(RECEIVED, output, HEADER_SIZE);
	GPRSCU_CON |= GPRSCU_CON_CIPH_CTRL;
	gprscu_lockstep_bytes(&RECEIVED[HEADER_SIZE], &output[HEADER_SIZE], sizeof(RECEIVED) - HEADER_SIZE);
	GPRSCU_CON = GPRSCU_CON_DIRECTION;

	test_eq_memory("downlink control cipher decrypts the body and FCS", PLAINTEXT, output, sizeof(output));
	test_eq_u32("downlink CRC covers the decrypted frame", 0x0C91B600, GPRSCU_FCS & 0xFFFFFF00);
}

static void test_control_gea3(void) {
	const struct gea3_case *vector = &GEA3_CASES[0];
	uint8_t zeros[ARRAY_SIZE(vector->expected)] = { 0 };
	uint8_t actual[ARRAY_SIZE(vector->expected)];

	GPRSCU_CON = GPRSCU_CON_SEGMENT_RESET;
	GPRSCU_INPUT(0) = vector->input;
	GPRSCU_INPUT(1) = vector->input_suffix;
	GPRSCU_KEY(0) = vector->key[0];
	GPRSCU_KEY(1) = vector->key[1];
	GPRSCU_KEY(2) = vector->key[0];
	GPRSCU_KEY(3) = vector->key[1];
	gprscu_initialize(GPRSCU_CON_GEA3);
	GPRSCU_POLYNOM = GPRSCU_LLC_POLYNOM;
	GPRSCU_FCS = GPRSCU_LLC_FCS_INIT;
	GPRSCU_CON = GPRSCU_CON_GEA3 | GPRSCU_CON_CRC_CTRL | GPRSCU_CON_CIPH_CTRL;
	gprscu_lockstep_bytes(zeros, actual, sizeof(actual));
	GPRSCU_CON = GPRSCU_CON_GEA3;

	test_eq_memory("control-register GEA3 matches the uplink vector", vector->expected, actual, sizeof(actual));
	test_eq_u32("control-register GEA3 CRC covers the plaintext", vector->expected_fcs, GPRSCU_FCS);
}

static void test_control_gea1_after_gea3(void) {
	const struct gea12_case *vector = &GEA12_CASES[2];
	uint8_t actual[GEA12_VECTOR_BYTES];

	gprscu_configure_control(vector->key, vector->input, 0);
	GPRSCU_CON = GPRSCU_CON_CIPH_CTRL;
	gprscu_lockstep_bytes(vector->plaintext, actual, sizeof(actual));
	GPRSCU_CON = 0;

	test_eq_memory("GEA1 after GEA3 without an intermediate GEA3 reset", vector->expected, actual, sizeof(actual));
}

static void test_control_gea3_registers(void) {
	/* Keystreams of the GEA3 vector 1 and vector 3 keys on PMB8875 hardware with modified registers. */
	static const struct {
		const char *name;
		uint32_t input;
		uint32_t input_suffix;
		uint32_t key[4];
		uint8_t expected[16];
	} CASES[] = {
		{
			"KEY2 holds the third key word from the top",
			0x8E9421A3, 0x00FF0000, { 0x82C5BC00, 0x2BD6459F, 0x00000001, 0x00000000 },
			{ 0xB2, 0xC6, 0x2E, 0x7F, 0x26, 0xC9, 0x3E, 0x0F, 0x0C, 0x10, 0x0B, 0xD3, 0xB4, 0x88, 0x50, 0x9C },
		},
		{
			"KEY3 holds the most significant key word",
			0x8E9421A3, 0x00FF0000, { 0x82C5BC00, 0x2BD6459F, 0x00000000, 0x00000001 },
			{ 0x12, 0x66, 0x0B, 0xF8, 0xC3, 0x3E, 0x33, 0xE4, 0x8A, 0xDA, 0x64, 0x3F, 0x27, 0x8E, 0xA1, 0x1F },
		},
		{
			"INPUT1 bits defined as zero still enter KGCORE",
			0x4BDBD5E5, 0x05FF0000, { 0x9E720C2A, 0xEFA8B222, 0x9E720C2A, 0xEFA8B222 },
			{ 0x03, 0x67, 0x79, 0x8A, 0x06, 0xBD, 0xB6, 0x25, 0xF2, 0xE1, 0x03, 0xF9, 0x62, 0x4E, 0x9D, 0xFD },
		},
	};
	uint8_t zeros[16] = { 0 };

	for (size_t c = 0; c < ARRAY_SIZE(CASES); c++) {
		uint8_t actual[16];

		GPRSCU_CON = GPRSCU_CON_SEGMENT_RESET;
		GPRSCU_INPUT(0) = CASES[c].input;
		GPRSCU_INPUT(1) = CASES[c].input_suffix;
		for (size_t i = 0; i < 4; i++)
			GPRSCU_KEY(i) = CASES[c].key[i];
		gprscu_initialize(GPRSCU_CON_GEA3);
		GPRSCU_CON = GPRSCU_CON_GEA3 | GPRSCU_CON_CIPH_CTRL;
		gprscu_lockstep_bytes(zeros, actual, sizeof(actual));
		GPRSCU_CON = 0;
		test_eq_memory(CASES[c].name, CASES[c].expected, actual, sizeof(actual));
	}
}

static void test_gea12(void) {
	for (size_t index = 0; index < ARRAY_SIZE(GEA12_CASES); index++) {
		test_category(GEA12_CASES[index].name);
		test_gea12_case(&GEA12_CASES[index]);
	}
}

static void test_gea3(void) {
	for (size_t index = 0; index < ARRAY_SIZE(GEA3_CASES); index++) {
		test_category(GEA3_CASES[index].name);
		test_gea3_case(&GEA3_CASES[index]);
	}
}

int main(void) {
	test_start("GPRSCU peripheral test");

	test_category("Reset values");
	test_reset_values();
	test_category("Register layout");
	test_register_layout();
	test_category("Unmodified data path");
	test_passthrough();
	test_category("FIFO access widths");
	test_access_widths();
	test_category("FIFO boundaries");
	test_fifo_boundaries();
	test_category("FIFO interleaving and wraparound");
	test_fifo_wraparound();
	test_category("Initialization with pending FIFO data");
	test_initialize_pending_fifo();
	test_category("LLC frame check sequence");
	test_llc_fcs();
	test_category("Control-register CRC without descriptors");
	test_control_crc();
	test_category("Segment length boundaries");
	test_segment_lengths();
	test_category("Segment reset and restart");
	test_segment_reset();
	test_category("Segment reset with pending FIFO data");
	test_segment_reset_pending_fifo();
	test_category("Segment completion interrupt");
	test_segment_completion_interrupt();
	test_category("Descriptor transition");
	test_descriptor_transition();
	test_gea12();
	test_gea3();
	test_category("Control-register cipher frame");
	test_control_cipher_frame();
	test_category("Control-register downlink residue");
	test_control_downlink_residue();
	test_category("Control-register downlink cipher");
	test_control_downlink_cipher();
	test_category("Control-register GEA3");
	test_control_gea3();
	test_category("Control-register GEA1 after GEA3");
	test_control_gea1_after_gea3();
	test_category("Control-register GEA3 key and input registers");
	test_control_gea3_registers();

	return test_finish();
}
