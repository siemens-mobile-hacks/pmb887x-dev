/* I2Cv2 module clock source: SCL timing of PASIC reads under different CGU clock trees. */

#include <pmb887x.h>

#include "cgu/clock.h"
#include "driver.h"
#include "test.h"

#ifdef PMB8876

#include <pmic/PASIC.h>

#define READ_RUNS 4
#define LONG_READ 8
#define SCL_BITS_PER_BYTE 9

struct clock_case {
	const char *name;
	uint32_t con1_mask;
	uint32_t con1;
	uint32_t con3_mask;
	uint32_t con3;
	bool pll;
	uint32_t rmc;
	uint32_t dec;
	uint32_t inc;
};

struct clock_result {
	bool ok;
	uint32_t byte_ns;
	uint32_t kernel_hz;
	uint32_t ahb_hz;
	uint32_t sys_hz;
	uint32_t fpi1_hz;
	uint32_t ahb_per_hz;
};

static uint32_t initial_osc;
static uint32_t initial_con1;
static uint32_t initial_con3;

static void restore_clocks(void) {
	CGU_CON3 = initial_con3;
	CGU_CON1 = initial_con1;
	CGU_OSC = initial_osc;
	stopwatch_usleep_wd(2000);
}

static uint32_t read_ticks(uint32_t size) {
	uint8_t data[LONG_READ];
	uint32_t best = UINT32_MAX;

	for (uint32_t i = 0; i < READ_RUNS; i++) {
		stopwatch_t start = stopwatch_get();
		if (i2c_v2_smbus_read(PASIC_I2C_ADDR, PASIC_TURNOFF_REASON, data, size) != I2C_V2_DONE)
			return 0;
		uint32_t ticks = (uint32_t) stopwatch_elapsed(start);
		if (ticks < best)
			best = ticks;
		wdt_serve();
	}
	return best;
}

/* Measures with the case's clock tree; prints only after the console clock is restored. */
static struct clock_result measure_case(const struct clock_case *clock) {
	struct clock_result result = {0};

	if (clock->pll)
		cgu_pll_set(3, 0);
	CGU_CON1 = (CGU_CON1 & ~clock->con1_mask) | clock->con1;
	CGU_CON3 = (CGU_CON3 & ~clock->con3_mask) | clock->con3;
	stopwatch_usleep_wd(2000);
	result.ahb_hz = cpu_get_ahb_freq();
	result.sys_hz = cpu_get_sys_freq();
	result.fpi1_hz = cpu_get_fpi1_freq();
	result.ahb_per_hz = cpu_get_ahb_per_freq();

	I2C_RUNCTRL = 0;
	I2C_CLC = clock->rmc << MOD_CLC_RMC_SHIFT;
	I2C_FDIVCFG = (clock->dec << I2C_FDIVCFG_DEC_SHIFT) | (clock->inc << I2C_FDIVCFG_INC_SHIFT);
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
	uint32_t one = read_ticks(1);
	uint32_t many = read_ticks(LONG_READ);
	restore_clocks();

	result.ok = one != 0 && many > one;
	if (result.ok) {
		uint64_t byte_ticks = many - one;
		uint64_t scl_cycles = 2 * clock->dec + 3 * clock->inc;

		result.byte_ns = (uint32_t) (byte_ticks * 1000 / stopwatch_ticks_per_us() / (LONG_READ - 1));
		result.kernel_hz = (uint32_t) ((uint64_t) SCL_BITS_PER_BYTE * scl_cycles * stopwatch_ticks_per_s() *
			(LONG_READ - 1) / clock->inc / byte_ticks);
	}
	printf("# %s: %s, %lu ns per byte, I2C kernel ~%lu Hz; AHB %lu Hz, FSYS %lu Hz, FPI1 %lu Hz, "
		"AHB_PER %lu Hz\n", clock->name, result.ok ? "read ok" : "read FAILED", result.byte_ns,
		result.kernel_hz, result.ahb_hz, result.sys_hz, result.fpi1_hz, result.ahb_per_hz);
	return result;
}

#define SLOW_DEC 0x7F
#define SLOW_INC 1

