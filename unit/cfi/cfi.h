#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FLASH_WINDOW_BASE 0xA0000000
#define FLASH_WINDOW_END 0xA8000000
#define FLASH_CHIP_SELECT_COUNT 3
#define CFI_MAX_ERASE_REGIONS 4

struct erase_region {
	uint32_t blocks;
	uint32_t block_size;
};

struct cfi_geometry {
	uint32_t command_set;
	uint32_t primary_address;
	uint32_t size_exponent;
	uint32_t size;
	uint32_t write_buffer_size;
	uint32_t regions;
	struct erase_region erase[CFI_MAX_ERASE_REGIONS];
};

extern const uint32_t cfi_chip_selects[];

uint16_t cfi_read_word(uint32_t base, uint32_t offset);
uint32_t cfi_read_u16(uint32_t base, uint32_t offset);
uint32_t cfi_read_u32(uint32_t base, uint32_t offset);
void cfi_disable_chip_selects(void);
void cfi_map(uint32_t cs, uint32_t base, uint32_t mask);
void cfi_enter_query(uint32_t base);
bool cfi_has_query_signature(uint32_t base);
void cfi_read_geometry(uint32_t base, struct cfi_geometry *geometry);
bool cfi_range_is_blank(uint32_t address, uint32_t size);
