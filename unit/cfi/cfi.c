#include <pmb887x.h>

#include "cfi/cfi.h"
#include "test.h"

#define FLASH_BUSCON (EBU_BUSCON_AALIGN | (1u << EBU_BUSCON_CTYPE_SHIFT) | \
	(1u << EBU_BUSCON_CMULT_SHIFT) | EBU_BUSCON_DLOAD | (1u << EBU_BUSCON_BCGEN_SHIFT) | \
	(1u << EBU_BUSCON_PORTW_SHIFT))
#define CFI_QUERY_ADDRESS 0x555
#define CFI_QUERY_COMMAND 0x98
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

void cfi_enter_query(uint32_t base) {
	MMIO16(base + CFI_QUERY_ADDRESS * 2) = CFI_QUERY_COMMAND;
}

bool cfi_has_query_signature(uint32_t base) {
	return (
		cfi_read_word(base, 0x10) == 'Q' &&
		cfi_read_word(base, 0x11) == 'R' &&
		cfi_read_word(base, 0x12) == 'Y'
	);
}

void cfi_read_geometry(uint32_t base, struct cfi_geometry *geometry) {
	geometry->command_set = cfi_read_u16(base, 0x13);
	geometry->primary_address = cfi_read_u16(base, 0x15);
	geometry->size_exponent = cfi_read_word(base, 0x27);
	geometry->size = geometry->size_exponent < 32 ? 1u << geometry->size_exponent : 0;
	uint32_t buffer_exponent = cfi_read_u16(base, 0x2A);
	geometry->write_buffer_size = buffer_exponent < 32 ? 1u << buffer_exponent : 0;
	geometry->regions = cfi_read_word(base, 0x2C);
	uint32_t regions = geometry->regions < CFI_MAX_ERASE_REGIONS ? geometry->regions : CFI_MAX_ERASE_REGIONS;
	for (uint32_t region = 0; region < regions; region++) {
		uint32_t offset = 0x2D + region * 4;
		geometry->erase[region].blocks = cfi_read_u16(base, offset) + 1;
		geometry->erase[region].block_size = cfi_read_u16(base, offset + 2) * 256;
	}
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
