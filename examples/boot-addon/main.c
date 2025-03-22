#include <pmb887x.h>
#include <printf.h>
#include "md5.h"

void _start(void (*serve_watchdog)(), int (*rx_byte)(), void (*tx_byte)(int data)) {
	MD5Context ctx;
	md5Init(&ctx);
	md5Update(&ctx, (uint8_t *)0xA0000000, 1024*1024*64);
	md5Finalize(&ctx);
}
