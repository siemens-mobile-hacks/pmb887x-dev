#include <pmb8876.h>

const char test_const_string[] = "Hello World!\n";
char test_rw_string[] = "Hello World!\n";

int a = 234434;
const aa = 433443332;

void test() {
	if (a == 234434 && aa == 433443332) {
		pmb8876_serial_print("Hello World!\n");
		pmb8876_serial_print(test_const_string);
		pmb8876_serial_print(test_rw_string);
		test_rw_string[1] = 'a';
		for (int i = 0; i < 10000; ++i) {
			pmb8876_serial_print(test_rw_string);
			serve_watchdog();
		}
		pmb8876_serial_print("OK!\n");
	}
}

int main() {
	init_watchdog_noinit();
	
	test();
	
	a = 2;
	
	return 0;
}
