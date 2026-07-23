#include <pmb887x.h>

#include "s1d13732.h"
#include "test.h"

#if !defined(BOARD_SIEMENS_CX75)
#error The DIFv1 S1D13732 memory-interface test requires BOARD=siemens-cx75
#endif

#define DIF_BCSEL_ALL_FROM_BCREG 0x55555555U
#define DIF_BCSEL_ALL_MODE_2 0xAAAAAAAAU
#define DIF_BCSEL_ALL_MODE_3 UINT32_MAX

static void test_memory_contents(
	const char *name,
	uint32_t address,
	const uint16_t *expected,
	uint32_t count
) {
	uint16_t actual[96] = { 0 };

	test_check("memory test vector fits scratch buffer", count <= ARRAY_SIZE(actual));
	if (count > ARRAY_SIZE(actual))
		return;

	s1d13732_read_memory16(address, actual, count);
	test_eq_memory(name, expected, actual, count * sizeof(expected[0]));
}

static void test_memory_region(
	const char *name,
	uint32_t address,
	const uint16_t *expected,
	uint32_t count
) {
	s1d13732_write_memory16(address, expected, count);
	test_memory_contents(name, address, expected, count);
}

static void reset_conversion(void) {
	DIF_PBCCON = 0;
	DIF_BMREG0 = 0x14830820;
	DIF_BMREG1 = 0x2D4920E6;
	DIF_BMREG2 = 0x460F39AC;
	DIF_BMREG3 = 0x5ED55272;
	DIF_BMREG4 = 0x779B6B38;
	DIF_BMREG5 = 0x000003FE;
	DIF_BCREG = 0;
	DIF_BCSEL0 = 0;
	DIF_BCSEL1 = 0;
}

static void set_bmreg_mapping(uint32_t output_bit, uint32_t input_bit) {
	static volatile uint32_t * const REGISTERS[] = {
		&DIF_BMREG0, &DIF_BMREG1, &DIF_BMREG2, &DIF_BMREG3, &DIF_BMREG4, &DIF_BMREG5,
	};
	static const uint8_t SHIFTS[] = { 0, 5, 10, 16, 21, 26 };
	uint32_t register_index = output_bit / ARRAY_SIZE(SHIFTS);
	uint32_t shift = SHIFTS[output_bit % ARRAY_SIZE(SHIFTS)];
	volatile uint32_t *reg = REGISTERS[register_index];

	*reg = (*reg & ~(GENMASK(4, 0) << shift)) | (input_bit << shift);
}

static void configure_pair_conversion(
	int32_t first_input_bit,
	int32_t input_bit_step,
	uint32_t bcsel,
	uint32_t bcreg
) {
	DIF_PBCCON = DIF_PBCCON_PBBCONV_MODE;
	for (uint32_t output_bit = 0; output_bit < 16; output_bit++) {
		int32_t input_bit = first_input_bit + (int32_t) output_bit * input_bit_step;

		set_bmreg_mapping(output_bit, input_bit);
	}
	DIF_BCSEL0 = bcsel;
	DIF_BCREG = bcreg;
}

static void write_memory_words(const uint16_t *data, uint32_t count) {
	for (uint32_t i = 0; i < count; i++)
		s1d13732_write_memory_word16(data[i]);
}

static void begin_converted_write(uint32_t address) {
	reset_conversion();
	s1d13732_begin_memory_write16(address);
}

static void end_converted_write(void) {
	s1d13732_end_memory_write16();
	reset_conversion();
}

