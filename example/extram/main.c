#include <pmb887x.h>

const char test_const_string[] = "Hello World!\n";
char test_rw_string[] = "Hello World!\n";

int a = 234434;
const aa = 433443332;

void test() {
	if (a == 234434 && aa == 433443332) {
		usart_print("Hello World!\n");
		usart_print(test_const_string);
		usart_print(test_rw_string);
		test_rw_string[1] = 'a';
		for (int i = 0; i < 10000; ++i) {
			usart_print(test_rw_string);
			serve_watchdog();
		}
		usart_print("OK!\n");
	}
}

int main() {
	init_watchdog_noinit();
	
	test();
	
	a = 2;
	
	return 0;
}
