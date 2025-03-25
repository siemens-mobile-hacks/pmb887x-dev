#include <pmb887x.h>
#include <printf.h>

void flush_cache() {
	/* test and clean, page 2-23 of arm926ejs manual */
	__asm__ volatile("0: mrc p15, 0, r15, c7, c10, 3\n\t" "bne 0b\n" : : : "memory");
	/* disable write buffer as well (page 2-22) */
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (0));
	return;
}


int main(void) {
	wdt_init();

	for (int i = 0x80000; i < 0x80000 + 96 * 1024; i += 4) {
		MMIO32(i) = 0xDEADCAFE;
		flush_cache();
	}

	for (uint32_t i = 0x00400000; i < 0x00400000 + 128 * 1024; i += 4) {
		flush_cache();
		if (MMIO32(i) == 0xDEADCAFE) {
			printf("BROM image size: %d\n", i - 0x00400000);
			break;
		}
	}

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
