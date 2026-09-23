#include <pmb887x.h>

#include "test.h"

#define PLL_LOCK_TIMEOUT_MS 20
#define CGU_IRQ_TIMEOUT_MS 100

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;

static bool wait_for_pll_lock(void) {
	uint32_t start = STM_TIM0;

	while ((CGU_STAT & CGU_STAT_LOCK) == 0 && STM_TIM0 - start < (CPU_OSC_FREQ / 1000) * PLL_LOCK_TIMEOUT_MS);

	return (CGU_STAT & CGU_STAT_LOCK) != 0;
}

static bool wait_for_pll_unlock(void) {
	stopwatch_t start = stopwatch_get();

	while ((CGU_STAT & CGU_STAT_LOCK) != 0 && stopwatch_elapsed_ms(start) < PLL_LOCK_TIMEOUT_MS)
		test_watchdog_serve();

	return (CGU_STAT & CGU_STAT_LOCK) == 0;
}

static bool wait_for_irq(void) {
	stopwatch_t start = stopwatch_get();

	while (irq_count == 0 && stopwatch_elapsed_ms(start) < CGU_IRQ_TIMEOUT_MS)
		test_watchdog_serve();

	return irq_count != 0;
}

static void restore_irq(uint32_t src, uint32_t vic_con, bool irq_was_disabled) {
	VIC_CON(VIC_CGU_IRQ) = vic_con;
	CGU_SRC = MOD_SRC_CLRR | (src & (MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE));
	if ((src & MOD_SRC_SRR) != 0)
		CGU_SRC |= MOD_SRC_SETR;
	cpu_enable_irq(!irq_was_disabled);
}

static void test_pll_lock(void) {
	uint32_t initial_osc = CGU_OSC;
	uint32_t ebu_source = CGU_CON2 & CGU_CON2_EBU_CLKSEL;
	bool safe_clock_tree =
		(CGU_CON1 & CGU_CON1_AHB_CLKSEL) == CGU_CON1_AHB_CLKSEL_BYPASS &&
		(CGU_CON1 & CGU_CON1_FSYS_CLKSEL) == CGU_CON1_FSYS_CLKSEL_BYPASS &&
		(ebu_source == CGU_CON2_EBU_CLKSEL_OSC || ebu_source == CGU_CON2_EBU_CLKSEL_AHB);
	if (!safe_clock_tree) {
		test_skip("power down clears PLL lock", "boot clock tree depends on the PLL");
		test_skip("power up reacquires PLL lock", "boot clock tree depends on the PLL");
		test_skip("PLL lock raises CGU IRQ", "boot clock tree depends on the PLL");
		return;
	}

	uint32_t initial_src = CGU_SRC;
	uint32_t initial_vic_con = VIC_CON(VIC_CGU_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	irq_count = 0;
	irq_number = 0;
	CGU_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_CGU_IRQ) = 1;
	CGU_OSC = initial_osc & ~(CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N);
	bool unlocked = wait_for_pll_unlock();

	CGU_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	irq_count = 0;
	irq_number = 0;
	cpu_enable_irq(true);
	CGU_OSC = (initial_osc | CGU_OSC_PLL_POWER_UP) & ~CGU_OSC_PLL_BYPASS_N;
	bool locked = wait_for_pll_lock();
	bool lock_irq = wait_for_irq();
	cpu_enable_irq(false);

	CGU_OSC = initial_osc;
	bool restored = wait_for_pll_lock();
	restore_irq(initial_src, initial_vic_con, irq_was_disabled);

	test_check("power down clears PLL lock", unlocked);
	test_check("power up reacquires PLL lock", locked);
	test_check("PLL lock raises CGU IRQ", lock_irq && irq_number == VIC_CGU_IRQ);
	test_check("PLL configuration is restored", restored && CGU_OSC == initial_osc);
}

static void test_irq_routing(void) {
	uint32_t initial_src = CGU_SRC;
	uint32_t initial_vic_con = VIC_CON(VIC_CGU_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	irq_count = 0;
	irq_number = 0;
	CGU_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_CGU_IRQ) = 1;
	cpu_enable_irq(true);
	CGU_SRC |= MOD_SRC_SETR;
	bool raised = wait_for_irq();
	cpu_enable_irq(false);
	bool cleared = (CGU_SRC & MOD_SRC_SRR) == 0;
	restore_irq(initial_src, initial_vic_con, irq_was_disabled);

	test_check("software request reaches CGU IRQ", raised && irq_number == VIC_CGU_IRQ);
	test_check("IRQ handler clears the request", cleared);
}

int main(void) {
	test_start("CGU PLL and IRQ test");
	test_check("PLL is powered after boot", (CGU_OSC & CGU_OSC_PLL_POWER_UP) != 0);
	test_check("PLL is locked after boot", (CGU_STAT & CGU_STAT_LOCK) != 0);
	test_pll_lock();
	test_irq_routing();
	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	irq_count++;
	CGU_SRC |= MOD_SRC_CLRR;
	VIC_IRQ_ACK = 1;
}
