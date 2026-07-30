#include <pmb887x.h>
#include <gen/dsp.h>

#include "test.h"

#define DSP_BOOT_PLOAD 0
#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_BRANCH 2
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_MAX_WORDS 507
#define DSP_WAIT_ITERATIONS 1000000
#define DSP_STARTUP_ADDRESS 0x0100
#define DSP_RESULT_OFFSET 0x0300
#define DSP_RESULT_STRIDE 0x0020
#define DSP_DONE_OFFSET 0x0340
#define DSP_DONE_VALUE 0xA55A
#define DSP1_SEGMENT_COUNT_OFFSET 0x10E
#define DSP1_SEGMENT_TABLE_OFFSET 0x120
#define DSP1_SEGMENT_ENTRY_SIZE 0x30
#define DSP1_SEGMENT_MEMORY_TYPE_OFFSET 0x0F
#define DSP1_MEMORY_DATA 2
#define DSP_CAPTURE_RUNS 2
#define DSP_CAPTURE_SNAPSHOTS 2

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

#include "safe-peripherals-8875.inc"
#include "safe-peripherals-8876.inc"

struct dsp_image_config {
	uint16_t mask_id;
	const uint8_t *image;
};

struct dsp_safe_register {
	const char *name;
	uint16_t address;
	uint16_t expected;
};

static const struct dsp_image_config DSP_IMAGE_CONFIGS[] = {
	{ 0x0602, DSP_SAFE_PERIPHERALS_IMAGE_8875 },
	{ 0x0604, DSP_SAFE_PERIPHERALS_IMAGE_8875 },
	{ 0x0801, DSP_SAFE_PERIPHERALS_IMAGE_8876 },
};

static const struct dsp_safe_register DSP_SAFE_REGISTERS[] = {
	{ "INT_FINTA0", TEAK_INT_FINTA0, 0x0000 },
	{ "INT_FINTB0", TEAK_INT_FINTB0, 0x0000 },
	{ "INT_FINT1", TEAK_INT_FINT1, 0x0000 },
	{ "INT_FINT2", TEAK_INT_FINT2, 0x0000 },
	{ "CIPH_CSTAT", TEAK_CIPH_CSTAT, 0x0000 },
	{ "TMR1_CNT", TEAK_TMR1_CNT, 0x0000 },
	{ "TMR2_CNT", TEAK_TMR2_CNT, 0x0000 },
	{ "EQ_STATUS", TEAK_EQ_STATUS, 0x0000 },
	{ "EQ_STAT_CNT", TEAK_EQ_STAT_CNT, 0x0000 },
	{ "EQ_SQUAL", TEAK_EQ_SQUAL, 0x0000 },
	{ "CHDEC_STATUS", TEAK_CHDEC_STATUS, 0x0000 },
	{ "CHDEC_STAT_CNT", TEAK_CHDEC_STAT_CNT, 0x0000 },
	{ "AFE_RWADDR", TEAK_AFE_RWADDR, 0x0000 },
	{ "BB_CTRL", TEAK_BB_CTRL, 0x0110 },
	{ "BB_WR_POINTER", TEAK_BB_WR_POINTER, 0x0000 },
	{ "BB_STATUS", TEAK_BB_STATUS, 0x0000 },
	{ "BB_PBASE_MSB", TEAK_BB_PBASE_MSB, 0x0000 },
	{ "BB_PBASE_LSB", TEAK_BB_PBASE_LSB, 0x0000 },
	{ "BB_PADJ_MSB", TEAK_BB_PADJ_MSB, 0x0000 },
	{ "BB_PADJ_LSB", TEAK_BB_PADJ_LSB, 0x0000 },
	{ "BB_IQ_IMBALANCE", TEAK_BB_IQ_IMBALANCE, 0x0000 },
	{ "MCS_CFSTA", TEAK_MCS_CFSTA, 0x0000 },
	{ "MCS_MCU_SEM", TEAK_MCS_MCU_SEM, 0xFFFF },
	{ "DSP_ID", TEAK_DSP_ID, 0xE101 },
	{ "MOD_STAT", TEAK_MOD_STAT, 0x0000 },
	{ "SSC_FSTAT", TEAK_SSC_FSTAT, 0x0000 },
	{ "I2S1_RWADDR", TEAK_I2S1_RWADDR, 0x0000 },
	{ "I2S2_RWADDR", TEAK_I2S2_RWADDR, 0x0000 },
	{ "I2S3_RADDR", TEAK_I2S3_RADDR, 0x0000 },
};

static const struct dsp_image_config *find_image_config(uint16_t mask_id) {
	for (size_t i = 0; i < ARRAY_SIZE(DSP_IMAGE_CONFIGS); i++) {
		if (DSP_IMAGE_CONFIGS[i].mask_id == mask_id)
			return &DSP_IMAGE_CONFIGS[i];
	}

	return NULL;
}

