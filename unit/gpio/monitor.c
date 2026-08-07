#include <pmb887x.h>

#include "test.h"

#define SCU_RST_REQ_PROBE 0
#define SCU_RST_REQ_DANGEROUS_BITS SCU_RST_REQ_USART0
#define CGU_ACTIVITY_SAMPLES 40000
#define CGU_PHASE_PROBE_CONFIG 0x22
#define CGU_PROBE_TIMEOUT_MS 300000
#define CGU_ACCESS_BENCHMARK_ITERATIONS 32768
#define CGU_KERNEL_MEASURE_STM_TICKS 260000
#define CGU_SIGNAL_CLK_48M_O 0x080D
#define CGU_SIGNAL_CLK_CLKOUT2_O 0x081D
#define CGU_SIGNAL_CLK_DSP_O 0x0808
#define CGU_SIGNAL_CLK_MMCI_O 0x0819
#define CGU_SIGNAL_CLK_MS_O 0x0814
#define CGU_SIGNAL_CLK_6M5_TRIG_O 0x081A
#define CGU_SIGNAL_CLK_AFC_O 0x0813
#define CGU_SIGNAL_CLK_CON_O 0x0809
#define CGU_SIGNAL_EN_CERB_O 0x081E

struct monitor_signal {
	const char *name;
	uint32_t selection;
};

struct cgu_value_probe {
	const char *name;
	volatile uint32_t *reg;
	uint32_t mask;
	uint32_t value;
	uint32_t required_osc;
	const char *transition_name;
	uint32_t transition_selection;
};

struct cgu_phase_probe {
	const char *name;
	uint32_t config_mask;
	uint32_t config_shift;
	uint32_t power;
	uint32_t selection;
};

#define CGU_VALUE_PROBE(probe_name, probe_reg, probe_mask, probe_value) \
	{ .name = probe_name, .reg = probe_reg, .mask = probe_mask, .value = probe_value }
#define CGU_CLOCK_PROBE(probe_name, probe_reg, probe_mask, probe_value, probe_osc, signal_name, signal_selection) \
	{ .name = probe_name, .reg = probe_reg, .mask = probe_mask, .value = probe_value, .required_osc = probe_osc, \
		.transition_name = signal_name, .transition_selection = signal_selection }

#if SCU_RST_REQ_PROBE
static const struct monitor_signal RESET_SIGNALS[] = {
	{ "RST_AHB_PER_N", 0x0968 },
	{ "RST_ANA_N", 0x092F },
	{ "RST_BOOT_N", 0x092C },
	{ "RST_CGU_N", 0x092E },
	{ "RST_CON_N", 0x0923 },
	{ "RST_DBG_N", 0x0925 },
	{ "RST_DISP_N", 0x0937 },
	{ "RST_DMA1_N", 0x0938 },
	{ "RST_DMA2_N", 0x0939 },
	{ "RST_DMA3_N", 0x093A },
	{ "RST_DSP_N", 0x0926 },
	{ "RST_FIRDA_N", 0x0966 },
	{ "RST_FPI_N", 0x0928 },
	{ "RST_I2C_N", 0x0967 },
	{ "RST_MMCI_N", 0x0936 },
	{ "RST_PADCTL_N", 0x092A },
	{ "RST_PADCTL_OPT_N", 0x092B },
	{ "RST_PLL_N", 0x0929 },
	{ "RST_RTC_N", 0x092D },
	{ "RST_SIM_N", 0x0927 },
	{ "RST_SSC0_N", 0x0932 },
	{ "RST_SSC1_N", 0x0933 },
	{ "RST_STM_N", 0x0924 },
	{ "RST_USART0_N", 0x0934 },
	{ "RST_USART1_N", 0x0935 },
	{ "RST_USB_N", 0x0930 },
};
#endif

static const struct monitor_signal CGU_CLOCK_SIGNALS[] = {
	{ "CLK32KOUT_O", 0x0817 },
	{ "CLK_104M_O", 0x081C },
	{ "CLK_32K_I", 0x0802 },
	{ "CLK_48M_O", CGU_SIGNAL_CLK_48M_O },
	{ "CLK_6M5_TRIG_O", 0x081A },
	{ "CLK_AFC_O", 0x0813 },
	{ "CLK_AHB_PER_O", 0x081B },
	{ "CLK_CLKOUT0_O", 0x0815 },
	{ "CLK_CLKOUT1_O", 0x0816 },
	{ "CLK_CLKOUT2_O", CGU_SIGNAL_CLK_CLKOUT2_O },
	{ "CLK_CON_O", 0x0809 },
	{ "CLK_DSP_O", CGU_SIGNAL_CLK_DSP_O },
	{ "CLK_EBU_O", 0x080C },
	{ "CLK_FPI1_O", 0x080E },
	{ "CLK_FPI2_O", 0x0812 },
	{ "CLK_FPI3_O", 0x0810 },
	{ "CLK_IN_1", 0x0801 },
	{ "CLK_MMCI_O", CGU_SIGNAL_CLK_MMCI_O },
	{ "CLK_MS_O", CGU_SIGNAL_CLK_MS_O },
	{ "CLK_PHS1_I", 0x0804 },
	{ "CLK_PHS2_I", 0x0805 },
	{ "CLK_PHS3_I", 0x0806 },
	{ "CLK_PHS4_I", 0x0807 },
	{ "CLK_PLL_I", 0x0803 },
};

static const struct monitor_signal CGU_LEVEL_SIGNALS[] = {
	{ "EN_AHB_O", 0x080B },
	{ "EN_ARM_O", 0x080A },
	{ "EN_CERB_O", 0x081E },
	{ "EN_FPI3_O", 0x0811 },
	{ "F32KON_O", 0x0818 },
};

struct cgu_state {
	uint32_t transitions[ARRAY_SIZE(CGU_CLOCK_SIGNALS)];
	bool levels[ARRAY_SIZE(CGU_LEVEL_SIGNALS)];
};

static const struct cgu_value_probe CGU_PHASE_PROBES[] = {
	CGU_VALUE_PROBE("PHASE1 + POWER_UP + BYPASS_N", &CGU_OSC,
		CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE1_BYPASS_N,
		CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE1_BYPASS_N),
	CGU_VALUE_PROBE("PHASE2 + POWER_UP + BYPASS_N", &CGU_OSC,
		CGU_OSC_PHASE2_POWER_UP | CGU_OSC_PHASE2_BYPASS_N,
		CGU_OSC_PHASE2_POWER_UP | CGU_OSC_PHASE2_BYPASS_N),
	CGU_VALUE_PROBE("PHASE3 + POWER_UP + BYPASS_N", &CGU_OSC,
		CGU_OSC_PHASE3_POWER_UP | CGU_OSC_PHASE3_BYPASS_N,
		CGU_OSC_PHASE3_POWER_UP | CGU_OSC_PHASE3_BYPASS_N),
	CGU_VALUE_PROBE("PHASE4 + POWER_UP + BYPASS_N", &CGU_OSC,
		CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N,
		CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N),
};

