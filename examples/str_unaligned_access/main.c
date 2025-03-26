#include <pmb887x.h>
#include <printf.h>

__attribute__((naked))
void test(uint32_t addr, uint32_t value) {
	__asm__ volatile("str r1, [r0]\nBX LR");
	__asm__ volatile("ldr r1, [r0]\nBX LR");
}

int main(void) {
	wdt_init();
	
	uint32_t addresses[] = {
		0x80000, 0x80001, 0x80002, 0x80003
	};

	for (int i = 0; i < 4; i++) {
		uint32_t addr = addresses[i];

		MMIO8(addr) = 0;
		MMIO8(addr+1) = 0;
		MMIO8(addr+2) = 0;
		MMIO8(addr+3) = 0;

		test(addr, 0xDEADBEEF);

		printf("%08X: %02X %02X %02X %02X\n", addr, MMIO8(addr), MMIO8(addr+1), MMIO8(addr+2), MMIO8(addr+3));
	}
	printf("Done!!!");

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
