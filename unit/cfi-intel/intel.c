#include <pmb887x.h>

#include "intel.h"
#include "test.h"

#define PRI_FEATURES 0x05
#define PRI_PROTECTION_FIELDS 0x0E
#define PRI_OTP_GROUP_COUNT_BYTES 2
#define PRI_OTP_SIZE_EXPONENT_BYTES 1
#define PRI_PAGE_MODE_BYTES 1
#define PRI_V14_RECORD_SIZE_BYTES 2
#define PRI_BANK_COUNT_BYTES 2
#define PRI_SIMULTANEOUS_OPS_BYTES 3
#define PRI_ERASE_REGION_BYTES 8
#define PRI_V14_ERASE_REGION_BYTES 6

void cfi_enter_read_array(uint32_t base) {
	MMIO16(base) = CFI_CMD_READ_ARRAY;
}

void cfi_enter_id(uint32_t base) {
	MMIO16(base) = CFI_CMD_READ_DEVICE_IDENTIFIER;
}

void cfi_enter_status(uint32_t base) {
	MMIO16(base) = CFI_CMD_READ_STATUS_REGISTER;
}

static uint32_t cfi_read_bank_region_header(uint32_t base, uint32_t *cursor, bool v14) {
	*cursor += v14 ? PRI_V14_RECORD_SIZE_BYTES : 0;
	uint32_t banks = cfi_read_u16(base, *cursor);
	*cursor += PRI_BANK_COUNT_BYTES + PRI_SIMULTANEOUS_OPS_BYTES;
	return banks;
}

static uint32_t cfi_erase_region_bytes(bool v14) {
	return PRI_ERASE_REGION_BYTES + (v14 ? PRI_V14_ERASE_REGION_BYTES : 0);
}

static void cfi_skip_pri_regions(uint32_t base, uint32_t *cursor, bool v14) {
	uint32_t regions = cfi_read_word(base, (*cursor)++);
	for (uint32_t region = 0; region < regions; region++) {
		cfi_read_bank_region_header(base, cursor, v14);
		uint32_t erase_regions = cfi_read_word(base, (*cursor)++);
		*cursor += erase_regions * cfi_erase_region_bytes(v14);
	}
}

static bool cfi_read_efa_regions(struct flash_device *flash, uint32_t *cursor, bool v14) {
	uint32_t regions = cfi_read_word(flash->base, (*cursor)++);
	for (uint32_t region = 0; region < regions; region++) {
		uint32_t banks = cfi_read_bank_region_header(flash->base, cursor, v14);
		uint32_t erase_regions = cfi_read_word(flash->base, (*cursor)++);
		if (erase_regions > CFI_MAX_ERASE_REGIONS)
			return false;

		struct erase_region bank[CFI_MAX_ERASE_REGIONS];
		for (uint32_t i = 0; i < erase_regions; i++) {
			bank[i].blocks = cfi_read_u16(flash->base, *cursor) + 1;
			bank[i].block_size = cfi_read_u16(flash->base, *cursor + 2) * 256;
			*cursor += cfi_erase_region_bytes(v14);
		}
		for (uint32_t i = 0; i < banks; i++) {
			for (uint32_t j = 0; j < erase_regions; j++) {
				if (flash->efa_regions == CFI_MAX_ERASE_REGIONS)
					return false;
				flash->efa_erase[flash->efa_regions++] = bank[j];
				flash->efa_size += bank[j].blocks * bank[j].block_size;
			}
		}
	}
	return true;
}