static void test_single_word_conversion(void) {
	static const uint16_t INPUT[] = { 0x0000, 0xFFFF, 0xA55A, 0x1234, 0x8001, 0x7FFE };
	uint16_t expected[ARRAY_SIZE(INPUT)];

	test_category("S1D13732 physical BMREG and BCREG conversion");
	for (uint32_t i = 0; i < ARRAY_SIZE(INPUT); i++)
		expected[i] = (INPUT[i] & ~3U) | ((INPUT[i] & 1U) << 1) | ((INPUT[i] & 2U) >> 1);
	begin_converted_write(0x10000);
	DIF_BMREG0 = 0x14830801;
	write_memory_words(INPUT, ARRAY_SIZE(INPUT));
	end_converted_write();
	test_memory_contents("BMREG swaps bit 0 and 1 on the S1D wire", 0x10000, expected, ARRAY_SIZE(expected));

	for (uint32_t i = 0; i < ARRAY_SIZE(INPUT); i++)
		expected[i] = INPUT[i] | 1U;
	begin_converted_write(0x10100);
	DIF_BCSEL0 = 1;
	DIF_BCREG = 1;
	write_memory_words(INPUT, ARRAY_SIZE(INPUT));
	end_converted_write();
	test_memory_contents("BCREG clamps bit 0 on the S1D wire", 0x10100, expected, ARRAY_SIZE(expected));

	for (uint32_t i = 0; i < ARRAY_SIZE(INPUT); i++)
		expected[i] = (INPUT[i] & 1U) != 0 ? UINT16_MAX : 0;
	begin_converted_write(0x10200);
	DIF_BMREG0 = 0;
	DIF_BMREG1 = 0;
	DIF_BMREG2 = 0;
	write_memory_words(INPUT, ARRAY_SIZE(INPUT));
	end_converted_write();
	test_memory_contents("BMREG fans input bit 0 out on the S1D wire", 0x10200, expected, ARRAY_SIZE(expected));
}

static void test_pair_conversion(void) {
	static const uint16_t SENTINEL[] = { 0xD00D };
	static const uint16_t EXPECTED[] = {
		0x1234, 0xABCD, 0xB3D5, UINT16_MAX, 0x0000, 0xA55A, 0x0000, 0x0000, 0x0770,
		0xD00D,
	};

	test_category("S1D13732 physical PBCCON conversion");
	s1d13732_write_memory16(0x10400 + (ARRAY_SIZE(EXPECTED) - 1) * 2, SENTINEL, 1);
	begin_converted_write(0x10400);
	configure_pair_conversion(0, 1, 0, 0);
	s1d13732_write_memory_word16(0x1234);
	s1d13732_write_memory_word16(0xABCD);
	configure_pair_conversion(16, 1, 0, 0);
	s1d13732_write_memory_word16(0x1234);
	s1d13732_write_memory_word16(0xABCD);
	configure_pair_conversion(31, -1, 0, 0);
	s1d13732_write_memory_word16(0x1234);
	s1d13732_write_memory_word16(0xABCD);
	configure_pair_conversion(20, 0, 0, 0);
	s1d13732_write_memory_word16(0x0000);
	s1d13732_write_memory_word16(BIT(4));
	s1d13732_write_memory_word16(UINT16_MAX);
	s1d13732_write_memory_word16(0x0000);
	configure_pair_conversion(0, 1, DIF_BCSEL_ALL_FROM_BCREG, 0xA55A);
	s1d13732_write_memory_word16(0x1234);
	s1d13732_write_memory_word16(0xABCD);
	configure_pair_conversion(0, 1, DIF_BCSEL_ALL_MODE_2, UINT16_MAX);
	s1d13732_write_memory_word16(0x1234);
	s1d13732_write_memory_word16(0xABCD);
	configure_pair_conversion(0, 1, DIF_BCSEL_ALL_MODE_3, UINT16_MAX);
	s1d13732_write_memory_word16(0x1234);
	s1d13732_write_memory_word16(0xABCD);
	configure_pair_conversion(0, 1, 0x11111111, 0xA55A);
	s1d13732_write_memory_word16(0x1234);
	s1d13732_write_memory_word16(0xABCD);
	end_converted_write();
	test_memory_contents(
		"PBCCON/BMREG/BCSEL output and word count on the S1D wire",
		0x10400,
		EXPECTED,
		ARRAY_SIZE(EXPECTED)
	);
}

static void test_full_conversion_matrix(void) {
	uint16_t expected[80];
	uint32_t output = 0;

	test_category("S1D13732 physical BMREG matrix");
	begin_converted_write(0x10800);
	for (uint32_t input_bit = 0; input_bit < 32; input_bit++) {
		uint16_t first = input_bit < 16 ? BIT(input_bit) : 0;
		uint16_t second = input_bit >= 16 ? BIT(input_bit - 16) : 0;

		configure_pair_conversion(input_bit, 0, 0, 0);
		s1d13732_write_memory_word16(first);
		s1d13732_write_memory_word16(second);
		expected[output++] = UINT16_MAX;
		s1d13732_write_memory_word16(0);
		s1d13732_write_memory_word16(0);
		expected[output++] = 0;
	}
	for (uint32_t output_bit = 0; output_bit < 16; output_bit++) {
		configure_pair_conversion(0, 0, 0, 0);
		set_bmreg_mapping(output_bit, 31);
		s1d13732_write_memory_word16(0);
		s1d13732_write_memory_word16(BIT(15));
		expected[output++] = BIT(output_bit);
	}
	end_converted_write();
	test_eq_u32("physical matrix vector size", ARRAY_SIZE(expected), output);
	test_memory_contents(
		"all BMREG input and output selectors on the S1D wire",
		0x10800,
		expected,
		ARRAY_SIZE(expected)
	);
}

