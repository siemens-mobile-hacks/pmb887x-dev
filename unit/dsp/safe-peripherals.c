#include <pmb887x.h>
#include <gen/dsp.h>

#include "test.h"

#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_DREAD 4
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_RESULT_OFFSET (DSP_BOOT_DATA_OFFSET + 3)
#define DSP_WAIT_ITERATIONS 1000000
#define RESET_REGISTER(name, address, value) { name, address, value, UINT16_MAX }
#define RESET_REGISTER_MASKED(name, address, value, mask) { name, address, value, mask }

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

struct dsp_reset_register {
	const char *name;
	uint16_t address;
	uint16_t expected;
	uint16_t mask;
};

struct dsp_i2s_registers {
	uint16_t ctrl;
	uint16_t ctrl_on;
	uint16_t csel;
	uint16_t num0;
	uint16_t den0;
	uint16_t num1;
	uint16_t den1;
	uint16_t rxconf;
	uint16_t rxintaddr;
	uint16_t txconf;
	uint16_t txintaddr;
};

// Expected reset values were captured twice from the connected EL71 hardware.
static const struct dsp_reset_register INT_RESET_REGISTERS[] = {
	RESET_REGISTER("FINTA0", TEAK_INT_FINTA0, 0x0000),
	RESET_REGISTER("EINTA0", TEAK_INT_EINTA0, 0x0000),
	RESET_REGISTER("FINTB0", TEAK_INT_FINTB0, 0x0000),
	RESET_REGISTER("EINTB0", TEAK_INT_EINTB0, 0x0000),
	RESET_REGISTER("FINT1", TEAK_INT_FINT1, 0x0000),
	RESET_REGISTER("EINT1", TEAK_INT_EINT1, 0x0000),
	RESET_REGISTER("FINT2", TEAK_INT_FINT2, 0x0000),
	RESET_REGISTER("EINT2", TEAK_INT_EINT2, 0x0000),
};

static const struct dsp_reset_register CIPHER_RESET_REGISTERS[] = {
	RESET_REGISTER("CSTAT", TEAK_CIPH_CSTAT, 0x0000),
	RESET_REGISTER("KEY0", TEAK_CIPH_KEY0, 0x0000),
	RESET_REGISTER("KEY1", TEAK_CIPH_KEY1, 0x0000),
	RESET_REGISTER("KEY2", TEAK_CIPH_KEY2, 0x0000),
	RESET_REGISTER("KEY3", TEAK_CIPH_KEY3, 0x0000),
	RESET_REGISTER("TMOD26", TEAK_CIPH_TMOD26, 0x0000),
	RESET_REGISTER("TMOD51", TEAK_CIPH_TMOD51, 0x0000),
	RESET_REGISTER("SFNUM", TEAK_CIPH_SFNUM, 0x0000),
	RESET_REGISTER("KEY4", TEAK_CIPH_KEY4, 0x0000),
	RESET_REGISTER("KEY5", TEAK_CIPH_KEY5, 0x0000),
	RESET_REGISTER("KEY6", TEAK_CIPH_KEY6, 0x0000),
	RESET_REGISTER("KEY7", TEAK_CIPH_KEY7, 0x0000),
	RESET_REGISTER("KDATA1", TEAK_CIPH_KDATA1, 0x0000),
	RESET_REGISTER("KDATA2", TEAK_CIPH_KDATA2, 0x0000),
	RESET_REGISTER("KDATA3", TEAK_CIPH_KDATA3, 0x0000),
	RESET_REGISTER("KDATA4", TEAK_CIPH_KDATA4, 0x0000),
};

static const struct dsp_reset_register TIMER1_RESET_REGISTERS[] = {
	RESET_REGISTER("CTRL", TEAK_TMR1_CTRL, 0x0000),
	RESET_REGISTER("CNT", TEAK_TMR1_CNT, 0x0000),
	RESET_REGISTER("INT0", TEAK_TMR1_INT0, 0x0FFF),
	RESET_REGISTER("INT1", TEAK_TMR1_INT1, 0x0FFF),
};

static const struct dsp_reset_register TIMER2_RESET_REGISTERS[] = {
	RESET_REGISTER("CTRL", TEAK_TMR2_CTRL, 0x0000),
	RESET_REGISTER("CNT", TEAK_TMR2_CNT, 0x0000),
	RESET_REGISTER("MAX", TEAK_TMR2_MAX, 0xFFFF),
};

