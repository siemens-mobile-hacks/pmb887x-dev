#include <pmb887x.h>

#include "cfi.h"
#include "test.h"

#define FLASH_BUSCON (EBU_BUSCON_AALIGN | (1u << EBU_BUSCON_CTYPE_SHIFT) | \
	(1u << EBU_BUSCON_CMULT_SHIFT) | EBU_BUSCON_DLOAD | (1u << EBU_BUSCON_BCGEN_SHIFT) | \
	(1u << EBU_BUSCON_PORTW_SHIFT))
#define CFI_QUERY_ADDRESS 0x55
#define EBU_CS_COUNT 7
#define RAM_CS 1

const uint32_t cfi_chip_selects[] = {0, 2, 3};

uint16_t cfi_read_word(uint32_t base, uint32_t offset) {
	return MMIO16(base + offset * 2);
}

uint32_t cfi_read_u16(uint32_t base, uint32_t offset) {
	return cfi_read_word(base, offset) | (cfi_read_word(base, offset + 1) << 8);
}

uint32_t cfi_read_u32(uint32_t base, uint32_t offset) {
	return cfi_read_u16(base, offset) | (cfi_read_u16(base, offset + 2) << 16);
}

void cfi_enter_read_array(uint32_t base) {
	MMIO16(base) = 0xFF;
}

void cfi_enter_id(uint32_t base) {
	MMIO16(base) = 0x90;
}

void cfi_enter_query(uint32_t base) {
	MMIO16(base + CFI_QUERY_ADDRESS * 2) = 0x98;
}

void cfi_enter_status(uint32_t base) {
	MMIO16(base) = 0x70;
}

void cfi_disable_chip_selects(void) {
	for (uint32_t cs = 0; cs < EBU_CS_COUNT; cs++) {
		if (cs != RAM_CS) {
			EBU_ADDRSEL(cs) = 0;
			EBU_BUSCON(cs) = 0;
		}
	}
}

void cfi_map(uint32_t cs, uint32_t base, uint32_t mask) {
	EBU_BUSCON(cs) = FLASH_BUSCON;
	EBU_ADDRSEL(cs) = EBU_ADDRSEL_REGENAB | (mask << EBU_ADDRSEL_MASK_SHIFT) | base;
}

static void cfi_read_geometry(struct flash_device *flash) {
	flash->primary_address = cfi_read_u16(flash->base, 0x15);
	flash->size_exponent = cfi_read_word(flash->base, 0x27);
	flash->size = flash->size_exponent < 32 ? 1u << flash->size_exponent : 0;
	uint32_t buffer_exponent = cfi_read_u16(flash->base, 0x2A);
	flash->write_buffer_size = buffer_exponent < 32 ? 1u << buffer_exponent : 0;
	flash->regions = cfi_read_word(flash->base, 0x2C);
	uint32_t regions = flash->regions < CFI_MAX_ERASE_REGIONS ? flash->regions : CFI_MAX_ERASE_REGIONS;
	for (uint32_t region = 0; region < regions; region++) {
		uint32_t offset = 0x2D + region * 4;
		flash->erase[region].blocks = cfi_read_u16(flash->base, offset) + 1;
		flash->erase[region].block_size = cfi_read_u16(flash->base, offset + 2) * 256;
	}
}

static void cfi_skip_pri_regions(uint32_t base, uint32_t *cursor, bool st) {
	uint32_t regions = cfi_read_word(base, (*cursor)++);
	for (uint32_t region = 0; region < regions; region++) {
		*cursor += st ? 2 : 0;
		*cursor += 5;
		uint32_t erase_regions = cfi_read_word(base, (*cursor)++);
		*cursor += erase_regions * (st ? 14 : 8);
	}
}

static void cfi_read_extended_geometry(struct flash_device *flash) {
	flash->features = cfi_read_u32(flash->base, flash->primary_address + 5);
	if (!(flash->features & BIT(9)) || !(flash->features & BIT(10))) {
		return;
	}

	uint32_t cursor = flash->primary_address + 0xE;
	if (flash->features & BIT(6)) {
		uint32_t fields = cfi_read_word(flash->base, cursor++);
		cursor += fields ? 4 + (fields - 1) * 10 : 0;
	}
	cursor++;
	uint32_t synchronous_modes = cfi_read_word(flash->base, cursor++);
	cursor += synchronous_modes;
	bool st = cfi_read_u16(flash->base, 0x13) == 0x0200;
	cfi_skip_pri_regions(flash->base, &cursor, st);

	uint32_t regions = cfi_read_word(flash->base, cursor++);
	if (!regions) {
		return;
	}
	cursor += st ? 2 : 0;
	cursor += 5;
	uint32_t erase_regions = cfi_read_word(flash->base, cursor++);
	if (!erase_regions) {
		return;
	}
	flash->efa_blocks = cfi_read_u16(flash->base, cursor) + 1;
	flash->efa_block_size = cfi_read_u16(flash->base, cursor + 2) * 256;
}

static void cfi_read_map_sample(struct flash_device *flash, uint32_t sample, uint32_t offset) {
	for (uint32_t word = 0; word < MAP_SAMPLE_WORDS; word++) {
		flash->map_samples[sample][word] = MMIO16(flash->base + offset + word * 2);
	}
}

