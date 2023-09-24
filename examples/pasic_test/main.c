#include <pmb887x.h>
#include <d1601aa.h>
#include <printf.h>

static void dump_all_regs(void) {
	printf("Dump all Dialog registers...\n");
	for (int i = 0; i <= 0xFF; ++i) {
		uint32_t v = i2c_smbus_read_byte(D1601AA_I2C_ADDR, i);
		printf(", 0x%02X", v);
		wdt_serve();
	}
}

int main(void) {
	wdt_init();
	stopwatch_t start = stopwatch_get();
	
	i2c_init();
	dump_all_regs();
	
	return 0;
}
