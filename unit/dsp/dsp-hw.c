#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp-container.h"
#include "dsp-hw.h"
#include "test.h"

#define DSP_BOOT_PLOAD 0
#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_BRANCH 2
#define DSP_BOOT_PREAD 3
#define DSP_BOOT_DREAD 4
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_RESULT_OFFSET (DSP_BOOT_DATA_OFFSET + 3)
#define DSP_WAIT_ITERATIONS 1000000

volatile uint16_t *const dsp_hw_shared_memory = (volatile uint16_t *) DSP_RAM_BASE;

static bool wait_for_boot_ready(void) {
	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool submit_boot_command(void) {
	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;

	return wait_for_boot_ready();
}

static bool load_words(uint16_t command, uint16_t destination, const uint16_t *values, size_t words) {
	volatile uint16_t *boot_data = dsp_hw_shared_memory + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = command;
	boot_data[1] = destination;
	boot_data[2] = (uint16_t) words;
	for (size_t i = 0; i < words; i++)
		boot_data[3 + i] = values[i];

	return submit_boot_command();
}

static bool read_words(uint16_t command, uint16_t source, uint16_t *values, size_t words) {
	volatile uint16_t *boot_data = dsp_hw_shared_memory + DSP_BOOT_DATA_OFFSET;

	if (words > DSP_HW_BOOT_MAX_WORDS)
		return false;

	boot_data[0] = command;
	boot_data[1] = source;
	boot_data[2] = (uint16_t) words;
	if (!submit_boot_command())
		return false;

	for (size_t i = 0; i < words; i++)
		values[i] = dsp_hw_shared_memory[DSP_BOOT_RESULT_OFFSET + i];
	return true;
}

bool dsp_hw_reset(void) {
	DSP_COM_CLEAR = 0xFFFF;
	SCU_DSP_INT = 0;
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	uint32_t reset_readback = SCU_RST_REQ;
	SCU_RST_REQ = 0;
	(void) reset_readback;

	return wait_for_boot_ready();
}

bool dsp_hw_read_reg(uint16_t address, uint16_t *value) {
	return read_words(DSP_BOOT_DREAD, address, value, 1);
}

bool dsp_hw_read_data(uint16_t address, uint16_t *values, size_t words) {
	return read_words(DSP_BOOT_DREAD, address, values, words);
}

bool dsp_hw_read_program(uint16_t address, uint16_t *values, size_t words) {
	return read_words(DSP_BOOT_PREAD, address, values, words);
}

bool dsp_hw_write_reg(uint16_t address, uint16_t value) {
	return load_words(DSP_BOOT_DLOAD, address, &value, 1);
}

static bool dsp_hw_load(const uint8_t *image, size_t image_size, bool execute_branch) {
	uint16_t payload[DSP_HW_BOOT_MAX_WORDS];
	dsp_container_reader_t reader = dsp_container_reader_init(image, image_size);
	dsp_container_record_t record;

	while (dsp_container_next(&reader, &record)) {
		if (record.command == DSP_CONTAINER_BRANCH)
			return !execute_branch || dsp_hw_branch(record.destination);
		if (record.words > DSP_HW_BOOT_MAX_WORDS)
			return false;
		for (size_t i = 0; i < record.words; i++)
			payload[i] = dsp_container_read_u16(record.data + i * sizeof(uint16_t));
		if (!load_words(record.command, record.destination, payload, record.words))
			return false;
	}

	return false;
}

bool dsp_hw_load_image(const uint8_t *image, size_t image_size) {
	return dsp_hw_load(image, image_size, false);
}

bool dsp_hw_load_container(const uint8_t *container, size_t container_size) {
	return dsp_hw_load(container, container_size, true);
}

bool dsp_hw_branch(uint16_t destination) {
	volatile uint16_t *boot_data = dsp_hw_shared_memory + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_BRANCH;
	boot_data[1] = destination;

	return submit_boot_command();
}

bool dsp_hw_wait_shared(size_t offset, uint16_t expected, uint32_t timeout_ms) {
	stopwatch_t start = stopwatch_get();

	while (dsp_hw_shared_memory[offset] != expected && stopwatch_elapsed_ms(start) < timeout_ms)
		test_watchdog_serve();

	return dsp_hw_shared_memory[offset] == expected;
}
