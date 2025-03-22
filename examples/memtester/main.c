#include "gen/pmb8876_regs.h"
#include "wdt.h"
#include <stdint.h>
#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();

	EBU_BUSCON(1) =
		EBU_BUSCON_AALIGN |
		(3 << EBU_BUSCON_BCGEN_SHIFT) |
		(1 << EBU_BUSCON_PORTW_SHIFT) |
		(3 << EBU_BUSCON_AGEN_SHIFT);

	EBU_ADDRSEL(1) =
		EBU_ADDRSEL_REGENAB |
		(2 << EBU_ADDRSEL_MASK_SHIFT) |
		(0xA8D8 << EBU_ADDRSEL_BASE_SHIFT);

	EBU_BUSAP(1) =
		(2 << EBU_BUSAP_WAITWRC_SHIFT) |
		(2 << EBU_BUSAP_WAITRDC_SHIFT) |
		(1 << EBU_BUSAP_ADDRC_SHIFT);

	EBU_SDRMCON(0) =
		(6 << EBU_SDRMCON_CRAS_SHIFT) |
		(2 << EBU_SDRMCON_CRFSH_SHIFT) |
		(1 << EBU_SDRMCON_CRSC_SHIFT) |
		(2 << EBU_SDRMCON_CRP_SHIFT) |
		(2 << EBU_SDRMCON_AWIDTH_SHIFT) |
		(2 << EBU_SDRMCON_CRCD_SHIFT) |
		(3 << EBU_SDRMCON_CRC_SHIFT) |
		(2 << EBU_SDRMCON_PAGEM_SHIFT) |
		(3 << EBU_SDRMCON_BANKM_SHIFT) |
		(1 << 25);

	EBU_SDRMOD(0) =
		(3 << EBU_SDRMOD_BURSTL_SHIFT) |
		(3 << EBU_SDRMOD_CASLAT_SHIFT) |
		(1 << 29);

	EBU_SDRMREF(0) =
		(0x33 << EBU_SDRMREF_REFRESHC_SHIFT) |
		(1 << EBU_SDRMREF_REFRESHR_SHIFT);

	uint32_t tests[] = {
		0x00000000,
		0xFFFFFFFF,
		0x55555555,
		0xAAAAAAAA,
	};

	printf("Hello World!\n");

	int memory_size = 32 * 1024 * 1024 / 4;
	uint32_t *ram = (uint32_t *) 0xA8D80000;
	bool stop = false;

	printf("Testing RAM for patterns...\n");
	for (int i = 0; i < memory_size; i++) {
		for (int j = 0; j < 4; j++) {
			ram[i] = tests[j];
			if (ram[i] != tests[j]) {
				printf("bad block: %08X [PTR: %08X]\n", i * 4, tests[j]);
				stop = true;
				break;
			}
		}

		if (stop)
			break;

		if ((i % 0x100000) == 0) {
			printf(".");
			wdt_serve();
		}
	}
	printf("\n");
	printf("done!\n");

	wdt_serve();

	printf("Filling entire ram with markers....\n");
	for (int i = memory_size - 1; i >= 0 ; i--) {
		ram[i] = (i * 4) ^ 0xDEAD926E;

		if ((i % 0x100000) == 0) {
			printf(".");
			wdt_serve();
		}
	}
	printf("\n");
	printf("done!\n");

	wdt_serve();

	printf("Checking ram capacity...\n");
	uint32_t end = 0;
	for (int i = 0; i < memory_size; i++) {
		end = i * 4;

		if (ram[i] != ((i * 4) ^ 0xDEAD926E)) {
			break;
		}

		if ((i % 0x100000) == 0) {
			printf(".");
			wdt_serve();
		}
	}
	printf("\n");
	printf("done!\n");
	printf("ram end: %08X %dM [marker: %08X]\n", end, end / 1024 / 1024, ram[end / 4] ^ 0xDEAD926E);

	return 0;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}