static const struct dsp_reset_register EQUALIZER_RESET_REGISTERS[] = {
	RESET_REGISTER("CONF2", TEAK_EQ_CONF2, 0x0000),
	RESET_REGISTER("STATUS", TEAK_EQ_STATUS, 0x0000),
	RESET_REGISTER("CONF_CNT", TEAK_EQ_CONF_CNT, 0x0000),
	RESET_REGISTER("STAT_CNT", TEAK_EQ_STAT_CNT, 0x0000),
	RESET_REGISTER("SC_SOUT", TEAK_EQ_SC_SOUT, 0x0000),
	RESET_REGISTER("SQUAL", TEAK_EQ_SQUAL, 0x0000),
};

static const struct dsp_reset_register CHANNEL_DECODER_RESET_REGISTERS[] = {
	RESET_REGISTER("CONF2", TEAK_CHDEC_CONF2, 0x0000),
	RESET_REGISTER("STATUS", TEAK_CHDEC_STATUS, 0x0000),
	RESET_REGISTER("CONF_CNT", TEAK_CHDEC_CONF_CNT, 0x0000),
	RESET_REGISTER("STAT_CNT", TEAK_CHDEC_STAT_CNT, 0x0000),
	RESET_REGISTER("REF_BR_BFLY0", TEAK_CHDEC_REF_BR_BFLY0, 0x0000),
	RESET_REGISTER("REF_BR_BFLY1", TEAK_CHDEC_REF_BR_BFLY1, 0x0000),
	RESET_REGISTER("REF_BR_BFLY2", TEAK_CHDEC_REF_BR_BFLY2, 0x0000),
	RESET_REGISTER("REF_BR_BFLY3", TEAK_CHDEC_REF_BR_BFLY3, 0x0000),
	RESET_REGISTER("REF_BR_BFLY4", TEAK_CHDEC_REF_BR_BFLY4, 0x0000),
	RESET_REGISTER("REF_BR_BFLY5", TEAK_CHDEC_REF_BR_BFLY5, 0x0000),
	RESET_REGISTER("REF_BR_BFLY6", TEAK_CHDEC_REF_BR_BFLY6, 0x0000),
	RESET_REGISTER("REF_BR_BFLY7", TEAK_CHDEC_REF_BR_BFLY7, 0x0000),
};

static const struct dsp_reset_register AFE_RESET_REGISTERS[] = {
	RESET_REGISTER("RWADDR", TEAK_AFE_RWADDR, 0x0000),
	RESET_REGISTER("BCON", TEAK_AFE_BCON, 0x0000),
	RESET_REGISTER("VRXCTRL1", TEAK_AFE_VRXCTRL1, 0x0000),
	RESET_REGISTER("VRXCTRL2", TEAK_AFE_VRXCTRL2, 0x0000),
	RESET_REGISTER("VTXCTRL", TEAK_AFE_VTXCTRL, 0x0000),
	RESET_REGISTER("RINGCTRL", TEAK_AFE_RINGCTRL, 0x0000),
};

static const struct dsp_reset_register BASEBAND_RESET_REGISTERS[] = {
	RESET_REGISTER("CTRL", TEAK_BB_CTRL, 0x0110),
	RESET_REGISTER("INT_POINTER", TEAK_BB_INT_POINTER, 0x0000),
	RESET_REGISTER("WR_POINTER", TEAK_BB_WR_POINTER, 0x0000),
	RESET_REGISTER("STATUS", TEAK_BB_STATUS, 0x0000),
	RESET_REGISTER("DCOFFSET_I", TEAK_BB_DCOFFSET_I, 0x0000),
	RESET_REGISTER("DCOFFSET_Q", TEAK_BB_DCOFFSET_Q, 0x0000),
	RESET_REGISTER("FSHIFT", TEAK_BB_FSHIFT, 0x0000),
	RESET_REGISTER("BRFILTER_CTRL", TEAK_BB_BRFILTER_CTRL, 0x0000),
	RESET_REGISTER("PBASE_MSB", TEAK_BB_PBASE_MSB, 0x0000),
	RESET_REGISTER("PBASE_LSB", TEAK_BB_PBASE_LSB, 0x0000),
	RESET_REGISTER("PADJ_MSB", TEAK_BB_PADJ_MSB, 0x0000),
	RESET_REGISTER("PADJ_LSB", TEAK_BB_PADJ_LSB, 0x0000),
	RESET_REGISTER("IQ_IMBALANCE", TEAK_BB_IQ_IMBALANCE, 0x0000),
};