bool cfi_probe(uint32_t cs, struct flash_device *flash) {
	cfi_disable_chip_selects();
	cfi_map(cs, FLASH_WINDOW_BASE, 0);
	cfi_enter_read_array(FLASH_WINDOW_BASE);
	cfi_enter_query(FLASH_WINDOW_BASE);
	bool valid_signature = (
		cfi_read_word(FLASH_WINDOW_BASE, 0x10) == 'Q' &&
		cfi_read_word(FLASH_WINDOW_BASE, 0x11) == 'R' &&
		cfi_read_word(FLASH_WINDOW_BASE, 0x12) == 'Y'
	);
	if (!valid_signature) {
		cfi_enter_read_array(FLASH_WINDOW_BASE);
		EBU_ADDRSEL(cs) = 0;
		EBU_BUSCON(cs) = 0;
		return false;
	}

	flash->cs = cs;
	flash->base = FLASH_WINDOW_BASE;
	cfi_read_geometry(flash);
	cfi_read_extended_geometry(flash);
	cfi_enter_id(flash->base);
	flash->manufacturer = cfi_read_word(flash->base, 0);
	flash->device = cfi_read_word(flash->base, 1);
	cfi_enter_read_array(flash->base);
	bool can_sample_map = (
		flash->size >= sizeof(flash->map_samples[0]) &&
		flash->size <= FLASH_WINDOW_END - FLASH_WINDOW_BASE
	);
	if (can_sample_map) {
		cfi_read_map_sample(flash, 0, 0);
		cfi_read_map_sample(flash, 1, flash->size / 2);
		cfi_read_map_sample(flash, 2, flash->size - sizeof(flash->map_samples[0]));
	}
	return (
		flash->size <= FLASH_WINDOW_END - FLASH_WINDOW_BASE &&
		flash->regions > 0 &&
		flash->regions <= CFI_MAX_ERASE_REGIONS &&
		flash->write_buffer_size >= 2 &&
		flash->write_buffer_size <= 1024
	);
}

bool cfi_range_is_blank(uint32_t address, uint32_t size) {
	for (uint32_t offset = 0; offset < size; offset += sizeof(uint32_t)) {
		if (MMIO32(address + offset) != 0xFFFFFFFF) {
			return false;
		}
		if ((offset & 0x3FFF) == 0) {
			test_watchdog_serve();
		}
	}
	return true;
}

static bool cfi_select_erase_block(struct flash_device *flash, uint32_t blank_offset, uint32_t alignment) {
	uint32_t offset = 0;
	for (uint32_t region = 0; region < flash->regions; region++) {
		for (uint32_t block = 0; block < flash->erase[region].blocks; block++) {
			uint32_t block_size = flash->erase[region].block_size;
			bool inside_blank_range = (
				offset >= blank_offset &&
				offset + block_size <= blank_offset + alignment
			);
			bool has_previous_sample = offset >= MAP_SAMPLE_WORDS * sizeof(uint16_t);
			bool has_next_sample = offset + block_size + MAP_SAMPLE_WORDS * sizeof(uint16_t) <= flash->size;
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
	for (uint32_t offset = 0; offset + alignment <= flash->size; offset += alignment) {
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
	MMIO16(address) = 0x50;
	MMIO16(address) = 0x70;
	uint16_t status = MMIO16(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_unlock_block(uint32_t address, bool efa) {
	MMIO16(address) = 0x50;
	MMIO16(address) = efa ? 0x64 : 0x60;
	MMIO16(address) = 0xD0;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_lock_block(uint32_t address, bool efa) {
	MMIO16(address) = 0x50;
	MMIO16(address) = efa ? 0x64 : 0x60;
	MMIO16(address) = 0x01;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_lock_down_block(uint32_t address, bool efa) {
	MMIO16(address) = efa ? 0x64 : 0x60;
	MMIO16(address) = 0x2F;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_program_word(const struct flash_device *flash, uint32_t address, uint16_t value, bool efa) {
	MMIO16(address) = 0x50;
	MMIO16(address) = efa ? 0x44 : flash->manufacturer == 0x0020 ? 0x41 : 0x40;
	MMIO16(address) = value;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_program_buffer(const struct flash_device *flash, uint32_t address, const uint16_t *data,
	uint32_t words, uint16_t *buffer_status) {
	MMIO16(address) = 0x50;
	MMIO16(address) = flash->manufacturer == 0x0020 ? 0xE9 : 0xE8;
	*buffer_status = cfi_wait_ready(address);
	MMIO16(address) = words - 1;
	for (uint32_t word = 0; word < words; word++) {
		MMIO16(address + word * 2) = data[word];
	}
	MMIO16(address) = 0xD0;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_erase_block(uint32_t address, bool efa) {
	MMIO16(address) = 0x50;
	MMIO16(address) = efa ? 0x24 : 0x20;
	MMIO16(address) = 0xD0;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_blank_check(uint32_t address) {
	MMIO16(address) = 0xBC;
	MMIO16(address) = 0xD0;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	MMIO16(address) = 0x50;
	return status;
}

void cfi_program_configuration(uint32_t base, uint16_t value, bool enhanced) {
	uint32_t address = base + value * 2;
	MMIO16(address) = 0x60;
	MMIO16(address) = enhanced ? 0x04 : 0x03;
}

void cfi_enter_efa(uint32_t address) {
	MMIO16(address) = 0x94;
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

bool cfi_find_blank_efa_block(const struct flash_device *flash, uint32_t *address) {
	for (uint32_t block = 0; block < flash->efa_blocks; block++) {
		uint32_t candidate = flash->base + block * flash->efa_block_size;
		test_watchdog_reset();
		if (cfi_efa_range_is_blank(candidate, flash->efa_block_size)) {
			*address = candidate;
			return true;
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
	MMIO16(address) = 0xC0;
	MMIO16(address) = value;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}