static const struct cgu_phase_probe CGU_PHASE_CONFIG_PROBES[] = {
	{ "PHASE1", CGU_CON0_PHASE1_CONFIG, CGU_CON0_PHASE1_CONFIG_SHIFT,
		CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE1_BYPASS_N, 0x0804 },
	{ "PHASE2", CGU_CON0_PHASE2_CONFIG, CGU_CON0_PHASE2_CONFIG_SHIFT,
		CGU_OSC_PHASE2_POWER_UP | CGU_OSC_PHASE2_BYPASS_N, 0x0805 },
	{ "PHASE3", CGU_CON0_PHASE3_CONFIG, CGU_CON0_PHASE3_CONFIG_SHIFT,
		CGU_OSC_PHASE3_POWER_UP | CGU_OSC_PHASE3_BYPASS_N, 0x0806 },
	{ "PHASE4", CGU_CON0_PHASE4_CONFIG, CGU_CON0_PHASE4_CONFIG_SHIFT,
		CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N, 0x0807 },
};

static const struct cgu_value_probe CGU_VALUE_PROBES[] = {
	CGU_VALUE_PROBE("PLL + POWER_UP", &CGU_OSC, CGU_OSC_PLL_POWER_UP, CGU_OSC_PLL_POWER_UP),
	CGU_VALUE_PROBE("FSYS + CLKSEL_PLL", &CGU_CON1, CGU_CON1_FSYS_CLKSEL, CGU_CON1_FSYS_CLKSEL_PLL),
	CGU_CLOCK_PROBE("DSP + CLKSEL_PHASE1", &CGU_CON2, CGU_CON2_DSP_CLKSEL, CGU_CON2_DSP_CLKSEL_PHASE1,
		CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE1_BYPASS_N, "CLK_DSP_O", CGU_SIGNAL_CLK_DSP_O),
	CGU_VALUE_PROBE("DSP + CLKSEL_DISABLE", &CGU_CON2, CGU_CON2_DSP_CLKSEL, CGU_CON2_DSP_CLKSEL_DISABLE),
	CGU_VALUE_PROBE("AFC32K + EN", &CGU_CON2, CGU_CON2_AFC32K_EN, CGU_CON2_AFC32K_EN),
	CGU_CLOCK_PROBE("CLK48M + CLKSEL_PHASE4", &CGU_CON2, CGU_CON2_CLK48M_CLKSEL,
		CGU_CON2_CLK48M_CLKSEL_PHASE4, CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N,
		"CLK_48M_O", CGU_SIGNAL_CLK_48M_O),
	CGU_VALUE_PROBE("CLK48M + CLKSEL_DISABLE", &CGU_CON2, CGU_CON2_CLK48M_CLKSEL,
		CGU_CON2_CLK48M_CLKSEL_DISABLE),
	CGU_VALUE_PROBE("CLKOUT0 + EN + CLKDIV_DIV1", &CGU_CON2,
		CGU_CON2_CLKOUT0_EN | CGU_CON2_CLKOUT0_CLKDIV, CGU_CON2_CLKOUT0_EN | CGU_CON2_CLKOUT0_CLKDIV_DIV1),
	CGU_VALUE_PROBE("CLKOUT0 + EN + CLKDIV_DIV2", &CGU_CON2,
		CGU_CON2_CLKOUT0_EN | CGU_CON2_CLKOUT0_CLKDIV, CGU_CON2_CLKOUT0_EN | CGU_CON2_CLKOUT0_CLKDIV_DIV2),
	CGU_VALUE_PROBE("CLKOUT0 + EN + CLKDIV_DIV4", &CGU_CON2,
		CGU_CON2_CLKOUT0_EN | CGU_CON2_CLKOUT0_CLKDIV, CGU_CON2_CLKOUT0_EN | CGU_CON2_CLKOUT0_CLKDIV_DIV4),
	CGU_VALUE_PROBE("CLKOUT0 + EN + CLKDIV_DIV8", &CGU_CON2,
		CGU_CON2_CLKOUT0_EN | CGU_CON2_CLKOUT0_CLKDIV, CGU_CON2_CLKOUT0_EN | CGU_CON2_CLKOUT0_CLKDIV_DIV8),
	CGU_VALUE_PROBE("CLKOUT1 + EN + CLKDIV_DIV1", &CGU_CON2,
		CGU_CON2_CLKOUT1_EN | CGU_CON2_CLKOUT1_CLKDIV, CGU_CON2_CLKOUT1_EN | CGU_CON2_CLKOUT1_CLKDIV_DIV1),
	CGU_VALUE_PROBE("CLKOUT1 + EN + CLKDIV_DIV2", &CGU_CON2,
		CGU_CON2_CLKOUT1_EN | CGU_CON2_CLKOUT1_CLKDIV, CGU_CON2_CLKOUT1_EN | CGU_CON2_CLKOUT1_CLKDIV_DIV2),
	CGU_VALUE_PROBE("CLKOUT1 + EN + CLKDIV_DIV4", &CGU_CON2,
		CGU_CON2_CLKOUT1_EN | CGU_CON2_CLKOUT1_CLKDIV, CGU_CON2_CLKOUT1_EN | CGU_CON2_CLKOUT1_CLKDIV_DIV4),
	CGU_VALUE_PROBE("CLKOUT1 + EN + CLKDIV_DIV8", &CGU_CON2,
		CGU_CON2_CLKOUT1_EN | CGU_CON2_CLKOUT1_CLKDIV, CGU_CON2_CLKOUT1_EN | CGU_CON2_CLKOUT1_CLKDIV_DIV8),
	CGU_VALUE_PROBE("CLK32K + EN", &CGU_CON2, CGU_CON2_CLK32K_EN, CGU_CON2_CLK32K_EN),
	CGU_CLOCK_PROBE("MS + CLKSEL_OSC", &CGU_CON2, CGU_CON2_MS_CLKSEL, CGU_CON2_MS_CLKSEL_OSC, 0,
		"CLK_MS_O", CGU_SIGNAL_CLK_MS_O),
	CGU_CLOCK_PROBE("MS + CLKSEL_CLK32K", &CGU_CON2, CGU_CON2_MS_CLKSEL, CGU_CON2_MS_CLKSEL_CLK32K, 0,
		"CLK_MS_O", CGU_SIGNAL_CLK_MS_O),
	CGU_CLOCK_PROBE("MS + CLKSEL_OSC_DIV_64", &CGU_CON2, CGU_CON2_MS_CLKSEL,
		CGU_CON2_MS_CLKSEL_OSC_DIV_64, 0, "CLK_MS_O", CGU_SIGNAL_CLK_MS_O),
	CGU_VALUE_PROBE("MS + CLKSEL_DISABLE", &CGU_CON2, CGU_CON2_MS_CLKSEL, CGU_CON2_MS_CLKSEL_DISABLE),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_CLK32K + CLKDIV_DIV1", &CGU_CON3,
		CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		CGU_CON3_AHB_PER_CLKSEL_CLK32K | CGU_CON3_AHB_PER_CLKDIV_DIV1),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_CLK32K + CLKDIV_DIV2", &CGU_CON3,
		CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		CGU_CON3_AHB_PER_CLKSEL_CLK32K | CGU_CON3_AHB_PER_CLKDIV_DIV2),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_CLK32K + CLKDIV_DIV4", &CGU_CON3,
		CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		CGU_CON3_AHB_PER_CLKSEL_CLK32K | CGU_CON3_AHB_PER_CLKDIV_DIV4),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_CLK32K + CLKDIV_DIV8", &CGU_CON3,
		CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		CGU_CON3_AHB_PER_CLKSEL_CLK32K | CGU_CON3_AHB_PER_CLKDIV_DIV8),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_PLL_DIV_2 + CLKDIV_DIV1", &CGU_CON3,
		CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2 | CGU_CON3_AHB_PER_CLKDIV_DIV1),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_PLL_DIV_2 + CLKDIV_DIV2", &CGU_CON3,
		CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2 | CGU_CON3_AHB_PER_CLKDIV_DIV2),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_PLL_DIV_2 + CLKDIV_DIV4", &CGU_CON3,
		CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2 | CGU_CON3_AHB_PER_CLKDIV_DIV4),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_PLL_DIV_2 + CLKDIV_DIV8", &CGU_CON3,
		CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
		CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2 | CGU_CON3_AHB_PER_CLKDIV_DIV8),
	CGU_VALUE_PROBE("AHB_PER + CLKSEL_DISABLE", &CGU_CON3, CGU_CON3_AHB_PER_CLKSEL,
		CGU_CON3_AHB_PER_CLKSEL_DISABLE),
	CGU_VALUE_PROBE("MMCI + CLKSEL_CLK32K + CLKDIV_DIV1", &CGU_CON3,
		CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV,
		CGU_CON3_MMCI_CLKSEL_CLK32K | CGU_CON3_MMCI_CLKDIV_DIV1),
	CGU_VALUE_PROBE("MMCI + CLKSEL_CLK32K + CLKDIV_DIV2", &CGU_CON3,
		CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV,
		CGU_CON3_MMCI_CLKSEL_CLK32K | CGU_CON3_MMCI_CLKDIV_DIV2),
	CGU_VALUE_PROBE("MMCI + CLKSEL_CLK32K + CLKDIV_DIV4", &CGU_CON3,
		CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV,
		CGU_CON3_MMCI_CLKSEL_CLK32K | CGU_CON3_MMCI_CLKDIV_DIV4),
	CGU_VALUE_PROBE("MMCI + CLKSEL_CLK32K + CLKDIV_DIV8", &CGU_CON3,
		CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV,
		CGU_CON3_MMCI_CLKSEL_CLK32K | CGU_CON3_MMCI_CLKDIV_DIV8),
	CGU_CLOCK_PROBE("MMCI + CLKSEL_PHASE4", &CGU_CON3, CGU_CON3_MMCI_CLKSEL, CGU_CON3_MMCI_CLKSEL_PHASE4,
		CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N, "CLK_MMCI_O", CGU_SIGNAL_CLK_MMCI_O),
	CGU_VALUE_PROBE("MMCI + CLKSEL_DISABLE", &CGU_CON3, CGU_CON3_MMCI_CLKSEL, CGU_CON3_MMCI_CLKSEL_DISABLE),
	CGU_CLOCK_PROBE("CLKOUT2 + EN + CLKDIV_DIV1", &CGU_CON3, CGU_CON3_CLKOUT2_EN | CGU_CON3_CLKOUT2_CLKDIV,
		CGU_CON3_CLKOUT2_EN | CGU_CON3_CLKOUT2_CLKDIV_DIV1,
		CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N, "CLK_CLKOUT2_O", CGU_SIGNAL_CLK_CLKOUT2_O),
	CGU_CLOCK_PROBE("CLKOUT2 + EN + CLKDIV_DIV2", &CGU_CON3, CGU_CON3_CLKOUT2_EN | CGU_CON3_CLKOUT2_CLKDIV,
		CGU_CON3_CLKOUT2_EN | CGU_CON3_CLKOUT2_CLKDIV_DIV2,
		CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N, "CLK_CLKOUT2_O", CGU_SIGNAL_CLK_CLKOUT2_O),
	CGU_CLOCK_PROBE("CLKOUT2 + EN + CLKDIV_DIV4", &CGU_CON3, CGU_CON3_CLKOUT2_EN | CGU_CON3_CLKOUT2_CLKDIV,
		CGU_CON3_CLKOUT2_EN | CGU_CON3_CLKOUT2_CLKDIV_DIV4,
		CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N, "CLK_CLKOUT2_O", CGU_SIGNAL_CLK_CLKOUT2_O),
	CGU_CLOCK_PROBE("CLKOUT2 + EN + CLKDIV_DIV8", &CGU_CON3, CGU_CON3_CLKOUT2_EN | CGU_CON3_CLKOUT2_CLKDIV,
		CGU_CON3_CLKOUT2_EN | CGU_CON3_CLKOUT2_CLKDIV_DIV8,
		CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N, "CLK_CLKOUT2_O", CGU_SIGNAL_CLK_CLKOUT2_O),
	CGU_VALUE_PROBE("DMA + CLK_DISABLE", &CGU_CON3, CGU_CON3_DMA_CLK_DISABLE, CGU_CON3_DMA_CLK_DISABLE),
};

