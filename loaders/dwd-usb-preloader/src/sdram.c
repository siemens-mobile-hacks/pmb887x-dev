#include "sdram.h"

#include <pmb887x.h>

#define SDRAM_BASE 0xB0000000
#define SDRAM_CS 1
#define SDRAM_CONTROLLER 0
#define SDRAM_ADDRSEL_MASK0_WINDOW_BITS 27
#define SDRAM_FIRST_ADDRESS_BIT 2
#define SDRAM_WINDOW_BITS 26
#define SDRAM_AWIDTH_COUNT 4
#define SDRAM_BANK_BITS 2
#define SDRAM_BANKM_BASE_BIT 19
#define SDRAM_MIN_ADDRESS_BITS (SDRAM_BANKM_BASE_BIT + SDRAM_BANK_BITS + 1)
#define SDRAM_TAG 0x5DA7C0DE
#define SDRAM_REFRESH_PERIOD_64_CYCLES 6

#define SDRAM_SAFE_TIMINGS ( \
	EBU_SDRMCON_CRAS | \
	EBU_SDRMCON_CRFSH | \
	EBU_SDRMCON_CRSC | \
	EBU_SDRMCON_CRP | \
	EBU_SDRMCON_CRCD | \
	EBU_SDRMCON_CRC \
)

static uint32_t sdram_get_addrsel(uint32_t address_bits) {
	uint32_t mask = SDRAM_ADDRSEL_MASK0_WINDOW_BITS - address_bits;
	return SDRAM_BASE | (mask << EBU_ADDRSEL_MASK_SHIFT) | EBU_ADDRSEL_REGENAB;
}

static void sdram_configure(uint32_t addrsel, uint32_t sdrmcon, uint32_t sdrmod) {
	EBU_ADDRSEL(SDRAM_CS) = addrsel;
	EBU_SDRMCON(SDRAM_CONTROLLER) = sdrmcon;
	EBU_SDRMOD(SDRAM_CONTROLLER) = sdrmod;
	while ((EBU_SDRSTAT(SDRAM_CONTROLLER) & EBU_SDRSTAT_SDRM_BUSY) != 0);
}

static uint32_t sdram_measure_address_bits(volatile uint32_t *sdram) {
	sdram[0] = SDRAM_TAG;
	for (uint32_t address_bits = SDRAM_FIRST_ADDRESS_BIT; address_bits < SDRAM_WINDOW_BITS; address_bits++) {
		sdram[BIT(address_bits) / 4] = SDRAM_TAG ^ BIT(address_bits);
		if (sdram[0] != SDRAM_TAG)
			return address_bits;
		for (uint32_t bit = SDRAM_FIRST_ADDRESS_BIT; bit <= address_bits; bit++) {
			if (sdram[BIT(bit) / 4] != (SDRAM_TAG ^ BIT(bit)))
				return address_bits;
		}
	}
	return SDRAM_WINDOW_BITS;
}

/*
 * TC11IB User's Manual, EBU: with a 16-bit port AWIDTH n makes Address(n + 7:0) the column, and PAGEM n
 * compares the page tag from bit n + 8, so the page tag starts right above the column for PAGEM = AWIDTH.
 * BANKM n compares the bank tag from bit 19 + n; the two bank address bits of a four-bank SDRAM are the
 * highest address bits.
 */
void sdram_init(void) {
	uint32_t sdrmod = EBU_SDRMOD_CASLAT_3 | EBU_SDRMOD_BURSTL_8;
	EBU_BUSCON(SDRAM_CS) = (
		EBU_BUSCON_AGEN_SDRAM_0 |
		EBU_BUSCON_PORTW_16BIT |
		EBU_BUSCON_BCGEN_DQM |
		EBU_BUSCON_AALIGN
	);
	EBU_SDRMREF(SDRAM_CONTROLLER) = SDRAM_REFRESH_PERIOD_64_CYCLES << EBU_SDRMREF_REFRESHC_SHIFT;

	uint32_t address_bits = 0;
	uint32_t awidth = 0;
	for (uint32_t candidate = 0; candidate < SDRAM_AWIDTH_COUNT; candidate++) {
		uint32_t sdrmcon = SDRAM_SAFE_TIMINGS | (candidate << EBU_SDRMCON_AWIDTH_SHIFT);
		sdram_configure(sdram_get_addrsel(SDRAM_WINDOW_BITS), sdrmcon, sdrmod);
		uint32_t candidate_bits = sdram_measure_address_bits((volatile uint32_t *) SDRAM_BASE);
		if (candidate_bits > address_bits) {
			address_bits = candidate_bits;
			awidth = candidate;
		}
	}

	if (address_bits < SDRAM_MIN_ADDRESS_BITS) {
		EBU_ADDRSEL(SDRAM_CS) = 0;
		return;
	}

	uint32_t bankm = address_bits - SDRAM_BANK_BITS - SDRAM_BANKM_BASE_BIT;
	uint32_t sdrmcon = (
		SDRAM_SAFE_TIMINGS |
		(awidth << EBU_SDRMCON_AWIDTH_SHIFT) |
		(awidth << EBU_SDRMCON_PAGEM_SHIFT) |
		(bankm << EBU_SDRMCON_BANKM_SHIFT)
	);
	sdram_configure(sdram_get_addrsel(address_bits), sdrmcon, sdrmod);
}
