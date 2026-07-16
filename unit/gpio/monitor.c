#include <pmb887x.h>

#include "test.h"

#define MONITOR_GPIO_BLOCK 0xB
#define MONITOR_GPIO22_SIGNAL 0x16

static volatile uint32_t *const monitor_control_registers[] = {
	&GPIO_MON_CR1,
	&GPIO_MON_CR2,
	&GPIO_MON_CR3,
	&GPIO_MON_CR4,
};

static void test_reset_values(void) {
	GPIO_CLC = MOD_CLC_DISR;
	test_eq_u32("clock control disabled value", MOD_CLC_DISR | MOD_CLC_DISS, GPIO_CLC);
	GPIO_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("module clock is enabled", GPIO_CLC);
	test_module_id("module ID", 0xF023C000, GPIO_ID);
}

static void test_register_layout(void) {
	test_eq_u32("signal selector occupies bits 7:0", GENMASK(7, 0), GPIO_MON_CR1_SIGNAL_SELECT);
	test_eq_u32("block selector occupies bits 11:8", GENMASK(11, 8), GPIO_MON_CR1_BLOCK_SELECT);
	test_eq_u32("pull selection occupies bits 14:13", GENMASK(14, 13), GPIO_MON_CR1_PDPU);
	test_eq_u32("output disable is bit 15", BIT(15), GPIO_MON_CR1_ENAQ);
}

static void test_monitor_control_registers(void) {
	uint32_t selection =
		GPIO_MON_CR1_ENAQ_TRISTATE |
		GPIO_MON_CR1_PDPU_PULLDOWN |
		(MONITOR_GPIO_BLOCK << GPIO_MON_CR1_BLOCK_SELECT_SHIFT) |
		MONITOR_GPIO22_SIGNAL;

	for (uint32_t monitor = 0; monitor < ARRAY_SIZE(monitor_control_registers); monitor++) {
		volatile uint32_t *control = monitor_control_registers[monitor];
		uint32_t initial = *control;
		char retain_name[] = "MON_CR0 retains the GPIO22 selection";
		char restore_name[] = "MON_CR0 is restored";
		retain_name[6] = (char) ('1' + monitor);
		restore_name[6] = (char) ('1' + monitor);

		printf("# GPIO MON_CR%u initial=%08X\n", (unsigned int) monitor + 1, (unsigned int) initial);
		*control = selection;
		test_eq_u32(retain_name, selection, *control);
		*control = initial;
		test_eq_u32(restore_name, initial, *control);
	}
}

int main(void) {
	test_start("GPIO monitor test");

	test_category("Reset values");
	test_reset_values();
	test_category("Register layout");
	test_register_layout();
	test_category("Monitor control registers");
	test_monitor_control_registers();

	return test_finish();
}
