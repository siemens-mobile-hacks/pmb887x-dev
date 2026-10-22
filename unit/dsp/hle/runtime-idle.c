#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/runtime.h"
#include "test.h"

#define DSP_RUNTIME_IDLE_RESULT_OFFSET 0x0200
#define DSP_RUNTIME_IDLE_TIMEOUT_US 2000
#define EL71_IDLE_WAIT_US 0x259

static bool wait_core_idle(uint32_t *elapsed_us) {
	stopwatch_t start = stopwatch_get();

	while ((CGU_STAT & CGU_STAT_DSP_CLK) != 0) {
		*elapsed_us = stopwatch_elapsed_us(start);
		if (*elapsed_us > DSP_RUNTIME_IDLE_TIMEOUT_US)
			return false;
		test_watchdog_serve();
	}

	*elapsed_us = stopwatch_elapsed_us(start);
	return true;
}

static void test_runtime_idle(void) {
	static const uint16_t READ_DSP_PARAMETERS[] = {
		0xE000, DSP_RUNTIME_IDLE_RESULT_OFFSET, 1,
	};
	const struct dsp_user_rom *rom;

	if (!test_check("platform User ROM starts for IDLE tests", dsp_runtime_start(&rom)))
		return;

	uint32_t initial_idle_us;
	if (!test_check("runtime enters IDLE after startup", wait_core_idle(&initial_idle_us)))
		return;

	dsp_runtime_command_prepare(0, DSP_COMMAND_IDLE, NULL, 0);
	DSP_COM_SET = BIT(0);
	dsp_runtime_interrupt_pulse(BIT(0));
	uint32_t active_after_interrupt = CGU_STAT & CGU_STAT_DSP_CLK;
	uint32_t idle_us;
	bool idle = wait_core_idle(&idle_us);
	test_eq_u32("IDLE command interrupt activates the DSP core", CGU_STAT_DSP_CLK, active_after_interrupt);
	test_check("IDLE command returns the DSP core to idle", idle);
	if (rom->uses_stm_command_timeout)
		test_check("IDLE command completes within the EL71 firmware wait budget", idle_us <= EL71_IDLE_WAIT_US);
	test_eq_u32("IDLE command is acknowledged before the core becomes idle",
		(uint16_t) -DSP_COMMAND_IDLE, dsp_runtime_pipe_get(0)[0]);
	test_eq_u32("IDLE command leaves no pending communication flag", 0, DSP_COM_STATUS & BIT(0));
	if (!idle)
		return;

	uint32_t cgu_con2 = CGU_CON2;
	CGU_CON2 = (cgu_con2 & ~CGU_CON2_DSP_CLKSEL) | CGU_CON2_DSP_CLKSEL_DISABLE;
	dsp_runtime_command_prepare(0, DSP_COMMAND_READ_DSP, READ_DSP_PARAMETERS, ARRAY_SIZE(READ_DSP_PARAMETERS));
	uint16_t staged_command = dsp_runtime_pipe_get(0)[0];
	CGU_CON2 = cgu_con2;
	test_eq_u32("Shared RAM stages the next command while the DSP clock source is disabled",
		DSP_COMMAND_READ_DSP, staged_command);

	DSP_COM_SET = BIT(0);
	dsp_runtime_interrupt_pulse(BIT(0));
	bool command_complete = dsp_runtime_wait(BIT(0));
	test_check("command immediately following IDLE completes",
		command_complete && dsp_runtime_pipe_get(0)[0] == (uint16_t) -DSP_COMMAND_READ_DSP);
	uint32_t recovered_idle_us;
	test_check("runtime returns to IDLE after the following command", wait_core_idle(&recovered_idle_us));

	printf("# IDLE timing: startup=%lu us command=%lu us recovery=%lu us\n",
		initial_idle_us, idle_us, recovered_idle_us);
}

int main(void) {
	test_start("DSP runtime IDLE test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_runtime_idle();
	return test_finish();
}