static bool wait_for_boot_ready(void) {
	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool reset_dsp(void) {
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

static bool load_words(uint16_t command, uint16_t destination, const uint16_t *values, size_t words) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = command;
	boot_data[1] = destination;
	boot_data[2] = (uint16_t) words;
	for (size_t i = 0; i < words; i++)
		boot_data[3 + i] = values[i];

	return submit_boot_command();
}

static uint32_t read_le32(const uint8_t *data) {
	return data[0] | (uint32_t) data[1] << 8 | (uint32_t) data[2] << 16 | (uint32_t) data[3] << 24;
}

static bool load_dsp1_image(const uint8_t *image) {
	uint16_t payload[DSP_BOOT_MAX_WORDS];
	size_t segments = image[DSP1_SEGMENT_COUNT_OFFSET];
	for (size_t i = 0; i < segments; i++) {
		const uint8_t *entry = image + DSP1_SEGMENT_TABLE_OFFSET + i * DSP1_SEGMENT_ENTRY_SIZE;
		uint32_t offset = read_le32(entry);
		uint32_t address = read_le32(entry + 4);
		uint32_t size = read_le32(entry + 8);
		uint8_t memory_type = entry[DSP1_SEGMENT_MEMORY_TYPE_OFFSET];
		size_t words = size / sizeof(uint16_t);
		uint16_t command = memory_type == DSP1_MEMORY_DATA ? DSP_BOOT_DLOAD : DSP_BOOT_PLOAD;

		for (size_t j = 0; j < words; j++)
			payload[j] = image[offset + j * 2] | (uint16_t) image[offset + j * 2 + 1] << 8;
		if (!load_words(command, (uint16_t) address, payload, words))
			return false;
	}

	return true;
}

static bool branch_to(uint16_t destination) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_BRANCH;
	boot_data[1] = destination;

	return submit_boot_command();
}

static bool wait_for_done(void) {
	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if (DSP_SHARED_MEMORY[DSP_DONE_OFFSET] == DSP_DONE_VALUE)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool capture_registers(const struct dsp_image_config *config,
	uint16_t result[DSP_CAPTURE_SNAPSHOTS][ARRAY_SIZE(DSP_SAFE_REGISTERS)])
{
	if (!reset_dsp())
		return false;
	if (!load_dsp1_image(config->image))
		return false;

	for (size_t snapshot = 0; snapshot < DSP_CAPTURE_SNAPSHOTS; snapshot++) {
		for (size_t reg = 0; reg < ARRAY_SIZE(DSP_SAFE_REGISTERS); reg++)
			DSP_SHARED_MEMORY[DSP_RESULT_OFFSET + snapshot * DSP_RESULT_STRIDE + reg] = 0xDEAD;
	}
	DSP_SHARED_MEMORY[DSP_DONE_OFFSET] = 0;
	if (!branch_to(DSP_STARTUP_ADDRESS))
		return false;
	if (!wait_for_done())
		return false;

	for (size_t snapshot = 0; snapshot < DSP_CAPTURE_SNAPSHOTS; snapshot++) {
		for (size_t reg = 0; reg < ARRAY_SIZE(DSP_SAFE_REGISTERS); reg++)
			result[snapshot][reg] = DSP_SHARED_MEMORY[DSP_RESULT_OFFSET + snapshot * DSP_RESULT_STRIDE + reg];
	}

	return true;
}

int main(void) {
	uint16_t captures[DSP_CAPTURE_RUNS][DSP_CAPTURE_SNAPSHOTS][ARRAY_SIZE(DSP_SAFE_REGISTERS)];

	test_start("DSP safe peripheral register test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("Mask ROM boot dispatcher becomes ready", reset_dsp()))
		return test_finish();

	uint16_t mask_id = DSP_SHARED_MEMORY[0];
	printf("# DSP mask ID: %04X\n", (uint32_t) mask_id);
	const struct dsp_image_config *config = find_image_config(mask_id);
	if (config == NULL) {
		test_skip("safe DSP peripheral reads", "Mask ID parameters are not known");
		return test_finish();
	}

	for (size_t run = 0; run < DSP_CAPTURE_RUNS; run++) {
		char name[64];

		tfp_sprintf(name, "capture run %u completes", (uint32_t) run + 1);
		if (!test_check(name, capture_registers(config, captures[run])))
			return test_finish();
	}

	printf("# TEAKREG,name,address,expected,run1a,run1b,run2a,run2b\n");
	for (size_t reg = 0; reg < ARRAY_SIZE(DSP_SAFE_REGISTERS); reg++) {
		const struct dsp_safe_register *safe_reg = &DSP_SAFE_REGISTERS[reg];
		char name[80];

		printf("# TEAKREG,%s,%04X,%04X,%04X,%04X,%04X,%04X\n", safe_reg->name,
			(uint32_t) safe_reg->address, (uint32_t) safe_reg->expected,
			(uint32_t) captures[0][0][reg], (uint32_t) captures[0][1][reg],
			(uint32_t) captures[1][0][reg], (uint32_t) captures[1][1][reg]);
		tfp_sprintf(name, "%s startup value", safe_reg->name);
		test_eq_u32(name, safe_reg->expected, captures[0][0][reg]);
	}

	test_eq_memory("consecutive passive reads are identical", captures[0][0], captures[0][1],
		sizeof(captures[0][0]));
	test_eq_memory("reset-isolated captures are identical", captures[0], captures[1], sizeof(captures[0]));
	(void) reset_dsp();

	return test_finish();
}
