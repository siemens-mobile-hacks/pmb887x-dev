#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#define RUNNING_MARKER 0x1111
#define VBTX_COUNT_OFFSET 0x0010
#define LATE_VBTX_COUNT_OFFSET 0x0011
#define REQUESTED_BCON_OFFSET 0x0020
#define APPLIED_BCON_OFFSET 0x0021
#define MIRROR_RWADDR_OFFSET 0x0030
#define MIRROR_REQUEST 0x8000
#define SETTLE_MS 20
#define MIRROR_MS 200
#define PACING_MS 500
#define AFE_RING_POSITIONS 64
#define PLL_LOCK_TIMEOUT_MS 100
#define PLL_CLOCK_HZ 104000000

#ifdef PMB8875
#include "afe-tx-pacing-8875.inc"
#define DSP_AFE_TX_PACING_IMAGE DSP_AFE_TX_PACING_8875
#else
#include "afe-tx-pacing-8876.inc"
#define DSP_AFE_TX_PACING_IMAGE DSP_AFE_TX_PACING_8876
#endif

#define BCON_BOTH_PATHS (TEAK_AFE_BCON_MODE | TEAK_AFE_BCON_RXSTART | TEAK_AFE_BCON_TXSTART)
#define BCON_FIRMWARE_AUDIO (BCON_BOTH_PATHS | TEAK_AFE_BCON_RXRATE_KHZ_47_619)

struct afe_rate_config {
	const char *name;
	uint16_t bcon;
	uint32_t read_position_hz;
	uint32_t write_position_hz;
};

struct afe_clock_case {
	const char *name;
	struct afe_rate_config rates[6];
	uint32_t firmware_vbtx_hz;
};

/*
 * Measured on PMB8875 and PMB8876: the rings advance in proportion to the mixed-signal clock, one word per sample at
 * 104 MHz, except that RX rates from 31.746 kHz take two words (a stereo pair) per sample.
 */
static const struct afe_clock_case CLK_IN_CASE = {
	"26 MHz DSP and mixed-signal clocks",
	{
		{ "RX 8 kHz, TX 8 kHz", BCON_BOTH_PATHS, 2000, 2000 },
		{ "RX 16 kHz, TX 8 kHz", BCON_BOTH_PATHS | TEAK_AFE_BCON_RXRATE_KHZ_16, 4000, 2000 },
		{ "RX 31.746 kHz, TX 8 kHz", BCON_BOTH_PATHS | TEAK_AFE_BCON_RXRATE_KHZ_31_746, 15873, 2000 },
		{ "RX 44.444 kHz, TX 8 kHz", BCON_BOTH_PATHS | TEAK_AFE_BCON_RXRATE_KHZ_44_444, 22222, 2000 },
		{ "RX 47.619 kHz, TX 8 kHz", BCON_FIRMWARE_AUDIO, 23810, 2000 },
		{ "RX 47.619 kHz, TX 16 kHz", BCON_FIRMWARE_AUDIO | TEAK_AFE_BCON_TXRATE_KHZ_16, 23810, 4000 },
	},
	2000,
};

static const struct afe_clock_case PLL_CASE = {
	"104 MHz DSP and mixed-signal clocks",
	{
		{ "RX 8 kHz, TX 8 kHz", BCON_BOTH_PATHS, 8000, 8000 },
		{ "RX 16 kHz, TX 8 kHz", BCON_BOTH_PATHS | TEAK_AFE_BCON_RXRATE_KHZ_16, 16000, 8000 },
		{ "RX 31.746 kHz, TX 8 kHz", BCON_BOTH_PATHS | TEAK_AFE_BCON_RXRATE_KHZ_31_746, 63492, 8000 },
		{ "RX 44.444 kHz, TX 8 kHz", BCON_BOTH_PATHS | TEAK_AFE_BCON_RXRATE_KHZ_44_444, 88889, 8000 },
		{ "RX 47.619 kHz, TX 8 kHz", BCON_FIRMWARE_AUDIO, 95238, 8000 },
		{ "RX 47.619 kHz, TX 16 kHz", BCON_FIRMWARE_AUDIO | TEAK_AFE_BCON_TXRATE_KHZ_16, 95238, 16000 },
	},
	8000,
};

static uint32_t count_to_rate_hz(uint32_t count, uint32_t elapsed_ticks) {
	return (uint64_t) count * stopwatch_ticks_per_ms() * 1000 / elapsed_ticks;
}

static bool rate_matches(uint32_t measured_hz, uint32_t expected_hz) {
	return test_u32_in_interval(measured_hz, expected_hz * 99 / 100, expected_hz * 101 / 100);
}

static void delay_ms(uint32_t milliseconds) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < milliseconds)
		test_watchdog_serve();
}

static uint32_t ring_steps(uint16_t from, uint16_t to) {
	return (to - from) & (AFE_RING_POSITIONS - 1);
}

static bool afe_apply_bcon(uint16_t request) {
	dsp_hw_shared_memory[REQUESTED_BCON_OFFSET] = request;
	if (!dsp_hw_wait_shared(APPLIED_BCON_OFFSET, request, 100))
		return false;
	delay_ms(SETTLE_MS);
	return true;
}

