#include <pmb887x.h>

#include "test.h"

#define TPU_GP_COUNT 5
#define TPU_GP_DECODER_BASE 10
#define TPU_GP_DECODER_SHIFT 6
/* The first generated GP IRQ name belongs to RF SSC; GP0..4 follow it. */
#define TPU_GP_VIC_BASE VIC_TPU_INT_GP1_IRQ
#define TPU_TIMER_RAM_BASE 512
#define TPU_TIMEOUT_MS 100

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;

static void clear_gp_requests(void) {
	for (uint32_t index = 0; index < TPU_GP_COUNT; index++)
		TPU_GP_SRC(index) = MOD_SRC_CLRR;
}

static bool wait_gp_request(uint32_t index) {
	stopwatch_t start = stopwatch_get();

	while ((TPU_GP_SRC(index) & MOD_SRC_SRR) == 0 && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return (TPU_GP_SRC(index) & MOD_SRC_SRR) != 0;
}

static bool wait_for_irq(void) {
	stopwatch_t start = stopwatch_get();

	while (irq_count == 0 && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return irq_count != 0;
}

static void test_only_gp_request(uint32_t selected) {
	for (uint32_t index = 0; index < TPU_GP_COUNT; index++) {
		if (index == selected)
			continue;
		test_eq_u32("other GP request remains clear", 0, TPU_GP_SRC(index) & MOD_SRC_SRR);
	}
}

static void configure_gp_event(uint32_t index) {
	TPU_PARAM = 0;
	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = 32 << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 0;
	TPU_TGER = BIT(0);
	TPU_EAPB = 0;
	TPU_EAPT = 3;
	TPU_RAM(TPU_TIMER_RAM_BASE + 0) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + 1) = 100;
	TPU_RAM(TPU_TIMER_RAM_BASE + 2) = (TPU_GP_DECODER_BASE + index) << TPU_GP_DECODER_SHIFT;
}

static void test_software_requests(void) {
	test_category("GP service request registers");
	clear_gp_requests();
	for (uint32_t index = 0; index < TPU_GP_COUNT; index++) {
		TPU_GP_SRC(index) = MOD_SRC_SETR;
		test_check("SETR raises selected GP request", (TPU_GP_SRC(index) & MOD_SRC_SRR) != 0);
		test_only_gp_request(index);
		TPU_GP_SRC(index) = MOD_SRC_CLRR;
		test_eq_u32("CLRR clears selected GP request", 0, TPU_GP_SRC(index) & MOD_SRC_SRR);
	}
}

static void test_interrupt_routing(void) {
	test_category("GP interrupt routing");
	for (uint32_t index = 0; index < TPU_GP_COUNT; index++) {
		uint32_t expected_irq = TPU_GP_VIC_BASE + index;

		irq_count = 0;
		irq_number = 0;
		VIC_CON(expected_irq) = 1;
		TPU_GP_SRC(index) = MOD_SRC_CLRR | MOD_SRC_SRE;
		cpu_enable_irq(true);
		TPU_GP_SRC(index) |= MOD_SRC_SETR;
		test_check("GP software request raises IRQ", wait_for_irq());
		cpu_enable_irq(false);
		test_eq_u32("GP request reaches expected VIC line", expected_irq, irq_number);
		test_eq_u32("GP IRQ handler clears request", 0, TPU_GP_SRC(index) & MOD_SRC_SRR);
		VIC_CON(expected_irq) = 0;
		TPU_GP_SRC(index) = MOD_SRC_CLRR;
	}
}

static void test_decoder_requests(void) {
	test_category("GSM timer GP decoder and IRQ routing");
	for (uint32_t index = 0; index < TPU_GP_COUNT; index++) {
		uint32_t expected_irq = TPU_GP_VIC_BASE + index;

		clear_gp_requests();
		configure_gp_event(index);
		irq_count = 0;
		irq_number = 0;
		VIC_CON(expected_irq) = 1;
		TPU_GP_SRC(index) = MOD_SRC_CLRR | MOD_SRC_SRE;
		cpu_enable_irq(true);
		stopwatch_usleep_wd(100);
		test_eq_u32("enabling GP route does not raise IRQ", 0, irq_count);
		TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
		test_check("timer event raises selected GP IRQ", wait_for_irq());
		TPU_PARAM = 0;
		cpu_enable_irq(false);
		test_eq_u32("GP event reaches expected VIC line", expected_irq, irq_number);
		test_only_gp_request(index);
		test_eq_u32("GP IRQ handler clears request", 0, TPU_GP_SRC(index) & MOD_SRC_SRR);
		VIC_CON(expected_irq) = 0;
		TPU_GP_SRC(index) = MOD_SRC_CLRR;
	}
}

static void test_decoder_repeats(void) {
	test_category("GSM timer GP toggle");
	clear_gp_requests();
	configure_gp_event(0);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("first GP toggle requests service", wait_gp_request(0));
	TPU_GP_SRC(0) = MOD_SRC_CLRR;
	test_check("GP toggle repeats in the next frame", wait_gp_request(0));
	TPU_PARAM = 0;
	TPU_GP_SRC(0) = MOD_SRC_CLRR;
}

int main(void) {
	test_start("TPU GP interrupt test");
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_id("module ID", 0xF021C000, TPU_ID);
	test_module_clock("module clock is enabled", TPU_CLC);
	test_software_requests();
	test_decoder_requests();
	test_decoder_repeats();
	test_interrupt_routing();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	irq_count++;
	if (irq_number >= TPU_GP_VIC_BASE && irq_number < TPU_GP_VIC_BASE + TPU_GP_COUNT)
		TPU_GP_SRC(irq_number - TPU_GP_VIC_BASE) = MOD_SRC_CLRR;
	VIC_IRQ_ACK = 1;
}
