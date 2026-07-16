#include <pmb887x.h>

#include "test.h"

#ifdef PMB8875
#define GPIO_PIN_COUNT 77
#else
#define GPIO_PIN_COUNT 114
#endif

#define PROBE_PIN 22

static void test_register_layout(void) {
	test_eq_u32("input selector occupies bits 2:0", GENMASK(2, 0), GPIO_IS);
	test_eq_u32("output selector occupies bits 6:4", GENMASK(6, 4), GPIO_OS);
	test_eq_u32("GPIO selection is bit 8", BIT(8), GPIO_PS);
	test_eq_u32("GPIO data is bit 9", BIT(9), GPIO_DATA);
	test_eq_u32("GPIO direction is bit 10", BIT(10), GPIO_DIR);
	test_eq_u32("open-drain selection is bit 12", BIT(12), GPIO_PPEN);
	test_eq_u32("pull selection occupies bits 14:13", GENMASK(14, 13), GPIO_PDPU);
	test_eq_u32("output disable is bit 15", BIT(15), GPIO_ENAQ);
}

static void test_reset_values(void) {
	GPIO_CLC = MOD_CLC_DISR;
	test_eq_u32("clock control disabled value", MOD_CLC_DISR | MOD_CLC_DISS, GPIO_CLC);
	GPIO_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("module clock is enabled", GPIO_CLC);
	test_module_id("module ID", 0xF023C000, GPIO_ID);
}

static void test_pad_register_width(void) {
	bool registers_are_16_bit = true;
	for (uint32_t pin = 0; pin < GPIO_PIN_COUNT; pin++) {
		uint32_t value = GPIO_PIN(pin);
		if ((value & GENMASK(31, 16)) != 0) {
			registers_are_16_bit = false;
		}
	}
	printf("# GPIO: pins=%u PIN22=%08X\n", GPIO_PIN_COUNT, (unsigned int) GPIO_PIN(PROBE_PIN));
	test_check("all pad-control registers are 16 bits wide", registers_are_16_bit);
}

static void test_unused_input_pad(void) {
	uint32_t initial = GPIO_PIN(PROBE_PIN);

	if (initial != 0xC100) {
		test_skip("GPIO22 pull resistor behavior", "GPIO22 is active on this board");
		return;
	}
	test_eq_u32("GPIO22 starts as a tristated GPIO input with pulldown", 0xC100, initial);

	GPIO_PIN(PROBE_PIN) = GPIO_ENAQ_TRISTATE | GPIO_PS_MANUAL | GPIO_PDPU_PULLUP;
	test_spin(1000);
	test_check("GPIO22 internal pullup drives an otherwise unused input high", (GPIO_PIN(PROBE_PIN) & GPIO_DATA) != 0);

	GPIO_PIN(PROBE_PIN) = GPIO_ENAQ_TRISTATE | GPIO_PS_MANUAL | GPIO_PDPU_PULLDOWN;
	test_spin(1000);
	test_eq_u32("GPIO22 internal pulldown drives the input low", 0, GPIO_PIN(PROBE_PIN) & GPIO_DATA);

	GPIO_PIN(PROBE_PIN) = initial;
	test_eq_u32("GPIO22 configuration is restored", initial, GPIO_PIN(PROBE_PIN));
}

int main(void) {
	test_start("GPIO peripheral test");

	test_category("Reset values");
	test_reset_values();
	test_category("Register layout");
	test_register_layout();
	test_category("Pad register width");
	test_pad_register_width();
	test_category("Unused input pad");
	test_unused_input_pad();

	return test_finish();
}
