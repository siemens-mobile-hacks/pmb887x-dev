#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	
	while (true) {
		printf("SCU_CHIPID %08X\n", SCU_CHIPID);
		wdt_serve();
	}
	return 0;
}
