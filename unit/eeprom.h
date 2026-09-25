#pragma once

#include <stdint.h>

#define EEPROM_BLOCK_INLINE_CAPACITY 0x200

enum eeprom_partition_type {
	EEPROM_PARTITION_EELITE,
	EEPROM_PARTITION_EEFULL,
	EEPROM_PARTITION_COUNT,
};

enum eeprom_result {
	EEPROM_RESULT_OK,
	EEPROM_RESULT_FLASH_NOT_MAPPED,
	EEPROM_RESULT_PARTITION_NOT_FOUND,
	EEPROM_RESULT_BLOCK_NOT_FOUND,
	EEPROM_RESULT_INVALID,
};

struct eeprom_block {
	const uint8_t *data;
	uint32_t size;
	uint16_t version;
	uint8_t inline_data[EEPROM_BLOCK_INLINE_CAPACITY];
};

enum eeprom_result eeprom_find_block_in_memory(
	const volatile uint8_t *data,
	uint32_t size,
	enum eeprom_partition_type type,
	uint32_t block_id,
	struct eeprom_block *block
);
enum eeprom_result eeprom_find_block(enum eeprom_partition_type type, uint32_t block_id, struct eeprom_block *block);