static void test_position_rates(const struct afe_rate_config *config) {
	char name[96];

	snprintf(name, sizeof(name), "%s: DSP mirrors RWADDR", config->name);
	if (!test_check(name, afe_apply_bcon(config->bcon | MIRROR_REQUEST)))
		return;

	uint16_t previous = dsp_hw_shared_memory[MIRROR_RWADDR_OFFSET];
	uint32_t read_steps = 0;
	uint32_t write_steps = 0;
	uint32_t max_step = 0;
	stopwatch_t start = stopwatch_get();
	while (stopwatch_elapsed_ms(start) < MIRROR_MS) {
		uint16_t current = dsp_hw_shared_memory[MIRROR_RWADDR_OFFSET];
		uint32_t read = ring_steps(previous & TEAK_AFE_RWADDR_RDADDR, current & TEAK_AFE_RWADDR_RDADDR);
		uint32_t write = ring_steps(previous >> TEAK_AFE_RWADDR_WRADDR_SHIFT, current >> TEAK_AFE_RWADDR_WRADDR_SHIFT);
		read_steps += read;
		write_steps += write;
		max_step = MAX(max_step, MAX(read, write));
		previous = current;
	}
	uint32_t elapsed_ticks = stopwatch_get() - start;

	uint32_t read_hz = count_to_rate_hz(read_steps, elapsed_ticks);
	uint32_t write_hz = count_to_rate_hz(write_steps, elapsed_ticks);
	printf("# %s: RD %lu positions/s, WR %lu positions/s, largest step %lu\n", config->name, read_hz, write_hz,
		max_step);
	snprintf(name, sizeof(name), "%s: read pointer advances at %lu positions/s", config->name,
		config->read_position_hz);
	test_check(name, rate_matches(read_hz, config->read_position_hz));
	snprintf(name, sizeof(name), "%s: write pointer advances at %lu positions/s", config->name,
		config->write_position_hz);
	test_check(name, rate_matches(write_hz, config->write_position_hz));
}

static bool clocks_select_pll(void) {
#ifdef PMB8875
	CGU_OSC |= CGU_OSC_PLL_POWER_UP;
#else
	uint32_t pll_mask = CGU_OSC_NDIV | CGU_OSC_MDIV | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
	uint32_t osc = (CGU_OSC & ~pll_mask) | (3 << CGU_OSC_NDIV_SHIFT);

	CGU_CON1 &= ~(CGU_CON1_AHB_CLKSEL | CGU_CON1_FSYS_CLKSEL);
	CGU_OSC = osc;
	CGU_OSC = osc | CGU_OSC_PLL_POWER_UP;
#endif
	stopwatch_t start = stopwatch_get();
	while ((CGU_STAT & CGU_STAT_LOCK) == 0) {
		if (stopwatch_elapsed_ms(start) >= PLL_LOCK_TIMEOUT_MS)
			return false;
		test_watchdog_serve();
	}
	CGU_OSC |= CGU_OSC_PLL_BYPASS_N;
	CGU_CON2 = (CGU_CON2 & ~(CGU_CON2_MS_CLKSEL | CGU_CON2_DSP_CLKSEL)) |
		CGU_CON2_MS_CLKSEL_PLL |
		CGU_CON2_DSP_CLKSEL_PLL;
	stopwatch_update();
	printf("# PLL clocks: DSP %lu Hz, mixed-signal %lu Hz\n", cpu_get_dsp_freq(), cpu_get_ms_freq());
	return cpu_get_dsp_freq() == PLL_CLOCK_HZ && cpu_get_ms_freq() == PLL_CLOCK_HZ;
}

static void test_firmware_pacing(uint32_t expected_vbtx_hz) {
	if (!test_check("firmware audio rates start with VBTX enabled", afe_apply_bcon(BCON_FIRMWARE_AUDIO)))
		return;

	uint16_t start_vbtx = dsp_hw_shared_memory[VBTX_COUNT_OFFSET];
	uint16_t start_late = dsp_hw_shared_memory[LATE_VBTX_COUNT_OFFSET];
	stopwatch_t start = stopwatch_get();
	delay_ms(PACING_MS);
	uint16_t vbtx = dsp_hw_shared_memory[VBTX_COUNT_OFFSET] - start_vbtx;
	uint16_t late = dsp_hw_shared_memory[LATE_VBTX_COUNT_OFFSET] - start_late;
	uint32_t vbtx_hz = count_to_rate_hz(vbtx, stopwatch_get() - start);

	printf("# firmware pacing: VBTX %lu Hz, late handlers %u\n", vbtx_hz, late);
	/* PMB8875 and PMB8876 lose up to 2% of VBTX here while 47.619 kHz playback keeps VBRX active. */
	test_check("re-arming TXINTPTR one position ahead raises VBTX at every write position",
		test_u32_in_interval(vbtx_hz, expected_vbtx_hz * 97 / 100, expected_vbtx_hz * 101 / 100));
	test_eq_u32("no VBTX handler finds the write pointer past the armed position", 0, late);
}

static void test_clock_case(const struct afe_clock_case *clock_case) {
	test_category(clock_case->name);
	for (size_t i = 0; i < ARRAY_SIZE(clock_case->rates); i++)
		test_position_rates(&clock_case->rates[i]);
	test_firmware_pacing(clock_case->firmware_vbtx_hz);
}

int main(void) {
	test_start("DSP AFE transmit pacing");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load AFE pacing runner",
		dsp_hw_load_image(DSP_AFE_TX_PACING_IMAGE, sizeof(DSP_AFE_TX_PACING_IMAGE))))
		return test_finish();
	if (!test_check("BRANCH starts AFE pacing runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("AFE pacing runner waits for BCON", dsp_hw_wait_shared(0, RUNNING_MARKER, 500)))
		return test_finish();

	test_clock_case(&CLK_IN_CASE);
	if (!test_check("PLL drives the DSP and mixed-signal clocks at 104 MHz", clocks_select_pll()))
		return test_finish();
	test_clock_case(&PLL_CASE);
	return test_finish();
}
