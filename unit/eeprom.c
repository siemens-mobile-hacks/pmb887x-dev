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
#define EEPROM_NSG_DATA_CHUNK_SIZE 0x10
#define EEPROM_NSG_EELITE_INLINE_MAX 0x10
#define EEPROM_NSG_EEFULL_INLINE_MAX 0x200
#define EEPROM_ENTRY_STATE_MASK 0xAAAAAAAA
#define EEPROM_ENTRY_FREE 0xAAAAAAAA
#define EEPROM_ENTRY_WRITING 0xAAAAAAA8
#define EEPROM_ENTRY_VALID 0xAAAAAA80
#define EEPROM_ENTRY_OBSOLETE 0xAAAAAA00
#define EEPROM_PARTITION_MARKER 0xFFFFFFF0
#define EEPROM_EEFULL_BLOCK_ID_BASE 5000
#define EEPROM_EEFULL_EXTENDED_VERSION_FLAG 0x80
#define EEPROM_EEFULL_METADATA_SIZE 1
#define EEPROM_EEFULL_EXTENDED_METADATA_SIZE 6

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
	return data[0] | ((uint16_t) data[1] << 8);
}

static uint32_t read_u32(const volatile uint8_t *data) {
	return data[0] | ((uint32_t) data[1] << 8) | ((uint32_t) data[2] << 16) | ((uint32_t) data[3] << 24);
}

static bool is_entry_state_valid(uint32_t state) {
	switch (state) {
		case EEPROM_ENTRY_FREE:
		case EEPROM_ENTRY_WRITING:
		case EEPROM_ENTRY_VALID:
		case EEPROM_ENTRY_OBSOLETE:
			return true;
	}

	return false;
}

static uint32_t align_nsg_data_offset(uint32_t offset) {
	if ((offset & EEPROM_NSG_DATA_CHUNK_SIZE) == 0)
		return offset;

	return offset + EEPROM_NSG_DATA_CHUNK_SIZE * 2 -
		(offset & (EEPROM_NSG_DATA_CHUNK_SIZE * 2 - 1));
}

static uint32_t nsg_data_byte_offset(uint32_t offset, uint32_t index) {
	uint32_t chunk_offset = offset & (EEPROM_NSG_DATA_CHUNK_SIZE - 1);

	return offset + index +
		((chunk_offset + index) / EEPROM_NSG_DATA_CHUNK_SIZE) * EEPROM_NSG_DATA_CHUNK_SIZE;
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
		uint8_t version = partition->data[offset];
		uint32_t metadata_size = (version & EEPROM_EEFULL_EXTENDED_VERSION_FLAG) != 0 && version != 0xFF ?
			EEPROM_EEFULL_EXTENDED_METADATA_SIZE : EEPROM_EEFULL_METADATA_SIZE;

		if (stored_size < metadata_size)
			return EEPROM_RESULT_INVALID;
		block->data = (const uint8_t *) (partition->data + offset + 1);
		block->size = stored_size - metadata_size;
		block->version = version;
	} else {
		block->data = (const uint8_t *) (partition->data + offset);
		block->size = stored_size - 1;
		block->version = partition->data[offset + block->size];
	}
	return EEPROM_RESULT_OK;
}

static enum eeprom_result copy_nsg_inline_block(
	const struct eeprom_partition *partition,
	uint32_t stored_offset,
	uint32_t source_offset,
	uint32_t size,
	struct eeprom_block *block
) {
	if (size > sizeof(block->inline_data))
		return EEPROM_RESULT_INVALID;

	for (uint32_t i = 0; i < size; i++) {
		uint32_t offset = nsg_data_byte_offset(stored_offset, source_offset + i);

		if (offset >= partition->size)
			return EEPROM_RESULT_INVALID;
		block->inline_data[i] = partition->data[offset];
	}
	block->data = block->inline_data;
	return EEPROM_RESULT_OK;
}

