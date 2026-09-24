#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum dsp_container_command {
	DSP_CONTAINER_PLOAD = 0,
	DSP_CONTAINER_DLOAD = 1,
	DSP_CONTAINER_BRANCH = 2,
};

typedef struct {
	const uint8_t *data;
	size_t size;
	size_t offset;
	bool complete;
} dsp_container_reader_t;

typedef struct {
	uint16_t command;
	uint16_t destination;
	uint16_t words;
	const uint8_t *data;
} dsp_container_record_t;

static inline uint16_t dsp_container_read_u16(const uint8_t *data) {
	return (uint16_t) data[0] | ((uint16_t) data[1] << 8);
}

static inline dsp_container_reader_t dsp_container_reader_init(const uint8_t *data, size_t size) {
	return (dsp_container_reader_t) {
		.data = data,
		.size = size,
	};
}

static inline bool dsp_container_next(dsp_container_reader_t *reader, dsp_container_record_t *record) {
	if (reader->complete || reader->offset > reader->size || reader->size - reader->offset < 4)
		return false;

	const uint8_t *header = reader->data + reader->offset;
	record->command = dsp_container_read_u16(header);
	record->destination = dsp_container_read_u16(header + 2);
	record->words = 0;
	record->data = NULL;

	if (record->command == DSP_CONTAINER_BRANCH) {
		if (reader->offset + 4 != reader->size)
			return false;
		reader->offset += 4;
		reader->complete = true;
		return true;
	}
	if (record->command != DSP_CONTAINER_PLOAD && record->command != DSP_CONTAINER_DLOAD)
		return false;
	if (reader->size - reader->offset < 6)
		return false;

	record->words = dsp_container_read_u16(header + 4);
	if ((uint32_t) record->destination + record->words > 0x10000)
		return false;
	size_t payload_size = (size_t) record->words * sizeof(uint16_t);
	if (payload_size > reader->size - reader->offset - 6)
		return false;

	record->data = header + 6;
	reader->offset += 6 + payload_size;
	return true;
}
