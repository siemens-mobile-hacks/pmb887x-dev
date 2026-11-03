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

struct sim_clock_case {
	const char *name;
	enum cgu_fpi1_source source;
	uint32_t divider;
	enum cgu_fsys_source fsys;
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

static const struct sim_clock_case CLOCK_CASES[] = {
	{ "Character time follows FPI1 PLL/2", CGU_FPI1_PLL_DIV_2, 0, CGU_FSYS_OSC },
	{ "Character time follows the FPI1 divider", CGU_FPI1_PLL_DIV_2, 2, CGU_FSYS_OSC },
	{ "fSYS PLL does not change character time", CGU_FPI1_PLL_DIV_2, 0, CGU_FSYS_PLL },
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

static void print_result(
	const char *name,
	const struct sim_tx_config *config,
	uint32_t fpi1_hz,
	struct sim_tx_result result
) {
	printf("# %s at FPI1 %lu Hz: %s %lu STM ticks per character, expected %lu\n", name, fpi1_hz,
		result.completed ? "ok" : "timeout", result.ticks, sim_expected_ticks(config, fpi1_hz));
}

static bool result_matches_model(const struct sim_tx_config *config, uint32_t fpi1_hz, struct sim_tx_result result) {
	uint32_t expected = sim_expected_ticks(config, fpi1_hz);
	uint32_t maximum = expected * 102 / 100 + SIM_TX_RESTART_TICKS_MAX;
	return result.completed && test_u32_in_interval(result.ticks, expected * 98 / 100, maximum);
}

static void test_tx_configs(void) {
	uint32_t fpi1_hz = cpu_get_fpi1_freq();
	bool matches = true;

	for (size_t i = 0; i < ARRAY_SIZE(TX_CONFIGS); i++) {
		struct sim_tx_result result = sim_measure_tx(&TX_CONFIGS[i]);
		print_result(TX_CONFIGS[i].name, &TX_CONFIGS[i], fpi1_hz, result);
		matches = matches && result_matches_model(&TX_CONFIGS[i], fpi1_hz, result);
	}
	test_check("Character time follows BRF, RMC and TXSPC", matches);
}

static void test_clock_case(const struct sim_clock_case *clock_case) {
	const struct sim_tx_config *config = &TX_CONFIGS[0];

	cgu_fpi1_select(clock_case->source, clock_case->divider);
	cgu_fsys_select(clock_case->fsys);
	uint32_t fpi1_hz = cpu_get_fpi1_freq();
	struct sim_tx_result result = sim_measure_tx(config);
	cgu_fsys_select(CGU_FSYS_OSC);
	cgu_fpi1_select(CGU_FPI1_OSC, 0);

	print_result(clock_case->name, config, fpi1_hz, result);
	test_check(clock_case->name, result_matches_model(config, fpi1_hz, result));
}

int main(void) {
	test_start("SIM clock");

	cgu_fpi1_select(CGU_FPI1_DISABLED, 0);
	cgu_pll_set(3, 0);
	cgu_fpi1_select(CGU_FPI1_OSC, 0);
	test_tx_configs();
	for (size_t i = 0; i < ARRAY_SIZE(CLOCK_CASES); i++)
		test_clock_case(&CLOCK_CASES[i]);
	return test_finish();
}
