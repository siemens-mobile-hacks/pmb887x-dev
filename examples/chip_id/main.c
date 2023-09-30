#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	
	printf("SCU_MANID=%04X\n", SCU_MANID);
	printf("SCU_CHIPID=%04X\n", SCU_CHIPID);
	
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
