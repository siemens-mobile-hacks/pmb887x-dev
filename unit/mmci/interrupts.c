#include <pmb887x.h>

#include "test.h"

#define MCI_WRITE_GAP_US 10
#define MCI_COMMAND_WAIT_MS 100

#ifdef PMB8876
static void vic_enable_all_lines(void) {
	for (uint32_t irq = 1; irq < VIC_IRQ_COUNT; irq++)
		VIC_CON(irq) = 1;
}

static void vic_disable_pending_lines(void) {
	for (uint32_t irq = VIC_IRQ_CON & VIC_IRQ_CON_NUM; irq != 0; irq = VIC_IRQ_CON & VIC_IRQ_CON_NUM) {
		printf("# VIC line %lu is already pending, disabled\n", irq);
		VIC_CON(irq) = 0;
	}
}

static void mci_configure(void) {
	MMCI_CLC = 1 << MOD_CLC_RMC_SHIFT;
	MCI_MASK0 = 0;
	MCI_MASK1 = 0;
	MCI_POWER = MCI_POWER_CTRL_POWER_UP;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	MCI_POWER = MCI_POWER_CTRL_POWER_ON;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	MCI_CLOCK = MCI_CLOCK_BYPASS | MCI_CLOCK_ENABLE;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
}

static bool mci_send_command_without_response(void) {
	MCI_COMMAND = 0;
	MCI_CLEAR = MCI_CLEAR_CMDSENTCLR;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	MCI_COMMAND = MCI_COMMAND_ENABLE;

	stopwatch_t start = stopwatch_get();
	while ((MCI_STATUS & MCI_STATUS_CMDSENT) == 0 && stopwatch_elapsed_ms(start) < MCI_COMMAND_WAIT_MS)
		test_watchdog_serve();

	MCI_COMMAND = 0;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	return (MCI_STATUS & MCI_STATUS_CMDSENT) != 0;
}

static void test_interrupt_output(
	const char *output,
	volatile uint32_t *mask,
	uint32_t cmdsent_mask,
	uint32_t expected_irq
) {
	test_category(output);
	bool sent = mci_send_command_without_response();
	test_check("command without response sets CMDSENT", sent);
	test_eq_u32("masked CMDSENT leaves VIC idle", 0, VIC_IRQ_CON & VIC_IRQ_CON_NUM);

	*mask = cmdsent_mask;
	uint32_t irq = VIC_IRQ_CON & VIC_IRQ_CON_NUM;
	printf("# %s: CMDSENT unmasked, VIC line %lu pending\n", output, irq);
	test_eq_u32("CMDSENT reaches the VIC line", expected_irq, irq);

	MCI_CLEAR = MCI_CLEAR_CMDSENTCLR;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	uint32_t irq_after_clear = VIC_IRQ_CON & VIC_IRQ_CON_NUM;
	printf("# %s: CMDSENT cleared, VIC line %lu pending\n", output, irq_after_clear);
	test_eq_u32("clearing CMDSENT drops the VIC request", 0, irq_after_clear);
	*mask = 0;
}
#endif

int main(void) {
	test_start("MMCI interrupt routing test");

#ifdef PMB8876
	cpu_enable_irq(false);
	mci_configure();
	vic_enable_all_lines();
	vic_disable_pending_lines();

	test_interrupt_output("MCIINTR0", &MCI_MASK0, MCI_MASK0_CMDSENT, VIC_MCI_INTR0_IRQ);
	test_interrupt_output("MCIINTR1", &MCI_MASK1, MCI_MASK1_CMDSENT, VIC_MCI_INTR1_IRQ);
#else
	test_skip("MMCI interrupts", "PMB8876 only");
#endif
	return test_finish();
}
