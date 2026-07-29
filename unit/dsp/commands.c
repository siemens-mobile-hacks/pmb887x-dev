#include <pmb887x.h>

#include "test.h"

#define DSP_BOOT_PLOAD 0
#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_BRANCH 2
#define DSP_BOOT_PREAD 3
#define DSP_BOOT_DREAD 4
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_RESULT_OFFSET (DSP_BOOT_DATA_OFFSET + 3)
#define DSP_BOOT_MAX_WORDS 507
#define DSP_WAIT_ITERATIONS 1000000
#define DSP_MASK_ID_FAMILY_MASK 0xFF00
#define DSP_MASK_ID_FAMILY_06XX 0x0600
#define DSP_MASK_ID_FAMILY_08XX 0x0800
#define DSP_STARTUP_ADDRESS 0x0100
#define DSP_USER_RESULT_OFFSET 0x0300
#define DSP1_HEADER_SIZE 0x300
#define DSP1_FILE_SIZE_OFFSET 0x104
#define DSP1_SEGMENT_COUNT_OFFSET 0x10E
#define DSP1_SEGMENT_TABLE_OFFSET 0x120
#define DSP1_SEGMENT_ENTRY_SIZE 0x30
#define DSP1_MAX_SEGMENTS 10

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

static const size_t DSP_RUNTIME_PIPE_OFFSETS[] = { 0x0005, 0x0021, 0x003D };

#ifdef PMB8875
#define DSP_MASK_ID_FAMILY DSP_MASK_ID_FAMILY_06XX
#else
#define DSP_MASK_ID_FAMILY DSP_MASK_ID_FAMILY_08XX
#endif

#include "commands-0602.inc"
#include "commands-0604.inc"
#include "commands-0801.inc"

struct dsp_mask_config {
	uint16_t mask_id;
	uint16_t user_command;
	uint16_t rejected_command;
	uint16_t callbacks_address;
	uint16_t interrupt_callbacks_address;
	uint16_t data_rom_address;
	uint16_t command_table_address;
	uint16_t fc_init_state_address;
	uint16_t modu_main_state_address;
	uint16_t modu_extra_state_addresses[2];
	uint16_t loop_state_address;
	uint16_t loop_parameters_address;
	uint16_t tone_duration_address;
	uint16_t tone_result_offset;
	uint16_t switch_state_addresses[3];
	bool vb_set_biquad_supported;
	uint16_t vb_biquad_coefficients_address;
	uint16_t program_rom_addresses[4];
	uint16_t program_rom_fingerprints[4][8];
	size_t program_rom_fingerprint_count;
	uint16_t data_rom_fingerprint[8];
	uint16_t command_table_fingerprint[8];
	const uint8_t *test_image;
	size_t test_image_size;
};