static const struct cgu_value_probe CGU_UNSAFE_PROBES[] = {
	CGU_VALUE_PROBE("FPI1 + CLKSEL_CLK32K + CLKDIV_DIV1", &CGU_CON1,
		CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
		CGU_CON1_FPI1_CLKSEL_CLK32K | CGU_CON1_FPI1_CLKDIV_DIV1),
	CGU_VALUE_PROBE("FPI1 + CLKSEL_CLK32K + CLKDIV_DIV2", &CGU_CON1,
		CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
		CGU_CON1_FPI1_CLKSEL_CLK32K | CGU_CON1_FPI1_CLKDIV_DIV2),
	CGU_VALUE_PROBE("FPI1 + CLKSEL_CLK32K + CLKDIV_DIV4", &CGU_CON1,
		CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
		CGU_CON1_FPI1_CLKSEL_CLK32K | CGU_CON1_FPI1_CLKDIV_DIV4),
	CGU_VALUE_PROBE("FPI1 + CLKSEL_CLK32K + CLKDIV_DIV8", &CGU_CON1,
		CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
		CGU_CON1_FPI1_CLKSEL_CLK32K | CGU_CON1_FPI1_CLKDIV_DIV8),
	CGU_VALUE_PROBE("FPI1 + CLKSEL_PLL_DIV_2", &CGU_CON1, CGU_CON1_FPI1_CLKSEL,
		CGU_CON1_FPI1_CLKSEL_PLL_DIV_2),
	CGU_VALUE_PROBE("FPI1 + CLKSEL_DISABLE", &CGU_CON1, CGU_CON1_FPI1_CLKSEL, CGU_CON1_FPI1_CLKSEL_DISABLE),
};

