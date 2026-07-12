#include <pmb887x.h>

#include "test.h"

#define ADC_MIDPOINT 0x800
#define ADC_SIGNED_MAX 0x7FF
#define ADC_REFERENCE_MV 1000
#define LI_ION_MIN_MV 3000
#define LI_ION_MAX_MV 4200

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;
static volatile uint32_t irq_src0;
static volatile uint32_t irq_src1;

static uint32_t read_sample(uint32_t config) {
	ADC_CTRL = ADC_CTRL_ADCON | ADC_CTRL_ENSTOP | config;
	ADC_CTRL |= ADC_CTRL_START;

	stopwatch_t start = stopwatch_get();
	while ((ADC_STAT & ADC_STAT_READY) == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return ADC_DATA(0);
}

static int32_t read_corrected(uint32_t config) {
	int32_t normal = 0;
	int32_t inverted = 0;

	for (uint32_t sample = 0; sample < 10; sample++)
		normal += (int32_t) read_sample(config) - ADC_MIDPOINT;
	for (uint32_t sample = 0; sample < 10; sample++)
		inverted += (int32_t) read_sample(config | ADC_CTRL_INV) - ADC_MIDPOINT;

	return (normal / 10 - inverted / 10) / 2;
}

static int32_t read_battery_mv(void) {
	int32_t adc_mv = read_corrected(ADC_CTRL_MX_M1) * ADC_REFERENCE_MV / ADC_SIGNED_MAX;
	int32_t input_mv = adc_mv * 100 / 44;

	return input_mv + input_mv * 330000 / 220000;
}

static void test_registers(void) {
	test_module_id("module ID", 0xF024C000, ADC_ID);
	test_module_clock("module clock", ADC_CLC);

	ADC_CLK = (4 << ADC_CLK_K_SHIFT) | (13 << ADC_CLK_L_SHIFT);
	test_eq_u32(
		"clock divider readback",
		(4 << ADC_CLK_K_SHIFT) | (13 << ADC_CLK_L_SHIFT),
		ADC_CLK & (ADC_CLK_K | ADC_CLK_L)
	);

	ADC_ANA_CTRL = (
		ADC_ANA_CTRL_BG_PWUP | ADC_ANA_CTRL_TX_DIS | ADC_ANA_CTRL_TXREF_PU |
		ADC_ANA_CTRL_PAOPM1_ENHANCED | (0x15 << ADC_ANA_CTRL_PA_CAL1_SHIFT) | ADC_ANA_CTRL_TREF_1_175V
	);
	test_eq_u32(
		"analog control readback",
		(
			ADC_ANA_CTRL_BG_PWUP | ADC_ANA_CTRL_TX_DIS | ADC_ANA_CTRL_TXREF_PU | ADC_ANA_CTRL_PAOPM1_ENHANCED |
			(0x15 << ADC_ANA_CTRL_PA_CAL1_SHIFT) | ADC_ANA_CTRL_TREF_1_175V
		),
		ADC_ANA_CTRL
	);

	ADC_CTRL = (
		ADC_CTRL_MX_M1 | ADC_CTRL_INV | ADC_CTRL_CSEL | ADC_CTRL_TC_I_120 | ADC_CTRL_FREQ_DIV_80 |
		ADC_CTRL_BUFSIZE_8 | ADC_CTRL_MXREF_M10 | ADC_CTRL_ENSTOP | ADC_CTRL_ADCON
	);
	test_eq_u32(
		"control register readback",
		(
			ADC_CTRL_MX_M1 | ADC_CTRL_INV | ADC_CTRL_CSEL | ADC_CTRL_TC_I_120 | ADC_CTRL_FREQ_DIV_80 |
			ADC_CTRL_BUFSIZE_8 | ADC_CTRL_MXREF_M10 | ADC_CTRL_ENSTOP | ADC_CTRL_ADCON
		),
		ADC_CTRL & ~ADC_CTRL_START
	);
}

static void test_single_shot(void) {
	ADC_CTRL = ADC_CTRL_ADCON;
	test_check("ADC becomes idle after power-up", (ADC_STAT & ADC_STAT_BUSY) == 0);

	ADC_CTRL = ADC_CTRL_ADCON | ADC_CTRL_ENSTOP | ADC_CTRL_MX_M1 | ADC_CTRL_START;
	stopwatch_t start = stopwatch_get();
	while ((ADC_STAT & ADC_STAT_READY) == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();
	test_check("single-shot conversion becomes ready", (ADC_STAT & ADC_STAT_READY) != 0);
	test_check("single-shot conversion is no longer busy", (ADC_STAT & ADC_STAT_BUSY) == 0);
	test_eq_u32("single-shot write pointer is zero", 0, ADC_STAT & ADC_STAT_WPTR);
	uint32_t sample = ADC_DATA(0);
	test_check("single-shot result is 12-bit", sample <= 0xFFF);
	test_check("reading DATA clears READY", (ADC_STAT & ADC_STAT_READY) == 0);
	test_check("single-shot START clears", (ADC_CTRL & ADC_CTRL_START) == 0);
}

static void test_inversion(void) {
	uint32_t normal = read_sample(ADC_CTRL_MX_M1);
	uint32_t inverted = read_sample(ADC_CTRL_MX_M1 | ADC_CTRL_INV);

	test_check("normal M1 sample is above midpoint", normal > ADC_MIDPOINT);
	test_check("inverted M1 sample is below midpoint", inverted < ADC_MIDPOINT);
	test_check(
		"INV samples are symmetric around midpoint",
		test_u32_in_interval(normal + inverted, 0xFF0, 0x1010)
	);
}

static void test_interrupt(void) {
	ADC_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	ADC_SRC(1) = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_ADC_INT0_IRQ) = 1;
	VIC_CON(VIC_ADC_INT1_IRQ) = 1;
	irq_count = 0;
	irq_number = 0;
	irq_src0 = 0;
	irq_src1 = 0;

	cpu_enable_irq(true);
	ADC_CTRL = ADC_CTRL_ADCON | ADC_CTRL_ENSTOP | ADC_CTRL_MX_M1 | ADC_CTRL_START;
	stopwatch_t start = stopwatch_get();
	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();
	cpu_enable_irq(false);

	test_check("single-shot conversion raises ADC IRQ", irq_count != 0);
	test_eq_u32("single-shot conversion IRQ count", 1, irq_count);
	test_eq_u32("RDYIRQ uses ADC INT0", VIC_ADC_INT0_IRQ, irq_number);
	test_check("RDYIRQ sets SRC0 SRR", (irq_src0 & MOD_SRC_SRR) != 0);
	test_check("RDYIRQ leaves SRC1 clear", (irq_src1 & MOD_SRC_SRR) == 0);
	ADC_DATA(0);

	irq_count = 0;
	irq_number = 0;
	ADC_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	ADC_SRC(1) = MOD_SRC_CLRR | MOD_SRC_SRE;
	cpu_enable_irq(true);
	ADC_CTRL = ADC_CTRL_ADCON | ADC_CTRL_ENSTOP | ADC_CTRL_BUFSIZE_2 | ADC_CTRL_MX_M1 | ADC_CTRL_START;
	start = stopwatch_get();
	while ((ADC_STAT & ADC_STAT_READY) == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();
	stopwatch_usleep_wd(1000);
	cpu_enable_irq(false);
	test_check(
		"single-shot with BUFSIZE greater than zero keeps READY clear",
		(ADC_STAT & ADC_STAT_READY) == 0
	);
	test_eq_u32("single-shot with BUFSIZE greater than zero has no IRQ", 0, irq_count);
	test_eq_u32("single-shot ignores BUFSIZE for WPTR", 0, ADC_STAT & ADC_STAT_WPTR);
	test_check("single-shot with BUFSIZE greater than zero writes DATA0", ADC_DATA(0) <= 0xFFF);

	irq_count = 0;
	irq_number = 0;
	irq_src0 = 0;
	irq_src1 = 0;
	ADC_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	ADC_SRC(1) = MOD_SRC_CLRR | MOD_SRC_SRE;
	cpu_enable_irq(true);
	ADC_CTRL = (
		ADC_CTRL_ADCON | ADC_CTRL_ENSTOP | ADC_CTRL_FREQ_DIV_640 | ADC_CTRL_BUFSIZE_4 |
		ADC_CTRL_MX_M1 | ADC_CTRL_START
	);
	start = stopwatch_get();
	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();
	cpu_enable_irq(false);
	test_eq_u32("repetitive linear buffer raises one IRQ", 1, irq_count);
	test_eq_u32("repetitive buffer RDYIRQ uses ADC INT0", VIC_ADC_INT0_IRQ, irq_number);
	test_eq_u32("repetitive buffer stops at configured WPTR", 3, ADC_STAT & ADC_STAT_WPTR);
	test_check("repetitive buffer sets READY", (ADC_STAT & ADC_STAT_READY) != 0);
	test_check("repetitive buffer stops after ENSTOP", (ADC_STAT & ADC_STAT_BUSY) == 0);
	test_check(
		"repetitive RDYIRQ sets SRC0 only",
		(irq_src0 & MOD_SRC_SRR) != 0 && (irq_src1 & MOD_SRC_SRR) == 0
	);
	ADC_DATA(0);

	VIC_CON(VIC_ADC_INT0_IRQ) = 0;
	VIC_CON(VIC_ADC_INT1_IRQ) = 0;
	ADC_SRC(0) = MOD_SRC_CLRR;
	ADC_SRC(1) = MOD_SRC_CLRR;
}

static void test_battery(void) {
	int32_t minimum = read_battery_mv();
	int32_t maximum = minimum;

	for (uint32_t sample = 1; sample < 8; sample++) {
		int32_t battery_mv = read_battery_mv();
		if (battery_mv < minimum)
			minimum = battery_mv;
		if (battery_mv > maximum)
			maximum = battery_mv;
	}

	test_check(
		"M1_VBAT is in 3000..4200 mV Li-ion range",
		minimum >= LI_ION_MIN_MV && maximum <= LI_ION_MAX_MV
	);
	test_check("M1_VBAT is stable", maximum - minimum <= 100);
	printf("# M1_VBAT: %d..%d mV\n", minimum, maximum);
	printf("# Li-ion range: %d..%d mV\n", LI_ION_MIN_MV, LI_ION_MAX_MV);
}

int main(void) {
	test_start("ADC peripheral test");
	ADC_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Registers");
	test_registers();

	ADC_CLK = (4 << ADC_CLK_K_SHIFT) | (13 << ADC_CLK_L_SHIFT);
	ADC_ANA_CTRL = ADC_ANA_CTRL_BG_PWUP;

	test_category("Conversion");
	test_single_shot();
	test_inversion();
	test_interrupt();

	test_category("Battery");
	test_battery();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	irq_src0 = ADC_SRC(0);
	irq_src1 = ADC_SRC(1);
	irq_count++;

	if (irq_number == VIC_ADC_INT0_IRQ)
		ADC_SRC(0) |= MOD_SRC_CLRR;
	else if (irq_number == VIC_ADC_INT1_IRQ)
		ADC_SRC(1) |= MOD_SRC_CLRR;

	VIC_IRQ_ACK = 1;
}