static const struct dsp_reset_register MCS_RESET_REGISTERS[] = {
	// DREAD raises CF0 before the DSP can return CFSTA, so the transport-owned bit is excluded.
	RESET_REGISTER_MASKED("CFSTA", TEAK_MCS_CFSTA, 0x0000, 0xFFFE),
	RESET_REGISTER("MCU_SEM", TEAK_MCS_MCU_SEM, 0xFFFF),
};

static const struct dsp_reset_register DSP_RESET_REGISTERS[] = {
	RESET_REGISTER("ID", TEAK_DSP_ID, 0xE101),
	RESET_REGISTER("DEBUG", TEAK_DSP_DEBUG, 0x0000),
	RESET_REGISTER("PAGE", TEAK_DSP_PAGE, 0x0000),
	RESET_REGISTER("DSPOUT", TEAK_DSP_DSPOUT, 0x0000),
};

static const struct dsp_reset_register MODULATOR_RESET_REGISTERS[] = {
	RESET_REGISTER("CTRL", TEAK_MOD_CTRL, 0x0600),
	RESET_REGISTER("STAT", TEAK_MOD_STAT, 0x0000),
	RESET_REGISTER("INT_ADDR", TEAK_MOD_INT_ADDR, 0x0000),
	RESET_REGISTER("OCI", TEAK_MOD_OCI, 0x0000),
	RESET_REGISTER("OCQ", TEAK_MOD_OCQ, 0x0000),
	RESET_REGISTER("ACI", TEAK_MOD_ACI, 0x00FF),
	RESET_REGISTER("ACQ", TEAK_MOD_ACQ, 0x00FF),
	RESET_REGISTER("FC", TEAK_MOD_FC, 0x0000),
};

static const struct dsp_reset_register SSC_RESET_REGISTERS[] = {
	RESET_REGISTER("CON", TEAK_SSC_CON, 0x0000),
	RESET_REGISTER("RXFCON", TEAK_SSC_RXFCON, 0x0100),
	RESET_REGISTER("TXFCON", TEAK_SSC_TXFCON, 0x0100),
	RESET_REGISTER("FSTAT", TEAK_SSC_FSTAT, 0x0000),
	RESET_REGISTER("BR", TEAK_SSC_BR, 0x0000),
	RESET_REGISTER("FDV", TEAK_SSC_FDV, 0x0000),
};

