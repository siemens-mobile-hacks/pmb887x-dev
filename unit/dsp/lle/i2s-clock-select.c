#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define COMMAND_RUN 1
#define COMMAND_DONE 2
#define COMMAND_WRITE 3
#define SAMPLES 0x2000

#if I2S_INSTANCE == 1
#define TEST_TITLE "DSP I2S1 clock select test"
#define TEST_MODULE "I2S1"
#define TEST_CTRL TEAK_I2S1_CTRL
#define TEST_CSEL TEAK_I2S1_CSEL
#define TEST_RWADDR TEAK_I2S1_RWADDR
#define TEST_NUM0 TEAK_I2S1_NUM0
#define TEST_DEN0 TEAK_I2S1_DEN0
#define TEST_NUM1 TEAK_I2S1_NUM1
#define TEST_DEN1 TEAK_I2S1_DEN1
#define TEST_TXCONF TEAK_I2S1_TXCONF
#define TEST_RXCONF TEAK_I2S1_RXCONF
#define TEST_TXINTADDR TEAK_I2S1_TXINTADDR
#define TEST_RXINTADDR TEAK_I2S1_RXINTADDR
#define TEST_TX_IRQ TEAK_INT_FINTB0_I2S1TX
#define TEST_CTRL_I2SON TEAK_I2S1_CTRL_I2SON
#define TEST_CTRL_I2STXSTART TEAK_I2S1_CTRL_I2STXSTART
#define TEST_CTRL_I2SRXSTART TEAK_I2S1_CTRL_I2SRXSTART
#define TEST_CTRL_TXPCM TEAK_I2S1_CTRL_TXPCM
#define TEST_CSEL_TXCLKSEL_SHIFT TEAK_I2S1_CSEL_TXCLKSEL_SHIFT
#define TEST_CSEL_CLK0SEL_SHIFT TEAK_I2S1_CSEL_CLK0SEL_SHIFT
#define TEST_CSEL_RXCLKSEL_SHIFT TEAK_I2S1_CSEL_RXCLKSEL_SHIFT
#define TEST_CSEL_CLK1SEL_SHIFT TEAK_I2S1_CSEL_CLK1SEL_SHIFT
#define FREF0_104MHZ TEAK_I2S1_NUM0_FREF_CLOCK_104MHZ
#define DEN0_SLOWEST TEAK_I2S1_DEN0_DENOMINATOR
#define FREF1_104MHZ TEAK_I2S1_NUM1_FREF_CLOCK_104MHZ
#define TX_PERIOD_32 TEAK_I2S1_TXCONF_PERIOD_CLOCKS_32
#define TX_PERIOD_48 TEAK_I2S1_TXCONF_PERIOD_CLOCKS_48
#define TX_PERIOD_64 TEAK_I2S1_TXCONF_PERIOD_CLOCKS_64
#define TX_WIDTH_18 TEAK_I2S1_TXCONF_WIDTH_BITS_18
#define TX_WIDTH_24 TEAK_I2S1_TXCONF_WIDTH_BITS_24
#define TX_WIDTH_32 TEAK_I2S1_TXCONF_WIDTH_BITS_32
#define RX_PERIOD_32 TEAK_I2S1_RXCONF_PERIOD_CLOCKS_32
#define RX_PERIOD_64 TEAK_I2S1_RXCONF_PERIOD_CLOCKS_64
#define RX_WIDTH_18 TEAK_I2S1_RXCONF_WIDTH_BITS_18
#define RX_WIDTH_32 TEAK_I2S1_RXCONF_WIDTH_BITS_32
#elif I2S_INSTANCE == 2
#define TEST_TITLE "DSP I2S2 clock select test"
#define TEST_MODULE "I2S2"
#define TEST_CTRL TEAK_I2S2_CTRL
#define TEST_CSEL TEAK_I2S2_CSEL
#define TEST_RWADDR TEAK_I2S2_RWADDR
#define TEST_NUM0 TEAK_I2S2_NUM0
#define TEST_DEN0 TEAK_I2S2_DEN0
#define TEST_NUM1 TEAK_I2S2_NUM1
#define TEST_DEN1 TEAK_I2S2_DEN1
#define TEST_TXCONF TEAK_I2S2_TXCONF
#define TEST_RXCONF TEAK_I2S2_RXCONF
#define TEST_TXINTADDR TEAK_I2S2_TXINTADDR
#define TEST_RXINTADDR TEAK_I2S2_RXINTADDR
#define TEST_TX_IRQ TEAK_INT_FINTB0_I2S2TX
#define TEST_CTRL_I2SON TEAK_I2S2_CTRL_I2SON
#define TEST_CTRL_I2STXSTART TEAK_I2S2_CTRL_I2STXSTART
#define TEST_CTRL_I2SRXSTART TEAK_I2S2_CTRL_I2SRXSTART
#define TEST_CTRL_TXPCM TEAK_I2S2_CTRL_TXPCM
#define TEST_CSEL_TXCLKSEL_SHIFT TEAK_I2S2_CSEL_TXCLKSEL_SHIFT
#define TEST_CSEL_CLK0SEL_SHIFT TEAK_I2S2_CSEL_CLK0SEL_SHIFT
#define TEST_CSEL_RXCLKSEL_SHIFT TEAK_I2S2_CSEL_RXCLKSEL_SHIFT
#define TEST_CSEL_CLK1SEL_SHIFT TEAK_I2S2_CSEL_CLK1SEL_SHIFT
#define FREF0_104MHZ TEAK_I2S2_NUM0_FREF_CLOCK_104MHZ
#define DEN0_SLOWEST TEAK_I2S2_DEN0_DENOMINATOR
#define FREF1_104MHZ TEAK_I2S2_NUM1_FREF_CLOCK_104MHZ
#define TX_PERIOD_32 TEAK_I2S2_TXCONF_PERIOD_CLOCKS_32
#define TX_PERIOD_48 TEAK_I2S2_TXCONF_PERIOD_CLOCKS_48
#define TX_PERIOD_64 TEAK_I2S2_TXCONF_PERIOD_CLOCKS_64
#define TX_WIDTH_18 TEAK_I2S2_TXCONF_WIDTH_BITS_18
#define TX_WIDTH_24 TEAK_I2S2_TXCONF_WIDTH_BITS_24
#define TX_WIDTH_32 TEAK_I2S2_TXCONF_WIDTH_BITS_32
#define RX_PERIOD_32 TEAK_I2S2_RXCONF_PERIOD_CLOCKS_32
#define RX_PERIOD_64 TEAK_I2S2_RXCONF_PERIOD_CLOCKS_64
#define RX_WIDTH_18 TEAK_I2S2_RXCONF_WIDTH_BITS_18
#define RX_WIDTH_32 TEAK_I2S2_RXCONF_WIDTH_BITS_32
#else
#error Unsupported I2S instance
#endif