static struct clock_case slow_case(const char *name, uint32_t rmc) {
	return (struct clock_case) {.name = name, .rmc = rmc, .dec = SLOW_DEC, .inc = SLOW_INC};
}

static void test_rmc(void) {
	char name[48];

	test_category("RMC divider, oscillator clock tree");
	for (uint32_t rmc = 1; rmc <= 8; rmc++) {
		snprintf(name, sizeof(name), "RMC=%lu DEC=0x7F INC=1", rmc);
		struct clock_case clock = slow_case(name, rmc);
		test_check(name, measure_case(&clock).ok);
	}
}

static void test_divider(void) {
	static const struct clock_case CASES[] = {
		{.name = "RMC=1 DEC=0x3F INC=1", .rmc = 1, .dec = 0x3F, .inc = 1},
		{.name = "RMC=1 DEC=0x7F INC=2", .rmc = 1, .dec = 0x7F, .inc = 2},
		{.name = "RMC=4 DEC=0x3D INC=4 (firmware)", .rmc = 4, .dec = 0x3D, .inc = 4},
	};

	test_category("Fractional divider");
	for (uint32_t i = 0; i < ARRAY_SIZE(CASES); i++)
		test_check(CASES[i].name, measure_case(&CASES[i]).ok);
}

static void test_sources(void) {
	static const struct clock_case CASES[] = {
		{.name = "FSYS PLL", .con1_mask = CGU_CON1_FSYS_CLKSEL, .con1 = CGU_CON1_FSYS_CLKSEL_PLL, .pll = true,
			.rmc = 1, .dec = SLOW_DEC, .inc = SLOW_INC},
		{.name = "FPI1 OSC /2", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_OSC | CGU_CON1_FPI1_CLKDIV_DIV2, .rmc = 1, .dec = SLOW_DEC,
			.inc = SLOW_INC},
		{.name = "FPI1 OSC /4", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_OSC | CGU_CON1_FPI1_CLKDIV_DIV4, .rmc = 1, .dec = SLOW_DEC,
			.inc = SLOW_INC},
		{.name = "FPI1 PLL/2", .con1_mask = CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV,
			.con1 = CGU_CON1_FPI1_CLKSEL_PLL_DIV_2, .pll = true, .rmc = 1, .dec = SLOW_DEC, .inc = SLOW_INC},
		{.name = "AHB_PER PLL/2", .con3_mask = CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
			.con3 = CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2, .pll = true, .rmc = 1, .dec = SLOW_DEC, .inc = SLOW_INC},
		{.name = "AHB_PER PLL/2 /2", .con3_mask = CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV,
			.con3 = CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2 | (1 << CGU_CON3_AHB_PER_CLKDIV_SHIFT), .pll = true,
			.rmc = 1, .dec = SLOW_DEC, .inc = SLOW_INC},
	};

	/*
	 * AHB is not switched: every AHB source other than the oscillator runs at 41 MHz or more, and the test
	 * code in external RAM then stops executing.
	 */
	test_category("CGU clock sources");
	for (uint32_t i = 0; i < ARRAY_SIZE(CASES); i++)
		test_check(CASES[i].name, measure_case(&CASES[i]).ok);
}

int main(void) {
	test_start("I2Cv2 clock source test");
	i2c_v2_init();
	cpu_enable_irq(false);
	initial_osc = CGU_OSC;
	initial_con1 = CGU_CON1;
	initial_con3 = CGU_CON3;
	printf("# boot CGU: OSC=%08lX CON0=%08lX CON1=%08lX CON3=%08lX; STM %lu ticks/us\n",
		initial_osc, CGU_CON0, initial_con1, initial_con3, stopwatch_ticks_per_us());

	test_rmc();
	test_divider();
	test_sources();

	return test_finish();
}

__IRQ void irq_handler(void) {
	i2c_v2_handle_irq(VIC_IRQ_CURRENT);
	VIC_IRQ_ACK = 1;
}

#else

int main(void) {
	test_start("I2Cv2 clock source test");
	test_skip("I2Cv2 clock source", "unsupported");

	return test_finish();
}

#endif
