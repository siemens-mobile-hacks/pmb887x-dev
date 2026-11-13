#include <pmb887x.h>

#include "cfi.h"
#include "test.h"

#define FLASH_BUSCON (EBU_BUSCON_AALIGN | (1u << EBU_BUSCON_CTYPE_SHIFT) | \
	(1u << EBU_BUSCON_CMULT_SHIFT) | EBU_BUSCON_DLOAD | (1u << EBU_BUSCON_BCGEN_SHIFT) | \
	(1u << EBU_BUSCON_PORTW_SHIFT))
#define CFI_QUERY_ADDRESS 0x55
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
	flash->command_set = cfi_read_u16(flash->base, 0x13);
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
	flash->features = cfi_read_u32(flash->base, flash->primary_address + PRI_FEATURES);
	uint32_t cursor = flash->primary_address + PRI_PROTECTION_FIELDS;
	if (flash->features & BIT(6))
		cfi_read_protection_fields(flash, &cursor);
	if (!(flash->features & BIT(9)) || !(flash->features & BIT(10))) {
		return;
	}

	cursor += PRI_PAGE_MODE_BYTES;
	uint32_t synchronous_modes = cfi_read_word(flash->base, cursor++);
	cursor += synchronous_modes;
	bool v14 = flash->command_set == P_ID_INTEL_PERFORMANCE;
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
	cfi_read_map_sample(flash, 1, flash->size / 2);
	cfi_read_map_sample(flash, 2, flash->size - sizeof(flash->map_samples[0]));
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

	*flash = (struct flash_device) { 0 };
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
	flash->window_size = flash->size;
	if (can_sample_map) {
		cfi_sample_map(flash);
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
	MMIO16(address) = efa ? 0x44 : flash->command_set == P_ID_INTEL_PERFORMANCE ? 0x41 : 0x40;
	MMIO16(address) = value;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}

uint16_t cfi_program_buffer(const struct flash_device *flash, uint32_t address, const uint16_t *data,
	uint32_t words, uint16_t *buffer_status) {
	MMIO16(address) = 0x50;
	MMIO16(address) = flash->command_set == P_ID_INTEL_PERFORMANCE ? 0xE9 : 0xE8;
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
	MMIO16(address) = 0xC0;
	MMIO16(address) = value;
	uint16_t status = cfi_wait_ready(address);
	cfi_enter_read_array(address);
	return status;
}
