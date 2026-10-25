#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#ifdef PMB8875

#define PMIC_I2C_ADDR 0x31
#define PMIC_LED_CONTROL_REG 0x14
#define READ_RUNS 4
#define READ_ATTEMPTS 16
#define LONG_READ 8
#define SLOW_RMC 8

struct clock_case {
	const char *name;
	uint32_t con1_mask;
	uint32_t con1;
	bool pll;
	uint32_t rmc;
	uint32_t percent_of_boot_time;
};

struct clock_result {
	bool ok;
	uint32_t byte_ns;
	uint32_t ahb_hz;
	uint32_t sys_hz;
	uint32_t fpi1_hz;
	uint32_t fpi2_hz;
};

static uint32_t initial_osc;
static uint32_t initial_con1;

static void restore_clocks(void) {
	CGU_CON1 = initial_con1;
	CGU_OSC = initial_osc;
	stopwatch_update();
	stopwatch_usleep_wd(2000);
}

static uint32_t read_ticks(uint32_t size, uint32_t *failures) {
	uint8_t data[LONG_READ];
	uint32_t best = UINT32_MAX;
	uint32_t successes = 0;

	for (uint32_t attempt = 0; attempt < READ_ATTEMPTS && successes < READ_RUNS; attempt++) {
		stopwatch_t start = stopwatch_get();
		bool ok = i2c_smbus_read(PMIC_I2C_ADDR, PMIC_LED_CONTROL_REG, size, data);
		uint32_t ticks = (uint32_t) stopwatch_elapsed(start);

		wdt_serve();
		if (!ok) {
			(*failures)++;
			continue;
		}
		successes++;
		if (ticks < best)
			best = ticks;
	}
	return successes == READ_RUNS ? best : 0;
}

static struct clock_result measure_case(const struct clock_case *clock) {
	struct clock_result result = {0};

	if (clock->pll)
		cgu_pll_set(3, 0);
	CGU_CON1 = (CGU_CON1 & ~clock->con1_mask) | clock->con1;
	stopwatch_update();
	stopwatch_usleep_wd(2000);
	result.ahb_hz = cpu_get_ahb_freq();
	result.sys_hz = cpu_get_sys_freq();
	result.fpi1_hz = cpu_get_fpi1_freq();
	result.fpi2_hz = cpu_get_fpi2_freq();
	uint32_t ticks_per_s = stopwatch_ticks_per_s();

	i2c_init();
	I2C_CLC = clock->rmc << MOD_CLC_RMC_SHIFT;
	uint32_t failures = 0;
	uint32_t one = read_ticks(1, &failures);
	uint32_t many = read_ticks(LONG_READ, &failures);
	restore_clocks();

	result.ok = one != 0 && many > one;
	if (result.ok)
		result.byte_ns = (uint32_t) ((uint64_t) (many - one) * 1000000000ULL / ticks_per_s / (LONG_READ - 1));
	printf("# %s: %s (%lu failed reads), %lu ns per byte; AHB %lu Hz, FSYS %lu Hz, FPI1 %lu Hz, FPI2 %lu Hz\n",
		clock->name, result.ok ? "read ok" : "read FAILED", failures, result.byte_ns, result.ahb_hz,
		result.sys_hz, result.fpi1_hz, result.fpi2_hz);
	return result;
}

static void test_sources(void) {
	static const struct clock_case CASES[] = {
		{.name = "FSYS PLL", .con1_mask = CGU_CON1_FSYS_CLKSEL, .con1 = CGU_CON1_FSYS_CLKSEL_PLL, .pll = true,
			.rmc = SLOW_RMC, .percent_of_boot_time = 100},
		{.name = "FPI1 OSC /2", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_OSC | CGU_CON1_FPI1_CLKDIV_DIV2, .rmc = SLOW_RMC, .percent_of_boot_time = 100},
		{.name = "FPI1 PLL/2", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_PLL_DIV_2, .pll = true, .rmc = SLOW_RMC, .percent_of_boot_time = 100},
		{.name = "FPI2 PLL/2", .con1_mask = CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV,
			.con1 = CGU_CON1_FPI2_CLKSEL_PLL, .pll = true, .rmc = SLOW_RMC, .percent_of_boot_time = 50},
		{.name = "RMC doubled", .rmc = 2 * SLOW_RMC, .percent_of_boot_time = 200},
		{.name = "FPI2 PLL/2, RMC doubled", .con1_mask = CGU_CON1_FPI2_CLKSEL | CGU_CON1_FPI2_CLKDIV,
			.con1 = CGU_CON1_FPI2_CLKSEL_PLL, .pll = true, .rmc = 2 * SLOW_RMC, .percent_of_boot_time = 100},
	};
	static const struct clock_case BOOT = {.name = "boot clocks", .rmc = SLOW_RMC};

	test_category("CGU clock sources");
	struct clock_result boot = measure_case(&BOOT);
	test_check("boot clocks", boot.ok);
	for (uint32_t i = 0; i < ARRAY_SIZE(CASES); i++) {
		uint64_t expected_ns = (uint64_t) boot.byte_ns * CASES[i].percent_of_boot_time / 100;
		struct clock_result result = measure_case(&CASES[i]);

		test_check(CASES[i].name, result.ok &&
			test_u32_in_interval(result.byte_ns, expected_ns * 90 / 100, expected_ns * 110 / 100));
	}
}

int main(void) {
	test_start("I2Cv1 clock source test");
	if (test_is_qemu()) {
		test_skip("I2Cv1 clock source", "QEMU does not model transfer time");
		return test_finish();
	}

	cpu_enable_irq(false);
	initial_osc = CGU_OSC;
	initial_con1 = CGU_CON1;
	printf("# boot CGU: OSC=%08lX CON0=%08lX CON1=%08lX; STM %lu ticks/us\n", initial_osc, CGU_CON0, initial_con1,
		stopwatch_ticks_per_us());

	test_sources();

	return test_finish();
}

#else

int main(void) {
	test_start("I2Cv1 clock source test");
	test_skip("I2Cv1 clock source", "unsupported");

	return test_finish();
}

#endif
