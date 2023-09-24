#include <pmb887x.h>
#include <d1601aa.h>
#include <printf.h>

static void dump_all_regs(void) {
	printf("Dialog on 0x31\n");
	for (int i = 0; i <= 0xFF; ++i) {
		uint32_t v = i2c_smbus_read_byte(0x31, i);
		printf("0x%02X, ", v);
		wdt_serve();
	}
	printf("\ndone!\n");
	
	printf("Dialog on 0x03\n");
	for (int i = 0; i <= 0xFF; ++i) {
		uint32_t v = i2c_smbus_read_byte(0x03, i);
		printf("0x%02X, ", v);
		wdt_serve();
	}
	printf("\ndone!\n");
}

int main(void) {
	wdt_init();
	i2c_init();
	
	gpio_init_output(GPIO_LED_FL_EN, GPIO_OS_NONE, GPIO_PS_MANUAL, false, GPIO_PPEN_PUSHPULL, GPIO_PDPU_NONE, false);
	
	dump_all_regs();
	
	return 0;
}
