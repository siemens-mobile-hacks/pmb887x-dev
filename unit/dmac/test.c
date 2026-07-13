#include <pmb887x.h>
#include <string.h>

#include "test.h"

#define DMA_CHANNEL 7
#define DMA_TRANSFER_TIMEOUT_MS 100
#define MATRIX_BUFFER_SIZE 16384
#define MATRIX_SOURCE_ADDR 0x00084000
#define MATRIX_DESTINATION_ADDR 0x00088000
#define ENDIAN_BUFFER_SIZE 64

struct dmac_lli {
	uint32_t src;
	uint32_t dst;
	uint32_t next;
	uint32_t control;
};

struct dmac_case {
	const char *name;
	uint16_t count;
	uint16_t src_offset;
	uint16_t dst_offset;
	uint8_t src_width_bytes;
	uint32_t src_burst;
	uint32_t dst_burst;
	uint32_t src_width;
	uint32_t dst_width;
};

struct dmac_endian_case {
	const char *name;
	uint8_t count;
	uint8_t src_width_bytes;
	uint8_t dst_width_bytes;
	uint32_t src_burst;
	uint32_t src_width;
	uint32_t dst_width;
};

struct dmac_unaligned_case {
	const char *name;
	uint8_t count;
	uint8_t width_bytes;
	uint16_t src_offset;
	uint16_t dst_offset;
	uint32_t widths;
};

struct dmac_request_results {
	bool completed;
	bool cleared;
	bool data_matches;
};

static const uint32_t source[] = {
	0x10203040, 0x11213141, 0x12223242, 0x13233343,
	0x50607080, 0x51617181, 0x52627282, 0x53637383,
	0x90A0B0C0, 0x91A1B1C1, 0x92A2B2C2, 0x93A3B3C3,
	0xD0E0F000, 0xD1E1F101, 0xD2E2F202, 0xD3E3F303,
};

static volatile uint32_t destination[ARRAY_SIZE(source)] __attribute__((aligned(16)));
static uint32_t endian_source __attribute__((aligned(4)));
static volatile uint32_t endian_destination __attribute__((aligned(4)));
static uint8_t endian_matrix_source[ENDIAN_BUFFER_SIZE] __attribute__((aligned(16)));
static volatile uint8_t endian_matrix_destination[ENDIAN_BUFFER_SIZE] __attribute__((aligned(16)));
static uint8_t endian_matrix_expected[ENDIAN_BUFFER_SIZE];
static struct dmac_lli lli __attribute__((aligned(16)));
static struct dmac_lli lli_chain[3] __attribute__((aligned(16)));
static struct dmac_lli endian_lli __attribute__((aligned(16)));
static uint32_t lli_alignment_storage[7] __attribute__((aligned(16)));
static uint8_t *const matrix_source = (uint8_t *) MATRIX_SOURCE_ADDR;
static volatile uint8_t *const matrix_destination = (volatile uint8_t *) MATRIX_DESTINATION_ADDR;

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;
static volatile uint32_t irq_raw_tc_status;
static volatile uint32_t irq_tc_status;
static volatile uint32_t irq_int_status;
static volatile uint8_t irq_order[8];

static void clear_destination(void) {
	for (size_t i = 0; i < ARRAY_SIZE(destination); i++)
		destination[i] = 0xDEADBEEF;
}

static void reset_dmac(void) {
	cpu_enable_irq(false);
	DMAC_CONFIG = 0;
	for (uint32_t channel = 0; channel < 8; channel++) {
		DMAC_CH_CONFIG(channel) = 0;
		VIC_CON(VIC_DMAC_CH0_IRQ + channel) = 0;
		irq_order[channel] = 0xFF;
	}
	DMAC_TC_CLEAR = 0xFF;
	DMAC_ERR_CLEAR = 0xFF;
	irq_count = 0;
	irq_number = 0;
	irq_raw_tc_status = 0;
	irq_tc_status = 0;
	irq_int_status = 0;
}

static bool wait_for_status(volatile uint32_t *reg, uint32_t mask) {
	stopwatch_t start = stopwatch_get();

	while ((*reg & mask) == 0 && stopwatch_elapsed_ms(start) < DMA_TRANSFER_TIMEOUT_MS)
		test_watchdog_serve();

	return (*reg & mask) != 0;
}

static bool wait_for_value(volatile uint32_t *reg, uint32_t mask, uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while ((*reg & mask) != expected && stopwatch_elapsed_ms(start) < DMA_TRANSFER_TIMEOUT_MS)
		test_watchdog_serve();

	return (*reg & mask) == expected;
}

static bool wait_for_irq_count(uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while (irq_count < expected && stopwatch_elapsed_ms(start) < DMA_TRANSFER_TIMEOUT_MS)
		test_watchdog_serve();

	return irq_count == expected;
}

static void start_transfer(uint32_t src, uint32_t dst, uint32_t next, uint32_t control, uint32_t config) {
	DMAC_CH_SRC_ADDR(DMA_CHANNEL) = src;
	DMAC_CH_DST_ADDR(DMA_CHANNEL) = dst;
	DMAC_CH_LLI(DMA_CHANNEL) = next;
	DMAC_CH_CONTROL(DMA_CHANNEL) = control;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_CH_CONFIG(DMA_CHANNEL) = config | DMAC_CH_CONFIG_ENABLE;
}

