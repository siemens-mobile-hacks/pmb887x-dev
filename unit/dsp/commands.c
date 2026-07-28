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
#define DSP_MASK_ID_0801 0x0801
#define DSP_STARTUP_ADDRESS 0x0100
#define DSP_USER_COMMAND 64
#define DSP_USER_RESULT_OFFSET 0x0300
#define DSP1_HEADER_SIZE 0x300
#define DSP1_FILE_SIZE_OFFSET 0x104
#define DSP1_SEGMENT_COUNT_OFFSET 0x10E
#define DSP1_SEGMENT_TABLE_OFFSET 0x120
#define DSP1_SEGMENT_ENTRY_SIZE 0x30
#define DSP1_MAX_SEGMENTS 10

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

static const size_t DSP_RUNTIME_PIPE_OFFSETS[] = { 0x0005, 0x0021, 0x003D };

#include "commands.inc"

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

static const uint16_t PROGRAM_ROM_2000[] = {
	0x0801, 0xFFFF, 0x4180, 0x2010, 0x4180, 0x4DED, 0x4180, 0x4E2D,
};

static const uint16_t PROGRAM_ROM_2779[] = {
	0xD5BC, 0xDEA3, 0x41C0, 0xC153, 0x5E19, 0x0000, 0xD5BC, 0xDEA3,
};

static const uint16_t PROGRAM_ROM_9520[] = {
	0x4E90, 0x60E7, 0x5E18, 0x012B, 0x8688, 0x583A, 0x86C0, 0x012C,
};

static const uint16_t DATA_ROM_8000[] = {
	0x8004, 0x8004, 0x803D, 0x803D, 0x0000, 0x0064, 0x0054, 0x0044,
};

static const uint16_t DATA_ROM_854E[] = {
	0x0000, 0x28DE, 0x28ED, 0x290C, 0x2914, 0x291C, 0x298D, 0x29A8,
};

static const uint16_t RUNTIME_DATA_PATTERN[] = {
	0x1020, 0x3040, 0x5060, 0x7080, 0x90A0, 0xB0C0, 0xD0E0, 0xF001,
};