#ifdef PMB8875
#include "i2s-clock-select-8875.inc"
#define DSP_I2S_CLOCK_SELECT_IMAGE DSP_I2S_CLOCK_SELECT_8875
#else
#include "i2s-clock-select-8876.inc"
#define DSP_I2S_CLOCK_SELECT_IMAGE DSP_I2S_CLOCK_SELECT_8876
#endif

#define CTRL_TX (TEST_CTRL_I2SON | TEST_CTRL_I2STXSTART)
#define CTRL_RX (TEST_CTRL_I2SON | TEST_CTRL_I2SRXSTART)
#define CTRL_BOTH (CTRL_TX | CTRL_RX)
#define CSEL_BOTH_DIVIDER_0 (3 << TEST_CSEL_RXCLKSEL_SHIFT)

typedef struct {
	uint16_t csel;
	uint16_t ctrl;
	uint16_t num0;
	uint16_t den0;
	uint16_t num1;
	uint16_t den1;
	uint16_t txconf;
	uint16_t rxconf;
} configuration_t;

typedef struct {
	uint16_t tx_positions;
	uint16_t rx_positions;
	uint16_t reset_pointer;
	uint16_t final_pointer;
	uint32_t elapsed_us;
} measurement_t;

static void load_parameters(void) {
	dsp_hw_shared_memory[0x0010] = TEST_CTRL;
	dsp_hw_shared_memory[0x0011] = TEST_CSEL;
	dsp_hw_shared_memory[0x0012] = TEST_RWADDR;
	dsp_hw_shared_memory[0x0013] = TEST_NUM0;
	dsp_hw_shared_memory[0x0014] = TEST_DEN0;
	dsp_hw_shared_memory[0x0015] = TEST_NUM1;
	dsp_hw_shared_memory[0x0016] = TEST_DEN1;
	dsp_hw_shared_memory[0x0017] = TEST_TXCONF;
	dsp_hw_shared_memory[0x0018] = TEST_RXCONF;
	dsp_hw_shared_memory[0x0029] = SAMPLES;
	dsp_hw_shared_memory[0x0048] = TEST_TXINTADDR;
}