#define I2S_RESET_REGISTERS(instance) \
	RESET_REGISTER("CTRL", TEAK_##instance##_CTRL, 0x0000), \
	RESET_REGISTER("CSEL", TEAK_##instance##_CSEL, 0x0000), \
	RESET_REGISTER("RWADDR", TEAK_##instance##_RWADDR, 0x0000), \
	RESET_REGISTER("NUM0", TEAK_##instance##_NUM0, 0x0001), \
	RESET_REGISTER("DEN0", TEAK_##instance##_DEN0, 0x0002), \
	RESET_REGISTER("NUM1", TEAK_##instance##_NUM1, 0x0001), \
	RESET_REGISTER("DEN1", TEAK_##instance##_DEN1, 0x0002), \
	RESET_REGISTER("RXCONF", TEAK_##instance##_RXCONF, 0x0000), \
	RESET_REGISTER("RXINTADDR", TEAK_##instance##_RXINTADDR, 0x0000), \
	RESET_REGISTER("TXCONF", TEAK_##instance##_TXCONF, 0x0000), \
	RESET_REGISTER("TXINTADDR", TEAK_##instance##_TXINTADDR, 0x0000)

static const struct dsp_reset_register I2S1_RESET_REGISTERS[] = {
	I2S_RESET_REGISTERS(I2S1),
};

static const struct dsp_reset_register I2S2_RESET_REGISTERS[] = {
	I2S_RESET_REGISTERS(I2S2),
};

static const struct dsp_reset_register I2S3_RESET_REGISTERS[] = {
	RESET_REGISTER("CTRL", TEAK_I2S3_CTRL, 0x0000),
	RESET_REGISTER("CSEL", TEAK_I2S3_CSEL, 0x0000),
	RESET_REGISTER("RADDR", TEAK_I2S3_RADDR, 0x0000),
	RESET_REGISTER("NUM", TEAK_I2S3_NUM, 0x0001),
	RESET_REGISTER("DEN", TEAK_I2S3_DEN, 0x0002),
	RESET_REGISTER("TXCONF", TEAK_I2S3_TXCONF, 0x0000),
	RESET_REGISTER("TXINTADDR", TEAK_I2S3_TXINTADDR, 0x0000),
};

static const struct dsp_i2s_registers I2S1_REGISTERS = {
	.ctrl = TEAK_I2S1_CTRL,
	.ctrl_on = TEAK_I2S1_CTRL_I2SON,
	.csel = TEAK_I2S1_CSEL,
	.num0 = TEAK_I2S1_NUM0,
	.den0 = TEAK_I2S1_DEN0,
	.num1 = TEAK_I2S1_NUM1,
	.den1 = TEAK_I2S1_DEN1,
	.rxconf = TEAK_I2S1_RXCONF,
	.rxintaddr = TEAK_I2S1_RXINTADDR,
	.txconf = TEAK_I2S1_TXCONF,
	.txintaddr = TEAK_I2S1_TXINTADDR,
};

static const struct dsp_i2s_registers I2S2_REGISTERS = {
	.ctrl = TEAK_I2S2_CTRL,
	.ctrl_on = TEAK_I2S2_CTRL_I2SON,
	.csel = TEAK_I2S2_CSEL,
	.num0 = TEAK_I2S2_NUM0,
	.den0 = TEAK_I2S2_DEN0,
	.num1 = TEAK_I2S2_NUM1,
	.den1 = TEAK_I2S2_DEN1,
	.rxconf = TEAK_I2S2_RXCONF,
	.rxintaddr = TEAK_I2S2_RXINTADDR,
	.txconf = TEAK_I2S2_TXCONF,
	.txintaddr = TEAK_I2S2_TXINTADDR,
};

static bool wait_for_boot_ready(void) {
	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool dsp_reset(void) {
	DSP_COM_CLEAR = UINT16_MAX;
	SCU_DSP_INT = 0;
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	uint32_t reset_readback = SCU_RST_REQ;
	SCU_RST_REQ = 0;
	(void) reset_readback;

	return wait_for_boot_ready();
}

static bool submit_boot_command(void) {
	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;

	return wait_for_boot_ready();
}

static bool dsp_read_reg(uint16_t address, uint16_t *value) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_DREAD;
	boot_data[1] = address;
	boot_data[2] = 1;
	if (!submit_boot_command())
		return false;

	*value = DSP_SHARED_MEMORY[DSP_BOOT_RESULT_OFFSET];
	return true;
}

static bool dsp_write_reg(uint16_t address, uint16_t value) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_DLOAD;
	boot_data[1] = address;
	boot_data[2] = 1;
	boot_data[3] = value;

	return submit_boot_command();
}

static bool capture_reset_value(uint16_t address, uint16_t *value) {
	return dsp_reset() && dsp_read_reg(address, value);
}

static void test_reset_registers(const char *module, const struct dsp_reset_register *registers, size_t count) {
	char category[48];

	tfp_sprintf(category, "%s / Reset values", module);
	test_category(category);
	for (size_t i = 0; i < count; i++) {
		const struct dsp_reset_register *reg = &registers[i];
		uint16_t first = 0xDEAD;
		uint16_t second = 0xDEAD;
		char name[80];

		tfp_sprintf(name, "%s.%s first reset capture completes", module, reg->name);
		bool first_completed = test_check(name, capture_reset_value(reg->address, &first));
		tfp_sprintf(name, "%s.%s second reset capture completes", module, reg->name);
		bool second_completed = test_check(name, capture_reset_value(reg->address, &second));
		printf("# TEAKRESET,%s,%s,%04X,%04X,%04X\n", module, reg->name, (uint32_t) reg->address,
			(uint32_t) first, (uint32_t) second);
		if (!first_completed || !second_completed)
			continue;

		tfp_sprintf(name, "%s.%s reset value is deterministic", module, reg->name);
		test_eq_u32(name, first, second);
		tfp_sprintf(name, "%s.%s reset value", module, reg->name);
		test_eq_u32(name, reg->expected & reg->mask, first & reg->mask);
	}
}

static bool prepare_behavior(const char *module) {
	char category[48];
	char name[64];

	tfp_sprintf(category, "%s / Behavior", module);
	test_category(category);
	tfp_sprintf(name, "%s reset before behavior", module);
	return test_check(name, dsp_reset());
}

static bool write_checked(const char *name, uint16_t address, uint16_t value) {
	return test_check(name, dsp_write_reg(address, value));
}

static bool read_checked(const char *name, uint16_t address, uint16_t *value) {
	return test_check(name, dsp_read_reg(address, value));
}

static void test_readback(const char *name, uint16_t address, uint16_t value, uint16_t mask) {
	char operation[80];
	uint16_t actual = 0;

	tfp_sprintf(operation, "%s write completes", name);
	if (!write_checked(operation, address, value))
		return;
	tfp_sprintf(operation, "%s read completes", name);
	if (!read_checked(operation, address, &actual))
		return;
	tfp_sprintf(operation, "%s reads back", name);
	test_eq_u32(operation, value & mask, actual & mask);
}

static void test_interrupt(void) {
	test_reset_registers("INT", INT_RESET_REGISTERS, ARRAY_SIZE(INT_RESET_REGISTERS));
	if (!prepare_behavior("INT"))
		return;

	test_readback("INT.EINT2", TEAK_INT_EINT2, 0xA55A, UINT16_MAX);
	if (!write_checked("INT.EINT2 clear completes", TEAK_INT_EINT2, 0))
		return;
	if (!write_checked("INT.SINT2 sets firmware flags", TEAK_INT_SINT2, TEAK_INT_SINT2_SFW5 | TEAK_INT_SINT2_SFW12))
		return;
	uint16_t flags;
	if (!read_checked("INT.FINT2 set flags read completes", TEAK_INT_FINT2, &flags))
		return;
	test_eq_u32("INT.SINT2 has write-one-to-set semantics", TEAK_INT_FINT2_FW5 | TEAK_INT_FINT2_FW12, flags);
	if (!write_checked("INT.RINT2 clears one firmware flag", TEAK_INT_RINT2, TEAK_INT_RINT2_RFW5))
		return;
	if (!read_checked("INT.FINT2 cleared flags read completes", TEAK_INT_FINT2, &flags))
		return;
	test_eq_u32("INT.RINT2 has write-one-to-clear semantics", TEAK_INT_FINT2_FW12, flags);
	(void) write_checked("INT.RINT2 cleanup completes", TEAK_INT_RINT2, UINT16_MAX);
}

static void test_cipher(void) {
	test_reset_registers("CIPHER", CIPHER_RESET_REGISTERS, ARRAY_SIZE(CIPHER_RESET_REGISTERS));
	if (!prepare_behavior("CIPHER"))
		return;

	test_readback("CIPHER.KEY0", TEAK_CIPH_KEY0, 0xA55A, TEAK_CIPH_KEY0_VALUE);
	if (!write_checked("CIPHER A5/3 selection completes", TEAK_CIPH_CSTAT, TEAK_CIPH_CSTAT_A53))
		return;
	test_readback("CIPHER.KEY7", TEAK_CIPH_KEY7, 0x5AA5, TEAK_CIPH_KEY7_VALUE);
	if (!test_check("CIPHER reset between A5/3 and A5/2 completes", dsp_reset()))
		return;
	test_readback("CIPHER.TMOD26", TEAK_CIPH_TMOD26, 0xFFFF, TEAK_CIPH_TMOD26_T26N);
	test_readback("CIPHER.TMOD51", TEAK_CIPH_TMOD51, 0xFFFF, TEAK_CIPH_TMOD51_T51N);
	test_readback("CIPHER.SFNUM", TEAK_CIPH_SFNUM, 0xFFFF, TEAK_CIPH_SFNUM_SFN);
	test_readback("CIPHER.CSTAT mode", TEAK_CIPH_CSTAT, TEAK_CIPH_CSTAT_A52 | TEAK_CIPH_CSTAT_EDGE,
		TEAK_CIPH_CSTAT_A52 | TEAK_CIPH_CSTAT_EDGE);
}

static void test_timer1(void) {
	test_reset_registers("TIMER1", TIMER1_RESET_REGISTERS, ARRAY_SIZE(TIMER1_RESET_REGISTERS));
	if (!prepare_behavior("TIMER1"))
		return;

	test_readback("TIMER1.INT0", TEAK_TMR1_INT0, 0xA55A, TEAK_TMR1_INT0_T1INT0);
	test_readback("TIMER1.INT1", TEAK_TMR1_INT1, 0x5AA5, TEAK_TMR1_INT1_T1INT1);
	if (!write_checked("TIMER1 enable completes", TEAK_TMR1_CTRL, TEAK_TMR1_CTRL_DT1ENA))
		return;
	if (!write_checked("TIMER1 restart completes", TEAK_TMR1_CTRL, TEAK_TMR1_CTRL_DT1ENA | TEAK_TMR1_CTRL_RESTART))
		return;
	uint16_t first;
	uint16_t second;
	if (!read_checked("TIMER1 first counter read completes", TEAK_TMR1_CNT, &first))
		return;
	if (!read_checked("TIMER1 second counter read completes", TEAK_TMR1_CNT, &second))
		return;
	test_check("TIMER1 counter advances after restart", first != second);
	(void) write_checked("TIMER1 stop completes", TEAK_TMR1_CTRL, 0);
}

static void test_timer2(void) {
	test_reset_registers("TIMER2", TIMER2_RESET_REGISTERS, ARRAY_SIZE(TIMER2_RESET_REGISTERS));
	if (!prepare_behavior("TIMER2"))
		return;

	test_readback("TIMER2.MAX", TEAK_TMR2_MAX, 0xF123, TEAK_TMR2_MAX_T2MAX);
	if (!write_checked("TIMER2 start value write completes", TEAK_TMR2_CNT, 0x1000))
		return;
	if (!write_checked("TIMER2 enable completes", TEAK_TMR2_CTRL, TEAK_TMR2_CTRL_DT2ACT))
		return;
	uint16_t first;
	uint16_t second;
	if (!read_checked("TIMER2 first counter read completes", TEAK_TMR2_CNT, &first))
		return;
	if (!read_checked("TIMER2 second counter read completes", TEAK_TMR2_CNT, &second))
		return;
	test_check("TIMER2 counter advances while active", first != second);
	(void) write_checked("TIMER2 stop completes", TEAK_TMR2_CTRL, 0);
}

static void test_equalizer(void) {
	test_reset_registers("EQUALIZER", EQUALIZER_RESET_REGISTERS, ARRAY_SIZE(EQUALIZER_RESET_REGISTERS));
	if (!prepare_behavior("EQUALIZER"))
		return;

	if (!write_checked("EQUALIZER hardware enable completes", TEAK_EQ_CONF2, TEAK_EQ_CONF2_HW_ENA_EQ))
		return;
	test_readback("EQUALIZER.CONF_CNT", TEAK_EQ_CONF_CNT, 0x35, TEAK_EQ_CONF_CNT_C_EQ);
	test_readback("EQUALIZER.SC_SOUT", TEAK_EQ_SC_SOUT, 0xA55A, UINT16_MAX);
	(void) write_checked("EQUALIZER hardware disable completes", TEAK_EQ_CONF2, 0);
}

static void test_channel_decoder(void) {
	test_reset_registers("CHANNEL_DECODER", CHANNEL_DECODER_RESET_REGISTERS,
		ARRAY_SIZE(CHANNEL_DECODER_RESET_REGISTERS));
	if (!prepare_behavior("CHANNEL_DECODER"))
		return;

	if (!write_checked("CHANNEL_DECODER hardware enable completes", TEAK_CHDEC_CONF2, TEAK_CHDEC_CONF2_HW_ENA_DEC))
		return;
	test_readback("CHANNEL_DECODER.CONF_CNT", TEAK_CHDEC_CONF_CNT, 0xA5, TEAK_CHDEC_CONF_CNT_C_DEC);
	test_readback("CHANNEL_DECODER.REF_BR_BFLY0", TEAK_CHDEC_REF_BR_BFLY0, 0x3210, UINT16_MAX);
	test_readback("CHANNEL_DECODER.REF_BR_BFLY1", TEAK_CHDEC_REF_BR_BFLY1, 0x7654, UINT16_MAX);
	test_readback("CHANNEL_DECODER.REF_BR_BFLY2", TEAK_CHDEC_REF_BR_BFLY2, 0xBA98, UINT16_MAX);
	test_readback("CHANNEL_DECODER.REF_BR_BFLY3", TEAK_CHDEC_REF_BR_BFLY3, 0xFEDC, UINT16_MAX);
	test_readback("CHANNEL_DECODER.REF_BR_BFLY4", TEAK_CHDEC_REF_BR_BFLY4, 0x1357, UINT16_MAX);
	test_readback("CHANNEL_DECODER.REF_BR_BFLY5", TEAK_CHDEC_REF_BR_BFLY5, 0x2468, UINT16_MAX);
	test_readback("CHANNEL_DECODER.REF_BR_BFLY6", TEAK_CHDEC_REF_BR_BFLY6, 0xA55A, UINT16_MAX);
	test_readback("CHANNEL_DECODER.REF_BR_BFLY7", TEAK_CHDEC_REF_BR_BFLY7, 0x5AA5, UINT16_MAX);
	(void) write_checked("CHANNEL_DECODER hardware disable completes", TEAK_CHDEC_CONF2, 0);
}

static void test_afe(void) {
	test_reset_registers("AFE", AFE_RESET_REGISTERS, ARRAY_SIZE(AFE_RESET_REGISTERS));
	if (!prepare_behavior("AFE"))
		return;

	uint16_t rates = TEAK_AFE_BCON_RXRATE_KHZ_44_444 | TEAK_AFE_BCON_TXRATE_KHZ_16;
	test_readback("AFE rates while stopped", TEAK_AFE_BCON, rates, TEAK_AFE_BCON_RXRATE | TEAK_AFE_BCON_TXRATE);
	test_skip("AFE analog controls", "power and routing behavior is not safe without board-specific audio isolation");
}

static void test_baseband(void) {
	test_reset_registers("BASEBAND", BASEBAND_RESET_REGISTERS, ARRAY_SIZE(BASEBAND_RESET_REGISTERS));
	if (!prepare_behavior("BASEBAND"))
		return;

	test_readback("BASEBAND.DCOFFSET_I", TEAK_BB_DCOFFSET_I, 0xA55A, TEAK_BB_DCOFFSET_I_VALUE);
	test_readback("BASEBAND.DCOFFSET_Q", TEAK_BB_DCOFFSET_Q, 0x5AA5, TEAK_BB_DCOFFSET_Q_VALUE);
	test_readback("BASEBAND.FSHIFT", TEAK_BB_FSHIFT, 0xC33C, TEAK_BB_FSHIFT_VALUE);
	test_readback("BASEBAND.BRFILTER_CTRL", TEAK_BB_BRFILTER_CTRL, 0x1A55,
		TEAK_BB_BRFILTER_CTRL_LENGTH | TEAK_BB_BRFILTER_CTRL_SCALING | TEAK_BB_BRFILTER_CTRL_DECIMATION);
}

static void test_mcs(void) {
	test_reset_registers("MCS", MCS_RESET_REGISTERS, ARRAY_SIZE(MCS_RESET_REGISTERS));
	if (!prepare_behavior("MCS"))
		return;

	if (!write_checked("MCS.CFSET flag 15 completes", TEAK_MCS_CFSET, TEAK_MCS_CFSET_CF15))
		return;
	uint16_t flags;
	if (!read_checked("MCS.CFSTA set flag read completes", TEAK_MCS_CFSTA, &flags))
		return;
	test_eq_u32("MCS.CFSET has write-one-to-set semantics", TEAK_MCS_CFSTA_CF15,
		flags & ~TEAK_MCS_CFSTA_CF0);
	if (!write_checked("MCS.CFR flag 15 completes", TEAK_MCS_CFR, TEAK_MCS_CFR_CF15))
		return;
	if (!read_checked("MCS.CFSTA cleared flag read completes", TEAK_MCS_CFSTA, &flags))
		return;
	test_eq_u32("MCS.CFR has write-one-to-clear semantics", 0, flags & ~TEAK_MCS_CFSTA_CF0);
}

static void test_dsp_control(void) {
	test_reset_registers("DSP", DSP_RESET_REGISTERS, ARRAY_SIZE(DSP_RESET_REGISTERS));
	if (!prepare_behavior("DSP"))
		return;

	test_skip("DSP.CTRL behavior", "DSPDIS stops the Mask ROM command transport used by this test");
	test_skip("DSP.PAGE behavior", "changing the active Mask ROM page would invalidate the command transport");
}

static void test_modulator(void) {
	test_reset_registers("MODULATOR", MODULATOR_RESET_REGISTERS, ARRAY_SIZE(MODULATOR_RESET_REGISTERS));
	if (!prepare_behavior("MODULATOR"))
		return;

	test_readback("MODULATOR.INT_ADDR", TEAK_MOD_INT_ADDR, 0xFFFF, TEAK_MOD_INT_ADDR_MINT_ADDR);
	test_readback("MODULATOR.OCI", TEAK_MOD_OCI, 0xA55A, TEAK_MOD_OCI_VALUE);
	test_readback("MODULATOR.OCQ", TEAK_MOD_OCQ, 0x5AA5, TEAK_MOD_OCQ_VALUE);
	test_readback("MODULATOR.ACI", TEAK_MOD_ACI, 0xA5, TEAK_MOD_ACI_VALUE);
	test_readback("MODULATOR.ACQ", TEAK_MOD_ACQ, 0x5A, TEAK_MOD_ACQ_VALUE);
	test_readback("MODULATOR.FC", TEAK_MOD_FC, 0xC33C, TEAK_MOD_FC_VALUE);
}

static void test_ssc(void) {
	test_reset_registers("SSC", SSC_RESET_REGISTERS, ARRAY_SIZE(SSC_RESET_REGISTERS));
	if (!prepare_behavior("SSC"))
		return;

	if (!write_checked("SSC clock enable completes", TEAK_SSC_CON, TEAK_SSC_CON_CLKON))
		return;
	uint16_t config = 0xB | TEAK_SSC_CON_HB | TEAK_SSC_CON_PH | TEAK_SSC_CON_PO | TEAK_SSC_CON_LB |
		TEAK_SSC_CON_CLKON;
	test_readback("SSC.CON programming fields", TEAK_SSC_CON, config,
		TEAK_SSC_CON_BM | TEAK_SSC_CON_HB | TEAK_SSC_CON_PH | TEAK_SSC_CON_PO | TEAK_SSC_CON_LB |
		TEAK_SSC_CON_CLKON);
	test_readback("SSC.BR", TEAK_SSC_BR, 0xA55A, TEAK_SSC_BR_VALUE);
	test_readback("SSC.FDV", TEAK_SSC_FDV, 0xFFFF, TEAK_SSC_FDV_VALUE);
	(void) write_checked("SSC clock disable completes", TEAK_SSC_CON, 0);
}

static void test_i2s(const char *module, const struct dsp_reset_register *reset_registers, size_t reset_count,
	const struct dsp_i2s_registers *registers)
{
	test_reset_registers(module, reset_registers, reset_count);
	if (!prepare_behavior(module))
		return;

	if (!write_checked("I2S clock enable completes", registers->ctrl, registers->ctrl_on))
		return;
	test_readback("I2S.CSEL", registers->csel, 0x00A5, 0x00FF);
	test_readback("I2S.NUM0", registers->num0, 0x1455, 0x37FF);
	test_readback("I2S.DEN0", registers->den0, 0xA55A, UINT16_MAX);
	test_readback("I2S.NUM1", registers->num1, 0x1233, 0x37FF);
	test_readback("I2S.DEN1", registers->den1, 0x5AA5, UINT16_MAX);
	test_readback("I2S.RXCONF", registers->rxconf, 0xE1D5, 0xE1FF);
	test_readback("I2S.RXINTADDR", registers->rxintaddr, 0xFFFF, 0x003F);
	test_readback("I2S.TXCONF", registers->txconf, 0xFFB5, UINT16_MAX);
	test_readback("I2S.TXINTADDR", registers->txintaddr, 0xFFFF, 0x003F);
	(void) write_checked("I2S clock disable completes", registers->ctrl, 0);
}

static void test_i2s1(void) {
	test_i2s("I2S1", I2S1_RESET_REGISTERS, ARRAY_SIZE(I2S1_RESET_REGISTERS), &I2S1_REGISTERS);
}

static void test_i2s2(void) {
	test_i2s("I2S2", I2S2_RESET_REGISTERS, ARRAY_SIZE(I2S2_RESET_REGISTERS), &I2S2_REGISTERS);
}

static void test_i2s3(void) {
	test_reset_registers("I2S3", I2S3_RESET_REGISTERS, ARRAY_SIZE(I2S3_RESET_REGISTERS));
	if (!prepare_behavior("I2S3"))
		return;

	if (!write_checked("I2S3 clock enable completes", TEAK_I2S3_CTRL, TEAK_I2S3_CTRL_I2SON))
		return;
	test_readback("I2S3.CSEL", TEAK_I2S3_CSEL, 0x000A, TEAK_I2S3_CSEL_TXCLKSEL | TEAK_I2S3_CSEL_CLKSEL);
	test_readback("I2S3.NUM", TEAK_I2S3_NUM, 0x1455, TEAK_I2S3_NUM_NUMERATOR | TEAK_I2S3_NUM_FREF);
	test_readback("I2S3.DEN", TEAK_I2S3_DEN, 0xA55A, TEAK_I2S3_DEN_DENOMINATOR);
	test_readback("I2S3.TXCONF", TEAK_I2S3_TXCONF, 0xFFB5, UINT16_MAX);
	test_readback("I2S3.TXINTADDR", TEAK_I2S3_TXINTADDR, 0xFFFF, TEAK_I2S3_TXINTADDR_TXINTPTR);
	(void) write_checked("I2S3 clock disable completes", TEAK_I2S3_CTRL, 0);
}

int main(void) {
	test_start("DSP peripheral register test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_reset()))
		return test_finish();

	printf("# DSP Mask ROM ID: %04X\n", (uint32_t) DSP_SHARED_MEMORY[0]);
	test_interrupt();
	test_cipher();
	test_timer1();
	test_timer2();
	test_equalizer();
	test_channel_decoder();
	test_afe();
	test_baseband();
	test_mcs();
	test_dsp_control();
	test_modulator();
	test_ssc();
	test_i2s1();
	test_i2s2();
	test_i2s3();

	(void) dsp_reset();
	return test_finish();
}
