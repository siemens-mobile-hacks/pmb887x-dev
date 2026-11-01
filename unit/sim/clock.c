#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#define SIM_IRQ_MASK (SIM_IMSC_ERR | SIM_IMSC_IN | SIM_IMSC_OK)
#define SIM_DEFAULT_BRF 0x5D
#define SIM_TX_CHARACTERS 8
#define SIM_TX_TIMEOUT_MS 100
#define SIM_CHARACTER_ETU 12
#define SIM_TX_RESTART_TICKS_MAX 128
/* One ETU is 4 * BRF SIM clocks, and the SIM clock is FPI1 / 4 / RMC. */
#define SIM_FPI1_CLOCKS_PER_BRF_RMC_ETU 16
#define FPI1_OSC_HZ 26000000
#define FPI1_PLL_DIV_2_HZ 52000000
#define FPI1_DIVIDERS 4

struct sim_tx_config {
	const char *name;
	uint32_t rmc;
	uint32_t brf;
	uint32_t con;
	uint32_t txspc;
};

struct sim_tx_result {
	bool completed;
	uint32_t ticks;
};

static const struct sim_tx_config TX_CONFIGS[] = {
	{ "RMC1 BRF93", 1, SIM_DEFAULT_BRF, 0, 0 },
	{ "RMC2 BRF93", 2, SIM_DEFAULT_BRF, 0, 0 },
	{ "RMC4 BRF93", 4, SIM_DEFAULT_BRF, 0, 0 },
	{ "RMC1 BRF16", 1, 16, 0, 0 },
	{ "RMC1 BRF8", 1, 8, 0, 0 },
	{ "RMC1 BRF93 CLKSEL", 1, SIM_DEFAULT_BRF, SIM_CON_CLKSEL, 0 },
	{ "RMC1 BRF93 TXSPC4", 1, SIM_DEFAULT_BRF, 0, 4 },
};

static void sim_configure(const struct sim_tx_config *config) {
	SCU_RST_REQ |= SCU_RST_REQ_SIM;
	SCU_RST_REQ &= ~SCU_RST_REQ_SIM;
	SIM_CLC = config->rmc << MOD_CLC_RMC_SHIFT;
	SIM_CON = 0;
	SIM_BRF = config->brf;
	SIM_TXSPC = config->txspc;
	SIM_IRQEN = SIM_IRQEN_ENOKINT;
	SIM_IMSC = 0;
	SIM_ICR = SIM_IRQ_MASK;
	SIM_CON = SIM_CON_SIMEN | config->con;
	SIM_CON |= SIM_CON_ERROFF | SIM_CON_RPTOFF | SIM_CON_SIMON | SIM_CON_UARTON;
}

static bool sim_tx_character(void) {
	SIM_ICR = SIM_IRQ_MASK;
	SIM_TXB = 0x55;
	stopwatch_t deadline = stopwatch_get() + (stopwatch_t) SIM_TX_TIMEOUT_MS * stopwatch_ticks_per_ms();
	while ((SIM_RIS & SIM_RIS_OK) == 0) {
		if (stopwatch_get() >= deadline)
			return false;
	}
	return true;
}

static struct sim_tx_result sim_measure_tx(const struct sim_tx_config *config) {
	struct sim_tx_result result = { false, 0 };

	sim_configure(config);
	if (!sim_tx_character())
		return result;

	uint32_t start = STM_TIM0;
	for (uint32_t i = 0; i < SIM_TX_CHARACTERS; i++) {
		if (!sim_tx_character())
			return result;
	}
	result.ticks = (STM_TIM0 - start) / SIM_TX_CHARACTERS;
	result.completed = true;
	return result;
}

static uint32_t sim_expected_ticks(const struct sim_tx_config *config, uint32_t fpi1_hz) {
	uint32_t etu_fpi1_clocks = SIM_FPI1_CLOCKS_PER_BRF_RMC_ETU * config->brf * config->rmc;
	uint32_t fpi1_clocks = (SIM_CHARACTER_ETU + config->txspc) * etu_fpi1_clocks;
	return (uint64_t) fpi1_clocks * stopwatch_ticks_per_ms() * 1000 / fpi1_hz;
}

