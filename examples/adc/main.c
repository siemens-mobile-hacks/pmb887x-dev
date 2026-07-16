#include <pmb887x.h>
#include <printf.h>

#define ADC_REF_VOLTAGE		1000
#define ADC_MAX_VALUE		0x7FF

enum adc_result_type_t {
	ADC_READ_VOLTS,
	ADC_READ_RESISTANCE,
	ADC_READ_DEBUG
};

struct adc_channel_t {
	const char *name;
	uint8_t ch;
	uint8_t gain; // 100 - 1.0
	int8_t polarity;
	uint32_t rdiv_r1;
	uint32_t rdiv_r2;
	enum adc_result_type_t result;
	uint8_t current;
};

// ADC channels used in all Siemens SGOLD & SGOLD2
static struct adc_channel_t adc_channels[] = {
#ifdef BOARD_PANASONIC_VS7
	{"M0_BAT_ID",	ADC_CTRL_MX_M0,			100,		1,		0,		0,		ADC_READ_VOLTS,				0},
	{"M10_PCB_ID",	ADC_CTRL_MX_M10,		100,		1,		0,		0,		ADC_READ_RESISTANCE,		60},
#elif defined(BOARD_LG_KE970)
	{"M8_VBAT",		ADC_CTRL_MX_M8,			96,		-1,		390000,	100000,		ADC_READ_VOLTS,				0},
#else
	{"M1_VBAT",		ADC_CTRL_MX_M1,			44,		1,		330000,	220000,		ADC_READ_VOLTS,				0},
	{"M7_AKKU_TYP",	ADC_CTRL_MX_M7,			100,	-1,		0,		0,			ADC_READ_RESISTANCE,		60},
	{"M0_TVCO",		ADC_CTRL_MX_M0,			100,	1,		0,		0,			ADC_READ_RESISTANCE,		60},
	{"M9_BREF",		ADC_CTRL_MX_M9,			50,		-1,		0,		0,			ADC_READ_RESISTANCE,		60},
	{"M0+M9_TVCO",	ADC_CTRL_MX_M0_M9_B,	100,	-1,		0,		0,			ADC_READ_VOLTS,				60},
#endif
};

static int32_t resistor_div_vin(int32_t v, int32_t r1, int32_t r2) {
	return v + (v * r1 / r2);
}

static int32_t adc_to_volts(int32_t v) {
	return (v * ADC_REF_VOLTAGE) / ADC_MAX_VALUE;
}

static int32_t volts_to_resistance(int32_t v, int32_t current) {
	if (current == 0)
		return 0;
	if (v < 0)
		v = -v;
	return v * 1000 / current;
}

static int32_t adc_read(uint32_t config) {
	ADC_CTRL = (ADC_CTRL & ~(ADC_CTRL_MX | ADC_CTRL_CSEL | ADC_CTRL_INV | ADC_CTRL_TC | ADC_CTRL_START | ADC_CTRL_BUFSIZE)) |
		ADC_CTRL_ENSTOP | config;
	ADC_CTRL |= ADC_CTRL_START;
	
	while ((ADC_STAT & ADC_STAT_READY) == 0);
	
	return ADC_DATA(0);
}

static int32_t adc_read_with_correction(uint32_t config) {
	int32_t a = 0;
	for (int i = 0; i < 10; i++)
		a += adc_read(config) - 0x800;
	a = a / 10;
	
	int32_t b = 0;
	for (int i = 0; i < 10; i++)
		b += adc_read(config | ADC_CTRL_INV) - 0x800;
	b = b / 10;
	
	return (a - b) / 2;
}

static void dump_adc_chan(struct adc_channel_t *channel) {
	int32_t adc_value = adc_read_with_correction((channel->ch << ADC_CTRL_MX_SHIFT) | ((channel->current / 30) << ADC_CTRL_TC_SHIFT));
	
	int32_t adc_volts = adc_to_volts(adc_value);
	int32_t real_volts = adc_volts * 100 / channel->gain;
	
	if (channel->rdiv_r1 && channel->rdiv_r2)
		real_volts = resistor_div_vin(real_volts, channel->rdiv_r1, channel->rdiv_r2);
	
	real_volts *= channel->polarity;
	
	if (channel->result == ADC_READ_RESISTANCE) {
		int32_t resistance = volts_to_resistance(real_volts, channel->current);
		printf("  %16s: %5d Ohm | RAW: %5d, %5d mV, %5d mV\n", channel->name, resistance, adc_value, adc_volts, real_volts);
	} else if (channel->result == ADC_READ_VOLTS) {
		 if (real_volts == adc_volts) {
			printf("  %16s: %5d mV\n", channel->name, real_volts);
		} else {
			printf("  %16s: %5d mV  | RAW: %5d, %5d mV\n", channel->name, real_volts, adc_value, adc_volts);
		}
	}
}

static void dump_adc_chan_debug(uint32_t ch) {
	int32_t adc_value_v = adc_read_with_correction((ch << ADC_CTRL_MX_SHIFT));
	int32_t adc_value_i = adc_read_with_correction((ch << ADC_CTRL_MX_SHIFT) | ADC_CTRL_TC_I_30);
	
	if (adc_value_v && adc_value_i) {
		printf("%02X: %5d mV | %5d mV\n", ch, adc_to_volts(adc_value_v), adc_to_volts(adc_value_i));
	}
}

int main(void) {
	wdt_init();
	
	ADC_CLC = (1 << MOD_CLC_RMC_SHIFT);
	
	ADC_CLK = (0x04 << ADC_CLK_K_SHIFT) | (0x0D << ADC_CLK_L_SHIFT);
	ADC_ANA_CTRL = 0x00000002;
	
	ADC_CTRL = ADC_CTRL_ADCON;
	while ((ADC_STAT & ADC_STAT_BUSY) != 0);
	
	printf("Known channels:\n");
	for (uint32_t i = 0; i < ARRAY_SIZE(adc_channels); i++)
		dump_adc_chan(&adc_channels[i]);
	printf("\n");
	
	/*
	printf("Bruteforce:\n");
	for (uint32_t ch = 1; ch <= 0xFF; ch++)
		dump_adc_chan_debug(ch);
	*/
	
	return 0;
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