static uint32_t positions_per_second(uint32_t positions, uint32_t elapsed_us) {
	return (uint64_t) positions * 1000000 / elapsed_us;
}

static bool measure(const configuration_t *configuration, measurement_t *result) {
	dsp_hw_shared_memory[0x0020] = configuration->ctrl;
	dsp_hw_shared_memory[0x0021] = configuration->csel;
	dsp_hw_shared_memory[0x0023] = configuration->num0;
	dsp_hw_shared_memory[0x0024] = configuration->den0;
	dsp_hw_shared_memory[0x0025] = configuration->num1;
	dsp_hw_shared_memory[0x0026] = configuration->den1;
	dsp_hw_shared_memory[0x0027] = configuration->txconf;
	dsp_hw_shared_memory[0x0028] = configuration->rxconf;
	stopwatch_t start = stopwatch_get();
	dsp_hw_shared_memory[0x0001] = COMMAND_RUN;
	if (!dsp_hw_wait_shared(0x0001, COMMAND_DONE, 1000))
		return false;

	result->elapsed_us = stopwatch_elapsed_us(start);
	result->tx_positions = dsp_hw_shared_memory[0x0030];
	result->rx_positions = dsp_hw_shared_memory[0x0031];
	result->reset_pointer = dsp_hw_shared_memory[0x0032];
	result->final_pointer = dsp_hw_shared_memory[0x0033];
	printf("# %s TX %lu words/s, RX %lu words/s over %lu us\n", TEST_MODULE,
		positions_per_second(result->tx_positions, result->elapsed_us),
		positions_per_second(result->rx_positions, result->elapsed_us), result->elapsed_us);
	printf("# csel=%02X ctrl=%02X div0=%04X/%04X div1=%04X/%04X txconf=%04X rxconf=%04X "
		"tx=%u rx=%u reset=%04X final=%04X\n",
		configuration->csel, configuration->ctrl, configuration->num0, configuration->den0,
		configuration->num1, configuration->den1, configuration->txconf, configuration->rxconf,
		result->tx_positions, result->rx_positions, result->reset_pointer, result->final_pointer);
	return true;
}

static uint16_t csel_value(uint16_t txclksel, uint16_t clk0sel, uint16_t rxclksel, uint16_t clk1sel) {
	return (txclksel << TEST_CSEL_TXCLKSEL_SHIFT) |
		(clk0sel << TEST_CSEL_CLK0SEL_SHIFT) |
		(rxclksel << TEST_CSEL_RXCLKSEL_SHIFT) |
		(clk1sel << TEST_CSEL_CLK1SEL_SHIFT);
}

typedef struct {
	uint16_t pointer;
	uint16_t tx_irq;
	uint16_t readback;
} write_result_t;