static const struct dsp_mask_config DSP_MASK_CONFIGS[] = {
	{
		.mask_id = 0x0602,
		.user_command = 35,
		.rejected_command = 53,
		.callbacks_address = 0x5850,
		.interrupt_callbacks_address = 0x5894,
		.data_rom_address = 0x6000,
		.command_table_address = 0x64B7,
		.fc_init_state_address = 0x578E,
		.modu_main_state_address = 0xE6B4,
		.modu_extra_state_addresses = { 0x5D84, 0x5D7E },
		.loop_parameters_address = 0x5D70,
		.tone_duration_address = 0x3AE1,
		.tone_result_offset = 0x00D6,
		.switch_state_addresses = { 0x5D7D, 0x5D8E, 0x5D95 },
		.program_rom_addresses = { 0x1000, 0x2000, 0x2779, 0x9520 },
		.program_rom_fingerprints = {
			{ 0x0602, 0xFFFF, 0x4180, 0x1010, 0x4180, 0x314B, 0x4180, 0x318B },
			{ 0x587A, 0x1F40, 0x848B, 0x1B48, 0x1F40, 0x848B, 0x1B48, 0x1F40 },
			{ 0x5E01, 0x5708, 0x5E02, 0x570E, 0x5E03, 0x5711, 0x5E04, 0x5714 },
			{ 0x5E03, 0xB48C, 0x5C23, 0x952C, 0x1CEB, 0x1C48, 0x1C30, 0x6760 },
		},
		.program_rom_fingerprint_count = 4,
		.data_rom_fingerprint = { 0x6004, 0x6004, 0x603D, 0x603D, 0x0000, 0x0064, 0x0054, 0x0044 },
		.command_table_fingerprint = { 0x0000, 0x186E, 0x187D, 0x1896, 0x189E, 0x18A6, 0x1917, 0x1932 },
		.test_image = DSP_TEST_IMAGE_0602,
		.test_image_size = sizeof(DSP_TEST_IMAGE_0602),
	},
	{
		.mask_id = 0x0604,
		.user_command = 46,
		.rejected_command = 63,
		.callbacks_address = 0x58A2,
		.interrupt_callbacks_address = 0x58E4,
		.data_rom_address = 0x6000,
		.command_table_address = 0x656D,
		.fc_init_state_address = 0x574E,
		.modu_main_state_address = 0xE6B4,
		.modu_extra_state_addresses = { 0x5D85, 0x5D7F },
		.loop_parameters_address = 0x5D70,
		.tone_duration_address = 0x5DAD,
		.tone_result_offset = 0x00D6,
		.switch_state_addresses = { 0x5D7E, 0x5D8F, 0x5D98 },
		.program_rom_addresses = { 0x1000, 0x2000, 0x2779, 0x9520 },
		.program_rom_fingerprints = {
			{ 0x0604, 0xFFFF, 0x4180, 0x1010, 0x4180, 0x2B51, 0x4180, 0x2B91 },
			{ 0xE620, 0x0000, 0x0000, 0x0000, 0x0000, 0x5E18, 0x0F00, 0xD4BC },
			{ 0x5B9A, 0x0E88, 0x9BEF, 0x3C77, 0x0E88, 0x9BEE, 0x3C79, 0x0E88 },
			{ 0xD008, 0xD658, 0xD400, 0x6790, 0x1B51, 0x1B91, 0x87E0, 0x0002 },
		},
		.program_rom_fingerprint_count = 4,
		.data_rom_fingerprint = { 0x6004, 0x6004, 0x603D, 0x603D, 0x0000, 0x0064, 0x0054, 0x0044 },
		.command_table_fingerprint = { 0x0000, 0x16C2, 0x16D1, 0x16EA, 0x16F2, 0x16FA, 0x176B, 0x1786 },
		.test_image = DSP_TEST_IMAGE_0604,
		.test_image_size = sizeof(DSP_TEST_IMAGE_0604),
	},
	{
		.mask_id = 0x0801,
		.user_command = 64,
		.rejected_command = 68,
		.callbacks_address = 0x7772,
		.interrupt_callbacks_address = 0x77BF,
		.data_rom_address = 0x8000,
		.command_table_address = 0x854E,
		.fc_init_state_address = 0x7599,
		.modu_main_state_address = 0xDEB4,
		.modu_extra_state_addresses = { 0x7D7B, 0x7D81 },
		.loop_state_address = 0x7969,
		.loop_parameters_address = 0x7D6B,
		.tone_duration_address = 0x7DA8,
		.tone_result_offset = 0x00EE,
		.switch_state_addresses = { 0x7D7A, 0x7D8B, 0x7D92 },
		.vb_set_biquad_supported = true,
		.vb_biquad_coefficients_address = 0xF71A,
		.program_rom_addresses = { 0x2000, 0x2779, 0x9520 },
		.program_rom_fingerprints = {
			{ 0x0801, 0xFFFF, 0x4180, 0x2010, 0x4180, 0x4DED, 0x4180, 0x4E2D },
			{ 0xD5BC, 0xDEA3, 0x41C0, 0xC153, 0x5E19, 0x0000, 0xD5BC, 0xDEA3 },
			{ 0x4E90, 0x60E7, 0x5E18, 0x012B, 0x8688, 0x583A, 0x86C0, 0x012C },
		},
		.program_rom_fingerprint_count = 3,
		.data_rom_fingerprint = { 0x8004, 0x8004, 0x803D, 0x803D, 0x0000, 0x0064, 0x0054, 0x0044 },
		.command_table_fingerprint = { 0x0000, 0x28DE, 0x28ED, 0x290C, 0x2914, 0x291C, 0x298D, 0x29A8 },
		.test_image = DSP_TEST_IMAGE_0801,
		.test_image_size = sizeof(DSP_TEST_IMAGE_0801),
	},
};

static const struct dsp_mask_config *dsp_mask_config;

static const struct dsp_mask_config *find_dsp_mask_config(uint16_t mask_id) {
	for (size_t i = 0; i < ARRAY_SIZE(DSP_MASK_CONFIGS); i++) {
		if (DSP_MASK_CONFIGS[i].mask_id == mask_id)
			return &DSP_MASK_CONFIGS[i];
	}

	return NULL;
}

