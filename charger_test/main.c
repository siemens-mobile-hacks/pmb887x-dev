#include <pmb887x.h>

int main(void) {
	wdt_init();
	
	int i = 0;
	
	REG(0xF43000D0) = 0x700;
	
	int state;
	while (true) {
		if (i % 200000 == 0) {
			REG(0xF4300064) = state ? 0x700 : 0x500;
			state = !state;
		}
		wdt_serve();
		++i;
	}
	
	return 0;
}