static write_result_t write_register(uint16_t address, uint16_t value) {
	dsp_hw_shared_memory[0x0040] = address;
	dsp_hw_shared_memory[0x0041] = value;
	dsp_hw_shared_memory[0x0001] = COMMAND_WRITE;
	if (!dsp_hw_wait_shared(0x0001, COMMAND_DONE, 100)) {
		test_check("register write completes", false);
		return (write_result_t) { 0 };
	}
	return (write_result_t) {
		.pointer = dsp_hw_shared_memory[0x0042],
		.tx_irq = dsp_hw_shared_memory[0x0043] & TEST_TX_IRQ,
		.readback = dsp_hw_shared_memory[0x0049],
	};
}

static write_result_t start_before_first_frame(uint16_t txconf, uint16_t txintaddr, uint16_t ctrl) {
	write_register(TEST_CTRL, 0);
	write_register(TEST_CSEL, CSEL_BOTH_DIVIDER_0);
	write_register(TEST_NUM0, FREF0_104MHZ | 1);
	write_register(TEST_DEN0, DEN0_SLOWEST);
	write_register(TEST_TXCONF, txconf);
	write_register(TEST_RXCONF, RX_PERIOD_32);
	write_register(TEST_CTRL, TEST_CTRL_I2SON);
	write_register(TEST_TXINTADDR, txintaddr);
	write_register(TEAK_INT_RINTB0, TEST_TX_IRQ);
	write_result_t result = write_register(TEST_CTRL, ctrl);
	printf("# %s START txconf=%04X txintaddr=%u ctrl=%02X pointer=%04X irq=%u\n", TEST_MODULE, txconf, txintaddr,
		ctrl, result.pointer, result.tx_irq != 0);
	return result;
}

// Divider 1 runs three times faster than divider 0, and TX and RX use different frame periods,
// so the measured rates identify the divider and whether a direction shares the other's clock.
static configuration_t select_configuration(uint16_t csel, uint16_t ctrl) {
	return (configuration_t) {
		.csel = csel,
		.ctrl = ctrl,
		.num0 = FREF0_104MHZ | 1,
		.den0 = 104,
		.num1 = FREF1_104MHZ | 3,
		.den1 = 104,
		.txconf = TX_PERIOD_32,
		.rxconf = RX_PERIOD_64,
	};
}

typedef struct {
	const char *name;
	configuration_t configuration;
	uint32_t tx_milli;
	uint32_t rx_milli;
} rate_case_t;

static const configuration_t REFERENCE_CONFIGURATION = {
	.csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 1, .den0 = 104,
	.txconf = TX_PERIOD_32, .rxconf = RX_PERIOD_32,
};

static const rate_case_t RATE_CASES[] = {
	{ "rate follows NUM",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 2, .den0 = 104,
		.txconf = TX_PERIOD_32, .rxconf = RX_PERIOD_32 }, 2000, 2000 },
	{ "rate follows NUM",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 4, .den0 = 104,
		.txconf = TX_PERIOD_32, .rxconf = RX_PERIOD_32 }, 4000, 4000 },
	{ "rate follows 1 / DEN",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 1, .den0 = 52,
		.txconf = TX_PERIOD_32, .rxconf = RX_PERIOD_32 }, 2000, 2000 },
	{ "rate follows 1 / DEN",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 1, .den0 = 208,
		.txconf = TX_PERIOD_32, .rxconf = RX_PERIOD_32 }, 500, 500 },
	{ "rate follows 1 / PERIOD",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 1, .den0 = 104,
		.txconf = TX_PERIOD_48, .rxconf = RX_PERIOD_64 }, 667, 500 },
	{ "rate follows 1 / PERIOD",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 1, .den0 = 104,
		.txconf = TX_PERIOD_64, .rxconf = RX_PERIOD_64 }, 500, 500 },
	{ "only TX words wider than 16 bits double the pointer rate",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 1, .den0 = 104,
		.txconf = TX_PERIOD_32 | TX_WIDTH_32, .rxconf = RX_PERIOD_32 }, 2000, 1000 },
	{ "only TX words wider than 16 bits double the pointer rate",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 1, .den0 = 104,
		.txconf = TX_PERIOD_32 | TX_WIDTH_24, .rxconf = RX_PERIOD_32 | RX_WIDTH_32 }, 2000, 1000 },
	{ "only TX words wider than 16 bits double the pointer rate",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 1, .den0 = 104,
		.txconf = TX_PERIOD_32 | TX_WIDTH_18, .rxconf = RX_PERIOD_32 | RX_WIDTH_18 }, 2000, 1000 },
	{ "fractional NUM / DEN rate",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 441, .den0 = 8125,
		.txconf = TX_PERIOD_32, .rxconf = RX_PERIOD_32 }, 5645, 5645 },
	{ "fractional NUM / DEN rate",
		{ .csel = CSEL_BOTH_DIVIDER_0, .ctrl = CTRL_BOTH, .num0 = FREF0_104MHZ | 441, .den0 = 32500,
		.txconf = TX_PERIOD_32, .rxconf = RX_PERIOD_32 }, 1411, 1411 },
	{ "EL71 audio configuration",
		{ .csel = 0x47, .ctrl = CTRL_BOTH, .num0 = 0x11B9, .den0 = 0x7EF4, .num1 = 0x11B9, .den1 = 0x1FBD,
		.txconf = 0x0710, .rxconf = 0x0015 }, 5645, 5645 },
};

