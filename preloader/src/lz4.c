#include "lz4.h"

static bool read_length(const uint8_t *input, uint32_t input_size, uint32_t *input_offset, uint32_t *length) {
	if (*length != 0x0F)
		return true;

	uint8_t value;
	do {
		if (*input_offset == input_size)
			return false;
		value = input[(*input_offset)++];
		*length += value;
	} while (value == 0xFF);

	return true;
}

bool lz4_decompress(const void *input_data, uint32_t input_size, void *output_data, uint32_t output_size) {
	const uint8_t *input = input_data;
	uint8_t *output = output_data;
	uint32_t input_offset = 0;
	uint32_t output_offset = 0;

	while (input_offset < input_size) {
		uint8_t token = input[input_offset++];
		uint32_t literal_size = (token >> 4);
		if (!read_length(input, input_size, &input_offset, &literal_size))
			return false;
		if (literal_size > input_size - input_offset)
			return false;
		if (literal_size > output_size - output_offset)
			return false;

		for (uint32_t i = 0; i < literal_size; i++)
			output[output_offset++] = input[input_offset++];

		if (input_offset == input_size)
			return output_offset == output_size;
		if (input_size - input_offset < 2)
			return false;

		uint32_t match_offset = (uint32_t) input[input_offset] | ((uint32_t) input[input_offset + 1] << 8);
		input_offset += 2;
		if (match_offset == 0 || match_offset > output_offset)
			return false;

		uint32_t match_size = (token & 0x0F);
		if (!read_length(input, input_size, &input_offset, &match_size))
			return false;
		match_size += 4;
		if (match_size > output_size - output_offset)
			return false;

		for (uint32_t i = 0; i < match_size; i++) {
			output[output_offset] = output[output_offset - match_offset];
			output_offset++;
		}
	}

	return output_offset == output_size;
}