static void cfi_read_protection_fields(struct flash_device *flash, uint32_t *cursor) {
	uint32_t fields = cfi_read_word(flash->base, (*cursor)++);
	for (uint32_t field = 0; field < fields; field++) {
		if (field == 0) {
			flash->otp0_address = cfi_read_u16(flash->base, *cursor);
			*cursor += 2;
			flash->otp0_factory_size = 1u << cfi_read_word(flash->base, (*cursor)++);
			flash->otp0_user_size = 1u << cfi_read_word(flash->base, (*cursor)++);
			continue;
		}

		uint32_t address = cfi_read_u32(flash->base, *cursor);
		*cursor += 4;
		*cursor += PRI_OTP_GROUP_COUNT_BYTES + PRI_OTP_SIZE_EXPONENT_BYTES + PRI_OTP_GROUP_COUNT_BYTES;
		uint32_t group_size = 1u << cfi_read_word(flash->base, (*cursor)++);
		if (field == 1) {
			flash->otp1_address = address;
			flash->otp1_group_size = group_size;
		}
	}
}

static void cfi_read_extended_geometry(struct flash_device *flash) {
	flash->features = cfi_read_u32(flash->base, flash->cfi.primary_address + PRI_FEATURES);
	uint32_t cursor = flash->cfi.primary_address + PRI_PROTECTION_FIELDS;
	if (flash->features & BIT(6))
		cfi_read_protection_fields(flash, &cursor);
	if (!(flash->features & BIT(9)) || !(flash->features & BIT(10))) {
		return;
	}

	cursor += PRI_PAGE_MODE_BYTES;
	uint32_t synchronous_modes = cfi_read_word(flash->base, cursor++);
	cursor += synchronous_modes;
	bool v14 = flash->cfi.command_set == P_ID_INTEL_PERFORMANCE;
	cfi_skip_pri_regions(flash->base, &cursor, v14);
	if (!cfi_read_efa_regions(flash, &cursor, v14)) {
		flash->efa_regions = 0;
		flash->efa_size = 0;
	}
}

static void cfi_read_map_sample(struct flash_device *flash, uint32_t sample, uint32_t offset) {
	for (uint32_t word = 0; word < MAP_SAMPLE_WORDS; word++) {
		flash->map_samples[sample][word] = MMIO16(flash->base + offset + word * 2);
	}
}

void cfi_sample_map(struct flash_device *flash) {
	cfi_read_map_sample(flash, 0, 0);
	cfi_read_map_sample(flash, 1, flash->cfi.size / 2);
	cfi_read_map_sample(flash, 2, flash->cfi.size - sizeof(flash->map_samples[0]));
}

static bool is_intel_command_set(uint32_t command_set) {
	return (
		command_set == P_ID_INTEL_EXT ||
		command_set == P_ID_INTEL_STD ||
		command_set == P_ID_INTEL_PERFORMANCE
	);
}

bool cfi_probe(uint32_t cs, struct flash_device *flash) {
	cfi_disable_chip_selects();
	cfi_map(cs, FLASH_WINDOW_BASE, 0);
	cfi_enter_read_array(FLASH_WINDOW_BASE);
	cfi_enter_query(FLASH_WINDOW_BASE);
	struct cfi_geometry geometry = { 0 };
	bool has_cfi = cfi_has_query_signature(FLASH_WINDOW_BASE);
	if (has_cfi)
		cfi_read_geometry(FLASH_WINDOW_BASE, &geometry);
	if (!has_cfi || !is_intel_command_set(geometry.command_set)) {
		if (has_cfi)
			printf("# CS%lu: command set %04lX is not Intel, skipped\n", cs, geometry.command_set);
		cfi_enter_read_array(FLASH_WINDOW_BASE);
		EBU_ADDRSEL(cs) = 0;
		EBU_BUSCON(cs) = 0;
		return false;
	}

	*flash = (struct flash_device) { 0 };
	flash->cs = cs;
	flash->base = FLASH_WINDOW_BASE;
	flash->cfi = geometry;
	cfi_read_extended_geometry(flash);
	cfi_enter_id(flash->base);
	flash->manufacturer = cfi_read_word(flash->base, 0);
	flash->device = cfi_read_word(flash->base, 1);
	cfi_enter_read_array(flash->base);
	bool can_sample_map = (
		flash->cfi.size >= sizeof(flash->map_samples[0]) &&
		flash->cfi.size <= FLASH_WINDOW_END - FLASH_WINDOW_BASE
	);
	flash->window_size = flash->cfi.size;
	if (can_sample_map) {
		cfi_sample_map(flash);
	}
	return (
		flash->cfi.size <= FLASH_WINDOW_END - FLASH_WINDOW_BASE &&
		flash->cfi.regions > 0 &&
		flash->cfi.regions <= CFI_MAX_ERASE_REGIONS &&
		flash->cfi.write_buffer_size >= 2 &&
		flash->cfi.write_buffer_size <= 1024
	);
}