#if SCU_RST_REQ_PROBE
static void print_asserted_reset_signals(uint32_t request) {
	for (uint32_t index = 0; index < ARRAY_SIZE(RESET_SIGNALS); index++) {
		const struct monitor_signal *signal = &RESET_SIGNALS[index];

		GPIO_MON_CR4 = signal->selection;
		test_spin(1000);
		SCU_RST_REQ |= request;
		uint32_t data = GPIO_PIN(GPIO_KP_OUT0) & GPIO_DATA;
		SCU_RST_REQ &= ~request;
		if (data == 0)
			printf("# LOW %s selector=%04X\n", signal->name, (unsigned int) signal->selection);
		test_watchdog_serve();
	}
}

static void probe_scu_reset_request_bits(void) {
	for (uint32_t bit = 0; bit < 32; bit++) {
		uint32_t request = BIT(bit);

		if ((request & SCU_RST_REQ_DANGEROUS_BITS) != 0) {
			printf("# SKIP SCU_RST_REQ bit=%u mask=%08X\n", (unsigned int) bit, (unsigned int) request);
			continue;
		}

		printf("# PROBE SCU_RST_REQ bit=%u mask=%08X\n", (unsigned int) bit, (unsigned int) request);
		print_asserted_reset_signals(request);
		test_watchdog_serve();
	}
}
#endif

static uint32_t count_monitor_transitions(uint32_t selection) {
	volatile uint32_t *pin = &GPIO_PIN(GPIO_KP_OUT0);

	GPIO_MON_CR4 = selection;
	test_spin(1000);
	uint32_t previous = *pin & GPIO_DATA;
	uint32_t transitions = 0;

	for (uint32_t sample = 0; sample < CGU_ACTIVITY_SAMPLES; sample += 4) {
		uint32_t current = *pin & GPIO_DATA;

		transitions += current != previous;
		previous = current;
		current = *pin & GPIO_DATA;
		transitions += current != previous;
		previous = current;
		current = *pin & GPIO_DATA;
		transitions += current != previous;
		previous = current;
		current = *pin & GPIO_DATA;
		transitions += current != previous;
		previous = current;
	}

	return transitions;
}

static bool read_monitor_level(uint32_t selection) {
	GPIO_MON_CR4 = selection;
	test_spin(1000);
	return (GPIO_PIN(GPIO_KP_OUT0) & GPIO_DATA) != 0;
}

static void capture_cgu_state(struct cgu_state *state) {
	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_CLOCK_SIGNALS); index++) {
		const struct monitor_signal *signal = &CGU_CLOCK_SIGNALS[index];
		uint32_t transitions = count_monitor_transitions(signal->selection);

		state->transitions[index] = transitions;
		test_watchdog_serve();
	}

	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_LEVEL_SIGNALS); index++) {
		state->levels[index] = read_monitor_level(CGU_LEVEL_SIGNALS[index].selection);
		test_watchdog_serve();
	}
}

static void print_cgu_levels(void) {
	printf("# CGU LEVELS");
	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_LEVEL_SIGNALS); index++)
		printf(" %s=%u", CGU_LEVEL_SIGNALS[index].name,
			read_monitor_level(CGU_LEVEL_SIGNALS[index].selection) ? 1U : 0U);
	printf("\n");
}

static bool wait_for_cgu_lock(void) {
	stopwatch_t start = stopwatch_get();

	while ((CGU_STAT & CGU_STAT_LOCK) == 0 && stopwatch_elapsed_ms(start) < 20)
		test_watchdog_serve();

	return (CGU_STAT & CGU_STAT_LOCK) != 0;
}

static bool apply_cgu_osc(uint32_t value) {
	uint32_t powered_down = value & ~(CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N);

	CGU_OSC = powered_down;
	if ((value & CGU_OSC_PLL_POWER_UP) == 0)
		return true;

	uint32_t locking = value | CGU_OSC_PLL_POWER_UP;
	locking &= ~CGU_OSC_PLL_BYPASS_N;
	CGU_OSC = locking;
	if (!wait_for_cgu_lock())
		return false;

	CGU_OSC = value;
	return true;
}

static bool apply_cgu_probe_value(const struct cgu_value_probe *probe, uint32_t value) {
	if (probe->reg == &CGU_OSC)
		return apply_cgu_osc(value);

	*probe->reg = value;
	return true;
}

static void print_cgu_diff(const struct cgu_state *before, const struct cgu_state *after) {
	bool changed = false;

	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_CLOCK_SIGNALS); index++) {
		uint32_t before_transitions = before->transitions[index];
		uint32_t after_transitions = after->transitions[index];
		bool before_active = before_transitions != 0;
		bool after_active = after_transitions != 0;

		if (before_active != after_active) {
			printf("# DIFF %c%s transitions=%u->%u\n", after_active ? '+' : '-', CGU_CLOCK_SIGNALS[index].name,
				(unsigned int) before_transitions, (unsigned int) after_transitions);
			changed = true;
			continue;
		}

		uint32_t smaller = MIN(before_transitions, after_transitions);
		uint32_t larger = MAX(before_transitions, after_transitions);
		if (smaller == 0 || larger / smaller < 2)
			continue;

		uint32_t ratio_x100 = (uint32_t) ((uint64_t) larger * 100 / smaller);
		char direction = after_transitions > before_transitions ? '+' : '-';
		printf("# DIFF ~%s transitions=%u->%u ratio=%c%u.%02ux\n", CGU_CLOCK_SIGNALS[index].name,
			(unsigned int) before_transitions, (unsigned int) after_transitions, direction,
			(unsigned int) (ratio_x100 / 100), (unsigned int) (ratio_x100 % 100));
		changed = true;
	}

	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_LEVEL_SIGNALS); index++) {
		if (before->levels[index] == after->levels[index])
			continue;

		printf("# DIFF %c%s\n", after->levels[index] ? '+' : '-', CGU_LEVEL_SIGNALS[index].name);
		changed = true;
	}

	if (!changed)
		printf("# DIFF none\n");
	test_watchdog_serve();
}

