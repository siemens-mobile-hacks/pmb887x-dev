#include <pmb887x.h>

#include "dsp/baseband-functional-8876.inc"
#include "dsp/dsp-hw.h"
#include "test.h"

#define TPU_TIMER_RAM_BASE 512
#define TPU_DECODER_SHIFT 6
#define TPU_TIMEOUT_MS 100
#define TPU_EVENT_GROUPS BIT(0)

#define TPU_DECODER_MONON_SET 2
#define TPU_DECODER_FCON_SET 4
#define TPU_DECODER_RECEIVE_CLEAR 5
#define TPU_DECODER_RXON_SET 6
#define TPU_DECODER_RXON_CLEAR 7

#define CGU_TIMEOUT_MS 20

#define DSP_READY_MARKER 0xA55A
#define DSP_COMPLETE_MARKER 0x5AA5
#define DSP_READY_OFFSET 0x000F
#define DSP_COMMAND_OFFSET 0x0010
#define DSP_INT_POINTER_OFFSET 0x0011
#define DSP_EXPECTED_IRQS_OFFSET 0x0012
#define DSP_CONTROL_OR_OFFSET 0x0013
#define DSP_CONFIGURED_OFFSET 0x0014
#define DSP_CONTROL_AND_OFFSET 0x0016
#define DSP_BRFILTER_CTRL_OFFSET 0x0017
#define DSP_COEFFICIENT0_OFFSET 0x0018
#define DSP_FINAL_POINTER_OFFSET 0x0022
#define DSP_IRQ_POINTER_BASE 0x0140

#define BASEBAND_INTERRUPT_POINTER 959
#define BASEBAND_EXPECTED_IRQS 2

struct tpu_event {
	uint16_t tick;
	uint16_t decoder;
};

struct clock_probe {
	uint16_t l;
	uint16_t expected_pointer;
	uint16_t final_pointer;
	uint16_t falling_pointer;
	uint32_t elapsed_us;
};

struct baseband_signal {
	const char *name;
	const struct tpu_event *events;
	size_t event_count;
};

static const struct tpu_event FCON_EVENTS[] = {
	{ 100, TPU_DECODER_RXON_SET },
	{ 400, TPU_DECODER_FCON_SET },
	{ 700, TPU_DECODER_RECEIVE_CLEAR },
	{ 800, TPU_DECODER_RXON_CLEAR },
};

static const struct tpu_event MONON_EVENTS[] = {
	{ 100, TPU_DECODER_RXON_SET },
	{ 400, TPU_DECODER_MONON_SET },
	{ 700, TPU_DECODER_RECEIVE_CLEAR },
	{ 800, TPU_DECODER_RXON_CLEAR },
};

static const struct baseband_signal SIGNALS[] = {
	{ "FCON", FCON_EVENTS, ARRAY_SIZE(FCON_EVENTS) },
	{ "MONON", MONON_EVENTS, ARRAY_SIZE(MONON_EVENTS) },
};

static void tpu_configure_probe(uint16_t l, const struct baseband_signal *signal) {
	TPU_PARAM = 0;
	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = l << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
	TPU_OVERFLOW = 5999;
	TPU_OFFSET = 0;
	TPU_TGER = TPU_EVENT_GROUPS;
	TPU_EAPB = 0;
	TPU_EAPT = signal->event_count * 3;

	for (size_t i = 0; i < signal->event_count; i++) {
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3) = 0;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 1) = signal->events[i].tick;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * 3 + 2) = signal->events[i].decoder << TPU_DECODER_SHIFT;
	}
}

static bool dsp_baseband_runner_start(void) {
	if (!dsp_hw_reset())
		return false;
	DSP_COM_CLEAR = 0xFFFF;
	if (!dsp_hw_load_image(DSP_BASEBAND_FUNCTIONAL_8876, sizeof(DSP_BASEBAND_FUNCTIONAL_8876)))
		return false;
	if (!dsp_hw_branch(DSP_HW_STARTUP_ADDRESS))
		return false;
	if (!dsp_hw_wait_shared(DSP_READY_OFFSET, DSP_READY_MARKER, TPU_TIMEOUT_MS))
		return false;

	dsp_hw_shared_memory[DSP_INT_POINTER_OFFSET] = BASEBAND_INTERRUPT_POINTER;
	dsp_hw_shared_memory[DSP_EXPECTED_IRQS_OFFSET] = BASEBAND_EXPECTED_IRQS;
	dsp_hw_shared_memory[DSP_CONTROL_OR_OFFSET] = 0;
	dsp_hw_shared_memory[DSP_CONTROL_AND_OFFSET] = 0xFFFF;
	dsp_hw_shared_memory[DSP_BRFILTER_CTRL_OFFSET] = 0;
	dsp_hw_shared_memory[DSP_COEFFICIENT0_OFFSET] = 0;
	dsp_hw_shared_memory[DSP_COMMAND_OFFSET] = 1;
	return dsp_hw_wait_shared(DSP_CONFIGURED_OFFSET, DSP_READY_MARKER, TPU_TIMEOUT_MS);
}

