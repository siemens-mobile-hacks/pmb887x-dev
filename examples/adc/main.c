#include <pmb887x.h>
#include <printf.h>

#define ADC_OFFSET			2650
#define ADC_REF_VOLTAGE		5000
#define ADC_MAX_VALUE		0xFFF

enum {
	ADC_VBAT = 0,
};

// Used in EL71
static const uint32_t adc_channels[] = {
	0x2,		0x4018,		0x4018,		0x4018,
	0x4008,		0x2008,		0x7008,		0x42,
	0x4058,		0x4058,		0x4058,		0x4048
};

static int32_t resistor_div_vin(int32_t v, int32_t r1, int32_t r2) {
	return v + (v * r1 / r2);
}

static int32_t adc_to_volts(int32_t v) {
	return (v * ADC_REF_VOLTAGE) / ADC_MAX_VALUE - ADC_OFFSET;
}

static int32_t adc_to_resistance(int32_t v) {
	return adc_to_volts(v) * 1000 / 60;
}

static uint32_t adc_read(uint32_t config) {
	AMC_CON1 &= ~(AMC_CON1_CH | AMC_CON1_PA_FAST | AMC_CON1_PA_INV | AMC_CON1_MODE | AMC_CON1_START);
	AMC_CON1 |= AMC_CON1_SINGLE;
	AMC_CON1 |= config;
	AMC_CON1 |= AMC_CON1_START;
	
	while ((AMC_STAT & AMC_STAT_BUSY) != 0);
	
	return AMC_FIFO(0);
}

static void dump_adc_chan(uint32_t config) {
	printf(
		"ch=%02X, inv=%d, cap=%d, mode=%d\n",
		(config & AMC_CON1_CH) >> AMC_CON1_CH_SHIFT,
		(config & AMC_CON1_PA_INV) != 0,
		(config & AMC_CON1_PA_FAST) != 0,
		(config & AMC_CON1_MODE) >> AMC_CON1_MODE_SHIFT
	);
	
	AMC_CON1 &= ~(AMC_CON1_CH | AMC_CON1_PA_FAST | AMC_CON1_PA_INV | AMC_CON1_MODE | AMC_CON1_START);
	AMC_CON1 |= AMC_CON1_SINGLE;
	AMC_CON1 |= config;
	AMC_CON1 |= AMC_CON1_START;
	
	while ((AMC_STAT & AMC_STAT_BUSY) != 0);
	
	uint32_t value = AMC_FIFO(0);
	printf("   ADC value=%02X / %d mV / %d OHm\n", AMC_FIFO(0), adc_to_volts(value), adc_to_resistance(value));
}

int main(void) {
	wdt_init();
	
	AMC_CLC = (1 << MOD_CLC_RMC_SHIFT);
	AMC_CON0 = 2;
	AMC_PLLCON = (0x04 << AMC_PLLCON_K_SHIFT) | (0x0D << AMC_PLLCON_L_SHIFT);
	
	AMC_CON1 |= AMC_CON1_ON;
	while ((AMC_STAT & AMC_STAT_BUSY) != 0);
	
	uint32_t vbat = adc_read(adc_channels[0]);
	printf("Vbat: %d mV (%d mV / %03X)\n", resistor_div_vin(adc_to_volts(vbat), 330000, 220000), adc_to_volts(vbat), vbat);
	
	for (uint32_t i = 0; i < ARRAY_SIZE(adc_channels); i++)
		dump_adc_chan(adc_channels[i]);
	
	while (1);
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}