static uint32_t get_cgu_transitions(const struct cgu_state *state, uint32_t selection) {
	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_CLOCK_SIGNALS); index++) {
		if (CGU_CLOCK_SIGNALS[index].selection == selection)
			return state->transitions[index];
	}

	return 0;
}

static void print_cgu_probe_transitions(
	const struct cgu_value_probe *probe,
	const struct cgu_state *before,
	const struct cgu_state *after
) {
	if (probe->transition_selection == 0)
		return;

	uint32_t before_transitions = get_cgu_transitions(before, probe->transition_selection);
	uint32_t after_transitions = get_cgu_transitions(after, probe->transition_selection);
	if (before_transitions == 0 || after_transitions == 0)
		return;

	printf("# TRANSITIONS %s %u->%u\n", probe->transition_name, (unsigned int) before_transitions,
		(unsigned int) after_transitions);
}

static void restore_cgu_probe(const struct cgu_value_probe *probe, uint32_t value, uint32_t osc) {
	apply_cgu_probe_value(probe, value);
	if (probe->required_osc != 0)
		apply_cgu_osc(osc);
}

static void probe_cgu_value(const struct cgu_value_probe *probe) {
	struct cgu_state cleared_state;
	struct cgu_state configured_state;
	struct cgu_state cleared_again_state;
	uint32_t initial_osc = CGU_OSC;

	printf("\n# PROBE %s mask=%08X value=%08X\n", probe->name, (unsigned int) probe->mask,
		(unsigned int) probe->value);
	if (probe->required_osc != 0 && !apply_cgu_osc(initial_osc | probe->required_osc)) {
		printf("# SETUP PLL_LOCK_TIMEOUT\n");
		return;
	}

	uint32_t initial = *probe->reg;
	uint32_t cleared = initial & ~probe->mask;
	uint32_t configured = cleared | probe->value;

	if (!apply_cgu_probe_value(probe, cleared)) {
		printf("# CLEAR PLL_LOCK_TIMEOUT\n");
		restore_cgu_probe(probe, initial, initial_osc);
		return;
	}
	capture_cgu_state(&cleared_state);

	stopwatch_usleep_wd(1000);
	if (!apply_cgu_probe_value(probe, configured)) {
		restore_cgu_probe(probe, initial, initial_osc);
		printf("# SET PLL_LOCK_TIMEOUT\n");
		return;
	}
	capture_cgu_state(&configured_state);

	if (!apply_cgu_probe_value(probe, cleared)) {
		restore_cgu_probe(probe, initial, initial_osc);
		printf("# CLEAR PLL_LOCK_TIMEOUT\n");
		return;
	}
	capture_cgu_state(&cleared_again_state);
	restore_cgu_probe(probe, initial, initial_osc);

	printf("# SET\n");
	print_cgu_diff(&cleared_state, &configured_state);
	print_cgu_probe_transitions(probe, &cleared_state, &configured_state);
	printf("# CLEAR\n");
	print_cgu_diff(&configured_state, &cleared_again_state);
	print_cgu_probe_transitions(probe, &configured_state, &cleared_again_state);
}

static void route_cgu_clocks_to_osc(void) {
	CGU_CON1 &= ~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FSYS_CLKSEL | CGU_CON1_AHB_CLKSEL);
	CGU_CON2 &= ~(CGU_CON2_DSP_CLKSEL | CGU_CON2_EBU_CLKSEL | CGU_CON2_CLK48M_CLKSEL);
	CGU_CON2 &= ~CGU_CON2_AFC32K_EN;
	CGU_CON3 &= ~(CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_MMCI_CLKSEL);
}

static void probe_afc32k_fsys_combination(void) {
	struct cgu_state oscillator_state;
	struct cgu_state pll_state;
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_con2 = CGU_CON2;
	uint32_t oscillator_con1 =
		(initial_con1 & ~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FSYS_CLKSEL)) |
		CGU_CON1_FPI1_CLKSEL_PLL_DIV_2;
	uint32_t oscillator_con2 = initial_con2 & ~CGU_CON2_AFC32K_EN;
	uint32_t pll_con1 = oscillator_con1 | CGU_CON1_FSYS_CLKSEL_PLL;
	uint32_t pll_con2 = oscillator_con2 | CGU_CON2_AFC32K_EN;

	printf("\n# PROBE FSYS + AFC32K combined state\n");
	CGU_CON1 = oscillator_con1;
	CGU_CON2 = oscillator_con2;
	capture_cgu_state(&oscillator_state);

	if (!apply_cgu_osc(initial_osc | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N)) {
		printf("# SET PLL_LOCK_TIMEOUT\n");
		CGU_CON1 = initial_con1;
		CGU_CON2 = initial_con2;
		apply_cgu_osc(initial_osc);
		return;
	}
	CGU_CON1 = pll_con1;
	CGU_CON2 = pll_con2;
	capture_cgu_state(&pll_state);

	CGU_CON1 = initial_con1;
	CGU_CON2 = initial_con2;
	apply_cgu_osc(initial_osc);

	printf("# OSC -> PLL\n");
	print_cgu_diff(&oscillator_state, &pll_state);
}