static void measure_configs(struct sim_tx_result results[ARRAY_SIZE(TX_CONFIGS)]) {
	for (size_t i = 0; i < ARRAY_SIZE(TX_CONFIGS); i++)
		results[i] = sim_measure_tx(&TX_CONFIGS[i]);
}

static void print_results(
	const char *clock_name,
	uint32_t fpi1_hz,
	const struct sim_tx_result results[ARRAY_SIZE(TX_CONFIGS)]
) {
	for (size_t i = 0; i < ARRAY_SIZE(TX_CONFIGS); i++) {
		printf("# %s %s: %s %lu STM ticks per character, expected %lu\n", clock_name, TX_CONFIGS[i].name,
			results[i].completed ? "ok" : "timeout", results[i].ticks, sim_expected_ticks(&TX_CONFIGS[i], fpi1_hz));
	}
}

static bool results_match_model(uint32_t fpi1_hz, const struct sim_tx_result results[ARRAY_SIZE(TX_CONFIGS)]) {
	bool matches = true;

	for (size_t i = 0; i < ARRAY_SIZE(TX_CONFIGS); i++) {
		uint32_t expected = sim_expected_ticks(&TX_CONFIGS[i], fpi1_hz);
		uint32_t maximum = expected * 102 / 100 + SIM_TX_RESTART_TICKS_MAX;
		matches = matches && results[i].completed &&
			test_u32_in_interval(results[i].ticks, expected * 98 / 100, maximum);
	}
	return matches;
}

int main(void) {
	static struct sim_tx_result osc_results[ARRAY_SIZE(TX_CONFIGS)];
	static struct sim_tx_result pll_results[FPI1_DIVIDERS][ARRAY_SIZE(TX_CONFIGS)];
	static struct sim_tx_result fsys_pll_results[ARRAY_SIZE(TX_CONFIGS)];

	test_start("SIM clock");

	cgu_fpi1_select(CGU_FPI1_DISABLED, 0);
	cgu_pll_set(3, 0);
	cgu_fpi1_select(CGU_FPI1_OSC, 0);
	measure_configs(osc_results);
	for (uint32_t divider = 0; divider < FPI1_DIVIDERS; divider++) {
		cgu_fpi1_select(CGU_FPI1_PLL_DIV_2, divider);
		measure_configs(pll_results[divider]);
	}
	cgu_fpi1_select(CGU_FPI1_PLL_DIV_2, 0);
	cgu_fsys_select(CGU_FSYS_PLL);
	measure_configs(fsys_pll_results);
	cgu_fsys_select(CGU_FSYS_OSC);
	cgu_fpi1_select(CGU_FPI1_OSC, 0);

	print_results("FPI1 OSC", FPI1_OSC_HZ, osc_results);
	for (uint32_t divider = 0; divider < FPI1_DIVIDERS; divider++) {
		char name[24];
		snprintf(name, sizeof(name), "FPI1 PLL/2 /%lu", 1UL << divider);
		print_results(name, FPI1_PLL_DIV_2_HZ >> divider, pll_results[divider]);
	}
	print_results("FPI1 PLL/2 fSYS PLL", FPI1_PLL_DIV_2_HZ, fsys_pll_results);

	test_check("Character time follows BRF, RMC and TXSPC at FPI1 OSC", results_match_model(FPI1_OSC_HZ, osc_results));
	bool follows_fpi1 = true;
	for (uint32_t divider = 0; divider < FPI1_DIVIDERS; divider++)
		follows_fpi1 = follows_fpi1 && results_match_model(FPI1_PLL_DIV_2_HZ >> divider, pll_results[divider]);
	test_check("Character time follows FPI1 PLL/2 and its dividers", follows_fpi1);
	test_check("fSYS selection does not change character time",
		results_match_model(FPI1_PLL_DIV_2_HZ, fsys_pll_results));
	return test_finish();
}