static const uint16_t PROGRAM_RAM_PATTERN_1[] = {
	0x1357, 0x2468, 0xAAAA, 0x5555, 0xDEAD, 0xBEEF, 0x0102, 0x0304,
};

static const uint16_t PROGRAM_RAM_PATTERN_2[] = {
	0x0000, 0x4380, 0x5E08, 0x0001, 0xFFFF, 0x8000, 0x55AA, 0xFEDC,
};

static const uint16_t DATA_RAM_PATTERN_1[] = {
	0x0000, 0xFFFF, 0xA55A, 0x5AA5, 0x1234, 0xFEDC, 0x8000, 0x7FFF,
};

static const uint16_t DATA_RAM_PATTERN_2[] = {
	0x1357, 0x2468, 0xAAAA, 0x5555, 0xDEAD, 0xBEEF, 0x0102, 0x0304,
};

static const uint16_t RUNTIME_DATA_PATTERN[] = {
	0x1020, 0x3040, 0x5060, 0x7080, 0x90A0, 0xB0C0, 0xD0E0, 0xF001,
};

static const uint16_t RUNTIME_PROGRAM_PATTERN[] = {
	0x4380, 0x0000, 0x5E08, 0x0001, 0x4180, 0x01A6, 0x55AA, 0xAA55,
};

static uint16_t data_rom_probe[8];

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

static bool load_dsp1_image(const uint8_t *image, size_t image_size) {
	if (image_size < DSP1_HEADER_SIZE || image[0x100] != 'D' || image[0x101] != 'S' || image[0x102] != 'P' ||
		image[0x103] != '1' || read_le32(image + DSP1_FILE_SIZE_OFFSET) != image_size ||
		image[DSP1_SEGMENT_COUNT_OFFSET] > DSP1_MAX_SEGMENTS)
		return false;

	uint16_t payload[DSP_BOOT_MAX_WORDS];
	size_t segments = image[DSP1_SEGMENT_COUNT_OFFSET];
	for (size_t i = 0; i < segments; i++) {
		const uint8_t *entry = image + DSP1_SEGMENT_TABLE_OFFSET + i * DSP1_SEGMENT_ENTRY_SIZE;
		uint32_t offset = read_le32(entry);
		uint32_t address = read_le32(entry + 4);
		uint32_t size = read_le32(entry + 8);
		uint8_t memory_type = entry[0x0F];
		size_t words = size / sizeof(uint16_t);

		if (size == 0 || (size & 1) != 0 || words > DSP_BOOT_MAX_WORDS || address > UINT16_MAX ||
			offset > image_size || size > image_size - offset || memory_type > 2)
			return false;
		for (size_t j = 0; j < words; j++)
			payload[j] = image[offset + j * 2] | (uint16_t) image[offset + j * 2 + 1] << 8;
		if (!load_words(memory_type == 2 ? DSP_BOOT_DLOAD : DSP_BOOT_PLOAD, (uint16_t) address, payload, words))
			return false;
	}

	return true;
}

static bool read_words(uint16_t command, uint16_t source, size_t words) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = command;
	boot_data[1] = source;
	boot_data[2] = (uint16_t) words;

	return submit_boot_command();
}

static bool branch_to(uint16_t destination) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_BRANCH;
	boot_data[1] = destination;

	return submit_boot_command();
}

