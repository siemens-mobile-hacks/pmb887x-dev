#include <pmb887x.h>
#include <gen/dsp.h>
#include <stopwatch.h>

#include "dsp-control-functional-8876.inc"
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
// PMB8876 has no fixed Data ROM: the data page selects the whole window.
#define DSP_DATA_WINDOW_FIRST 0x8000
#define DSP_DATA_WINDOW_WORDS 0x5000
#define DSP_PROGRAM_FIXED_FIRST 0x2000
#define DSP_PROGRAM_FIXED_WORDS 0x8000
#define DSP_PROGRAM_WINDOW_FIRST 0xA000
#define DSP_PROGRAM_WINDOW_WORDS 0x6000

struct dsp_mask_expectations {
	uint16_t mask_id;
	uint32_t fixed_program_hash;
	uint32_t program_page_hashes[4];
	uint32_t data_page_hashes[4];
	uint16_t data_page_signatures[4];
};

static const struct dsp_mask_expectations DSP_MASK_EXPECTATIONS[] = {
	{
		.mask_id = 0x0801,
		.fixed_program_hash = 0x3B018B00,
		.program_page_hashes = { 0x61DCDE5D, 0x041E8769, 0x436D3807, 0xF4101DC5 },
		.data_page_hashes = { 0xEB1485E1, 0xFCF12CBB, 0x6E384738, 0x6E3CDDC5 },
		.data_page_signatures = { 0x8004, 0xDA6C, 0x3666, 0x0000 },
	},
};

static const struct dsp_mask_expectations *find_dsp_mask_expectations(uint16_t mask_id) {
	for (size_t i = 0; i < ARRAY_SIZE(DSP_MASK_EXPECTATIONS); i++) {
		if (DSP_MASK_EXPECTATIONS[i].mask_id == mask_id)
			return &DSP_MASK_EXPECTATIONS[i];
	}

	return NULL;
}

static bool hash_words(bool program, uint16_t first, size_t words, uint32_t *hash) {
	uint16_t values[DSP_HW_BOOT_TRANSFER_WORDS];
	uint32_t source = first;
	uint32_t value_hash = 0x811C9DC5U;

	while (words != 0) {
		size_t block_words = words < DSP_HW_BOOT_TRANSFER_WORDS ? words : DSP_HW_BOOT_TRANSFER_WORDS;
		bool read;

		if (program) {
			read = dsp_hw_read_program((uint16_t) source, values, block_words);
		} else {
			read = dsp_hw_read_data((uint16_t) source, values, block_words);
		}

		if (!read)
			return false;
		for (size_t i = 0; i < block_words; i++)
			value_hash = (value_hash ^ values[i]) * 0x01000193U;
		source += block_words;
		words -= block_words;
		test_watchdog_serve();
	}

	*hash = value_hash;
	return true;
}

static bool check_rom_hash(
	const char *read_name,
	const char *hash_name,
	bool program,
	uint16_t first,
	size_t words,
	uint32_t expected
) {
	uint32_t hash;

	if (!test_check(read_name, hash_words(program, first, words, &hash)))
		return false;
	printf("# %s: %08lX\n", hash_name, hash);
	return test_eq_u32(hash_name, expected, hash);
}

static bool test_program_pages(const struct dsp_mask_expectations *expectations) {
	if (!check_rom_hash("PREAD reads fixed Program ROM", "fixed Program ROM FNV-1a", true,
		DSP_PROGRAM_FIXED_FIRST, DSP_PROGRAM_FIXED_WORDS, expectations->fixed_program_hash))
		return false;

	for (size_t page = 0; page < ARRAY_SIZE(expectations->program_page_hashes); page++) {
		uint16_t page_value = (uint16_t) page << TEAK_DSP_PAGE_PROG_PAGE_SHIFT;

		if (!test_check("DLOAD selects a Program ROM page", dsp_hw_write_reg(TEAK_DSP_PAGE, page_value)))
			return false;
		if (!check_rom_hash("PREAD reads the selected Program ROM page", "Program ROM page FNV-1a", true,
			DSP_PROGRAM_WINDOW_FIRST, DSP_PROGRAM_WINDOW_WORDS, expectations->program_page_hashes[page]))
			return false;
	}
	return true;
}

static bool test_data_pages(const struct dsp_mask_expectations *expectations) {
	for (size_t page = 0; page < ARRAY_SIZE(expectations->data_page_hashes); page++) {
		if (!test_check("DLOAD selects a Data ROM page", dsp_hw_write_reg(TEAK_DSP_PAGE, (uint16_t) page)))
			return false;
		if (!check_rom_hash("DREAD reads the selected Data ROM page", "Data ROM page FNV-1a", false,
			DSP_DATA_WINDOW_FIRST, DSP_DATA_WINDOW_WORDS, expectations->data_page_hashes[page]))
			return false;
	}
	return true;
}

static void validate_pram_pages(const struct dsp_mask_expectations *expectations) {
	for (size_t page = 0; page < ARRAY_SIZE(expectations->data_page_signatures); page++) {
		char name[64];

		sprintf(name, "PRAM reads the Data ROM page %lu signature", (uint32_t) page);
		test_eq_u32(name, expectations->data_page_signatures[page], dsp_hw_shared_memory[0x0020 + page]);
	}
	test_eq_u32("PRAM continues on Program ROM page 1", 0x1001, dsp_hw_shared_memory[0x0030]);
	test_eq_u32("PRAM continues on Program ROM page 2", 0x1002, dsp_hw_shared_memory[0x0031]);
	test_eq_u32("PRAM continues after restoring Program ROM page 0", 0x1000, dsp_hw_shared_memory[0x0032]);
	test_eq_u32("PRAM restores Data ROM page 0 without the ROM dispatcher",
		expectations->data_page_signatures[0], dsp_hw_shared_memory[0x0033]);
}