static void test_mem2mem(void) {
	reset_dmac();
	clear_destination();

	start_transfer(
		(uint32_t) source,
		(uint32_t) destination,
		0,
		(
			ARRAY_SIZE(source) | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
			DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
			DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
	);

	test_check("MEM2MEM completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("MEM2MEM data", source, destination, sizeof(source));
	test_check("raw terminal count is set", (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) != 0);
	test_check("masked terminal count is clear", (DMAC_TC_STATUS & BIT(DMA_CHANNEL)) == 0);
	test_check("combined interrupt status is clear", (DMAC_INT_STATUS & BIT(DMA_CHANNEL)) == 0);
	test_check("channel is disabled", (DMAC_EN_CHAN & BIT(DMA_CHANNEL)) == 0);
	test_eq_u32(
		"source address is last item",
		(uint32_t) (source + ARRAY_SIZE(source) - 1),
		DMAC_CH_SRC_ADDR(DMA_CHANNEL)
	);
	test_eq_u32(
		"destination address is last item",
		(uint32_t) (destination + ARRAY_SIZE(destination) - 1),
		DMAC_CH_DST_ADDR(DMA_CHANNEL)
	);
	test_eq_u32("transfer count reaches zero", 0, DMAC_CH_CONTROL(DMA_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE);
	DMAC_CH_CONFIG(DMA_CHANNEL) |= DMAC_CH_CONFIG_INT_MASK_TC;
	test_check("enabling TC mask exposes pending status", (DMAC_TC_STATUS & BIT(DMA_CHANNEL)) != 0);
	test_check("pending TC appears in combined status", (DMAC_INT_STATUS & BIT(DMA_CHANNEL)) != 0);
	DMAC_CH_CONFIG(DMA_CHANNEL) &= ~DMAC_CH_CONFIG_INT_MASK_TC;
	test_check("disabling TC mask hides pending status", (DMAC_TC_STATUS & BIT(DMA_CHANNEL)) == 0);
	test_check("disabling TC mask keeps raw status", (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) != 0);

	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
	test_check("raw terminal count clears", (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0);
}

static void test_endian_transfer(
	const char *name,
	uint32_t config,
	uint32_t width,
	uint32_t count,
	uint32_t expected
) {
	reset_dmac();
	endian_source = 0x87654321;
	endian_destination = 0;
	DMAC_CH_SRC_ADDR(DMA_CHANNEL) = (uint32_t) &endian_source;
	DMAC_CH_DST_ADDR(DMA_CHANNEL) = (uint32_t) &endian_destination;
	DMAC_CH_LLI(DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(DMA_CHANNEL) = (
		count |
		DMAC_CH_CONTROL_SB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		width |
		DMAC_CH_CONTROL_S_AHB1 |
		DMAC_CH_CONTROL_D_AHB2 |
		DMAC_CH_CONTROL_SI |
		DMAC_CH_CONTROL_DI |
		DMAC_CH_CONTROL_I
	);
	DMAC_CONFIG = config | DMAC_CONFIG_ENABLE;
	DMAC_CH_CONFIG(DMA_CHANNEL) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_ENABLE;
	test_check("endian transfer completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_u32(name, expected, endian_destination);
}

static void test_endianness(void) {
	test_endian_transfer(
		"32-bit LE to LE keeps byte order",
		0,
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD,
		1,
		0x87654321
	);
	test_endian_transfer(
		"32-bit LE to BE swaps bytes",
		DMAC_CONFIG_M2_BE,
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD,
		1,
		0x21436587
	);
	test_endian_transfer(
		"32-bit BE to LE swaps bytes",
		DMAC_CONFIG_M1_BE,
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD,
		1,
		0x21436587
	);
	test_endian_transfer(
		"32-bit BE to BE keeps byte order",
		DMAC_CONFIG_M1_BE | DMAC_CONFIG_M2_BE,
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD,
		1,
		0x87654321
	);
	test_endian_transfer(
		"16-bit LE to BE swaps bytes",
		DMAC_CONFIG_M2_BE,
		DMAC_CH_CONTROL_S_WIDTH_WORD | DMAC_CH_CONTROL_D_WIDTH_WORD,
		1,
		0x00002143
	);
	test_endian_transfer(
		"16-bit BE to LE reads the high source lane",
		DMAC_CONFIG_M1_BE,
		DMAC_CH_CONTROL_S_WIDTH_WORD | DMAC_CH_CONTROL_D_WIDTH_WORD,
		1,
		0x00006587
	);
}

static void test_endian_matrix_transfer(const struct dmac_endian_case *item) {
	reset_dmac();
	uint32_t size = item->count * item->src_width_bytes;
	for (uint32_t i = 0; i < size; i++) {
		endian_matrix_source[i] = (uint8_t) (i * 29 + 0x13);
		endian_matrix_destination[i] = 0xA5;
	}
	for (uint32_t unit = 0; unit < size; unit += item->dst_width_bytes)
		for (uint32_t byte = 0; byte < item->dst_width_bytes; byte++)
			endian_matrix_expected[unit + byte] = endian_matrix_source[unit + item->dst_width_bytes - 1 - byte];

	DMAC_CH_SRC_ADDR(DMA_CHANNEL) = (uint32_t) endian_matrix_source;
	DMAC_CH_DST_ADDR(DMA_CHANNEL) = (uint32_t) endian_matrix_destination;
	DMAC_CH_LLI(DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(DMA_CHANNEL) = (
		item->count |
		item->src_burst |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		item->src_width |
		item->dst_width |
		DMAC_CH_CONTROL_S_AHB1 |
		DMAC_CH_CONTROL_D_AHB2 |
		DMAC_CH_CONTROL_SI |
		DMAC_CH_CONTROL_DI |
		DMAC_CH_CONTROL_I
	);
	DMAC_CONFIG = DMAC_CONFIG_M2_BE | DMAC_CONFIG_ENABLE;
	DMAC_CH_CONFIG(DMA_CHANNEL) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_ENABLE;
	test_check("big-endian matrix transfer completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory(item->name, endian_matrix_expected, endian_matrix_destination, size);
}

static void test_endian_matrix(void) {
	static const struct dmac_endian_case cases[] = {
		{"SB1, three dwords", 3, 4, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_1,
			DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"SB4, five dwords with partial burst", 5, 4, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_4,
			DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"SB8, nine dwords with partial burst", 9, 4, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_8,
			DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"SB4, seven halfwords", 7, 2, 2, DMAC_CH_CONTROL_SB_SIZE_SZ_4,
			DMAC_CH_CONTROL_S_WIDTH_WORD, DMAC_CH_CONTROL_D_WIDTH_WORD},
		{"SB8, byte to dword packing", 16, 1, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_8,
			DMAC_CH_CONTROL_S_WIDTH_BYTE, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"SB4, halfword to dword packing", 8, 2, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_4,
			DMAC_CH_CONTROL_S_WIDTH_WORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"SB4, dword to halfword splitting", 4, 4, 2, DMAC_CH_CONTROL_SB_SIZE_SZ_4,
			DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_WORD},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++)
		test_endian_matrix_transfer(&cases[i]);
}

static void fill_matrix_buffers(void) {
	for (size_t i = 0; i < MATRIX_BUFFER_SIZE; i++) {
		matrix_source[i] = (uint8_t) ((i * 29 + 0x53) ^ (i >> 3));
		matrix_destination[i] = 0xA5;
	}
}

static void test_bursts_and_widths(void) {
	static const struct dmac_case cases[] = {
		{"burst 1, byte, partial packet", 37, 3, 19, 1, DMAC_CH_CONTROL_SB_SIZE_SZ_1,
			DMAC_CH_CONTROL_DB_SIZE_SZ_1, DMAC_CH_CONTROL_S_WIDTH_BYTE, DMAC_CH_CONTROL_D_WIDTH_BYTE},
		{"burst 4, byte, partial packet", 37, 5, 67, 1, DMAC_CH_CONTROL_SB_SIZE_SZ_4,
			DMAC_CH_CONTROL_DB_SIZE_SZ_4, DMAC_CH_CONTROL_S_WIDTH_BYTE, DMAC_CH_CONTROL_D_WIDTH_BYTE},
		{"burst 8, word, partial packet", 31, 2, 130, 2, DMAC_CH_CONTROL_SB_SIZE_SZ_8,
			DMAC_CH_CONTROL_DB_SIZE_SZ_8, DMAC_CH_CONTROL_S_WIDTH_WORD, DMAC_CH_CONTROL_D_WIDTH_WORD},
		{"burst 16, dword, partial packet", 31, 64, 256, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_16,
			DMAC_CH_CONTROL_DB_SIZE_SZ_16, DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"burst 32, dword, partial packet", 65, 0, 512, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_32,
			DMAC_CH_CONTROL_DB_SIZE_SZ_32, DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"burst 64, dword, partial packet", 129, 0, 768, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_64,
			DMAC_CH_CONTROL_DB_SIZE_SZ_64, DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"burst 128, dword, partial packet", 257, 0, 1024, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_128,
			DMAC_CH_CONTROL_DB_SIZE_SZ_128, DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"burst 256, dword, partial packet", 257, 0, 1024, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_256,
			DMAC_CH_CONTROL_DB_SIZE_SZ_256, DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"byte to dword packing", 64, 3, 128, 1, DMAC_CH_CONTROL_SB_SIZE_SZ_16,
			DMAC_CH_CONTROL_DB_SIZE_SZ_4, DMAC_CH_CONTROL_S_WIDTH_BYTE, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"word to dword packing", 32, 2, 256, 2, DMAC_CH_CONTROL_SB_SIZE_SZ_8,
			DMAC_CH_CONTROL_DB_SIZE_SZ_4, DMAC_CH_CONTROL_S_WIDTH_WORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"dword to byte splitting", 16, 64, 3, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_4,
			DMAC_CH_CONTROL_DB_SIZE_SZ_16, DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_BYTE},
		{"burst crosses 1 KiB boundary", 64, 1008, 512, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_16,
			DMAC_CH_CONTROL_DB_SIZE_SZ_16, DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
		{"maximum transfer size", 4095, 0, 0, 4, DMAC_CH_CONTROL_SB_SIZE_SZ_256,
			DMAC_CH_CONTROL_DB_SIZE_SZ_256, DMAC_CH_CONTROL_S_WIDTH_DWORD, DMAC_CH_CONTROL_D_WIDTH_DWORD},
	};
	bool completed = true;

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		const struct dmac_case *item = &cases[i];
		size_t size = item->count * item->src_width_bytes;
		reset_dmac();
		fill_matrix_buffers();

		uint32_t control = (
			item->count | item->src_burst | item->dst_burst | item->src_width | item->dst_width |
			DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI |
			DMAC_CH_CONTROL_I
		);
		start_transfer(
			(uint32_t) (matrix_source + item->src_offset),
			(uint32_t) (matrix_destination + item->dst_offset),
			0,
			control,
			DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
		);

		completed &= wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL));
		test_eq_memory(
			item->name,
			matrix_source + item->src_offset,
			matrix_destination + item->dst_offset,
			size
		);
		DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
	}

	test_check("burst matrix completes", completed);
}

static void test_unaligned_addresses(void) {
	static const struct dmac_unaligned_case cases[] = {
		{"unaligned halfword source aligns down", 9, 2, 129, 256,
			DMAC_CH_CONTROL_S_WIDTH_WORD | DMAC_CH_CONTROL_D_WIDTH_WORD},
		{"unaligned halfword destination aligns down", 9, 2, 128, 257,
			DMAC_CH_CONTROL_S_WIDTH_WORD | DMAC_CH_CONTROL_D_WIDTH_WORD},
		{"unaligned dword addresses align down", 7, 4, 129, 259,
			DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		const struct dmac_unaligned_case *item = &cases[i];
		uint32_t aligned_src = item->src_offset & ~(item->width_bytes - 1);
		uint32_t aligned_dst = item->dst_offset & ~(item->width_bytes - 1);
		uint32_t size = item->count * item->width_bytes;

		reset_dmac();
		fill_matrix_buffers();
		start_transfer(
			(uint32_t) (matrix_source + item->src_offset),
			(uint32_t) (matrix_destination + item->dst_offset),
			0,
			(
				item->count | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
				item->widths |
				DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
				DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
			),
			DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
		);

		test_check("unaligned transfer completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
		test_eq_memory(item->name, matrix_source + aligned_src, matrix_destination + aligned_dst, size);
		test_eq_u32("unaligned transfer keeps byte before destination", 0xA5, matrix_destination[aligned_dst - 1]);
		test_eq_u32("unaligned transfer keeps byte after destination", 0xA5, matrix_destination[aligned_dst + size]);
	}
}

static void test_overlap(void) {
	uint8_t expected[128];

	reset_dmac();
	fill_matrix_buffers();
	for (size_t i = 0; i < sizeof(expected); i++)
		expected[i] = matrix_source[64 + i];

	start_transfer(
		(uint32_t) (matrix_source + 64),
		(uint32_t) (matrix_source + 32),
		0,
		(
			32 | DMAC_CH_CONTROL_SB_SIZE_SZ_16 | DMAC_CH_CONTROL_DB_SIZE_SZ_16 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
	);
	test_check("backward-overlap transfer completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("backward-overlap data", expected, matrix_source + 32, sizeof(expected));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
}

static void test_transfer_boundaries(void) {
	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			1 | DMAC_CH_CONTROL_SB_SIZE_SZ_256 | DMAC_CH_CONTROL_DB_SIZE_SZ_256 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
	);
	test_check(
		"single item with oversized burst completes",
		wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL))
	);
	test_eq_memory("single item with oversized burst data", matrix_source, matrix_destination, sizeof(uint32_t));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) (matrix_source + 4080),
		(uint32_t) (matrix_destination + 4080),
		0,
		(
			32 | DMAC_CH_CONTROL_SB_SIZE_SZ_16 | DMAC_CH_CONTROL_DB_SIZE_SZ_16 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
	);
	test_check(
		"transfer crossing 4 KiB boundary completes",
		wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL))
	);
	test_eq_memory(
		"transfer crossing 4 KiB boundary data",
		matrix_source + 4080,
		matrix_destination + 4080,
		32 * sizeof(uint32_t)
	);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			DMAC_CH_CONTROL_SB_SIZE_SZ_256 | DMAC_CH_CONTROL_DB_SIZE_SZ_256 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
	);
	stopwatch_usleep_wd(1000);
	test_check("zero transfer size has no terminal count", (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0);
	test_check("zero transfer size keeps channel enabled", (DMAC_EN_CHAN & BIT(DMA_CHANNEL)) != 0);
	test_check(
		"zero transfer size keeps channel idle",
		(DMAC_CH_CONFIG(DMA_CHANNEL) & DMAC_CH_CONFIG_ACTIVE) == 0
	);
	test_eq_u32(
		"zero transfer size leaves destination unchanged",
		0xA5A5A5A5,
		*(volatile uint32_t *) matrix_destination
	);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
}

static void test_address_increment(void) {
	uint8_t expected[32];

	reset_dmac();
	fill_matrix_buffers();
	for (size_t i = 0; i < sizeof(expected); i++)
		expected[i] = matrix_source[i % sizeof(uint32_t)];

	uint32_t fixed_source = (
		8 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		fixed_source,
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
	);
	wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL));
	test_eq_memory("fixed source address", expected, matrix_destination, sizeof(expected));
	test_eq_u32("fixed source register", (uint32_t) matrix_source, DMAC_CH_SRC_ADDR(DMA_CHANNEL));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	uint32_t fixed_destination = (
		8 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I
	);
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		fixed_destination,
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
	);
	wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL));
	test_eq_memory(
		"fixed destination address",
		matrix_source + 7 * sizeof(uint32_t),
		matrix_destination,
		sizeof(uint32_t)
	);
	test_eq_u32("fixed destination register", (uint32_t) matrix_destination, DMAC_CH_DST_ADDR(DMA_CHANNEL));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
}

