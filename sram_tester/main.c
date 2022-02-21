#include <pmb887x.h>
#include <printf.h>

const char *get_mod_name(uint32_t id, bool is32);
void data_abort_handler2(void);

volatile bool is_data_abort = false;
volatile bool need_ignore_data_abort = false;
volatile uint32_t data_abort_context[16];
volatile uint8_t data_abort_stack[0x4000];

static void write_addr(uint32_t addr, uint32_t v) {
	need_ignore_data_abort = true;
	REG(addr) = v;
	need_ignore_data_abort = false;
}

static uint32_t read_addr(uint32_t addr) {
	is_data_abort = false;
	need_ignore_data_abort = true;
	uint32_t v = REG(addr);
	need_ignore_data_abort = false;
	return is_data_abort ? 0xFFFFFFFF : v;
}

void data_abort_handler2(void) {
	is_data_abort = true;
	if (!need_ignore_data_abort) {
		printf("data abort!\n");
		while (true);
	}
}

int main(void) {
	wdt_init();
	
	const uint32_t magic = 0xDEAD0926;
	
	for (uint32_t i = 0; i < 0x18000; i += 4) {
		write_addr(i, i ^ magic);
		wdt_serve();
	}
	
	for (uint32_t base = 0x00000000; base < 0x08000000; base += 0x20000) {
		printf("base %08X\n", base);
		for (uint32_t i = 0; i < 0x18000; i += 4) {
			if (read_addr(base + i) != (i ^ magic)) {
				printf("%08X\n", base + i);
				break;
			}
			wdt_serve();
		}
	}
	
	printf("Done.\n");
	return 0;
}