static bool start_pram_runner(void) {
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load the PMB8876 control runner",
		dsp_hw_load_image(DSP_CONTROL_FUNCTIONAL_8876, sizeof(DSP_CONTROL_FUNCTIONAL_8876))))
		return false;
	return test_check("BRANCH starts the PRAM control runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS));
}

int main(void) {
	test_start("DSP control functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("PMB8876 Program and Data ROM pages");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	uint16_t mask_id = dsp_hw_shared_memory[0];
	printf("# DSP mask ID: %04lX\n", (uint32_t) mask_id);
	const struct dsp_mask_expectations *expectations = find_dsp_mask_expectations(mask_id);
	if (!test_check("DSP Mask ROM ID is supported", expectations != NULL))
		return test_finish();
	bool rom_pages_passed = test_program_pages(expectations);
	if (rom_pages_passed)
		rom_pages_passed = test_data_pages(expectations);
	if (!test_check("DLOAD restores Program and Data ROM page 0", dsp_hw_write_reg(TEAK_DSP_PAGE, 0)))
		return test_finish();
	if (!rom_pages_passed)
		return test_finish();

	test_category("PRAM execution across page switches");
	if (!start_pram_runner())
		return test_finish();
	if (!test_check("PRAM runner reaches the core-idle point", dsp_hw_wait_shared(0x0040, 0x1111, 500)))
		return test_finish();
	validate_pram_pages(expectations);

	test_category("DSP core idle and MCU interrupt resume");
	stopwatch_usleep_wd(1000);
	test_eq_u32("DSP_CTRL stops execution before the post-idle point", 0, dsp_hw_shared_memory[0x0043]);
	test_eq_u32("the MCU0 INT0 handler has not run before the MCU request", 0, dsp_hw_shared_memory[0x0041]);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;
	if (!test_check("MCU0 INT0 resumes the PRAM runner", dsp_hw_wait_shared(0x0043, 0x3333, 500)))
		return test_finish();
	test_eq_u32("wake-up enters the installed MCU0 INT0 handler", 0x2222, dsp_hw_shared_memory[0x0041]);
	test_eq_u32("idle and interrupt preserve the PRAM register context", 0x5AA5, dsp_hw_shared_memory[0x0042]);
	test_eq_u32("execution resumes after the DSP_CTRL write", 0x3333, dsp_hw_shared_memory[0x0043]);

	test_category("DSP core idle with an active peripheral clock");
	if (!test_check("Timer 2 wakes core INT1 from DSP_CTRL idle", dsp_hw_wait_shared(0x0046, COMPLETE_MARKER, 500)))
		return test_finish();
	test_eq_u32("PRAM runner armed Timer 2 before entering idle", 0x4444, dsp_hw_shared_memory[0x0044]);
	test_eq_u32("Timer 2 enters the installed INT1 handler", 0x5555, dsp_hw_shared_memory[0x0045]);
	test_eq_u32("Timer 2 interrupt is delivered once", 1, dsp_hw_shared_memory[0x004A]);
	test_eq_u32("peripheral wake-up preserves the PRAM register context", COMPLETE_MARKER,
		dsp_hw_shared_memory[0x0046]);

	test_category("DSP subsystem clock off through DSP_CLC");
	if (!test_check("PRAM runner enters documented DSP_CTRL idle before clock off",
		dsp_hw_wait_shared(0x0047, 0x7777, 500)))
		return test_finish();
	stopwatch_usleep_wd(1000);
	test_eq_u32("DSP_CTRL stops the core before DSP_CLC is disabled", 0, dsp_hw_shared_memory[0x0049]);
	DSP_CLC = MOD_CLC_DISR;
	test_eq_u32("DSP_CLC reports the subsystem clock disabled", MOD_CLC_DISR | MOD_CLC_DISS, DSP_CLC);
	stopwatch_usleep_wd(80000);
	test_eq_u32("DSP_CLC clock-gates the MCU Shared RAM window", 0, dsp_hw_shared_memory[0x0047]);
	test_eq_u32("clock-gated Shared RAM reads as zero at another offset", 0, dsp_hw_shared_memory[0x004A]);

	stopwatch_t resume_start = stopwatch_get();
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("DSP_CLC reports the subsystem clock enabled", 1 << MOD_CLC_RMC_SHIFT, DSP_CLC);
	if (!test_check("Timer 2 resumes and wakes the core after DSP_CLC is enabled",
		dsp_hw_wait_shared(0, COMPLETE_MARKER, 500)))
		return test_finish();
	uint32_t resume_ms = stopwatch_elapsed_ms(resume_start);
	printf("# Timer 2 completion after DSP_CLC enable: %lu ms\n", resume_ms);
	test_check("Timer 2 was paused rather than only hidden while DSP_CLC was disabled", resume_ms >= 40);
	test_eq_u32("Shared RAM contents survive DSP_CLC clock off", 0x7777, dsp_hw_shared_memory[0x0047]);
	test_eq_u32("clock off and resume preserve the PRAM register context", 0xC33C,
		dsp_hw_shared_memory[0x0048]);
	test_eq_u32("execution resumes after the clock-off interval", 0x9999, dsp_hw_shared_memory[0x0049]);
	test_eq_u32("resumed Timer 2 interrupt is delivered exactly once", 2, dsp_hw_shared_memory[0x004A]);

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