static bool submit_runtime_command(size_t pipe_index, uint16_t command, const uint16_t *parameters, size_t words) {
	volatile uint16_t *pipe = DSP_SHARED_MEMORY + DSP_RUNTIME_PIPE_OFFSETS[pipe_index];
	uint16_t expected_response = (uint16_t) -command;

	pipe[0] = command;
	for (size_t i = 0; i < words; i++)
		pipe[1 + i] = parameters[i];
	DSP_COM_SET = BIT(pipe_index);
	SCU_DSP_INT = BIT(pipe_index);
	SCU_DSP_INT = 0;

	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if (pipe[0] == expected_response && (DSP_COM_STATUS & BIT(pipe_index)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool submit_rejected_runtime_command(size_t pipe_index, uint16_t command) {
	volatile uint16_t *pipe = DSP_SHARED_MEMORY + DSP_RUNTIME_PIPE_OFFSETS[pipe_index];

	pipe[0] = command;
	DSP_COM_SET = BIT(pipe_index);
	SCU_DSP_INT = BIT(pipe_index);
	SCU_DSP_INT = 0;

	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if (pipe[0] == 0 && (DSP_COM_STATUS & BIT(pipe_index)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static void test_read_words(const char *name, uint16_t command, uint16_t source, const uint16_t *expected,
	size_t words) {
	char command_name[80];
	char contents_name[80];

	tfp_sprintf(command_name, "%s command completes", name);
	if (!test_check(command_name, read_words(command, source, words)))
		return;
	tfp_sprintf(contents_name, "%s returns expected words", name);
	test_eq_memory(contents_name, expected, DSP_SHARED_MEMORY + DSP_BOOT_RESULT_OFFSET, words * sizeof(uint16_t));
}

static void test_program_ram(void) {
	test_check(
		"PLOAD writes first Program RAM pattern",
		load_words(DSP_BOOT_PLOAD, 0x0100, PROGRAM_RAM_PATTERN_1, ARRAY_SIZE(PROGRAM_RAM_PATTERN_1))
	);
	test_read_words(
		"PREAD first Program RAM pattern",
		DSP_BOOT_PREAD,
		0x0100,
		PROGRAM_RAM_PATTERN_1,
		ARRAY_SIZE(PROGRAM_RAM_PATTERN_1)
	);
	test_check(
		"PLOAD overwrites Program RAM pattern",
		load_words(DSP_BOOT_PLOAD, 0x0100, PROGRAM_RAM_PATTERN_2, ARRAY_SIZE(PROGRAM_RAM_PATTERN_2))
	);
	test_read_words(
		"PREAD overwritten Program RAM pattern",
		DSP_BOOT_PREAD,
		0x0100,
		PROGRAM_RAM_PATTERN_2,
		ARRAY_SIZE(PROGRAM_RAM_PATTERN_2)
	);
}

static void test_data_ram(void) {
	test_check(
		"DLOAD writes first Data RAM pattern",
		load_words(DSP_BOOT_DLOAD, 0x0100, DATA_RAM_PATTERN_1, ARRAY_SIZE(DATA_RAM_PATTERN_1))
	);
	test_read_words(
		"DREAD first Data RAM pattern",
		DSP_BOOT_DREAD,
		0x0100,
		DATA_RAM_PATTERN_1,
		ARRAY_SIZE(DATA_RAM_PATTERN_1)
	);
	test_check(
		"DLOAD overwrites Data RAM pattern",
		load_words(DSP_BOOT_DLOAD, 0x0100, DATA_RAM_PATTERN_2, ARRAY_SIZE(DATA_RAM_PATTERN_2))
	);
	test_read_words(
		"DREAD overwritten Data RAM pattern",
		DSP_BOOT_DREAD,
		0x0100,
		DATA_RAM_PATTERN_2,
		ARRAY_SIZE(DATA_RAM_PATTERN_2)
	);
}

static bool test_mask_rom_read(const char *name, uint16_t command, uint16_t source, const uint16_t *expected,
	size_t words, bool verify_fingerprint) {
	char command_name[80];
	char contents_name[80];

	tfp_sprintf(command_name, "%s command completes", name);
	if (!test_check(command_name, read_words(command, source, words)))
		return false;
	tfp_sprintf(contents_name, "%s returns expected words", name);
	if (verify_fingerprint)
		test_eq_memory(contents_name, expected, DSP_SHARED_MEMORY + DSP_BOOT_RESULT_OFFSET,
			words * sizeof(uint16_t));
	else
		test_skip(contents_name, " fingerprint is not known for this Mask ID");

	return true;
}

static void test_program_mask_rom(void) {
	if (dsp_mask_config == NULL) {
		test_skip("Program Mask ROM fingerprints", "Mask ID parameters are not known");
		return;
	}

	for (size_t i = 0; i < dsp_mask_config->program_rom_fingerprint_count; i++) {
		char name[48];

		tfp_sprintf(name, "PREAD Program Mask ROM P:%04X",
			(uint32_t) dsp_mask_config->program_rom_addresses[i]);
		test_mask_rom_read(name, DSP_BOOT_PREAD, dsp_mask_config->program_rom_addresses[i],
			dsp_mask_config->program_rom_fingerprints[i], ARRAY_SIZE(dsp_mask_config->program_rom_fingerprints[i]),
			true);
	}
}

static void test_data_mask_rom(void) {
	if (dsp_mask_config == NULL) {
		test_skip("Data Mask ROM fingerprints", "Mask ID parameters are not known");
		return;
	}

	if (!test_mask_rom_read("DREAD fixed Data Mask ROM", DSP_BOOT_DREAD, dsp_mask_config->data_rom_address,
		dsp_mask_config->data_rom_fingerprint, ARRAY_SIZE(dsp_mask_config->data_rom_fingerprint), true))
		return;
	for (size_t i = 0; i < ARRAY_SIZE(data_rom_probe); i++)
		data_rom_probe[i] = DSP_SHARED_MEMORY[DSP_BOOT_RESULT_OFFSET + i];
	test_mask_rom_read("DREAD runtime command table", DSP_BOOT_DREAD, dsp_mask_config->command_table_address,
		dsp_mask_config->command_table_fingerprint, ARRAY_SIZE(dsp_mask_config->command_table_fingerprint), true);
}

static bool load_test_startup(void) {
	static const uint16_t ZERO_CALLBACKS[] = { 0, 0, 0, 0, 0 };
	static const uint16_t ZERO_INTERRUPT_CALLBACKS[] = { 0, 0 };

	return load_dsp1_image(dsp_mask_config->test_image, dsp_mask_config->test_image_size) &&
		load_words(DSP_BOOT_DLOAD, dsp_mask_config->callbacks_address, ZERO_CALLBACKS, ARRAY_SIZE(ZERO_CALLBACKS)) &&
		load_words(DSP_BOOT_DLOAD, dsp_mask_config->interrupt_callbacks_address, ZERO_INTERRUPT_CALLBACKS,
			ARRAY_SIZE(ZERO_INTERRUPT_CALLBACKS));
}

static bool test_user_command_pipe(size_t pipe_index) {
	uint16_t parameters[] = { 0xA550 + (uint16_t) pipe_index };
	char accepted_name[64];
	char executed_name[64];

	DSP_SHARED_MEMORY[DSP_USER_RESULT_OFFSET] = 0;
	tfp_sprintf(accepted_name, "pipe %u accepts custom user command", (uint32_t) pipe_index);
	if (!test_check(accepted_name, submit_runtime_command(pipe_index, dsp_mask_config->user_command, parameters,
		ARRAY_SIZE(parameters))))
		return false;
	tfp_sprintf(executed_name, "pipe %u executes custom user handler", (uint32_t) pipe_index);
	return test_eq_u32(executed_name, parameters[0], DSP_SHARED_MEMORY[DSP_USER_RESULT_OFFSET]);
}

static bool test_switch_command(const char *name, uint16_t command, uint16_t state_address,
	uint16_t result_offset) {
	uint16_t parameters[] = { 1 };
	uint16_t read_parameters[] = { state_address, result_offset, 1 };
	char command_name[64];
	char read_name[64];
	char state_name[64];

	for (uint16_t state = 0; state <= 1; state++) {
		parameters[0] = state;
		tfp_sprintf(command_name, "%s accepts state %u", name, (uint32_t) state);
		if (!test_check(command_name, submit_runtime_command(0, command, parameters, ARRAY_SIZE(parameters))))
			return false;
		tfp_sprintf(read_name, "READ_DSP reads %s state %u", name, (uint32_t) state);
		if (!test_check(read_name, submit_runtime_command(0, 33, read_parameters, ARRAY_SIZE(read_parameters))))
			return false;
		tfp_sprintf(state_name, "%s stores state %u", name, (uint32_t) state);
		if (!test_eq_u32(state_name, state, DSP_SHARED_MEMORY[result_offset]))
			return false;
	}

	parameters[0] = 0;
	tfp_sprintf(command_name, "%s restores off state", name);
	return test_check(command_name, submit_runtime_command(0, command, parameters, ARRAY_SIZE(parameters)));
}

static void test_runtime_read(const char *name, uint16_t source, uint16_t result_offset, const uint16_t *expected,
	size_t words) {
	uint16_t parameters[] = { source, result_offset, (uint16_t) words };
	char read_name[80];
	char contents_name[80];

	tfp_sprintf(read_name, "READ_DSP reads %s", name);
	if (!test_check(read_name, submit_runtime_command(0, 33, parameters, ARRAY_SIZE(parameters))))
		return;
	tfp_sprintf(contents_name, "%s has expected contents", name);
	test_eq_memory(contents_name, expected, DSP_SHARED_MEMORY + result_offset, words * sizeof(uint16_t));
}

static void test_fc_init(void) {
	static const uint16_t PARAMETERS[] = { 0x1357, 0x2468, 0x369C };

	if (!test_check("FC_INIT built-in command is accepted", submit_runtime_command(0, 1, PARAMETERS,
		ARRAY_SIZE(PARAMETERS))))
		return;
	test_runtime_read("FC_INIT state", dsp_mask_config->fc_init_state_address, 0x0320, PARAMETERS,
		ARRAY_SIZE(PARAMETERS));
}

static void test_modu_init(void) {
#ifdef PMB8875
	static const uint16_t PARAMETERS[] = {
		0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	};

	if (!test_check("MODU_INIT built-in command is accepted", submit_runtime_command(0, 2, PARAMETERS,
		ARRAY_SIZE(PARAMETERS))))
		return;
	test_runtime_read("MODU_INIT main state", dsp_mask_config->modu_main_state_address, 0x0420, PARAMETERS, 5);
	test_runtime_read("MODU_INIT extra state 0", dsp_mask_config->modu_extra_state_addresses[0], 0x0430,
		PARAMETERS + 5, 1);
	test_runtime_read("MODU_INIT extra state 1", dsp_mask_config->modu_extra_state_addresses[1], 0x0431,
		PARAMETERS + 6, 1);
#else
	static const uint16_t PARAMETERS[] = {
		0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0001, 0x0007, 0x0008,
	};
	const uint16_t DEB4_EXPECTED[] = {
		PARAMETERS[0], PARAMETERS[1], PARAMETERS[2], PARAMETERS[3], PARAMETERS[4], PARAMETERS[7], PARAMETERS[8],
	};

	if (!test_check("MODU_INIT built-in command is accepted", submit_runtime_command(0, 2, PARAMETERS,
		ARRAY_SIZE(PARAMETERS))))
		return;
	test_runtime_read("MODU_INIT main state", dsp_mask_config->modu_main_state_address, 0x0420, DEB4_EXPECTED,
		ARRAY_SIZE(DEB4_EXPECTED));
	test_runtime_read("MODU_INIT extra state 0", dsp_mask_config->modu_extra_state_addresses[0], 0x0430,
		PARAMETERS + 6, 1);
	test_runtime_read("MODU_INIT extra state 1", dsp_mask_config->modu_extra_state_addresses[1], 0x0431,
		PARAMETERS + 5, 1);
#endif
}

static void test_loop(void) {
	static const uint16_t PARAMETERS[] = { 0x1234, 0x5678, 0x9ABC };
#ifdef PMB8875
	if (!test_check("LOOP built-in command is accepted", submit_runtime_command(0, 10, PARAMETERS,
		ARRAY_SIZE(PARAMETERS))))
		return;
	test_runtime_read("LOOP parameters", dsp_mask_config->loop_parameters_address, 0x0450, PARAMETERS,
		ARRAY_SIZE(PARAMETERS));
#else
	static const uint16_t STATE_SEED[] = { 0xAAAA, 0xBBBB, 0xCCCC };
	static const uint16_t ZERO_STATE[] = { 0, 0, 0 };
	uint16_t write_state_parameters[] = { 0x0410, dsp_mask_config->loop_state_address, ARRAY_SIZE(STATE_SEED) };

	for (size_t i = 0; i < ARRAY_SIZE(STATE_SEED); i++)
		DSP_SHARED_MEMORY[0x0410 + i] = STATE_SEED[i];
	if (!test_check("WRITE_DSP seeds LOOP state", submit_runtime_command(0, 32, write_state_parameters,
		ARRAY_SIZE(write_state_parameters))))
		return;
	if (!test_check("LOOP built-in command is accepted", submit_runtime_command(0, 10, PARAMETERS,
		ARRAY_SIZE(PARAMETERS))))
		return;
	test_runtime_read("LOOP cleared state", dsp_mask_config->loop_state_address, 0x0440, ZERO_STATE,
		ARRAY_SIZE(ZERO_STATE));
	test_runtime_read("LOOP parameters", dsp_mask_config->loop_parameters_address, 0x0450, PARAMETERS,
		ARRAY_SIZE(PARAMETERS));
#endif
}

static void test_vb_set_biquad(void) {
	static const uint16_t PARAMETERS[] = {
		0x1001, 0x1002, 0x1003, 0x1004, 0x1005,
		0x2001, 0x2002, 0x2003, 0x2004, 0x2005,
		0x3001, 0x3002, 0x3003, 0x3004, 0x3005,
		0x4001, 0x4002, 0x4003, 0x4004, 0x4005,
	};

	if (!dsp_mask_config->vb_set_biquad_supported) {
		test_skip("VB_SET_BIQUAD built-in command", "this Mask ROM requires voiceband initialization");
		return;
	}
	if (!test_check("VB_SET_BIQUAD built-in command is accepted", submit_runtime_command(0, 16, PARAMETERS,
		ARRAY_SIZE(PARAMETERS))))
		return;
	test_runtime_read("VB_SET_BIQUAD coefficients", dsp_mask_config->vb_biquad_coefficients_address, 0x0460,
		PARAMETERS, ARRAY_SIZE(PARAMETERS));
}

static void test_vb_tone_duration(void) {
	static const uint16_t DURATION[] = { 0x5A5A };
	uint16_t write_duration_parameters[] = {
		0x0490, dsp_mask_config->tone_duration_address, ARRAY_SIZE(DURATION),
	};

	DSP_SHARED_MEMORY[0x0490] = DURATION[0];
	if (!test_check("WRITE_DSP seeds voiceband tone duration",
		submit_runtime_command(0, 32, write_duration_parameters, ARRAY_SIZE(write_duration_parameters))))
		return;
	if (!test_check("VB_READ_DURATION built-in command is accepted", submit_runtime_command(0, 20, NULL, 0)))
		return;
	test_eq_u32("VB_READ_DURATION copies tone duration to Shared RAM", DURATION[0],
		DSP_SHARED_MEMORY[dsp_mask_config->tone_result_offset]);

	if (!test_check("VB_STOP_TONE built-in command is accepted", submit_runtime_command(0, 19, NULL, 0)))
		return;
	DSP_SHARED_MEMORY[dsp_mask_config->tone_result_offset] = UINT16_MAX;
	if (!test_check("VB_READ_DURATION reads stopped tone", submit_runtime_command(0, 20, NULL, 0)))
		return;
	test_eq_u32("VB_STOP_TONE clears tone duration", 0,
		DSP_SHARED_MEMORY[dsp_mask_config->tone_result_offset]);
}

#ifndef PMB8875
static void test_vb_set_cbuf_gain(void) {
	static const uint16_t PARAMETERS[] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555 };
	const uint16_t F1_EXPECTED[] = { PARAMETERS[0], PARAMETERS[1], PARAMETERS[4] };

	if (!test_check("VB_SET_CBUF_GAIN built-in command is accepted", submit_runtime_command(0, 26, PARAMETERS,
		ARRAY_SIZE(PARAMETERS))))
		return;
	test_runtime_read("VB_SET_CBUF_GAIN 7DCB..7DCC state", 0x7DCB, 0x04A0, PARAMETERS + 2, 2);
	test_runtime_read("VB_SET_CBUF_GAIN 7DF1..7DF3 state", 0x7DF1, 0x04B0, F1_EXPECTED,
		ARRAY_SIZE(F1_EXPECTED));
}
#endif

static void test_runtime_commands(void) {
	static const uint16_t WRITE_DSP_PARAMETERS[] = { 0x0360, 0x0100, ARRAY_SIZE(RUNTIME_DATA_PATTERN) };
	static const uint16_t READ_DSP_PARAMETERS[] = { 0x0100, 0x0380, ARRAY_SIZE(RUNTIME_DATA_PATTERN) };
	static const uint16_t WRITE_PROG_PARAMETERS[] = { 0x03A0, 0x01A0, ARRAY_SIZE(RUNTIME_PROGRAM_PATTERN) };
	uint16_t read_data_rom_parameters[] = {
		dsp_mask_config->data_rom_address, 0x0340, ARRAY_SIZE(data_rom_probe),
	};

	for (size_t pipe_index = 0; pipe_index < ARRAY_SIZE(DSP_RUNTIME_PIPE_OFFSETS); pipe_index++) {
		if (!test_user_command_pipe(pipe_index))
			return;
	}
	if (!test_check("first command above the runtime table is rejected",
		submit_rejected_runtime_command(0, dsp_mask_config->rejected_command)))
		return;

	test_fc_init();
	test_modu_init();
	test_loop();
	test_vb_tone_duration();
#ifndef PMB8875
	test_vb_set_cbuf_gain();
#endif

	if (!test_check("READ_DSP built-in command is accepted",
		submit_runtime_command(0, 33, read_data_rom_parameters, ARRAY_SIZE(read_data_rom_parameters))))
		return;
	test_eq_memory("READ_DSP returns Data Mask ROM", data_rom_probe, DSP_SHARED_MEMORY + 0x0340,
		sizeof(data_rom_probe));

	for (size_t i = 0; i < ARRAY_SIZE(RUNTIME_DATA_PATTERN); i++)
		DSP_SHARED_MEMORY[0x0360 + i] = RUNTIME_DATA_PATTERN[i];
	if (!test_check("WRITE_DSP built-in command is accepted",
		submit_runtime_command(0, 32, WRITE_DSP_PARAMETERS, ARRAY_SIZE(WRITE_DSP_PARAMETERS))))
		return;
	if (!test_check("READ_DSP reads WRITE_DSP result",
		submit_runtime_command(0, 33, READ_DSP_PARAMETERS, ARRAY_SIZE(READ_DSP_PARAMETERS))))
		return;
	test_eq_memory("WRITE_DSP updates Data RAM", RUNTIME_DATA_PATTERN, DSP_SHARED_MEMORY + 0x0380,
		sizeof(RUNTIME_DATA_PATTERN));

	for (size_t i = 0; i < ARRAY_SIZE(RUNTIME_PROGRAM_PATTERN); i++)
		DSP_SHARED_MEMORY[0x03A0 + i] = RUNTIME_PROGRAM_PATTERN[i];
	if (!test_check("WRITE_PROG built-in command is accepted",
		submit_runtime_command(0, 34, WRITE_PROG_PARAMETERS, ARRAY_SIZE(WRITE_PROG_PARAMETERS))))
		return;

	if (!test_switch_command("IQ_SWAP_1", 3, dsp_mask_config->switch_state_addresses[0], 0x03C0) ||
		!test_switch_command("IQ_SWAP_2", 4, dsp_mask_config->switch_state_addresses[1], 0x03C1) ||
		!test_switch_command("DTX_ON", 30, dsp_mask_config->switch_state_addresses[2], 0x03C2))
		return;
}

static void test_isolated_vb_set_biquad(void) {
	if (!test_check("boot commands reinstall isolated runtime", load_test_startup()))
		return;
	if (!test_check("BRANCH starts isolated runtime", branch_to(DSP_STARTUP_ADDRESS)))
		return;

	test_vb_set_biquad();
	test_check("DSP reset ends isolated runtime", reset_dsp());
}

static void test_branch_and_runtime_commands(void) {
	if (!test_check("boot commands install test startup and user handler", load_test_startup()))
		return;
	if (!test_check("BRANCH starts runtime command processing", branch_to(DSP_STARTUP_ADDRESS)))
		return;
	test_runtime_commands();

	if (!test_check("DSP reset returns to Mask ROM boot dispatcher", reset_dsp()))
		return;
	test_read_words("PREAD WRITE_PROG result", DSP_BOOT_PREAD, 0x01A0, RUNTIME_PROGRAM_PATTERN,
		ARRAY_SIZE(RUNTIME_PROGRAM_PATTERN));

	test_isolated_vb_set_biquad();
}

int main(void) {
	test_start("DSP Mask ROM command test");
	if (test_is_qemu()) {
		test_skip("Mask ROM commands", "QEMU does not execute DSP firmware");
		return test_finish();
	}

	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("DSP Mask ROM boot dispatcher becomes ready", reset_dsp()))
		return test_finish();

	uint16_t mask_id = DSP_SHARED_MEMORY[0];
	printf("# DSP mask ID: %04X\n", (uint32_t) mask_id);
	uint16_t mask_family = mask_id & DSP_MASK_ID_FAMILY_MASK;
	if (!test_check("DSP Mask ROM family matches the CPU", mask_family == DSP_MASK_ID_FAMILY))
		return test_finish();
	dsp_mask_config = find_dsp_mask_config(mask_id);

	test_category("Program RAM boot commands");
	test_program_ram();
	test_category("Data RAM boot commands");
	test_data_ram();
	test_category("Program Mask ROM boot command");
	test_program_mask_rom();
	test_category("Data Mask ROM boot command");
	test_data_mask_rom();
	test_category("Branch and runtime commands");
	if (dsp_mask_config != NULL)
		test_branch_and_runtime_commands();
	else
		test_skip("Branch and runtime commands", "Mask ID parameters are not known");

	return test_finish();
}
