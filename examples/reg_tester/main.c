#include <pmb887x.h>
#include <printf.h>
/*
F4600010: 00000000000000000000000011111011

F4600014: 00000000000000000001111111111111
F4600018: 00000000000000000000000000000000
F460001C: 00000000000000000000000000101100

F4600020: 00000000000000000001111111111111
F4600024: 00000000000000000000000000000000
F4600028: 00000000000000000000000011111111

F460002C: 00000000000000000000000000000111
F4600030: 00000000000000110000000001111111
F4600034: 00000000000000010011011100000000
*/
void test(uint32_t addr) {
	printf("%08X: ", addr);
	for (int i = 32; i-- > 0;) {
		uint32_t old_bit = (MMIO32(addr) >> i) & 1;
		
		if (old_bit) {
			MMIO32(addr) &= ~(1 << i);
		} else {
			MMIO32(addr) |= (1 << i);
		}
		
		uint32_t new_bit = (MMIO32(addr) >> i) & 1;
		
		printf("%d", old_bit != new_bit);
	}
	
	MMIO32(addr) = 0xFFFFFFFF;
	printf(" %08X", MMIO32(addr));
	
	MMIO32(addr) = 0;
	printf(" %08X", MMIO32(addr));
	
	printf("\n");
}

int main(void) {
	wdt_init();
	cpu_enable_irq(false);
	
	SCU_CLC = 0x200;
	
	for (int i = 0x10; i < 0x50; i += 4) {
		if (i != 0x38 && i != 0x3C && i != 0x40 && i != 0x44 && i != 0x48 && i != 0x4C)
			test(0xF4600000 + i);
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