static void probe_clk48m_divider(void) {
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con0 = CGU_CON0;
	uint32_t initial_con2 = CGU_CON2;
	uint32_t initial_con3 = CGU_CON3;
	uint32_t con3_base = initial_con3 & ~CGU_CON3_CLK48M_CLKDIV;
	uint32_t transitions[4];

	printf("\n# PROBE CLK48M divider with PHASE4 source\n");
	CGU_CON0 = (initial_con0 & ~CGU_CON0_PHASE4_CONFIG) |
		(CGU_PHASE_PROBE_CONFIG << CGU_CON0_PHASE4_CONFIG_SHIFT);
	if (!apply_cgu_osc(initial_osc | CGU_OSC_PHASE4_POWER_UP | CGU_OSC_PHASE4_BYPASS_N)) {
		printf("# SETUP PLL_LOCK_TIMEOUT\n");
		CGU_CON0 = initial_con0;
		apply_cgu_osc(initial_osc);
		return;
	}
	CGU_CON2 = (initial_con2 & ~CGU_CON2_CLK48M_CLKSEL) | CGU_CON2_CLK48M_CLKSEL_PHASE4;
	for (uint32_t divider = 0; divider < ARRAY_SIZE(transitions); divider++) {
		CGU_CON3 = con3_base | (divider << CGU_CON3_CLK48M_CLKDIV_SHIFT);
		transitions[divider] = count_monitor_transitions(CGU_SIGNAL_CLK_48M_O);
		test_watchdog_serve();
	}

	CGU_CON2 = initial_con2;
	CGU_CON3 = initial_con3;
	CGU_CON0 = initial_con0;
	apply_cgu_osc(initial_osc);

	for (uint32_t divider = 0; divider < ARRAY_SIZE(transitions); divider++)
		printf("# CLKDIV=DIV%u transitions=%u\n", (1U << divider), (unsigned int) transitions[divider]);
}

static void probe_phase_configurations(void) {
	static const uint32_t CONFIGS[] = { 0x11, 0x22, 0x23 };
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con0 = CGU_CON0;

	printf("\n# PROBE phase K1/K2 configurations\n");
	for (uint32_t phase = 0; phase < ARRAY_SIZE(CGU_PHASE_CONFIG_PROBES); phase++) {
		const struct cgu_phase_probe *probe = &CGU_PHASE_CONFIG_PROBES[phase];

		if (!apply_cgu_osc(initial_osc | probe->power)) {
			printf("# %s PLL_LOCK_TIMEOUT\n", probe->name);
			CGU_CON0 = initial_con0;
			apply_cgu_osc(initial_osc);
			continue;
		}
		for (uint32_t config = 0; config < ARRAY_SIZE(CONFIGS); config++) {
			CGU_CON0 = (initial_con0 & ~probe->config_mask) | (CONFIGS[config] << probe->config_shift);
			uint32_t transitions = count_monitor_transitions(probe->selection);

			printf("# %s CONFIG=%02X transitions=%u\n", probe->name, (unsigned int) CONFIGS[config],
				(unsigned int) transitions);
			test_watchdog_serve();
		}
		CGU_CON0 = initial_con0;
		apply_cgu_osc(initial_osc);
	}
}

static uint32_t benchmark_register_reads(volatile uint32_t *reg, uint32_t *checksum) {
	uint32_t sum = 0;
	stopwatch_t start = stopwatch_get();
	for (uint32_t iteration = 0; iteration < CGU_ACCESS_BENCHMARK_ITERATIONS; iteration++)
		sum += *reg;
	uint32_t elapsed = (uint32_t) stopwatch_elapsed(start);

	*checksum = sum;
	return elapsed;
}

#ifdef PMB8876
static void probe_ahb_per_access_timing(void) {
	uint32_t initial_con3 = CGU_CON3;
	uint32_t initial_mmicif_clc = MMICIF_CLC;
	uint32_t elapsed[4];
	uint32_t checksum[4];

	MMICIF_CLC = (1 << MOD_CLC_RMC_SHIFT);
	for (uint32_t divider = 0; divider < ARRAY_SIZE(elapsed); divider++) {
		uint32_t selected_con3 =
			(initial_con3 & ~(CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV)) |
			CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2 |
			(divider << CGU_CON3_AHB_PER_CLKDIV_SHIFT);

		CGU_CON3 = selected_con3;
		test_spin(1000);
		elapsed[divider] = benchmark_register_reads(&MMICIF_ID, &checksum[divider]);
		test_watchdog_serve();
	}
	CGU_CON3 = initial_con3;
	MMICIF_CLC = initial_mmicif_clc;

	printf("\n# PROBE AHB_PER MMICIF access timing\n");
	for (uint32_t divider = 0; divider < ARRAY_SIZE(elapsed); divider++)
		printf("# CONTROL=%u stm_ticks=%u checksum=%08X\n", (unsigned int) divider,
			(unsigned int) elapsed[divider], (unsigned int) checksum[divider]);
}
#endif

static void probe_fpi1_access_timing(void) {
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_usart1_clc = USART_CLC(USART1);
	uint32_t elapsed[4];
	uint32_t checksum[4];

	USART_CLC(USART1) = (1 << MOD_CLC_RMC_SHIFT);
	for (uint32_t divider = 0; divider < ARRAY_SIZE(elapsed); divider++) {
		uint32_t selected_con1 =
			(initial_con1 & ~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV)) |
			CGU_CON1_FPI1_CLKSEL_PLL_DIV_2 |
			(divider << CGU_CON1_FPI1_CLKDIV_SHIFT);

		stopwatch_usleep_wd(1000);
		CGU_CON1 = selected_con1;
		test_spin(1000);
		elapsed[divider] = benchmark_register_reads(&USART_ID(USART1), &checksum[divider]);
		CGU_CON1 = initial_con1;
		test_watchdog_serve();
	}
	USART_CLC(USART1) = initial_usart1_clc;

	printf("\n# PROBE FPI1 USART1 access timing\n");
	for (uint32_t divider = 0; divider < ARRAY_SIZE(elapsed); divider++)
		printf("# CONTROL=%u stm_ticks=%u checksum=%08X\n", (unsigned int) divider,
			(unsigned int) elapsed[divider], (unsigned int) checksum[divider]);
}

static void probe_fpi1_fsys_access_timing(void) {
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_usart1_clc = USART_CLC(USART1);
	uint32_t checksum[2];
	uint32_t elapsed[2];
	uint32_t bypass_con1 =
		(initial_con1 & ~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FSYS_CLKSEL | CGU_CON1_FPI1_CLKDIV)) |
		CGU_CON1_FPI1_CLKSEL_PLL_DIV_2;

	USART_CLC(USART1) = (1 << MOD_CLC_RMC_SHIFT);
	CGU_CON1 = bypass_con1;
	elapsed[0] = benchmark_register_reads(&USART_ID(USART1), &checksum[0]);

	bool locked = apply_cgu_osc(initial_osc | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N);
	if (locked) {
		CGU_CON1 = bypass_con1 | CGU_CON1_FSYS_CLKSEL_PLL;
		elapsed[1] = benchmark_register_reads(&USART_ID(USART1), &checksum[1]);
	} else {
		elapsed[1] = 0;
		checksum[1] = 0;
	}

	CGU_CON1 = initial_con1;
	USART_CLC(USART1) = initial_usart1_clc;
	apply_cgu_osc(initial_osc);

	printf("\n# PROBE FPI1 MMIO dependency on fSYS\n");
	printf("# FSYS=BYPASS stm_ticks=%u checksum=%08X\n", (unsigned int) elapsed[0], (unsigned int) checksum[0]);
	printf("# FSYS=PLL stm_ticks=%u checksum=%08X locked=%u\n", (unsigned int) elapsed[1],
		(unsigned int) checksum[1], locked ? 1U : 0U);
}