static void test_software_requests(void) {
	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			8 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER
	);
	test_check("MEM2PER waits for request", (DMAC_CH_CONTROL(DMA_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE) == 8);
	DMAC_SOFT_BREQ = BIT(0);
	test_check(
		"MEM2PER burst request transfers four items",
		wait_for_value(&DMAC_CH_CONTROL(DMA_CHANNEL), DMAC_CH_CONTROL_TRANSFER_SIZE, 4)
	);
	test_check("MEM2PER software request clears", (DMAC_SOFT_BREQ & BIT(0)) == 0);
	DMAC_SOFT_BREQ = BIT(0);
	test_check("MEM2PER second request completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("MEM2PER software-request data", matrix_source, matrix_destination, 8 * sizeof(uint32_t));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			8 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM
	);
	DMAC_SOFT_BREQ = BIT(0);
	test_check(
		"PER2MEM burst request transfers four items",
		wait_for_value(&DMAC_CH_CONTROL(DMA_CHANNEL), DMAC_CH_CONTROL_TRANSFER_SIZE, 4)
	);
	DMAC_SOFT_BREQ = BIT(0);
	test_check("PER2MEM second request completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("PER2MEM software-request data", matrix_source, matrix_destination, 8 * sizeof(uint32_t));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			DMAC_CH_CONTROL_SB_SIZE_SZ_1 | DMAC_CH_CONTROL_DB_SIZE_SZ_1 | DMAC_CH_CONTROL_S_WIDTH_BYTE |
			DMAC_CH_CONTROL_D_WIDTH_BYTE | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER
	);
	DMAC_SOFT_LSREQ = BIT(0);
	test_check(
		"peripheral-controlled MEM2PER completes on LSREQ",
		wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL))
	);
	test_check("MEM2PER last single request clears", (DMAC_SOFT_LSREQ & BIT(0)) == 0);
	test_eq_memory("peripheral-controlled MEM2PER data", matrix_source, matrix_destination, 1);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			DMAC_CH_CONTROL_SB_SIZE_SZ_1 | DMAC_CH_CONTROL_DB_SIZE_SZ_1 | DMAC_CH_CONTROL_S_WIDTH_BYTE |
			DMAC_CH_CONTROL_D_WIDTH_BYTE | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER
	);
	DMAC_SOFT_LSREQ = BIT(0);
	test_check(
		"peripheral-controlled PER2MEM completes on LSREQ",
		wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL))
	);
	test_check("PER2MEM last single request clears", (DMAC_SOFT_LSREQ & BIT(0)) == 0);
	test_eq_memory("peripheral-controlled PER2MEM data", matrix_source, matrix_destination, 1);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_BYTE |
			DMAC_CH_CONTROL_D_WIDTH_BYTE | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER
	);
	DMAC_SOFT_BREQ = BIT(0);
	test_check("peripheral-controlled MEM2PER accepts BREQ", wait_for_value(&DMAC_SOFT_BREQ, BIT(0), 0));
	test_check("MEM2PER BREQ is not terminal", (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0);
	DMAC_SOFT_LBREQ = BIT(0);
	test_check(
		"peripheral-controlled MEM2PER completes on LBREQ",
		wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL))
	);
	test_check("MEM2PER last burst request clears", (DMAC_SOFT_LBREQ & BIT(0)) == 0);
	test_eq_memory("peripheral-controlled MEM2PER burst data", matrix_source, matrix_destination, 8);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_BYTE |
			DMAC_CH_CONTROL_D_WIDTH_BYTE | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER
	);
	DMAC_SOFT_BREQ = BIT(0);
	test_check("peripheral-controlled PER2MEM accepts BREQ", wait_for_value(&DMAC_SOFT_BREQ, BIT(0), 0));
	test_check("PER2MEM BREQ is not terminal", (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0);
	DMAC_SOFT_LBREQ = BIT(0);
	test_check(
		"peripheral-controlled PER2MEM completes on LBREQ",
		wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL))
	);
	test_check("PER2MEM last burst request clears", (DMAC_SOFT_LBREQ & BIT(0)) == 0);
	test_eq_memory("peripheral-controlled PER2MEM burst data", matrix_source, matrix_destination, 8);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		(1 << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_PER2PER
	);
	DMAC_SOFT_BREQ = BIT(0) | BIT(1);
	test_check("PER2PER software requests complete", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("PER2PER software-request data", matrix_source, matrix_destination, 4 * sizeof(uint32_t));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
}