static enum eeprom_result decode_nsg_block(
	const struct eeprom_partition *partition,
	enum eeprom_partition_type type,
	uint32_t entry_offset,
	const volatile uint8_t *entry,
	struct eeprom_block *block
) {
	uint32_t stored_size = type == EEPROM_PARTITION_EEFULL ? read_u32(entry + 8) : read_u16(entry + 8);
	if (stored_size == 0)
		return EEPROM_RESULT_INVALID;

	uint32_t inline_limit = type == EEPROM_PARTITION_EEFULL ?
		EEPROM_NSG_EEFULL_INLINE_MAX : EEPROM_NSG_EELITE_INLINE_MAX;
	bool is_inline = stored_size <= inline_limit;
	uint32_t stored_offset;
	if (is_inline) {
		uint32_t reserved_size = EEPROM_NSG_DATA_CHUNK_SIZE + stored_size +
			(stored_size & ~(EEPROM_NSG_DATA_CHUNK_SIZE - 1));

		if (entry_offset < reserved_size)
			return EEPROM_RESULT_INVALID;
		stored_offset = align_nsg_data_offset(entry_offset - reserved_size);
	} else {
		stored_offset = read_u32(entry + 12);
		if (stored_offset >= partition->size)
			return EEPROM_RESULT_INVALID;
		if (stored_size > partition->size - stored_offset)
			return EEPROM_RESULT_INVALID;
	}

	if (type == EEPROM_PARTITION_EEFULL) {
		uint32_t version_offset = is_inline ? nsg_data_byte_offset(stored_offset, 0) : stored_offset;
		if (version_offset >= partition->size)
			return EEPROM_RESULT_INVALID;
		uint8_t version = partition->data[version_offset];
		uint32_t metadata_size = (version & EEPROM_EEFULL_EXTENDED_VERSION_FLAG) != 0 && version != 0xFF ?
			EEPROM_EEFULL_EXTENDED_METADATA_SIZE : EEPROM_EEFULL_METADATA_SIZE;

		if (stored_size < metadata_size)
			return EEPROM_RESULT_INVALID;
		block->size = stored_size - metadata_size;
		block->version = version;
		if (is_inline)
			return copy_nsg_inline_block(partition, stored_offset, 1, block->size, block);
		block->data = (const uint8_t *) (partition->data + stored_offset + 1);
	} else {
		block->size = stored_size;
		block->version = entry[7];
		if (is_inline)
			return copy_nsg_inline_block(partition, stored_offset, 0, block->size, block);
		block->data = (const uint8_t *) (partition->data + stored_offset);
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
		uint32_t state = read_u32(entry) & EEPROM_ENTRY_STATE_MASK;
		if (!is_entry_state_valid(state)) {
			if (partition->layout == EEPROM_LAYOUT_SG)
				return EEPROM_RESULT_INVALID;
			continue;
		}
		if (partition->layout == EEPROM_LAYOUT_SG && state == EEPROM_ENTRY_FREE)
			return *matched ? EEPROM_RESULT_OK : EEPROM_RESULT_BLOCK_NOT_FOUND;
		if (state != EEPROM_ENTRY_VALID)
			continue;

		bool short_block_id = partition->layout == EEPROM_LAYOUT_NSG && type == EEPROM_PARTITION_EELITE;
		uint32_t entry_block_id = short_block_id ? read_u16(entry + 4) : read_u32(entry + 4);
		uint32_t maximum_id = type == EEPROM_PARTITION_EELITE ?
			EEPROM_EEFULL_BLOCK_ID_BASE - 1 : UINT32_MAX - EEPROM_EEFULL_BLOCK_ID_BASE;
		if (entry_block_id > maximum_id) {
			if (partition->layout == EEPROM_LAYOUT_SG)
				return EEPROM_RESULT_INVALID;
			continue;
		}
		if (type == EEPROM_PARTITION_EEFULL)
			entry_block_id += EEPROM_EEFULL_BLOCK_ID_BASE;
		if (entry_block_id != block_id)
			continue;
		*matched = true;

		enum eeprom_result result = partition->layout == EEPROM_LAYOUT_NSG ?
			decode_nsg_block(partition, type, offset, entry, block) :
			decode_sg_block(partition, type, entry, block);
		if (result != EEPROM_RESULT_OK)
			return result;
		if (partition->layout == EEPROM_LAYOUT_SG)
			return EEPROM_RESULT_OK;
	}

	return *matched ? EEPROM_RESULT_OK : EEPROM_RESULT_BLOCK_NOT_FOUND;
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
		if (result == EEPROM_RESULT_INVALID)
			return result;
		if (*matched)
			return EEPROM_RESULT_OK;
	}

	return EEPROM_RESULT_BLOCK_NOT_FOUND;
}

enum eeprom_result eeprom_find_block_in_memory(
	const volatile uint8_t *data,
	uint32_t size,
	enum eeprom_partition_type type,
	uint32_t block_id,
	struct eeprom_block *block
) {
	if ((uint32_t) type >= EEPROM_PARTITION_COUNT)
		return EEPROM_RESULT_INVALID;
	if (data == NULL || block == NULL)
		return EEPROM_RESULT_INVALID;

	struct flash_map flash = {
		.data = data,
		.size = size,
	};
	bool partition_found = false;
	bool matched;
	enum eeprom_result result = find_flash_block(&flash, type, block_id, block, &partition_found, &matched);
	if (result == EEPROM_RESULT_INVALID || matched)
		return result;

	return partition_found ? EEPROM_RESULT_BLOCK_NOT_FOUND : EEPROM_RESULT_PARTITION_NOT_FOUND;
}

enum eeprom_result eeprom_find_block(enum eeprom_partition_type type, uint32_t block_id, struct eeprom_block *block) {
	uint32_t flash_size = find_flash_size();
	if (flash_size == 0)
		return EEPROM_RESULT_FLASH_NOT_MAPPED;

	return eeprom_find_block_in_memory(
		(const volatile uint8_t *) (uintptr_t) FLASH_ADDRESS_MIN,
		flash_size,
		type,
		block_id,
		block
	);
}
