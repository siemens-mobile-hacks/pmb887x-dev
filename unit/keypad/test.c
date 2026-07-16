#include <pmb887x.h>

#include "test.h"

#define KEYPAD_CON_FIELDS (KEYPAD_CON_NEXT_REPEAT_DELAY | KEYPAD_CON_FIRST_REPEAT_DELAY)

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;

static volatile uint32_t *const sources[] = {
	&KEYPAD_INT0_SRC,
	&KEYPAD_INT1_SRC,
	&KEYPAD_INT2_SRC,
	&KEYPAD_INT3_SRC,
};

static const uint32_t irqs[] = {
	VIC_KEYPAD_INT0_IRQ,
	VIC_KEYPAD_INT1_IRQ,
	VIC_KEYPAD_INT2_IRQ,
	VIC_KEYPAD_INT3_IRQ,
};

static bool wait_for_irq(void) {
	stopwatch_t start = stopwatch_get();

	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return irq_count != 0;
}

static void test_reset_values(void) {
	test_module_id("module ID", 0xF046C000, KEYPAD_ID);
	test_eq_u32("control contains only documented fields after boot", 0, KEYPAD_CON & ~KEYPAD_CON_FIELDS);
	for (uint32_t index = 0; index < ARRAY_SIZE(sources); index++)
		test_eq_u32("interrupt source reset value", 0, *sources[index]);
	test_eq_u32("internal status reset value", 0, KEYPAD_ISR);
	for (uint32_t index = 0; index < 3; index++)
		test_eq_u32("idle key matrix state", UINT32_MAX, KEYPAD_PORT(index));

	printf("# KEYPAD boot control value: %08X\n", (unsigned int) KEYPAD_CON);
}

static void test_register_layout(void) {
	test_eq_u32("next-repeat delay occupies bits 3:0", GENMASK(3, 0), KEYPAD_CON_NEXT_REPEAT_DELAY);
	test_eq_u32("first-repeat delay occupies bits 15:8", GENMASK(15, 8), KEYPAD_CON_FIRST_REPEAT_DELAY);
	test_eq_u32("FSM state 0 is bit 0", BIT(0), KEYPAD_ISR_FSM_STATE0);
	test_eq_u32("FSM state 1 is bit 1", BIT(1), KEYPAD_ISR_FSM_STATE1);
	test_eq_u32("internal interrupt 0 is bit 2", BIT(2), KEYPAD_ISR_INT0);
	test_eq_u32("internal interrupt 1 is bit 3", BIT(3), KEYPAD_ISR_INT1);
	test_eq_u32("internal interrupt 2 is bit 4", BIT(4), KEYPAD_ISR_INT2);
	test_eq_u32("internal interrupt 3 is bit 5", BIT(5), KEYPAD_ISR_INT3);
	test_eq_u32("key-pressed status is bit 6", BIT(6), KEYPAD_ISR_KEY_PRESSED);
	test_eq_u32("matrix state register is 32 bits wide", UINT32_MAX, KEYPAD_PORT_STATE);
}

static void test_interrupt_routing(void) {
	for (uint32_t index = 0; index < ARRAY_SIZE(sources); index++) {
		irq_count = 0;
		irq_number = 0;
		VIC_CON(irqs[index]) = 1;
		*sources[index] = MOD_SRC_CLRR | MOD_SRC_SRE;
		cpu_enable_irq(true);
		*sources[index] |= MOD_SRC_SETR;
		test_check("software request raises keypad IRQ", wait_for_irq());
		cpu_enable_irq(false);
		test_eq_u32("keypad SRC is routed to the expected VIC line", irqs[index], irq_number);
		test_eq_u32("IRQ handler clears the keypad request", 0, *sources[index] & MOD_SRC_SRR);
		VIC_CON(irqs[index]) = 0;
		*sources[index] = MOD_SRC_CLRR;
	}

	test_eq_u32("software SRC requests do not alter internal status", 0, KEYPAD_ISR);
}

int main(void) {
	test_start("KEYPAD peripheral test");

	test_category("Reset values");
	test_reset_values();
	test_category("Register layout");
	test_register_layout();
	test_category("Interrupt routing");
	test_interrupt_routing();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	irq_count++;
	for (uint32_t index = 0; index < ARRAY_SIZE(sources); index++) {
		if (irq_number == irqs[index])
			*sources[index] |= MOD_SRC_CLRR;
	}
	VIC_IRQ_ACK = 1;
}
