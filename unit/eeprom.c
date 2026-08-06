#include <pmb887x.h>

#include "eeprom.h"

#define FLASH_ADDRESS_MIN 0xA0000000
#define FLASH_ADDRESS_MAX 0xB0000000
#define FLASH_WINDOW_MAX_EXPONENT 27
#define EBU_CHIP_SELECT_COUNT 4
#define EEPROM_PARTITION_BLOCK_SIZE 0x10000
#define EEPROM_SG_PARTITION_SIZE (EEPROM_PARTITION_BLOCK_SIZE * 2)
#define EEPROM_NSG_PARTITION_SIZE (EEPROM_PARTITION_BLOCK_SIZE * 4)
#define EEPROM_SG_HEADER_SIZE 16
#define EEPROM_NSG_HEADER_SIZE 32
#define EEPROM_SG_EIT_ENTRY_SIZE 16
#define EEPROM_NSG_EIT_ENTRY_SIZE 32
#define EEPROM_ENTRY_FREE 0xFFFFFFFF
#define EEPROM_ENTRY_VALID 0xFFFFFFC0
#define EEPROM_PARTITION_MARKER 0xFFFFFFF0
#define EEPROM_EEFULL_BLOCK_ID_BASE 5000

enum eeprom_layout {
	EEPROM_LAYOUT_SG,
	EEPROM_LAYOUT_NSG,
};

struct flash_map {
	const volatile uint8_t *data;
	uint32_t size;
};

struct eeprom_partition {
	const volatile uint8_t *data;
	uint32_t size;
	enum eeprom_layout layout;
};

static const uint8_t EEPROM_PARTITION_NAMES[EEPROM_PARTITION_COUNT][8] = {
	[EEPROM_PARTITION_EELITE] = { 'E', 'E', 'L', 'I', 'T', 'E', 0, 0 },
	[EEPROM_PARTITION_EEFULL] = { 'E', 'E', 'F', 'U', 'L', 'L', 0, 0 },
};

static uint16_t read_u16(const volatile uint8_t *data) {
	return data[0] | (uint16_t) data[1] << 8;
}

static uint32_t read_u32(const volatile uint8_t *data) {
	return data[0] | (uint32_t) data[1] << 8 | (uint32_t) data[2] << 16 | (uint32_t) data[3] << 24;
}

static uint32_t find_flash_size(void) {
	uint32_t flash_size = 0;

	for (uint32_t chip_select = 0; chip_select < EBU_CHIP_SELECT_COUNT; chip_select++) {
		uint32_t address_select = EBU_ADDRSEL(chip_select);
		uint32_t address_generation = EBU_BUSCON(chip_select) & EBU_BUSCON_AGEN;
		bool is_sdram = address_generation == EBU_BUSCON_AGEN_SDRAM_0 ||
			address_generation == EBU_BUSCON_AGEN_SDRAM_1;
		uint32_t mask = (address_select & EBU_ADDRSEL_MASK) >> EBU_ADDRSEL_MASK_SHIFT;
		uint32_t base = address_select & EBU_ADDRSEL_BASE;

		if ((address_select & EBU_ADDRSEL_REGENAB) == 0)
			continue;
		if (is_sdram)
			continue;
		if (mask > FLASH_WINDOW_MAX_EXPONENT)
			continue;
		if (base < FLASH_ADDRESS_MIN || base >= FLASH_ADDRESS_MAX)
			continue;

		uint32_t size = 1u << (FLASH_WINDOW_MAX_EXPONENT - mask);
		if (size > FLASH_ADDRESS_MAX - base)
			continue;

		uint32_t window_end = base + size;
		uint32_t mapped_size = window_end - FLASH_ADDRESS_MIN;
		if (mapped_size > flash_size)
			flash_size = mapped_size;
	}

	return flash_size;
}

static bool partition_header_matches(const volatile uint8_t *header, uint32_t size, const uint8_t name[8]) {
	for (size_t i = 0; i < sizeof(EEPROM_PARTITION_NAMES[0]); i++)
		if (header[i] != name[i])
			return false;
	if (read_u16(header + 10) != 0)
		return false;
	if (read_u32(header + 12) != EEPROM_PARTITION_MARKER)
		return false;
	for (uint32_t i = EEPROM_SG_HEADER_SIZE; i < size; i++)
		if (header[i] != 0xFF)
			return false;
	return true;
}

static enum eeprom_result decode_sg_block(
	const struct eeprom_partition *partition,
	enum eeprom_partition_type type,
	const volatile uint8_t *entry,
	struct eeprom_block *block
) {
	uint32_t stored_size = read_u32(entry + 8);
	uint32_t offset = read_u32(entry + 12);
	if (stored_size == 0)
		return EEPROM_RESULT_INVALID;
	if (offset >= partition->size)
		return EEPROM_RESULT_INVALID;
	if (stored_size > partition->size - offset)
		return EEPROM_RESULT_INVALID;

	if (type == EEPROM_PARTITION_EEFULL) {
		block->data = (const uint8_t *) (partition->data + offset + 1);
		block->size = stored_size - 1;
		block->version = partition->data[offset];
	} else {
		block->data = (const uint8_t *) (partition->data + offset);
		block->size = stored_size - 1;
		block->version = partition->data[offset + block->size];
	}
	return EEPROM_RESULT_OK;
}

