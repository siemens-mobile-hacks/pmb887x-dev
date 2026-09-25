#include <pmb887x.h>
#include <string.h>

#include "eeprom.h"
#include "test.h"

#define X65_PARTITION_SIZE 0x20000
#define X85_PARTITION_SIZE 0x40000
#define X65_ENTRY_SIZE 16
#define X85_ENTRY_SIZE 32
#define X85_HEADER_SIZE 32
#define X85_DATA_CHUNK_SIZE 16
#define ENTRY_FREE 0xFFFFFFFF
#define ENTRY_VALID 0xFFFFFFC0
#define ENTRY_OBSOLETE 0xFFFFFF00

static uint8_t flash[X85_PARTITION_SIZE];

static void write_u16(uint8_t *data, uint16_t value) {
	data[0] = value;
	data[1] = (value >> 8);
}

static void write_u32(uint8_t *data, uint32_t value) {
	data[0] = value;
	data[1] = (value >> 8);
	data[2] = (value >> 16);
	data[3] = (value >> 24);
}

static void write_partition_header(uint32_t offset, const char *name, uint32_t size) {
	memcpy(flash + offset, name, 6);
	flash[offset + 6] = 0;
	flash[offset + 7] = 0;
	flash[offset + 10] = 0;
	flash[offset + 11] = 0;
	flash[offset + 12] = 0xF0;
	memset(flash + offset + 13, 0xFF, size - 13);
}

static uint32_t align_x85_data_offset(uint32_t offset) {
	if ((offset & X85_DATA_CHUNK_SIZE) == 0)
		return offset;

	return offset + X85_DATA_CHUNK_SIZE * 2 - (offset & (X85_DATA_CHUNK_SIZE * 2 - 1));
}

static uint32_t x85_data_byte_offset(uint32_t offset, uint32_t index) {
	uint32_t chunk_offset = offset & (X85_DATA_CHUNK_SIZE - 1);

	return offset + index + ((chunk_offset + index) / X85_DATA_CHUNK_SIZE) * X85_DATA_CHUNK_SIZE;
}

static uint32_t write_x85_inline_entry(
	uint32_t entry_offset,
	uint32_t state,
	uint32_t id,
	uint8_t version,
	const uint8_t *data,
	uint32_t size,
	bool full
) {
	write_u32(flash + entry_offset, state);
	if (full) {
		write_u32(flash + entry_offset + 4, id - 5000);
		write_u32(flash + entry_offset + 8, size);
	} else {
		write_u16(flash + entry_offset + 4, id);
		flash[entry_offset + 7] = version;
		write_u16(flash + entry_offset + 8, size);
	}

	uint32_t reserved_size = X85_DATA_CHUNK_SIZE + size + (size & ~(X85_DATA_CHUNK_SIZE - 1));
	uint32_t stored_offset = align_x85_data_offset(entry_offset - reserved_size);
	for (uint32_t i = 0; i < size; i++)
		flash[x85_data_byte_offset(stored_offset, i)] = data[i];

	return (stored_offset & ~(X85_DATA_CHUNK_SIZE * 2 - 1)) - X85_ENTRY_SIZE;
}

static void reset_flash(void) {
	memset(flash, 0xFF, sizeof(flash));
}

static void test_x85_eelite_inline_and_duplicates(void) {
	static const uint8_t FIRST[] = { 0x11, 0x12, 0x13, 0x14 };
	static const uint8_t SECOND[] = { 0x21, 0x22, 0x23, 0x24 };
	struct eeprom_block block;

	reset_flash();
	write_partition_header(X85_PARTITION_SIZE - X85_HEADER_SIZE, "EELITE", X85_HEADER_SIZE);
	uint32_t entry_offset = X85_PARTITION_SIZE - X85_HEADER_SIZE - X85_ENTRY_SIZE;
	entry_offset = write_x85_inline_entry(entry_offset, ENTRY_OBSOLETE, 75, 1, FIRST, sizeof(FIRST), false);
	entry_offset = write_x85_inline_entry(entry_offset, ENTRY_VALID, 76, 2, FIRST, sizeof(FIRST), false);
	write_x85_inline_entry(entry_offset, ENTRY_VALID, 76, 3, SECOND, sizeof(SECOND), false);
	write_u32(flash + 0x3F000, ENTRY_VALID);
	write_u16(flash + 0x3F004, 0x8031);

	test_eq_u32("X85 ignores valid-state data collisions after a matching EIT record",
		EEPROM_RESULT_OK,
		eeprom_find_block_in_memory(flash, sizeof(flash), EEPROM_PARTITION_EELITE, 76, &block));
	test_eq_u32("X85 keeps the last valid duplicate", 3, block.version);
	test_eq_u32("X85 inline EELITE size excludes no data", sizeof(SECOND), block.size);
	test_eq_memory("X85 reconstructs strided inline EELITE data", SECOND, block.data, sizeof(SECOND));
}