static struct dmac_request_results run_request_selectors(uint32_t shift, uint32_t flow) {
	struct dmac_request_results results = {
		.completed = true,
		.cleared = true,
		.data_matches = true,
	};
	for (uint32_t request = 0; request < 16; request++) {
		reset_dmac();
		fill_matrix_buffers();
		start_transfer(
			(uint32_t) matrix_source,
			(uint32_t) matrix_destination,
			0,
			(
				4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
				DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
				DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
			),
			(request << shift) | flow
		);
		DMAC_SOFT_BREQ = BIT(request);
		results.completed &= wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL));
		results.cleared &= (DMAC_SOFT_BREQ & BIT(request)) == 0;
		results.data_matches &= memcmp(matrix_source, (const void *) matrix_destination,
			4 * sizeof(uint32_t)) == 0;
		DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
	}

	return results;
}

static void test_request_selectors(void) {
	struct dmac_request_results mem2per = run_request_selectors(
		DMAC_CH_CONFIG_DST_PERIPH_SHIFT,
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER
	);
	test_check("all MEM2PER request selectors complete", mem2per.completed);
	test_check("all MEM2PER burst requests clear", mem2per.cleared);
	test_check("all MEM2PER request selectors transfer data", mem2per.data_matches);

	struct dmac_request_results per2mem = run_request_selectors(
		DMAC_CH_CONFIG_SRC_PERIPH_SHIFT,
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM
	);
	test_check("all PER2MEM request selectors complete", per2mem.completed);
	test_check("all PER2MEM burst requests clear", per2mem.cleared);
	test_check("all PER2MEM request selectors transfer data", per2mem.data_matches);
}