static const uint16_t RUNTIME_PROGRAM_PATTERN[] = {
	0x4380, 0x0000, 0x5E08, 0x0001, 0x4180, 0x01A6, 0x55AA, 0xAA55,
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

static bool load_dsp1_image(void) {
	if (sizeof(DSP_TEST_IMAGE) < DSP1_HEADER_SIZE || DSP_TEST_IMAGE[0x100] != 'D' ||
		DSP_TEST_IMAGE[0x101] != 'S' || DSP_TEST_IMAGE[0x102] != 'P' || DSP_TEST_IMAGE[0x103] != '1' ||
		read_le32(DSP_TEST_IMAGE + DSP1_FILE_SIZE_OFFSET) != sizeof(DSP_TEST_IMAGE) ||
		DSP_TEST_IMAGE[DSP1_SEGMENT_COUNT_OFFSET] > DSP1_MAX_SEGMENTS)
		return false;

	uint16_t payload[DSP_BOOT_MAX_WORDS];
	size_t segments = DSP_TEST_IMAGE[DSP1_SEGMENT_COUNT_OFFSET];
	for (size_t i = 0; i < segments; i++) {
		const uint8_t *entry = DSP_TEST_IMAGE + DSP1_SEGMENT_TABLE_OFFSET + i * DSP1_SEGMENT_ENTRY_SIZE;
		uint32_t offset = read_le32(entry);
		uint32_t address = read_le32(entry + 4);
		uint32_t size = read_le32(entry + 8);
		uint8_t memory_type = entry[0x0F];
		size_t words = size / sizeof(uint16_t);

		if (size == 0 || (size & 1) != 0 || words > DSP_BOOT_MAX_WORDS || address > UINT16_MAX ||
			offset > sizeof(DSP_TEST_IMAGE) || size > sizeof(DSP_TEST_IMAGE) - offset || memory_type > 2)
			return false;
		for (size_t j = 0; j < words; j++)
			payload[j] = DSP_TEST_IMAGE[offset + j * 2] | (uint16_t) DSP_TEST_IMAGE[offset + j * 2 + 1] << 8;
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

static void test_program_mask_rom(void) {
	test_read_words("PREAD Program Mask ROM P:2000", DSP_BOOT_PREAD, 0x2000, PROGRAM_ROM_2000,
		ARRAY_SIZE(PROGRAM_ROM_2000));
	test_read_words("PREAD Program Mask ROM P:2779", DSP_BOOT_PREAD, 0x2779, PROGRAM_ROM_2779,
		ARRAY_SIZE(PROGRAM_ROM_2779));
	test_read_words("PREAD Program Mask ROM P:9520", DSP_BOOT_PREAD, 0x9520, PROGRAM_ROM_9520,
		ARRAY_SIZE(PROGRAM_ROM_9520));
}

static void test_data_mask_rom(void) {
	test_read_words("DREAD Data Mask ROM D:8000", DSP_BOOT_DREAD, 0x8000, DATA_ROM_8000,
		ARRAY_SIZE(DATA_ROM_8000));
	test_read_words("DREAD command table D:854E", DSP_BOOT_DREAD, 0x854E, DATA_ROM_854E,
		ARRAY_SIZE(DATA_ROM_854E));
}

static bool load_test_startup(void) {
	static const uint16_t ZERO_CALLBACKS[] = { 0, 0, 0, 0, 0 };
	static const uint16_t ZERO_INTERRUPT_CALLBACKS[] = { 0, 0 };

	return load_dsp1_image() &&
		load_words(DSP_BOOT_DLOAD, 0x7772, ZERO_CALLBACKS, ARRAY_SIZE(ZERO_CALLBACKS)) &&
		load_words(DSP_BOOT_DLOAD, 0x77BF, ZERO_INTERRUPT_CALLBACKS, ARRAY_SIZE(ZERO_INTERRUPT_CALLBACKS));
}

static void test_user_command_pipe(size_t pipe_index) {
	uint16_t parameters[] = { 0xA550 + (uint16_t) pipe_index };
	char accepted_name[64];
	char executed_name[64];

	DSP_SHARED_MEMORY[DSP_USER_RESULT_OFFSET] = 0;
	tfp_sprintf(accepted_name, "pipe %u accepts USER_0 custom command", (uint32_t) pipe_index);
	test_check(accepted_name, submit_runtime_command(pipe_index, DSP_USER_COMMAND, parameters,
		ARRAY_SIZE(parameters)));
	tfp_sprintf(executed_name, "pipe %u executes USER_0 custom handler", (uint32_t) pipe_index);
	test_eq_u32(executed_name, parameters[0], DSP_SHARED_MEMORY[DSP_USER_RESULT_OFFSET]);
}

static void test_switch_command(const char *name, uint16_t command, uint16_t state_address,
	uint16_t result_offset) {
	uint16_t parameters[] = { 1 };
	uint16_t read_parameters[] = { state_address, result_offset, 1 };
	char command_name[64];
	char read_name[64];
	char state_name[64];

	for (uint16_t state = 0; state <= 1; state++) {
		parameters[0] = state;
		tfp_sprintf(command_name, "%s accepts state %u", name, (uint32_t) state);
		test_check(command_name, submit_runtime_command(0, command, parameters, ARRAY_SIZE(parameters)));
		tfp_sprintf(read_name, "READ_DSP reads %s state %u", name, (uint32_t) state);
		test_check(read_name, submit_runtime_command(0, 33, read_parameters, ARRAY_SIZE(read_parameters)));
		tfp_sprintf(state_name, "%s stores state %u", name, (uint32_t) state);
		test_eq_u32(state_name, state, DSP_SHARED_MEMORY[result_offset]);
	}

	parameters[0] = 0;
	tfp_sprintf(command_name, "%s restores off state", name);
	test_check(command_name, submit_runtime_command(0, command, parameters, ARRAY_SIZE(parameters)));
}

static void test_runtime_commands(void) {
	static const uint16_t BB_INIT_PARAMETERS[] = { 0x1357, 0x2468, 0x369C };
	static const uint16_t READ_BB_INIT_PARAMETERS[] = { 0x7599, 0x0320, ARRAY_SIZE(BB_INIT_PARAMETERS) };
	static const uint16_t READ_DATA_ROM_PARAMETERS[] = { 0x8000, 0x0340, ARRAY_SIZE(DATA_ROM_8000) };
	static const uint16_t WRITE_DSP_PARAMETERS[] = { 0x0360, 0x0100, ARRAY_SIZE(RUNTIME_DATA_PATTERN) };
	static const uint16_t READ_DSP_PARAMETERS[] = { 0x0100, 0x0380, ARRAY_SIZE(RUNTIME_DATA_PATTERN) };
	static const uint16_t WRITE_PROG_PARAMETERS[] = { 0x03A0, 0x01A0, ARRAY_SIZE(RUNTIME_PROGRAM_PATTERN) };

	for (size_t pipe_index = 0; pipe_index < ARRAY_SIZE(DSP_RUNTIME_PIPE_OFFSETS); pipe_index++)
		test_user_command_pipe(pipe_index);
	test_check("runtime command ID 68 is rejected", submit_rejected_runtime_command(0, 68));

	test_check("BB_INIT built-in command is accepted", submit_runtime_command(0, 1, BB_INIT_PARAMETERS,
		ARRAY_SIZE(BB_INIT_PARAMETERS)));
	test_check("READ_DSP reads BB_INIT state", submit_runtime_command(0, 33, READ_BB_INIT_PARAMETERS,
		ARRAY_SIZE(READ_BB_INIT_PARAMETERS)));
	test_eq_memory("BB_INIT stores its parameters", BB_INIT_PARAMETERS, DSP_SHARED_MEMORY + 0x0320,
		sizeof(BB_INIT_PARAMETERS));

	test_check("READ_DSP built-in command is accepted", submit_runtime_command(0, 33, READ_DATA_ROM_PARAMETERS,
		ARRAY_SIZE(READ_DATA_ROM_PARAMETERS)));
	test_eq_memory("READ_DSP returns Data Mask ROM", DATA_ROM_8000, DSP_SHARED_MEMORY + 0x0340,
		sizeof(DATA_ROM_8000));

	for (size_t i = 0; i < ARRAY_SIZE(RUNTIME_DATA_PATTERN); i++)
		DSP_SHARED_MEMORY[0x0360 + i] = RUNTIME_DATA_PATTERN[i];
	test_check("WRITE_DSP built-in command is accepted", submit_runtime_command(0, 32, WRITE_DSP_PARAMETERS,
		ARRAY_SIZE(WRITE_DSP_PARAMETERS)));
	test_check("READ_DSP reads WRITE_DSP result", submit_runtime_command(0, 33, READ_DSP_PARAMETERS,
		ARRAY_SIZE(READ_DSP_PARAMETERS)));
	test_eq_memory("WRITE_DSP updates Data RAM", RUNTIME_DATA_PATTERN, DSP_SHARED_MEMORY + 0x0380,
		sizeof(RUNTIME_DATA_PATTERN));

	for (size_t i = 0; i < ARRAY_SIZE(RUNTIME_PROGRAM_PATTERN); i++)
		DSP_SHARED_MEMORY[0x03A0 + i] = RUNTIME_PROGRAM_PATTERN[i];
	test_check("WRITE_PROG built-in command is accepted", submit_runtime_command(0, 34, WRITE_PROG_PARAMETERS,
		ARRAY_SIZE(WRITE_PROG_PARAMETERS)));

	test_switch_command("IQ_SWAP_1", 3, 0x7D7A, 0x03C0);
	test_switch_command("IQ_SWAP_2", 4, 0x7D8B, 0x03C1);
	test_switch_command("DTX_ON", 30, 0x7D92, 0x03C2);
}

static void test_branch_and_runtime_commands(void) {
	test_check("boot commands install test startup and USER_0 handler", load_test_startup());
	bool completed = branch_to(DSP_STARTUP_ADDRESS);

	if (!test_check("BRANCH starts runtime command processing", completed))
		return;
	test_runtime_commands();

	if (!test_check("DSP reset returns to Mask ROM boot dispatcher", reset_dsp()))
		return;
	test_read_words("PREAD WRITE_PROG result", DSP_BOOT_PREAD, 0x01A0, RUNTIME_PROGRAM_PATTERN,
		ARRAY_SIZE(RUNTIME_PROGRAM_PATTERN));
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
	if (!test_eq_u32("DSP Mask ROM ID is supported", DSP_MASK_ID_0801, mask_id))
		return test_finish();

	test_category("Program RAM boot commands");
	test_program_ram();
	test_category("Data RAM boot commands");
	test_data_ram();
	test_category("Program Mask ROM boot command");
	test_program_mask_rom();
	test_category("Data Mask ROM boot command");
	test_data_mask_rom();
	test_category("Branch and runtime commands");
	test_branch_and_runtime_commands();

	return test_finish();
}
