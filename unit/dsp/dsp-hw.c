#include <pmb887x.h>
#include <stopwatch.h>

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
#define DSP1_HEADER_SIZE 0x300
#define DSP1_FILE_SIZE_OFFSET 0x104
#define DSP1_SEGMENT_COUNT_OFFSET 0x10E
#define DSP1_SEGMENT_TABLE_OFFSET 0x120
#define DSP1_SEGMENT_ENTRY_SIZE 0x30
#define DSP1_MAX_SEGMENTS 10

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

static uint32_t read_le32(const uint8_t *data) {
	return data[0] | (uint32_t) data[1] << 8 | (uint32_t) data[2] << 16 | (uint32_t) data[3] << 24;
}

static uint16_t read_le16(const uint8_t *data) {
	return data[0] | (uint16_t) data[1] << 8;
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

bool dsp_hw_load_image(const uint8_t *image, size_t image_size) {
	if (image_size < DSP1_HEADER_SIZE)
		return false;
	if (image[0x100] != 'D' || image[0x101] != 'S' || image[0x102] != 'P' || image[0x103] != '1')
		return false;
	if (read_le32(image + DSP1_FILE_SIZE_OFFSET) != image_size)
		return false;
	if (image[DSP1_SEGMENT_COUNT_OFFSET] > DSP1_MAX_SEGMENTS)
		return false;

	uint16_t payload[DSP_HW_BOOT_MAX_WORDS];
	size_t segments = image[DSP1_SEGMENT_COUNT_OFFSET];
	for (size_t i = 0; i < segments; i++) {
		const uint8_t *entry = image + DSP1_SEGMENT_TABLE_OFFSET + i * DSP1_SEGMENT_ENTRY_SIZE;
		uint32_t offset = read_le32(entry);
		uint32_t address = read_le32(entry + 4);
		uint32_t size = read_le32(entry + 8);
		uint8_t memory_type = entry[0x0F];
		size_t words = size / sizeof(uint16_t);

		if (size == 0 || (size & 1) != 0 || address > UINT16_MAX)
			return false;
		if (offset > image_size || size > image_size - offset || memory_type > 2)
			return false;
		if (words > (size_t) UINT16_MAX - address + 1)
			return false;

		for (size_t first = 0; first < words; first += DSP_HW_BOOT_MAX_WORDS) {
			size_t count = MIN(words - first, DSP_HW_BOOT_MAX_WORDS);

			for (size_t j = 0; j < count; j++) {
				size_t source = offset + (first + j) * sizeof(uint16_t);
				payload[j] = image[source] | (uint16_t) image[source + 1] << 8;
			}
			uint16_t destination = (uint16_t) (address + first);
			uint16_t command = memory_type == 2 ? DSP_BOOT_DLOAD : DSP_BOOT_PLOAD;
			if (!load_words(command, destination, payload, count))
				return false;
		}
	}

	return true;
}

bool dsp_hw_load_container(const uint8_t *container, size_t container_size) {
	uint16_t payload[DSP_HW_BOOT_MAX_WORDS];
	size_t offset = 0;

	while (container_size - offset >= 4) {
		uint16_t command = read_le16(container + offset);
		uint16_t destination = read_le16(container + offset + 2);

		if (command == DSP_BOOT_BRANCH)
			return offset + 4 == container_size && dsp_hw_branch(destination);
		if (command != DSP_BOOT_PLOAD && command != DSP_BOOT_DLOAD)
			return false;
		if (container_size - offset < 6)
			return false;

		size_t words = read_le16(container + offset + 4);
		size_t payload_size = words * sizeof(uint16_t);

		if (words > DSP_HW_BOOT_MAX_WORDS || payload_size > container_size - offset - 6)
			return false;
		for (size_t i = 0; i < words; i++)
			payload[i] = read_le16(container + offset + 6 + i * sizeof(uint16_t));
		if (!load_words(command, destination, payload, words))
			return false;
		offset += 6 + payload_size;
	}

	return false;
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
