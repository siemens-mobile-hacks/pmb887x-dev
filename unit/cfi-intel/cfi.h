#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FLASH_WINDOW_BASE 0xA0000000
#define FLASH_WINDOW_END 0xA8000000
#define CFI_MAX_ERASE_REGIONS 4
#define MAP_SAMPLE_WORDS 4
#define FLASH_CHIP_SELECT_COUNT 3
#define STATUS_READY BIT(7)
#define STATUS_ERRORS (BIT(1) | BIT(3) | BIT(4) | BIT(5) | BIT(8) | BIT(9))

struct erase_region {
	uint32_t blocks;
	uint32_t block_size;
};

struct flash_device {
	uint32_t cs;
	uint32_t base;
	uint32_t manufacturer;
	uint32_t device;
	uint32_t primary_address;
	uint32_t size_exponent;
	uint32_t size;
	uint32_t regions;
	uint32_t write_buffer_size;
	uint32_t features;
	uint32_t efa_blocks;
	uint32_t efa_block_size;
	uint32_t blank_base;
	uint32_t blank_size;
	uint32_t test_block;
	uint32_t test_block_size;
	bool mapped;
	struct erase_region erase[CFI_MAX_ERASE_REGIONS];
	uint16_t map_samples[3][MAP_SAMPLE_WORDS];
};

extern const uint32_t cfi_chip_selects[];

uint16_t cfi_read_word(uint32_t base, uint32_t offset);
uint32_t cfi_read_u16(uint32_t base, uint32_t offset);
uint32_t cfi_read_u32(uint32_t base, uint32_t offset);
void cfi_enter_read_array(uint32_t base);
void cfi_enter_id(uint32_t base);
void cfi_enter_query(uint32_t base);
void cfi_enter_status(uint32_t base);
void cfi_disable_chip_selects(void);
void cfi_map(uint32_t cs, uint32_t base, uint32_t mask);
bool cfi_probe(uint32_t cs, struct flash_device *flash);
bool cfi_range_is_blank(uint32_t address, uint32_t size);
bool cfi_find_blank_block(struct flash_device *flash, uint32_t alignment);
uint16_t cfi_wait_ready(uint32_t address);
uint16_t cfi_read_lock_status(uint32_t address);
uint16_t cfi_clear_status(uint32_t address);
uint16_t cfi_unlock_block(uint32_t address, bool efa);
uint16_t cfi_lock_block(uint32_t address, bool efa);
uint16_t cfi_lock_down_block(uint32_t address, bool efa);
uint16_t cfi_program_word(const struct flash_device *flash, uint32_t address, uint16_t value, bool efa);
uint16_t cfi_program_buffer(const struct flash_device *flash, uint32_t address, const uint16_t *data,
	uint32_t words, uint16_t *buffer_status);
uint16_t cfi_erase_block(uint32_t address, bool efa);
uint16_t cfi_blank_check(uint32_t address);
void cfi_program_configuration(uint32_t base, uint16_t value, bool enhanced);
void cfi_enter_efa(uint32_t address);
bool cfi_efa_range_is_blank(uint32_t address, uint32_t size);
bool cfi_find_blank_efa_block(const struct flash_device *flash, uint32_t *address);
uint16_t cfi_read_efa_word(uint32_t address);
uint32_t cfi_read_efa_lock_status(uint32_t address);
uint16_t cfi_program_otp_word(uint32_t address, uint16_t value);