static uint32_t measure_capcom_timer(void) {
	CAPCOM_T01CON(CAPCOM0) = 0;
	CAPCOM_T0(CAPCOM0) = 0;
	CAPCOM_T0REL(CAPCOM0) = 0;
	CAPCOM_T01CON(CAPCOM0) = CAPCOM_T01CON_T0R_ENABLED;
	stopwatch_t start = stopwatch_get();
	while (stopwatch_elapsed(start) < CGU_KERNEL_MEASURE_STM_TICKS)
		test_watchdog_serve();
	uint32_t ticks = CAPCOM_T0(CAPCOM0) & CAPCOM_T0_T0;
	CAPCOM_T01CON(CAPCOM0) = 0;

	return ticks;
}

static void probe_afc32k_dependencies(void) {
	uint32_t initial_osc = CGU_OSC;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_con2 = CGU_CON2;
	uint32_t initial_con3 = CGU_CON3;
	uint32_t initial_mmicif_clc = MMICIF_CLC;
	uint32_t initial_usart1_clc = USART_CLC(USART1);
	uint32_t checksum[2];
	uint32_t elapsed[2];
	uint32_t ahb_per_checksum[2];
	uint32_t ahb_per_elapsed[2];
	uint32_t fpi1_checksum[2];
	uint32_t fpi1_elapsed[2];
	uint32_t con_transitions[2];
	uint32_t afc_transitions[2];
	uint32_t trigger_transitions[2];
	uint32_t capcom_ticks[2];
	bool cerb_level[2];
	uint32_t initial_capcom_clc = CAPCOM_CLC(CAPCOM0);
	uint32_t initial_capcom_t01con = CAPCOM_T01CON(CAPCOM0);
	uint32_t initial_capcom_t0 = CAPCOM_T0(CAPCOM0);
	uint32_t initial_capcom_t0rel = CAPCOM_T0REL(CAPCOM0);

	CAPCOM_CLC(CAPCOM0) = (1 << MOD_CLC_RMC_SHIFT);
	MMICIF_CLC = (1 << MOD_CLC_RMC_SHIFT);
	USART_CLC(USART1) = (1 << MOD_CLC_RMC_SHIFT);
	bool locked = apply_cgu_osc(initial_osc | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N);
	CGU_CON1 = (initial_con1 & ~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV |
		CGU_CON1_FSYS_CLKSEL)) | CGU_CON1_FPI1_CLKSEL_PLL_DIV_2 | CGU_CON1_FSYS_CLKSEL_PLL;
	CGU_CON3 = (initial_con3 & ~(CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV)) |
		CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2;
	CGU_CON2 = initial_con2 & ~CGU_CON2_AFC32K_EN;
	elapsed[0] = benchmark_register_reads(&GPIO_ID, &checksum[0]);
	ahb_per_elapsed[0] = benchmark_register_reads(&MMICIF_ID, &ahb_per_checksum[0]);
	fpi1_elapsed[0] = benchmark_register_reads(&USART_ID(USART1), &fpi1_checksum[0]);
	capcom_ticks[0] = measure_capcom_timer();
	con_transitions[0] = count_monitor_transitions(CGU_SIGNAL_CLK_CON_O);
	afc_transitions[0] = count_monitor_transitions(CGU_SIGNAL_CLK_AFC_O);
	trigger_transitions[0] = count_monitor_transitions(CGU_SIGNAL_CLK_6M5_TRIG_O);
	cerb_level[0] = read_monitor_level(CGU_SIGNAL_EN_CERB_O);

	if (locked) {
		CGU_CON2 = initial_con2 | CGU_CON2_AFC32K_EN;
		elapsed[1] = benchmark_register_reads(&GPIO_ID, &checksum[1]);
		ahb_per_elapsed[1] = benchmark_register_reads(&MMICIF_ID, &ahb_per_checksum[1]);
		fpi1_elapsed[1] = benchmark_register_reads(&USART_ID(USART1), &fpi1_checksum[1]);
		capcom_ticks[1] = measure_capcom_timer();
		con_transitions[1] = count_monitor_transitions(CGU_SIGNAL_CLK_CON_O);
		afc_transitions[1] = count_monitor_transitions(CGU_SIGNAL_CLK_AFC_O);
		trigger_transitions[1] = count_monitor_transitions(CGU_SIGNAL_CLK_6M5_TRIG_O);
		cerb_level[1] = read_monitor_level(CGU_SIGNAL_EN_CERB_O);
	} else {
		elapsed[1] = 0;
		checksum[1] = 0;
		ahb_per_elapsed[1] = 0;
		ahb_per_checksum[1] = 0;
		fpi1_elapsed[1] = 0;
		fpi1_checksum[1] = 0;
		capcom_ticks[1] = 0;
		con_transitions[1] = 0;
		afc_transitions[1] = 0;
		trigger_transitions[1] = 0;
		cerb_level[1] = false;
	}

	CGU_CON1 = initial_con1;
	CGU_CON2 = initial_con2;
	CGU_CON3 = initial_con3;
	apply_cgu_osc(initial_osc);
	USART_CLC(USART1) = initial_usart1_clc;
	MMICIF_CLC = initial_mmicif_clc;
	CAPCOM_T0REL(CAPCOM0) = initial_capcom_t0rel;
	CAPCOM_T0(CAPCOM0) = initial_capcom_t0;
	CAPCOM_T01CON(CAPCOM0) = initial_capcom_t01con;
	CAPCOM_CLC(CAPCOM0) = initial_capcom_clc;

	printf("\n# PROBE AFC32K dependencies\n");
	printf("# AFC32K=DISABLED GPIO=%u/%08X AHB_PER=%u/%08X FPI1=%u/%08X CAPCOM=%u CLK_CON=%u CLK_AFC=%u "
		"CLK_6M5=%u EN_CERB=%u\n", (unsigned int) elapsed[0], (unsigned int) checksum[0],
		(unsigned int) ahb_per_elapsed[0], (unsigned int) ahb_per_checksum[0],
		(unsigned int) fpi1_elapsed[0], (unsigned int) fpi1_checksum[0], (unsigned int) capcom_ticks[0],
		(unsigned int) con_transitions[0], (unsigned int) afc_transitions[0],
		(unsigned int) trigger_transitions[0], cerb_level[0] ? 1U : 0U);
	printf("# AFC32K=ENABLED GPIO=%u/%08X AHB_PER=%u/%08X FPI1=%u/%08X CAPCOM=%u CLK_CON=%u CLK_AFC=%u "
		"CLK_6M5=%u EN_CERB=%u locked=%u\n", (unsigned int) elapsed[1], (unsigned int) checksum[1],
		(unsigned int) ahb_per_elapsed[1], (unsigned int) ahb_per_checksum[1],
		(unsigned int) fpi1_elapsed[1], (unsigned int) fpi1_checksum[1], (unsigned int) capcom_ticks[1],
		(unsigned int) con_transitions[1], (unsigned int) afc_transitions[1],
		(unsigned int) trigger_transitions[1], cerb_level[1] ? 1U : 0U, locked ? 1U : 0U);
}

