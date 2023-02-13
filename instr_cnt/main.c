#include <pmb887x.h>
#include <printf.h>

static void my_delay(uint32_t cycles) {
	__asm__ volatile (
		"1: \n"
		"NOP\n"
		"ADD %0, #1\n"
		"SUB %0, #1\n"
		"MOV %0, %0\n"
		"CMP %0, #1\n"
		"NOP\n"
		"SUBS %0, #1\n"
		"BCS 1b \n" : : "r" (cycles) : "memory"
	);
}

int main(void) {
	wdt_init();
	
	int cnt = 1000000;
	
	stopwatch_t start = stopwatch_get();
	my_delay(cnt);
	uint32_t elapsed = stopwatch_elapsed_us(start);
	
	printf("elapsed=%d\n", elapsed);
	
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