static enum eeprom_result decode_nsg_block(
	const struct eeprom_partition *partition,
	enum eeprom_partition_type type,
	const volatile uint8_t *entry,
	struct eeprom_block *block
) {
	uint32_t stored_size = type == EEPROM_PARTITION_EEFULL ? read_u32(entry + 8) : read_u16(entry + 8);
	uint32_t offset = read_u32(entry + 12);
	if (stored_size == 0)
		return EEPROM_RESULT_INVALID;
	if (offset >= partition->size)
		return EEPROM_RESULT_INVALID;
	if (stored_size > partition->size - offset)
		return EEPROM_RESULT_INVALID;

	if (type == EEPROM_PARTITION_EEFULL) {
		block->data = (const uint8_t *) (partition->data + offset + 1);
		block->size = stored_size - 1;
		block->version = partition->data[offset];
	} else {
		block->data = (const uint8_t *) (partition->data + offset);
		block->size = stored_size;
		block->version = entry[7];
	}
	return EEPROM_RESULT_OK;
}

static enum eeprom_result find_partition_block(
	const struct eeprom_partition *partition,
	enum eeprom_partition_type type,
	uint32_t block_id,
	struct eeprom_block *block,
	bool *matched
) {
	uint32_t entry_size = partition->layout == EEPROM_LAYOUT_NSG ?
		EEPROM_NSG_EIT_ENTRY_SIZE : EEPROM_SG_EIT_ENTRY_SIZE;
	uint32_t entry_count = partition->size / entry_size - 1;
	*matched = false;

	for (uint32_t number = 1; number <= entry_count; number++) {
		uint32_t offset = partition->size - entry_size * number;
		if (partition->layout == EEPROM_LAYOUT_NSG)
			offset -= entry_size;

		const volatile uint8_t *entry = partition->data + offset;
		uint32_t flags = read_u32(entry);
		if (flags == EEPROM_ENTRY_FREE)
			continue;

		bool short_block_id = partition->layout == EEPROM_LAYOUT_NSG && type == EEPROM_PARTITION_EELITE;
		uint32_t entry_block_id = short_block_id ? read_u16(entry + 4) : read_u32(entry + 4);
		uint32_t free_block_id = short_block_id ? 0xFFFF : EEPROM_ENTRY_FREE;
		if (entry_block_id == free_block_id)
			continue;
		if (type == EEPROM_PARTITION_EEFULL)
			entry_block_id += EEPROM_EEFULL_BLOCK_ID_BASE;
		if (entry_block_id != block_id)
			continue;
		if (flags != EEPROM_ENTRY_VALID)
			continue;
		*matched = true;

		if (partition->layout == EEPROM_LAYOUT_NSG)
			return decode_nsg_block(partition, type, entry, block);
		return decode_sg_block(partition, type, entry, block);
	}

	return EEPROM_RESULT_BLOCK_NOT_FOUND;
}

static enum eeprom_result find_flash_block(
	const struct flash_map *flash,
	enum eeprom_partition_type type,
	uint32_t block_id,
	struct eeprom_block *block,
	bool *partition_found,
	bool *matched
) {
	const uint8_t *name = EEPROM_PARTITION_NAMES[type];
	uint32_t flash_block_count = flash->size / EEPROM_PARTITION_BLOCK_SIZE;
	*matched = false;

	for (uint32_t block_index = 0; block_index < flash_block_count; block_index++) {
		uint32_t offset = block_index * EEPROM_PARTITION_BLOCK_SIZE;
		const volatile uint8_t *flash_block = flash->data + offset;
		struct eeprom_partition partition;
		bool found = false;
		bool sg_partition_fits = flash->size >= EEPROM_SG_PARTITION_SIZE &&
			offset <= flash->size - EEPROM_SG_PARTITION_SIZE;
		if (sg_partition_fits && partition_header_matches(flash_block, EEPROM_SG_HEADER_SIZE, name)) {
			partition.data = flash_block;
			partition.size = EEPROM_SG_PARTITION_SIZE;
			partition.layout = EEPROM_LAYOUT_SG;
			found = true;
		} else {
			const volatile uint8_t *header = flash_block + EEPROM_PARTITION_BLOCK_SIZE - EEPROM_NSG_HEADER_SIZE;
			bool can_be_nsg_partition = offset >= EEPROM_PARTITION_BLOCK_SIZE * 3;
			if (can_be_nsg_partition && partition_header_matches(header, EEPROM_NSG_HEADER_SIZE, name)) {
				partition.data = flash_block - EEPROM_PARTITION_BLOCK_SIZE * 3;
				partition.size = EEPROM_NSG_PARTITION_SIZE;
				partition.layout = EEPROM_LAYOUT_NSG;
				found = true;
			}
		}
		if (!found)
			continue;

		*partition_found = true;
		enum eeprom_result result = find_partition_block(&partition, type, block_id, block, matched);
		if (*matched)
			return result;
	}

	return EEPROM_RESULT_BLOCK_NOT_FOUND;
}

enum eeprom_result eeprom_find_block(enum eeprom_partition_type type, uint32_t block_id, struct eeprom_block *block) {
	if ((uint32_t) type >= EEPROM_PARTITION_COUNT)
		return EEPROM_RESULT_INVALID;

	uint32_t flash_size = find_flash_size();
	if (flash_size == 0)
		return EEPROM_RESULT_FLASH_NOT_MAPPED;

	struct flash_map flash = {
		.data = (const volatile uint8_t *) (uintptr_t) FLASH_ADDRESS_MIN,
		.size = flash_size,
	};
	bool partition_found = false;
	bool matched;
	enum eeprom_result result = find_flash_block(&flash, type, block_id, block, &partition_found, &matched);
	if (matched)
		return result;

	return partition_found ? EEPROM_RESULT_BLOCK_NOT_FOUND : EEPROM_RESULT_PARTITION_NOT_FOUND;
}