static bool cfi_select_erase_block(struct flash_device *flash, uint32_t blank_offset, uint32_t alignment) {
	uint32_t offset = 0;
	for (uint32_t region = 0; region < flash->cfi.regions; region++) {
		for (uint32_t block = 0; block < flash->cfi.erase[region].blocks; block++) {
			uint32_t block_size = flash->cfi.erase[region].block_size;
			bool inside_blank_range = (
				offset >= blank_offset &&
				offset + block_size <= blank_offset + alignment
			);
			bool has_previous_sample = offset >= MAP_SAMPLE_WORDS * sizeof(uint16_t);
			bool has_next_sample = offset + block_size + MAP_SAMPLE_WORDS * sizeof(uint16_t) <= flash->cfi.size;
			if (inside_blank_range && has_previous_sample && has_next_sample) {
				flash->test_block = flash->base + offset;
				flash->test_block_size = block_size;
				return true;
			}
			offset += block_size;
		}
	}
	return false;
}

bool cfi_find_blank_block(struct flash_device *flash, uint32_t alignment) {
	for (uint32_t offset = 0; offset + alignment <= flash->cfi.size; offset += alignment) {
		test_watchdog_reset();
		if (!cfi_range_is_blank(flash->base + offset, alignment)) {
			continue;
		}
		if (cfi_select_erase_block(flash, offset, alignment)) {
			return true;
		}
	}
	return false;
}

uint16_t cfi_wait_ready(uint32_t address) {
	uint16_t status = 0;
	for (uint32_t poll = 0; poll < 1000000; poll++) {
		status = MMIO16(address);
		if (status & STATUS_READY) {
			break;
		}
		test_watchdog_serve();
	}
	return status;
}