static void test_x85_eefull_extended_metadata(void) {
	static const uint8_t STORED[] = {
		0x80, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	};
	static const uint8_t EXPECTED[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
	struct eeprom_block block;

	reset_flash();
	write_partition_header(X85_PARTITION_SIZE - X85_HEADER_SIZE, "EEFULL", X85_HEADER_SIZE);
	uint32_t entry_offset = X85_PARTITION_SIZE - X85_HEADER_SIZE - X85_ENTRY_SIZE;
	write_x85_inline_entry(entry_offset, ENTRY_VALID, 5009, 0, STORED, sizeof(STORED), true);

	test_eq_u32("X85 finds an inline EEFULL block",
		EEPROM_RESULT_OK,
		eeprom_find_block_in_memory(flash, sizeof(flash), EEPROM_PARTITION_EEFULL, 5009, &block));
	test_eq_u32("X85 exposes the extended EEFULL version", 0x80, block.version);
	test_eq_u32("X85 excludes all extended EEFULL metadata", sizeof(EXPECTED), block.size);
	test_eq_memory("X85 reconstructs extended inline EEFULL data", EXPECTED, block.data, sizeof(EXPECTED));
}

static void write_x65_entry(
	uint32_t entry_offset,
	uint32_t state,
	uint32_t id,
	uint32_t data_offset,
	const uint8_t *data,
	uint32_t size
) {
	write_u32(flash + entry_offset, state);
	if (state == ENTRY_FREE)
		return;
	write_u32(flash + entry_offset + 4, id);
	write_u32(flash + entry_offset + 8, size);
	write_u32(flash + entry_offset + 12, data_offset);
	memcpy(flash + data_offset, data, size);
}

static void test_x65_free_entry_and_extended_metadata(void) {
	static const uint8_t EELITE_STORED[] = { 0x31, 0x32, 0xFF };
	static const uint8_t EEFULL_STORED[] = { 0x80, 1, 2, 3, 4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	static const uint8_t EXPECTED[] = { 1, 2, 3, 4 };
	struct eeprom_block block;

	reset_flash();
	write_partition_header(0, "EELITE", 16);
	uint32_t entry_offset = X65_PARTITION_SIZE - X65_ENTRY_SIZE;
	write_x65_entry(entry_offset, ENTRY_VALID, 1, 16, EELITE_STORED, sizeof(EELITE_STORED));
	write_x65_entry(entry_offset - X65_ENTRY_SIZE, ENTRY_FREE, 0, 0, NULL, 0);
	write_x65_entry(entry_offset - X65_ENTRY_SIZE * 2, ENTRY_VALID, 2, 32, EELITE_STORED, sizeof(EELITE_STORED));
	test_eq_u32("X65 stops at the first free EIT entry",
		EEPROM_RESULT_BLOCK_NOT_FOUND,
		eeprom_find_block_in_memory(flash, X65_PARTITION_SIZE, EEPROM_PARTITION_EELITE, 2, &block));

	reset_flash();
	write_partition_header(0, "EEFULL", 16);
	write_x65_entry(X65_PARTITION_SIZE - X65_ENTRY_SIZE, ENTRY_VALID, 9, 16,
		EEFULL_STORED, sizeof(EEFULL_STORED));
	test_eq_u32("X65 finds an extended EEFULL block",
		EEPROM_RESULT_OK,
		eeprom_find_block_in_memory(flash, X65_PARTITION_SIZE, EEPROM_PARTITION_EEFULL, 5009, &block));
	test_eq_u32("X65 excludes all extended EEFULL metadata", sizeof(EXPECTED), block.size);
	test_eq_memory("X65 exposes extended EEFULL payload", EXPECTED, block.data, sizeof(EXPECTED));
}

int main(void) {
	test_start("Siemens EEPROM parser test");
	test_category("X85 EELITE");
	test_x85_eelite_inline_and_duplicates();
	test_category("X85 EEFULL");
	test_x85_eefull_extended_metadata();
	test_category("X65");
	test_x65_free_entry_and_extended_metadata();
	return test_finish();
}