static measurement_t reference;

static void check_rate(const char *name, uint32_t expected_milli, uint32_t positions, uint32_t reference_positions) {
	uint32_t expected = reference_positions * expected_milli;
	uint32_t measured = positions * 1000;
	uint32_t tolerance = expected / 50;
	if (!test_check(name, measured + tolerance >= expected && measured <= expected + tolerance))
		printf("# expected %lu.%03lu x %lu positions, measured %lu\n", expected_milli / 1000, expected_milli % 1000,
			reference_positions, positions);
}

static void check_tx(const char *name, uint16_t txclksel, const measurement_t *result) {
	check_rate(name, txclksel == 0 ? 1000 : 3000, result->tx_positions, reference.tx_positions);
}

static void check_rx(const char *name, uint16_t rxclksel, const measurement_t *result) {
	check_rate(name, rxclksel == 0 ? 1500 : 500, result->rx_positions, reference.rx_positions);
}

int main(void) {
	test_start(TEST_TITLE);
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load I2S clock select runner",
		dsp_hw_load_image(DSP_I2S_CLOCK_SELECT_IMAGE, sizeof(DSP_I2S_CLOCK_SELECT_IMAGE)))) {
		return test_finish();
	}
	dsp_hw_shared_memory[0x0000] = 0;
	dsp_hw_shared_memory[0x0001] = 0;
	load_parameters();
	if (!test_check("BRANCH starts I2S clock select runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("I2S clock select runner becomes ready", dsp_hw_wait_shared(0, READY_MARKER, 100)))
		return test_finish();

	test_category("Reference rate");
	if (!test_check("reference measurement completes", measure(&REFERENCE_CONFIGURATION, &reference)))
		return test_finish();
	test_check("the reference transmitter moves", reference.tx_positions > 100);
	test_check("the reference receiver moves", reference.rx_positions > 100);

	static const uint16_t DIVIDER_CLKSELS[] = { 0, 3 };
	static const uint16_t MASTER_CLK_PIN_SOURCES[] = { 0, 1 };

	test_category("TXCLKSEL and RXCLKSEL with both directions running");
	for (size_t tx = 0; tx < ARRAY_SIZE(DIVIDER_CLKSELS); tx++) {
		for (size_t rx = 0; rx < ARRAY_SIZE(DIVIDER_CLKSELS); rx++) {
			uint16_t csel = csel_value(DIVIDER_CLKSELS[tx], 0, DIVIDER_CLKSELS[rx], 0);
			configuration_t configuration = select_configuration(csel, CTRL_BOTH);
			measurement_t result;
			if (!test_check("measurement completes", measure(&configuration, &result)))
				return test_finish();
			check_tx("TXCLKSEL 0 selects divider 0, 3 divider 1", DIVIDER_CLKSELS[tx], &result);
			check_rx("RXCLKSEL 0 selects divider 1, 3 divider 0", DIVIDER_CLKSELS[rx], &result);
		}
	}

	test_category("RXCLKSEL with only the receiver running");
	for (size_t rx = 0; rx < ARRAY_SIZE(DIVIDER_CLKSELS); rx++) {
		configuration_t configuration = select_configuration(csel_value(3, 0, DIVIDER_CLKSELS[rx], 0), CTRL_RX);
		measurement_t result;
		if (!test_check("measurement completes", measure(&configuration, &result)))
			return test_finish();
		test_eq_u32("the stopped transmitter does not move", 0, result.tx_positions);
		check_rx("the receiver runs without the transmitter", DIVIDER_CLKSELS[rx], &result);
	}

	test_category("CLK0SEL and CLK1SEL in master mode");
	for (size_t clk0 = 0; clk0 < ARRAY_SIZE(MASTER_CLK_PIN_SOURCES); clk0++) {
		for (size_t clk1 = 0; clk1 < ARRAY_SIZE(MASTER_CLK_PIN_SOURCES); clk1++) {
			uint16_t csel = csel_value(3, MASTER_CLK_PIN_SOURCES[clk0], 0, MASTER_CLK_PIN_SOURCES[clk1]);
			configuration_t configuration = select_configuration(csel, CTRL_BOTH);
			measurement_t result;
			if (!test_check("measurement completes", measure(&configuration, &result)))
				return test_finish();
			check_tx("the CLK0 pin source does not change the transmit clock", 3, &result);
			check_rx("the CLK1 pin source does not change the receive clock", 0, &result);
		}
	}

	test_category("CLK0 and CLK1 line clocks without an external clock");
	static const uint16_t LINE_CLKSELS[] = { 1, 2 };
	static const uint16_t CLK_LINE_SOURCES[] = { 0, 1, 2 };
	for (size_t line = 0; line < ARRAY_SIZE(LINE_CLKSELS); line++) {
		for (size_t source = 0; source < ARRAY_SIZE(CLK_LINE_SOURCES); source++) {
			uint16_t csel = csel_value(LINE_CLKSELS[line], CLK_LINE_SOURCES[source], LINE_CLKSELS[line],
				CLK_LINE_SOURCES[source]);
			configuration_t configuration = select_configuration(csel, CTRL_BOTH);
			measurement_t result;
			if (!test_check("measurement completes", measure(&configuration, &result)))
				return test_finish();
			test_eq_u32("the transmitter stops after its start load", 4, result.tx_positions);
			test_eq_u32("the receiver does not move", 0, result.rx_positions);
		}
	}

#if I2S_INSTANCE == 2
	test_category("CLK0 line clock with the I2S2 pads routed");
	static const uint32_t PAD_MODES[] = {
		GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_DIR_OUT,
		GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_DIR_IN,
	};
	for (size_t mode = 0; mode < ARRAY_SIZE(PAD_MODES); mode++) {
		GPIO_PIN(GPIO_I2S2_CLK0) = PAD_MODES[mode];
		GPIO_PIN(GPIO_I2S2_WA0) = PAD_MODES[mode];
		for (size_t source = 0; source < ARRAY_SIZE(CLK_LINE_SOURCES); source++) {
			uint16_t csel = csel_value(2, CLK_LINE_SOURCES[source], 1, CLK_LINE_SOURCES[source]);
			configuration_t configuration = select_configuration(csel, CTRL_BOTH);
			measurement_t result;
			if (!test_check("measurement completes", measure(&configuration, &result)))
				return test_finish();
			test_eq_u32("the transmitter stops after its start load", 4, result.tx_positions);
			test_eq_u32("the receiver does not move", 0, result.rx_positions);
		}
	}
#endif

	test_category("Fractional divider, frame period, and word width rates");
	for (size_t i = 0; i < ARRAY_SIZE(RATE_CASES); i++) {
		measurement_t result;
		if (!test_check("measurement completes", measure(&RATE_CASES[i].configuration, &result)))
			return test_finish();
		check_rate(RATE_CASES[i].name, RATE_CASES[i].tx_milli, result.tx_positions, reference.tx_positions);
		check_rate(RATE_CASES[i].name, RATE_CASES[i].rx_milli, result.rx_positions, reference.rx_positions);
	}

	test_category("Interrupt addresses while the interface is off");
	static const uint16_t INTERRUPT_ADDRESSES[] = { TEST_TXINTADDR, TEST_RXINTADDR };
	for (size_t i = 0; i < ARRAY_SIZE(INTERRUPT_ADDRESSES); i++) {
		dsp_hw_shared_memory[0x0048] = INTERRUPT_ADDRESSES[i];
		write_register(TEST_CTRL, 0);
		test_eq_u32("an interrupt address write is ignored while I2SON is clear", 0,
			write_register(INTERRUPT_ADDRESSES[i], 5).readback);
		test_eq_u32("setting I2SON keeps the interrupt address clear", 0,
			write_register(TEST_CTRL, TEST_CTRL_I2SON).readback);
		test_eq_u32("an interrupt address write takes effect while I2SON is set", 7,
			write_register(INTERRUPT_ADDRESSES[i], 7).readback);
		test_eq_u32("clearing I2SON clears the interrupt address", 0, write_register(TEST_CTRL, 0).readback);
	}

	test_category("Transmit start load");
	dsp_hw_shared_memory[0x0048] = TEST_CTRL;
	write_result_t start = start_before_first_frame(TX_PERIOD_32, 32, CTRL_TX);
	test_eq_u32("starting the transmitter loads four words", 0x0004, start.pointer);
	write_result_t stopped = write_register(TEST_CTRL, TEST_CTRL_I2SON);
	write_result_t restarted = write_register(TEST_CTRL, CTRL_TX);
	test_eq_u32("stopping keeps the loaded pointer", 0x0004, stopped.pointer);
	test_eq_u32("restarting does not load more words", 0x0004, restarted.pointer);
	for (uint16_t txintaddr = 0; txintaddr <= 6; txintaddr += 2) {
		start = start_before_first_frame(TX_PERIOD_32, txintaddr, CTRL_TX);
		test_eq_u32("the start load stops at position 4", 0x0004, start.pointer);
		test_eq_u32("each loaded word that reaches TXINTADDR raises the TX interrupt",
			txintaddr == 2 || txintaddr == 4 ? TEST_TX_IRQ : 0, start.tx_irq);
		test_eq_u32("normal mode keeps running at TXINTADDR", CTRL_TX, start.readback);
	}
	start = start_before_first_frame(TX_PERIOD_32 | TX_WIDTH_32, 32, CTRL_TX);
	test_eq_u32("32-bit words load the same four words", 0x0004, start.pointer);
	start = start_before_first_frame(TX_PERIOD_32, 2, CTRL_TX | TEST_CTRL_TXPCM);
	test_eq_u32("PCM mode loads one word", 0x0001, start.pointer);
	test_eq_u32("the PCM start load below TXINTADDR raises no TX interrupt", 0, start.tx_irq);
	start = start_before_first_frame(TX_PERIOD_32, 1, CTRL_TX | TEST_CTRL_TXPCM);
	test_eq_u32("the PCM start load reaching TXINTADDR raises the TX interrupt", TEST_TX_IRQ, start.tx_irq);
	test_eq_u32("the PCM start load reaching TXINTADDR stops the transmitter",
		TEST_CTRL_I2SON | TEST_CTRL_TXPCM, start.readback);
	start = start_before_first_frame(TX_PERIOD_32, 32, CTRL_BOTH);
	test_eq_u32("the receiver loads nothing at start", 0x0004, start.pointer);

	return test_finish();
}