uint16_t cfi_read_lock_status(uint32_t address) {
	cfi_enter_id(address);
	uint16_t status = cfi_read_word(address, 2);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_clear_status(uint32_t address) {
	MMIO16(address) = CFI_CMD_CLEAR_STATUS_REGISTER;
	MMIO16(address) = CFI_CMD_READ_STATUS_REGISTER;
	uint16_t status = MMIO16(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_unlock_block(uint32_t address, bool efa) {
	MMIO16(address) = CFI_CMD_CLEAR_STATUS_REGISTER;
	MMIO16(address) = efa ? CFI_CMD_EFA_BLOCK_LOCK_SETUP : CFI_CMD_BLOCK_LOCK_SETUP;
	MMIO16(address) = CFI_CMD_CONFIRM;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_lock_block(uint32_t address, bool efa) {
	MMIO16(address) = CFI_CMD_CLEAR_STATUS_REGISTER;
	MMIO16(address) = efa ? CFI_CMD_EFA_BLOCK_LOCK_SETUP : CFI_CMD_BLOCK_LOCK_SETUP;
	MMIO16(address) = CFI_CMD_BLOCK_LOCK_CONFIRM;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_lock_down_block(uint32_t address, bool efa) {
	MMIO16(address) = efa ? CFI_CMD_EFA_BLOCK_LOCK_SETUP : CFI_CMD_BLOCK_LOCK_SETUP;
	MMIO16(address) = CFI_CMD_BLOCK_LOCK_DOWN_CONFIRM;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_program_word(const struct flash_device *flash, uint32_t address, uint16_t value, bool efa) {
	MMIO16(address) = CFI_CMD_CLEAR_STATUS_REGISTER;
	uint16_t command = CFI_CMD_WORD_PROGRAM_SETUP;
	if (efa) {
		command = CFI_CMD_EFA_PROGRAM_SETUP;
	} else if (flash->cfi.command_set == P_ID_INTEL_PERFORMANCE) {
		command = CFI_CMD_PROGRAM_SETUP;
	}
	MMIO16(address) = command;
	MMIO16(address) = value;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_program_buffer(const struct flash_device *flash, uint32_t address, const uint16_t *data,
	uint32_t words, uint16_t *buffer_status) {
	MMIO16(address) = CFI_CMD_CLEAR_STATUS_REGISTER;
	MMIO16(address) = flash->cfi.command_set == P_ID_INTEL_PERFORMANCE ? CFI_CMD_BUFFER_PROGRAM : CFI_CMD_BUFFERED_PROGRAM;
	*buffer_status = cfi_wait_ready(address);
	MMIO16(address) = words - 1;
	for (uint32_t word = 0; word < words; word++) {
		MMIO16(address + word * 2) = data[word];
	}
	MMIO16(address) = CFI_CMD_CONFIRM;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_erase_block(uint32_t address, bool efa) {
	MMIO16(address) = CFI_CMD_CLEAR_STATUS_REGISTER;
	MMIO16(address) = efa ? CFI_CMD_EFA_BLOCK_ERASE_SETUP : CFI_CMD_BLOCK_ERASE_SETUP;
	MMIO16(address) = CFI_CMD_CONFIRM;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_blank_check(uint32_t address) {
	MMIO16(address) = CFI_CMD_BLANK_CHECK_SETUP;
	MMIO16(address) = CFI_CMD_CONFIRM;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	MMIO16(address) = CFI_CMD_CLEAR_STATUS_REGISTER;
	return status;
}

void cfi_program_configuration(uint32_t base, uint16_t value, bool enhanced) {
	uint32_t address = base + value * 2;
	MMIO16(address) = CFI_CMD_BLOCK_LOCK_SETUP;
	MMIO16(address) = enhanced ?
		CFI_CMD_SET_ENHANCED_CONFIGURATION_REGISTER_CONFIRM :
		CFI_CMD_SET_CONFIGURATION_REGISTER_CONFIRM;
}

void cfi_enter_efa(uint32_t address) {
	MMIO16(address) = CFI_CMD_READ_EFA;
}

bool cfi_efa_range_is_blank(uint32_t address, uint32_t size) {
	cfi_enter_efa(address);
	bool blank = true;
	for (uint32_t offset = 0; offset < size; offset += sizeof(uint32_t)) {
		if (MMIO32(address + offset) != 0xFFFFFFFF) {
			blank = false;
			break;
		}
		test_watchdog_serve();
	}
	cfi_enter_read_array(address);
	return blank;
}

bool cfi_find_blank_efa_block(const struct flash_device *flash, uint32_t *address, uint32_t *size) {
	uint32_t candidate = flash->base;
	for (uint32_t region = 0; region < flash->efa_regions; region++) {
		const struct erase_region *erase = &flash->efa_erase[region];
		for (uint32_t block = 0; block < erase->blocks; block++) {
			test_watchdog_reset();
			if (cfi_efa_range_is_blank(candidate, erase->block_size)) {
				*address = candidate;
				*size = erase->block_size;
				return true;
			}
			candidate += erase->block_size;
		}
	}
	return false;
}

uint16_t cfi_read_efa_word(uint32_t address) {
	cfi_enter_efa(address);
	uint16_t value = MMIO16(address);
	cfi_enter_read_array(address);
	return value;
}

uint32_t cfi_read_efa_lock_status(uint32_t address) {
	cfi_enter_id(address);
	uint32_t status = (cfi_read_word(address, 2) >> 4) & 0x3;
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_program_otp_word(uint32_t address, uint16_t value) {
	MMIO16(address) = CFI_CMD_PROTECTION_REGISTER_PROGRAM;
	MMIO16(address) = value;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}
