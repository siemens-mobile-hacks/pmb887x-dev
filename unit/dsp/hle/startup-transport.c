#include <pmb887x.h>
#include <gen/dsp.h>
#include <stopwatch.h>

#include "dsp/dsp-container.h"
#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

#define DSP_BOOT_DATA_OFFSET 2
#define DSP_RUNTIME_RESULT_OFFSET 0x0200
#define DSP_RUNTIME_FIRMWARE_POLL_LIMIT 1000

static bool wait_for_boot_record(size_t *polls, uint32_t *elapsed_us) {
	stopwatch_t start = stopwatch_get();

	for (size_t poll = 1; poll <= DSP_RUNTIME_FIRMWARE_POLL_LIMIT; poll++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0) {
			*polls = poll;
			*elapsed_us = stopwatch_elapsed_us(start);
			return true;
		}
	}

	*polls = DSP_RUNTIME_FIRMWARE_POLL_LIMIT;
	*elapsed_us = stopwatch_elapsed_us(start);
	return false;
}

static bool submit_record(const dsp_container_record_t *record, size_t *polls, uint32_t *elapsed_us) {
	volatile uint16_t *boot_data = dsp_hw_shared_memory + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = record->command;
	boot_data[1] = record->destination;
	if (record->command != DSP_CONTAINER_BRANCH) {
		boot_data[2] = record->words;
		for (size_t i = 0; i < record->words; i++)
			boot_data[i + 3] = dsp_container_read_u16(record->data + i * sizeof(uint16_t));
	}

	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;
	return wait_for_boot_record(polls, elapsed_us);
}

static bool readback_record(const dsp_container_record_t *record, uint16_t *readback) {
	if (record->command == DSP_CONTAINER_PLOAD)
		return dsp_hw_read_program(record->destination, readback, record->words);
	return dsp_hw_read_data(record->destination, readback, record->words);
}

static bool load_and_verify_user_rom(void) {
	uint16_t readback[DSP_HW_BOOT_TRANSFER_WORDS];

	if (!dsp_hw_reset())
		return false;
	const struct dsp_user_rom *user_rom = dsp_user_rom_find(dsp_hw_shared_memory[0]);
	if (user_rom == NULL)
		return false;
	printf("# DSP User ROM: %s (%lu bytes)\n", user_rom->name, (uint32_t) user_rom->container_size);

	for (size_t i = 0; i < TEAK_SHARED_RAM_SIZE / sizeof(*dsp_hw_shared_memory); i++)
		dsp_hw_shared_memory[i] = 0;
	DSP_COM_CLEAR = UINT16_MAX;
	dsp_container_reader_t reader = dsp_container_reader_init(user_rom->container, user_rom->container_size);
	dsp_container_record_t record;
	size_t record_index = 0;

	while (dsp_container_next(&reader, &record)) {
		char name[104];

		size_t polls;
		uint32_t elapsed_us;
		bool completed = submit_record(&record, &polls, &elapsed_us);
		sprintf(name, "startup record %lu completes within firmware's 1000-poll wait",
			(uint32_t) record_index);
		test_check(name, completed);
		printf("# startup record %lu command=%lu address=%04lX words=%lu elapsed=%lu us polls=%lu\n",
			(uint32_t) record_index, (uint32_t) record.command, (uint32_t) record.destination,
			(uint32_t) record.words, elapsed_us, (uint32_t) polls);
		if (!completed)
			return false;

		if (record.command != DSP_CONTAINER_BRANCH) {
			bool read_complete = readback_record(&record, readback);
			sprintf(name, "startup record %lu can be read back completely", (uint32_t) record_index);
			if (!test_check(name, read_complete))
				return false;
			bool contents_match = true;
			for (size_t i = 0; i < record.words; i++) {
				uint16_t expected = dsp_container_read_u16(record.data + i * sizeof(uint16_t));

				contents_match &= readback[i] == expected;
			}
			sprintf(name, "startup record %lu hardware contents match the original User ROM",
				(uint32_t) record_index);
			test_check(name, contents_match);
		}

		record_index++;
	}

	if (!test_check("startup stream ends with a valid branch at the exact container end", reader.complete))
		return false;

	dsp_runtime_firmware_delay();
	return test_check("runtime accepts the exact firmware startup probe after the complete stream",
		dsp_runtime_startup_probe(DSP_RUNTIME_RESULT_OFFSET));
}

static void test_user_rom(void) {
	test_category("Selected original User ROM startup stream");
	test_check("complete startup stream satisfies firmware transport contracts", load_and_verify_user_rom());
}

static void test_busy_transport_recovery(void) {
	static const uint16_t PARAMETERS[] = {
		0xE000, DSP_RUNTIME_RESULT_OFFSET, 2,
	};

	dsp_runtime_command_prepare(0, DSP_COMMAND_READ_DSP, PARAMETERS, ARRAY_SIZE(PARAMETERS));
	DSP_COM_SET = BIT(0);
	dsp_runtime_firmware_delay();
	test_eq_u32("runtime pipe remains busy when its interrupt is intentionally omitted",
		BIT(0), DSP_COM_STATUS & BIT(0));
	test_check("reset and complete User ROM reload recover a busy runtime transport", load_and_verify_user_rom());
}

int main(void) {
	test_start("DSP complete startup transport test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_user_rom();

	test_category("Recovery from busy runtime transport");
	test_busy_transport_recovery();

	return test_finish();
}
