#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	
	EBU_ADDRSEL(5) = 0x90000011;
	
	while (true) {
		printf("0x90000000 %08X\n", REG(0x90000000));
		// wdt_serve();
	}
	return 0;
}