static void probe_afc_and_gsm_clocks(void) {
	uint32_t initial_afc_clc = AFC_CLC;
	uint32_t initial_afc_value = AFC_AFCVAL;
	uint32_t initial_tpu_clc = TPU_CLC;
	uint32_t initial_gsmclk1 = TPU_GSMCLK1;
	uint32_t initial_gsmclk2 = TPU_GSMCLK2;
	uint32_t initial_gsmclk3 = TPU_GSMCLK3;

	AFC_CLC = MOD_CLC_DISR;
	uint32_t afc_disabled = count_monitor_transitions(CGU_SIGNAL_CLK_AFC_O);
	AFC_CLC = (1 << MOD_CLC_RMC_SHIFT);
	AFC_AFCVAL = 0x4800 | AFC_AFCVAL_ENAFC;
	uint32_t afc_enabled = count_monitor_transitions(CGU_SIGNAL_CLK_AFC_O);

	TPU_CLC = MOD_CLC_DISR;
	uint32_t trigger_disabled = count_monitor_transitions(CGU_SIGNAL_CLK_6M5_TRIG_O);
	TPU_CLC = (1 << MOD_CLC_RMC_SHIFT);
	TPU_GSMCLK1 = (1 << TPU_GSMCLK1_K_SHIFT);
	TPU_GSMCLK2 = (4 << TPU_GSMCLK2_L_SHIFT);
	TPU_GSMCLK3 = TPU_GSMCLK3_INIT;
	uint32_t trigger_enabled = count_monitor_transitions(CGU_SIGNAL_CLK_6M5_TRIG_O);

	TPU_GSMCLK1 = initial_gsmclk1;
	TPU_GSMCLK2 = initial_gsmclk2;
	TPU_GSMCLK3 = initial_gsmclk3;
	TPU_CLC = initial_tpu_clc;
	AFC_AFCVAL = initial_afc_value;
	AFC_CLC = initial_afc_clc;

	printf("\n# PROBE AFC and GSM trigger clocks\n");
	printf("# CLK_AFC_O disabled=%u enabled=%u\n", (unsigned int) afc_disabled, (unsigned int) afc_enabled);
	printf("# CLK_6M5_TRIG_O disabled=%u enabled=%u\n", (unsigned int) trigger_disabled,
		(unsigned int) trigger_enabled);
}

static void probe_cgu_clock_values(void) {
	uint32_t initial_con0 = CGU_CON0;
	uint32_t initial_con1 = CGU_CON1;
	uint32_t initial_con2 = CGU_CON2;
	uint32_t initial_con3 = CGU_CON3;

	printf("# CGU REG CTL1=%08X CTL3=%08X CTL4=%08X CTL5=%08X CTL6=%08X STAT=%08X\n",
		(unsigned int) CGU_OSC, (unsigned int) CGU_CON0, (unsigned int) CGU_CON1,
		(unsigned int) CGU_CON2, (unsigned int) CGU_CON3, (unsigned int) CGU_STAT);
	CGU_CON0 = (CGU_PHASE_PROBE_CONFIG << CGU_CON0_PHASE1_CONFIG_SHIFT) |
		(CGU_PHASE_PROBE_CONFIG << CGU_CON0_PHASE2_CONFIG_SHIFT) |
		(CGU_PHASE_PROBE_CONFIG << CGU_CON0_PHASE3_CONFIG_SHIFT) |
		(CGU_PHASE_PROBE_CONFIG << CGU_CON0_PHASE4_CONFIG_SHIFT);
	route_cgu_clocks_to_osc();
	print_cgu_levels();
	printf("# MON transition count may be zero for a running clock synchronous with the 26 MHz sampling loop\n");

	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_PHASE_PROBES); index++)
		probe_cgu_value(&CGU_PHASE_PROBES[index]);
	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_VALUE_PROBES); index++)
		probe_cgu_value(&CGU_VALUE_PROBES[index]);
	probe_afc32k_fsys_combination();
	probe_clk48m_divider();
	probe_phase_configurations();

#ifdef PMB8876
	probe_ahb_per_access_timing();
#endif
	probe_fpi1_access_timing();
	probe_fpi1_fsys_access_timing();
	probe_afc32k_dependencies();
	probe_afc_and_gsm_clocks();

	printf("\n# CGU UNSAFE PROBES\n");
	for (uint32_t index = 0; index < ARRAY_SIZE(CGU_UNSAFE_PROBES); index++)
		probe_cgu_value(&CGU_UNSAFE_PROBES[index]);

	CGU_CON0 = initial_con0;
	CGU_CON1 = initial_con1;
	CGU_CON2 = initial_con2;
	CGU_CON3 = initial_con3;
}

int main(void) {
	stopwatch_init();

#if SCU_RST_REQ_PROBE
	test_start("SCU reset request monitor probe");
#else
	test_start("CGU clock monitor probe");
	wdt_set_max_execution_time(CGU_PROBE_TIMEOUT_MS);
#endif

	GPIO_CLC = (1 << MOD_CLC_RMC_SHIFT);

#if SCU_RST_REQ_PROBE
	DSP_CLC = (1 << MOD_CLC_RMC_SHIFT);
#endif

	GPIO_PIN(GPIO_KP_OUT0) = GPIO_OS_ALT1 | GPIO_PS_ALT;
	stopwatch_msleep_wd(1);

#if SCU_RST_REQ_PROBE
	printf("# BASELINE\n");
	print_asserted_reset_signals(0);
	probe_scu_reset_request_bits();
#else
	probe_cgu_clock_values();
#endif

	return test_finish();
}