static bool run_clock_probe(struct clock_probe *probe, const struct baseband_signal *signal) {
	if (!dsp_baseband_runner_start())
		return false;

	tpu_configure_probe(probe->l, signal);
	stopwatch_t start = stopwatch_get();
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool completed = dsp_hw_wait_shared(0, DSP_COMPLETE_MARKER, TPU_TIMEOUT_MS);
	probe->elapsed_us = stopwatch_elapsed_us(start);
	TPU_PARAM = 0;

	probe->falling_pointer = dsp_hw_shared_memory[DSP_IRQ_POINTER_BASE + 1];
	probe->final_pointer = dsp_hw_shared_memory[DSP_FINAL_POINTER_OFFSET];
	return completed;
}

static bool configure_pll_clock(void) {
	CGU_OSC = 3 << CGU_OSC_NDIV_SHIFT;
	CGU_OSC |= CGU_OSC_PLL_POWER_UP;

	stopwatch_t start = stopwatch_get();
	while ((CGU_STAT & CGU_STAT_LOCK) == 0 && stopwatch_elapsed_ms(start) < CGU_TIMEOUT_MS)
		test_watchdog_serve();
	if ((CGU_STAT & CGU_STAT_LOCK) == 0)
		return false;

	CGU_OSC |= CGU_OSC_PLL_BYPASS_N;
	USART_CLC(USART0) = 2 << MOD_CLC_RMC_SHIFT;
	CGU_CON1 = CGU_CON1_FPI1_CLKSEL_PLL_DIV_2 | CGU_CON1_FSYS_CLKSEL_PLL | CGU_CON1_AHB_CLKSEL_PLL;
	return true;
}

int main(void) {
	test_start("TPU and Baseband clock relationship");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t signal_index = 0; signal_index < ARRAY_SIZE(SIGNALS); signal_index++) {
		const struct baseband_signal *signal = &SIGNALS[signal_index];
		struct clock_probe probes[] = {
			{ .l = 4, .expected_pointer = 74 },
			{ .l = 8, .expected_pointer = 150 },
			{ .l = 16, .expected_pointer = 300 },
			{ .l = 32, .expected_pointer = 600 },
		};

		test_category(signal->name);
		for (size_t i = 0; i < ARRAY_SIZE(probes); i++) {
			char name[96];

			sprintf(name, "%s K=1 L=%u Baseband probe completes", signal->name, probes[i].l);
			if (!test_check(name, run_clock_probe(&probes[i], signal)))
				break;
			printf("# TPU_BASEBAND_CLOCK,signal=%s,K=1,L=%lu,elapsed_us=%lu,falling_pointer=%lu,final_pointer=%lu\n",
				signal->name, (uint32_t) probes[i].l, (uint32_t) probes[i].elapsed_us,
				(uint32_t) probes[i].falling_pointer, (uint32_t) probes[i].final_pointer);
			sprintf(name, "%s stores the hardware-observed number of Baseband words", signal->name);
			test_eq_u32(name, probes[i].expected_pointer, probes[i].falling_pointer);
			sprintf(name, "%s falling-edge and final write pointers match", signal->name);
			test_eq_u32(name, probes[i].falling_pointer, probes[i].final_pointer);
		}
	}

	test_category("104 MHz PLL clock source");
	if (test_check("PLL locks and leaves bypass mode", configure_pll_clock())) {
		for (size_t i = 0; i < ARRAY_SIZE(SIGNALS); i++) {
			const struct baseband_signal *signal = &SIGNALS[i];
			struct clock_probe pll_probe = { .l = 4, .expected_pointer = 38 };
			char name[96];

			sprintf(name, "%s K=1 L=4 PLL Baseband probe completes", signal->name);
			if (!test_check(name, run_clock_probe(&pll_probe, signal)))
				continue;
			printf("# TPU_BASEBAND_PLL,signal=%s,K=1,L=%lu,elapsed_us=%lu,falling_pointer=%lu,final_pointer=%lu\n",
				signal->name, (uint32_t) pll_probe.l, (uint32_t) pll_probe.elapsed_us,
				(uint32_t) pll_probe.falling_pointer, (uint32_t) pll_probe.final_pointer);
			sprintf(name, "%s PLL clock source preserves the Baseband sample rate", signal->name);
			test_eq_u32(name, pll_probe.expected_pointer, pll_probe.falling_pointer);
			sprintf(name, "%s PLL falling-edge and final write pointers match", signal->name);
			test_eq_u32(name, pll_probe.falling_pointer, pll_probe.final_pointer);
		}
	}

	TPU_PARAM = 0;
	DSP_COM_CLEAR = 0xFFFF;
	(void) dsp_hw_reset();
	return test_finish();
}