static void test_incomplete_pair(void) {
	static const uint16_t SENTINEL[] = { 0xCAFE };

	test_category("S1D13732 physical incomplete PBCCON pair");
	s1d13732_write_memory16(0x10C00, SENTINEL, 1);
	begin_converted_write(0x10C00);
	configure_pair_conversion(0, 1, 0, 0);
	s1d13732_write_memory_word16(0x1234);
	DIF_PBCCON = 0;
	end_converted_write();
	test_memory_contents("incomplete pair writes no S1D word", 0x10C00, SENTINEL, 1);
}

int main(void) {
	static const uint16_t WRITE_DATA[] = {
		0x0000, 0xFFFF, 0xA55A, 0x5AA5, 0x1234, 0xFEDC, 0x8001, 0x7FFE,
	};
	static const uint16_t HIGH_PAGE_DATA[] = {
		0x1357, 0x2468, 0xDEAD, 0xBEEF, 0x0F0F, 0xF0F0, 0x55AA, 0xAA55,
	};
	static const uint16_t SINGLE_DATA[] = { 0xC33C };

	test_start("DIFv1 LCD interface through S1D13732 SRAM");
	test_category("S1D13732 bridge");
	s1d13732_init();
	test_id_u32("bridge ID", 0x706B, s1d13732_read_register(0x0000));
	s1d13732_enable_memory_interface();
	test_eq_u32("PLL enabled", 0, s1d13732_read_register(0x0012) & BIT(0));
	test_eq_u32("power save disabled", 0, s1d13732_read_register(0x0014) & BIT(0));
	test_eq_u32("system clock configuration", 0x1027, s1d13732_read_register(0x000C));
	test_eq_u32("PLL configuration 0", 0x1E48, s1d13732_read_register(0x000E));
	test_eq_u32("PLL configuration 1", 0x0103, s1d13732_read_register(0x0010));
	s1d13732_write_register(0x0024, 0x0003);
	test_eq_u32("indirect address high", 0x0003, s1d13732_read_register(0x0024));
	s1d13732_write_register(0x0022, 0x1234);
	test_eq_u32("indirect address low", 0x1234, s1d13732_read_register(0x0022));

	test_category("S1D13732 SRAM");
	test_memory_region("single word", 0x00000, SINGLE_DATA, ARRAY_SIZE(SINGLE_DATA));
	test_memory_region("aligned burst", 0x00020, WRITE_DATA, ARRAY_SIZE(WRITE_DATA));
	test_memory_region("burst across 64 KiB boundary", 0x0FFF8, WRITE_DATA, ARRAY_SIZE(WRITE_DATA));
	test_memory_region("separate high-address page", 0x30020, HIGH_PAGE_DATA, ARRAY_SIZE(HIGH_PAGE_DATA));
	test_memory_contents("low page remains independent", 0x00020, WRITE_DATA, ARRAY_SIZE(WRITE_DATA));
	test_memory_contents("high page remains independent", 0x30020, HIGH_PAGE_DATA, ARRAY_SIZE(HIGH_PAGE_DATA));
	test_memory_region("last SRAM word", 0x6FFFE, SINGLE_DATA, ARRAY_SIZE(SINGLE_DATA));
	test_eq_u32(
		"no SRAM read/write error",
		0,
		s1d13732_read_register(0x0A20) & (BIT(0) | BIT(1))
	);
	test_single_word_conversion();
	test_pair_conversion();
	test_full_conversion_matrix();
	test_incomplete_pair();
	test_eq_u32(
		"no SRAM error after converted writes",
		0,
		s1d13732_read_register(0x0A20) & (BIT(0) | BIT(1))
	);

	return test_finish();
}
