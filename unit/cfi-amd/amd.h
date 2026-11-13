#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "cfi/cfi.h"

#define AMD_MAX_BANKS 32
#define AMD_UNLOCK1_ADDRESS 0x555
#define AMD_UNLOCK2_ADDRESS 0x2AA
#define AMD_SAMPLE_WORDS 4
#define AMD_DEVICE_ID_WORDS 3
#define AMD_STATUS_WRITE_BUFFER_ABORT BIT(1)
#define AMD_STATUS_ERASE_TOGGLE BIT(2)
#define AMD_STATUS_ERASE_STARTED BIT(3)
#define AMD_STATUS_EXCEEDED_TIMING_LIMITS BIT(5)
#define AMD_STATUS_TOGGLE BIT(6)
#define AMD_STATUS_DATA_POLLING BIT(7)

enum amd_command_set {
	P_ID_AMD_STD = 0x0002,
};

enum amd_command {
	AMD_CMD_COMMAND_SET_EXIT_CONFIRM = 0x00,
	AMD_CMD_DYB_SET = 0x00,
	AMD_CMD_DYB_CLEAR = 0x01,
	AMD_CMD_WRITE_TO_BUFFER = 0x25,
	AMD_CMD_PROGRAM_BUFFER_TO_FLASH = 0x29,
	AMD_CMD_SECTOR_ERASE = 0x30,
	AMD_CMD_ERASE_RESUME = 0x30,
	AMD_CMD_LOCK_REGISTER_ENTRY = 0x40,
	AMD_CMD_GLOBAL_PPB_LOCK_ENTRY = 0x50,
	AMD_CMD_UNLOCK2 = 0x55,
	AMD_CMD_ERASE_SETUP = 0x80,
	AMD_CMD_SECSI_SECTOR_ENTRY = 0x88,
	AMD_CMD_AUTOSELECT = 0x90,
	AMD_CMD_COMMAND_SET_EXIT = 0x90,
	AMD_CMD_PROGRAM = 0xA0,
	AMD_CMD_UNLOCK1 = 0xAA,
	AMD_CMD_ERASE_SUSPEND = 0xB0,
	AMD_CMD_PPB_ENTRY = 0xC0,
	AMD_CMD_READ_CONFIGURATION_REGISTER = 0xC6,
	AMD_CMD_DYB_ENTRY = 0xE0,
	AMD_CMD_RESET = 0xF0,
};

enum amd_autoselect_word {
	AMD_AUTOSELECT_MANUFACTURER = 0x00,
	AMD_AUTOSELECT_DEVICE1 = 0x01,
	AMD_AUTOSELECT_SECTOR_PROTECTION = 0x02,
	AMD_AUTOSELECT_INDICATOR_BITS = 0x03,
	AMD_AUTOSELECT_DEVICE2 = 0x0E,
	AMD_AUTOSELECT_DEVICE3 = 0x0F,
};

enum amd_operation_result {
	AMD_OPERATION_DONE,
	AMD_OPERATION_FAILED,
	AMD_OPERATION_TIMEOUT,
};

struct amd_flash {
	uint32_t cs;
	uint32_t base;
	struct cfi_geometry cfi;
	uint32_t pri_major;
	uint32_t pri_minor;
	bool address_sensitive_unlock;
	uint32_t banks;
	uint32_t bank_sectors[AMD_MAX_BANKS];
	uint32_t program_timeout_ms;
	uint32_t erase_timeout_ms;
	uint16_t manufacturer;
	uint16_t device[AMD_DEVICE_ID_WORDS];
};

void amd_reset(uint32_t address);
void amd_unlock(uint32_t bank);
void amd_command(uint32_t bank, uint16_t command);
void amd_exit_command_set(uint32_t bank);
bool amd_probe(uint32_t cs, struct amd_flash *flash);
uint32_t amd_sector_count(const struct amd_flash *flash);
uint32_t amd_bank_base(const struct amd_flash *flash, uint32_t bank);
uint32_t amd_bank_of(const struct amd_flash *flash, uint32_t address);
uint16_t amd_read_autoselect(uint32_t bank, enum amd_autoselect_word word);
bool amd_find_blank_sector(const struct amd_flash *flash, uint32_t *sector, uint32_t *size);
enum amd_operation_result amd_wait_ready(uint32_t address, uint32_t timeout_ms, uint32_t *toggles);
uint16_t amd_read_dyb(const struct amd_flash *flash, uint32_t sector);
void amd_write_dyb(const struct amd_flash *flash, uint32_t sector, enum amd_command command);
void amd_start_program_word(const struct amd_flash *flash, uint32_t address, uint16_t value);
void amd_start_program_buffer(const struct amd_flash *flash, uint32_t sector, uint32_t address,
	const uint16_t *data, uint32_t words);
void amd_start_erase_sector(const struct amd_flash *flash, uint32_t sector);