static void test_request_selector_isolation(void) {
	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER
	);
	DMAC_SOFT_BREQ = BIT(1);
	stopwatch_usleep_wd(1000);
	test_eq_u32(
		"unselected request leaves transfer count unchanged",
		4,
		DMAC_CH_CONTROL(DMA_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_check("unselected request remains pending", (DMAC_SOFT_BREQ & BIT(1)) != 0);
	test_eq_u32(
		"unselected request leaves destination unchanged",
		0xA5A5A5A5,
		*(volatile uint32_t *) matrix_destination
	);
	DMAC_CONFIG = 0;
	test_check("global disable clears unselected request", (DMAC_SOFT_BREQ & BIT(1)) == 0);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SOFT_BREQ = BIT(0);
	test_check("selected request completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("selected request transfers data", matrix_source, matrix_destination, 4 * sizeof(uint32_t));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
}

static void test_peripheral_controlled_per2per(void) {
	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		(1 << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_PER2PER_DST
	);
	DMAC_SOFT_BREQ = BIT(0);
	stopwatch_usleep_wd(1000);
	test_check("source burst waits for destination request", (DMAC_SOFT_BREQ & BIT(0)) != 0);
	DMAC_SOFT_LBREQ = BIT(1);
	test_check(
		"destination-controlled PER2PER completes",
		wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL))
	);
	test_eq_memory(
		"destination-controlled PER2PER data",
		matrix_source,
		matrix_destination,
		4 * sizeof(uint32_t)
	);
	test_check("source burst request clears after destination request", (DMAC_SOFT_BREQ & BIT(0)) == 0);
	test_check("destination last burst request clears", (DMAC_SOFT_LBREQ & BIT(1)) == 0);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		(1 << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_PER2PER_SRC
	);
	DMAC_SOFT_LBREQ = BIT(0);
	test_check(
		"source-controlled PER2PER accepts last source burst",
		wait_for_value(&DMAC_SOFT_LBREQ, BIT(0), 0)
	);
	DMAC_SOFT_BREQ = BIT(1);
	test_check("source-controlled PER2PER completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("source-controlled PER2PER data", matrix_source, matrix_destination, 4 * sizeof(uint32_t));
	test_check("destination burst request clears", (DMAC_SOFT_BREQ & BIT(1)) == 0);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
}

static void test_registers(void) {
	reset_dmac();

	uint32_t config = (
		(3 << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) | (11 << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2PER | DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_HALT
	);
	DMAC_CH_CONFIG(DMA_CHANNEL) = config;
	test_eq_u32("channel config readback", config, DMAC_CH_CONFIG(DMA_CHANNEL) & ~DMAC_CH_CONFIG_ACTIVE);
	test_check("ACTIVE is read-only and idle", (DMAC_CH_CONFIG(DMA_CHANNEL) & DMAC_CH_CONFIG_ACTIVE) == 0);

	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	test_eq_u32("global config readback", DMAC_CONFIG_ENABLE, DMAC_CONFIG);
	test_check("AHB masters remain little endian", (DMAC_CONFIG & (DMAC_CONFIG_M1 | DMAC_CONFIG_M2)) == 0);
	DMAC_CONFIG = DMAC_CONFIG_M1_BE | DMAC_CONFIG_M2_BE;
	test_eq_u32("AHB endian bits readback", DMAC_CONFIG_M1_BE | DMAC_CONFIG_M2_BE, DMAC_CONFIG);
	DMAC_CONFIG = 0;
}

static void test_channel_control(void) {
	reset_dmac();
	fill_matrix_buffers();

	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			8 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER | DMAC_CH_CONFIG_HALT
	);
	test_check("halted channel is enabled", (DMAC_EN_CHAN & BIT(DMA_CHANNEL)) != 0);
	test_check("halted channel is inactive", (DMAC_CH_CONFIG(DMA_CHANNEL) & DMAC_CH_CONFIG_ACTIVE) == 0);

	DMAC_SOFT_BREQ = BIT(0);
	stopwatch_usleep_wd(1000);
	test_eq_u32(
		"HALT blocks pending request",
		8,
		DMAC_CH_CONTROL(DMA_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_check("request remains pending while halted", (DMAC_SOFT_BREQ & BIT(0)) != 0);

	DMAC_CH_CONFIG(DMA_CHANNEL) &= ~DMAC_CH_CONFIG_HALT;
	test_check(
		"pending request runs after resume",
		wait_for_value(&DMAC_CH_CONTROL(DMA_CHANNEL), DMAC_CH_CONTROL_TRANSFER_SIZE, 4)
	);
	DMAC_SOFT_BREQ = BIT(0);
	test_check("resumed channel completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("HALT resume preserves data", matrix_source, matrix_destination, 8 * sizeof(uint32_t));
	test_check("channel is inactive after completion", (DMAC_CH_CONFIG(DMA_CHANNEL) & DMAC_CH_CONFIG_ACTIVE) == 0);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);

	reset_dmac();
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_CH_CONFIG(DMA_CHANNEL) = (
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER | DMAC_CH_CONFIG_HALT |
		DMAC_CH_CONFIG_ENABLE
	);
	test_check("EN_CHAN mirrors channel enable", (DMAC_EN_CHAN & BIT(DMA_CHANNEL)) != 0);
	DMAC_CH_CONFIG(DMA_CHANNEL) = 0;
	test_check("clearing channel enable updates EN_CHAN", (DMAC_EN_CHAN & BIT(DMA_CHANNEL)) == 0);

	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER
	);
	DMAC_CONFIG = 0;
	DMAC_SOFT_BREQ = BIT(0);
	stopwatch_usleep_wd(1000);
	test_eq_u32(
		"global disable blocks request",
		4,
		DMAC_CH_CONTROL(DMA_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_check("global disable discards request", (DMAC_SOFT_BREQ & BIT(0)) == 0);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SOFT_BREQ = BIT(0);
	test_check("global re-enable resumes request", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("global disable preserves data", matrix_source, matrix_destination, 4 * sizeof(uint32_t));
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
}

static void test_halt_after_partial_transfer(void) {
	reset_dmac();
	fill_matrix_buffers();
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			8 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER
	);
	DMAC_SOFT_BREQ = BIT(0);
	test_check(
		"first burst completes before HALT",
		wait_for_value(&DMAC_CH_CONTROL(DMA_CHANNEL), DMAC_CH_CONTROL_TRANSFER_SIZE, 4)
	);
	DMAC_CH_CONFIG(DMA_CHANNEL) |= DMAC_CH_CONFIG_HALT;
	stopwatch_usleep_wd(1000);
	test_eq_u32(
		"HALT preserves partial transfer count",
		4,
		DMAC_CH_CONTROL(DMA_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_check(
		"HALT waits while channel has buffered data",
		(DMAC_CH_CONFIG(DMA_CHANNEL) & DMAC_CH_CONFIG_ACTIVE) != 0
	);
	test_eq_memory("HALT preserves completed burst", matrix_source, matrix_destination, 4 * sizeof(uint32_t));
	DMAC_SOFT_BREQ = BIT(0);
	test_check("final request drains halted channel", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_check(
		"halted channel becomes inactive after drain",
		(DMAC_CH_CONFIG(DMA_CHANNEL) & DMAC_CH_CONFIG_ACTIVE) == 0
	);
	test_eq_memory("halted partial transfer data", matrix_source, matrix_destination, 8 * sizeof(uint32_t));
	test_check("final request clears after drain", (DMAC_SOFT_BREQ & BIT(0)) == 0);
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
}

static void test_multiple_channels(void) {
	reset_dmac();
	fill_matrix_buffers();

	DMAC_CH_SRC_ADDR(0) = (uint32_t) matrix_source;
	DMAC_CH_DST_ADDR(0) = (uint32_t) matrix_destination;
	DMAC_CH_LLI(0) = 0;
	DMAC_CH_CONTROL(0) = (
		128 | DMAC_CH_CONTROL_SB_SIZE_SZ_16 | DMAC_CH_CONTROL_DB_SIZE_SZ_16 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(0) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE;

	DMAC_CH_SRC_ADDR(7) = (uint32_t) (matrix_source + 4096);
	DMAC_CH_DST_ADDR(7) = (uint32_t) (matrix_destination + 4096);
	DMAC_CH_LLI(7) = 0;
	DMAC_CH_CONTROL(7) = (
		128 | DMAC_CH_CONTROL_SB_SIZE_SZ_16 | DMAC_CH_CONTROL_DB_SIZE_SZ_16 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(7) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE;

	test_eq_u32("two channels enabled", BIT(0) | BIT(7), DMAC_EN_CHAN & (BIT(0) | BIT(7)));
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	test_check("two channels complete", wait_for_value(&DMAC_RAW_TC_STATUS, BIT(0) | BIT(7), BIT(0) | BIT(7)));
	test_eq_memory("channel 0 data", matrix_source, matrix_destination, 128 * sizeof(uint32_t));
	test_eq_memory("channel 7 data", matrix_source + 4096, matrix_destination + 4096, 128 * sizeof(uint32_t));

	DMAC_TC_CLEAR = BIT(0);
	test_check("clearing channel 0 keeps channel 7 pending", (DMAC_TC_STATUS & (BIT(0) | BIT(7))) == BIT(7));
	test_check("combined status keeps channel 7", (DMAC_INT_STATUS & (BIT(0) | BIT(7))) == BIT(7));
	DMAC_TC_CLEAR = BIT(7);
	test_check("independent clears remove both channels", (DMAC_INT_STATUS & (BIT(0) | BIT(7))) == 0);

	reset_dmac();
	fill_matrix_buffers();
	DMAC_CH_SRC_ADDR(0) = (uint32_t) matrix_source;
	DMAC_CH_DST_ADDR(0) = (uint32_t) matrix_destination;
	DMAC_CH_LLI(0) = 0;
	DMAC_CH_CONTROL(0) = (
		32 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(0) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE;
	DMAC_CH_SRC_ADDR(7) = (uint32_t) (matrix_source + 4096);
	DMAC_CH_DST_ADDR(7) = (uint32_t) (matrix_destination + 4096);
	DMAC_CH_LLI(7) = 0;
	DMAC_CH_CONTROL(7) = (
		32 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(7) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE;
	VIC_CON(VIC_DMAC_CH0_IRQ) = 1;
	VIC_CON(VIC_DMAC_CH7_IRQ) = 1;
	cpu_enable_irq(true);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	test_check("both channel IRQs arrive", wait_for_irq_count(2));
	cpu_enable_irq(false);
	test_eq_u32("higher-priority channel IRQ is first", 0, irq_order[0]);
	test_eq_u32("lower-priority channel IRQ is second", 7, irq_order[1]);
}

static void test_all_channels(void) {
	reset_dmac();
	fill_matrix_buffers();

	for (uint32_t channel = 0; channel < 8; channel++) {
		DMAC_CH_SRC_ADDR(channel) = (uint32_t) (matrix_source + channel * 512);
		DMAC_CH_DST_ADDR(channel) = (uint32_t) (matrix_destination + channel * 512);
		DMAC_CH_LLI(channel) = 0;
		DMAC_CH_CONTROL(channel) = (
			128 | DMAC_CH_CONTROL_SB_SIZE_SZ_16 | DMAC_CH_CONTROL_DB_SIZE_SZ_16 |
			DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
			DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		);
		DMAC_CH_CONFIG(channel) = (
			DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_INT_MASK_TC |
			DMAC_CH_CONFIG_ENABLE
		);
		VIC_CON(VIC_DMAC_CH0_IRQ + channel) = 1;
	}

	cpu_enable_irq(true);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	test_check("all channel IRQs arrive", wait_for_irq_count(8));
	cpu_enable_irq(false);

	bool priority_order = true;
	for (uint32_t channel = 0; channel < 8; channel++)
		priority_order &= irq_order[channel] == channel;
	test_check("all channels follow priority order", priority_order);
	test_eq_memory("all channel data", matrix_source, matrix_destination, 8 * 512);
	test_check("all channels disable after completion", (DMAC_EN_CHAN & 0xFF) == 0);
}

static void test_lli(void) {
	reset_dmac();
	clear_destination();

	lli.src = (uint32_t) (source + 8);
	lli.dst = (uint32_t) (destination + 8);
	lli.next = 0;
	lli.control = (
		8 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);

	start_transfer(
		(uint32_t) source,
		(uint32_t) destination,
		(uint32_t) &lli,
		(
			8 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_INT_MASK_TC
	);

	test_check("LLI transfer completes", wait_for_status(&DMAC_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory("LLI data", source, destination, sizeof(source));
	test_eq_u32("LLI chain ends", 0, DMAC_CH_LLI(DMA_CHANNEL));
	test_eq_u32(
		"LLI source address is last item",
		(uint32_t) (source + ARRAY_SIZE(source) - 1),
		DMAC_CH_SRC_ADDR(DMA_CHANNEL)
	);
	test_eq_u32(
		"LLI destination address is last item",
		(uint32_t) (destination + ARRAY_SIZE(destination) - 1),
		DMAC_CH_DST_ADDR(DMA_CHANNEL)
	);
	test_eq_u32("LLI transfer count reaches zero", 0, DMAC_CH_CONTROL(DMA_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE);
	test_check("LLI channel is disabled", (DMAC_EN_CHAN & BIT(DMA_CHANNEL)) == 0);
	test_check("LLI raw terminal count is set", (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) != 0);
	test_check("LLI masked terminal count is set", (DMAC_TC_STATUS & BIT(DMA_CHANNEL)) != 0);
	test_check("LLI combined interrupt status is set", (DMAC_INT_STATUS & BIT(DMA_CHANNEL)) != 0);

	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
	test_check("LLI interrupt status clears", (DMAC_INT_STATUS & BIT(DMA_CHANNEL)) == 0);
}

static void test_endian_lli(const char *name, uint32_t lli_master, uint32_t global_config, uint32_t data_master) {
	reset_dmac();
	fill_matrix_buffers();

	uint32_t control = (
		4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | data_master |
		DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	endian_lli.src = __builtin_bswap32((uint32_t) (matrix_source + 16));
	endian_lli.dst = __builtin_bswap32((uint32_t) (matrix_destination + 16));
	endian_lli.next = 0;
	endian_lli.control = __builtin_bswap32(control);

	DMAC_CH_SRC_ADDR(DMA_CHANNEL) = (uint32_t) matrix_source;
	DMAC_CH_DST_ADDR(DMA_CHANNEL) = (uint32_t) matrix_destination;
	DMAC_CH_LLI(DMA_CHANNEL) = (uint32_t) &endian_lli | lli_master;
	DMAC_CH_CONTROL(DMA_CHANNEL) = control & ~DMAC_CH_CONTROL_I;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE | global_config;
	DMAC_CH_CONFIG(DMA_CHANNEL) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_ENABLE;

	test_check("big-endian LLI transfer completes", wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
	test_eq_memory(name, matrix_source, matrix_destination, 8 * sizeof(uint32_t));
}

static void test_lli_endianness(void) {
	test_endian_lli(
		"LLI loads through big-endian AHB1",
		DMAC_CH_LLI_LM_AHB1,
		DMAC_CONFIG_M1_BE,
		DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2
	);
	test_endian_lli(
		"LLI loads through big-endian AHB2",
		DMAC_CH_LLI_LM_AHB2,
		DMAC_CONFIG_M2_BE,
		DMAC_CH_CONTROL_S_AHB1 | DMAC_CH_CONTROL_D_AHB1
	);
}

static void test_lli_alignment(void) {
	static const struct {
		const char *name;
		uint8_t offset;
	} cases[] = {
		{"LLI aligned to 4 bytes", 1},
		{"LLI aligned to 8 bytes", 2},
		{"LLI aligned to 12 bytes", 3},
	};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		reset_dmac();
		fill_matrix_buffers();

		uint32_t control = (
			4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
			DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
			DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI
		);
		uint32_t *item = &lli_alignment_storage[cases[i].offset];
		item[0] = (uint32_t) (matrix_source + 4);
		item[1] = (uint32_t) (matrix_destination + 4);
		item[2] = 0;
		item[3] = control | DMAC_CH_CONTROL_I;

		start_transfer(
			(uint32_t) matrix_source,
			(uint32_t) matrix_destination,
			(uint32_t) item,
			control,
			DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM
		);

		test_check(cases[i].name, wait_for_status(&DMAC_RAW_TC_STATUS, BIT(DMA_CHANNEL)));
		test_eq_memory("word-aligned LLI data", matrix_source, matrix_destination, 8);
	}
}

static void test_lli_interrupts(void) {
	reset_dmac();
	fill_matrix_buffers();

	lli_chain[0] = (struct dmac_lli) {
		.src = (uint32_t) (matrix_source + 16),
		.dst = (uint32_t) (matrix_destination + 16),
		.next = (uint32_t) &lli_chain[1],
		.control = (
			16 | DMAC_CH_CONTROL_SB_SIZE_SZ_16 | DMAC_CH_CONTROL_DB_SIZE_SZ_16 |
			DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE | DMAC_CH_CONTROL_S_AHB2 |
			DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
	};
	lli_chain[1] = (struct dmac_lli) {
		.src = (uint32_t) (matrix_source + 32),
		.dst = (uint32_t) (matrix_destination + 32),
		.next = (uint32_t) &lli_chain[2],
		.control = (
			8 | DMAC_CH_CONTROL_SB_SIZE_SZ_8 | DMAC_CH_CONTROL_DB_SIZE_SZ_8 |
			DMAC_CH_CONTROL_S_WIDTH_WORD | DMAC_CH_CONTROL_D_WIDTH_WORD | DMAC_CH_CONTROL_S_AHB2 |
			DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI
		),
	};
	lli_chain[2] = (struct dmac_lli) {
		.src = (uint32_t) (matrix_source + 48),
		.dst = (uint32_t) (matrix_destination + 48),
		.next = 0,
		.control = (
			4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
			DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
			DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
	};

	VIC_CON(VIC_DMAC_CH7_IRQ) = 1;
	cpu_enable_irq(true);
	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		(uint32_t) &lli_chain[0],
		(
			4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER | DMAC_CH_CONFIG_INT_MASK_TC
	);

	DMAC_SOFT_BREQ = BIT(0);
	bool first_packet = wait_for_value(
		&DMAC_CH_LLI(DMA_CHANNEL),
		DMAC_CH_LLI_ITEM,
		(uint32_t) &lli_chain[1]
	);
	test_check("LLI packet without I advances", first_packet);
	test_eq_u32("LLI packet without I has no IRQ", 0, irq_count);

	DMAC_SOFT_BREQ = BIT(0);
	test_check("LLI packet with I raises IRQ", wait_for_irq_count(1));

	DMAC_SOFT_BREQ = BIT(0);
	bool third_packet = wait_for_value(&DMAC_CH_LLI(DMA_CHANNEL), DMAC_CH_LLI_ITEM, 0);
	test_check("second LLI packet without I advances", third_packet);
	test_eq_u32("second LLI packet without I has no IRQ", 1, irq_count);

	DMAC_SOFT_BREQ = BIT(0);
	test_check("final LLI packet with I raises IRQ", wait_for_irq_count(2));
	cpu_enable_irq(false);

	test_eq_u32("only selected LLI packets raise IRQ", 2, irq_count);
	test_eq_memory("mixed-width LLI data", matrix_source, matrix_destination, 64);
	test_eq_u32("paced LLI chain ends", 0, DMAC_CH_LLI(DMA_CHANNEL));
	test_check("paced LLI channel disables", (DMAC_EN_CHAN & BIT(DMA_CHANNEL)) == 0);
}

static void test_interrupt(void) {
	reset_dmac();
	clear_destination();

	VIC_CON(VIC_DMAC_CH7_IRQ) = 1;
	cpu_enable_irq(true);

	start_transfer(
		(uint32_t) source,
		(uint32_t) destination,
		0,
		(
			4 | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD |
			DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_INT_MASK_TC
	);

	wait_for_irq_count(1);
	cpu_enable_irq(false);
	test_eq_u32("channel IRQ fires once", 1, irq_count);
	test_eq_u32("channel IRQ number", VIC_DMAC_CH7_IRQ, irq_number);
	test_check("IRQ sees raw terminal count", (irq_raw_tc_status & BIT(DMA_CHANNEL)) != 0);
	test_check("IRQ sees masked terminal count", (irq_tc_status & BIT(DMA_CHANNEL)) != 0);
	test_check("IRQ sees combined status", (irq_int_status & BIT(DMA_CHANNEL)) != 0);
	test_check("IRQ clear removes terminal count", (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0);
	test_eq_memory("IRQ transfer data", source, destination, 4 * sizeof(source[0]));
}

static void test_sreq_behavior(void) {
	reset_dmac();
	fill_matrix_buffers();

	DMAC_CH_SRC_ADDR(0) = (uint32_t) (matrix_source + 16);
	DMAC_CH_DST_ADDR(0) = (uint32_t) (matrix_destination + 16);
	DMAC_CH_LLI(0) = 0;
	DMAC_CH_CONTROL(0) = (
		1 | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_CH_CONFIG(0) = (
		(2 << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM |
		DMAC_CH_CONFIG_ENABLE
	);

	start_transfer(
		(uint32_t) matrix_source,
		(uint32_t) matrix_destination,
		0,
		(
			1 | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | DMAC_CH_CONTROL_DB_SIZE_SZ_1 | DMAC_CH_CONTROL_S_WIDTH_BYTE |
			DMAC_CH_CONTROL_D_WIDTH_BYTE | DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI |
			DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
		),
		(1 << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER
	);
	DMAC_SOFT_SREQ = BIT(1) | BIT(2);
	stopwatch_usleep_wd(1000);
	test_eq_u32("MEM2PER SREQ remains pending", BIT(1), DMAC_SOFT_SREQ & BIT(1));
	test_eq_u32("PER2MEM SREQ clears", 0, DMAC_SOFT_SREQ & BIT(2));
	test_eq_u32(
		"MEM2PER SREQ leaves transfer count unchanged",
		1,
		DMAC_CH_CONTROL(DMA_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_eq_u32("PER2MEM SREQ completes one item", 0, DMAC_CH_CONTROL(0) & DMAC_CH_CONTROL_TRANSFER_SIZE);
	test_eq_u32("MEM2PER SREQ leaves destination unchanged", 0xA5, matrix_destination[0]);
	test_eq_u32("PER2MEM SREQ transfers data", matrix_source[16], matrix_destination[16]);
}

int main(void) {
	test_start("DMAC peripheral test");

	test_category("Identification and registers");
	test_amba_part_id("peripheral ID", 0x080, DMAC_PERIPH_ID0, DMAC_PERIPH_ID1);
	test_registers();

	test_category("Memory transfers");
	test_mem2mem();
	test_bursts_and_widths();
	test_unaligned_addresses();
	test_transfer_boundaries();
	test_address_increment();
	test_overlap();

	test_category("Endianness");
	test_endianness();
	test_endian_matrix();

	test_category("Software requests and flow control");
	test_software_requests();
	test_request_selectors();
	test_request_selector_isolation();
	test_peripheral_controlled_per2per();

	test_category("Channel control and arbitration");
	test_channel_control();
	test_halt_after_partial_transfer();
	test_multiple_channels();
	test_all_channels();

	test_category("Linked lists");
	test_lli();
	test_lli_endianness();
	test_lli_alignment();
	test_lli_interrupts();

	test_category("Interrupts");
	test_interrupt();

	test_category("Known hardware behavior");
	test_sreq_behavior();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;

	if (irq_number >= VIC_DMAC_CH0_IRQ && irq_number <= VIC_DMAC_CH7_IRQ) {
		uint32_t channel = irq_number - VIC_DMAC_CH0_IRQ;
		if (irq_count < ARRAY_SIZE(irq_order))
			irq_order[irq_count] = channel;
		irq_count++;
		irq_raw_tc_status = DMAC_RAW_TC_STATUS;
		irq_tc_status = DMAC_TC_STATUS;
		irq_int_status = DMAC_INT_STATUS;
		DMAC_TC_CLEAR = BIT(channel);
	}

	VIC_IRQ_ACK = 1;
}
