#include "crc.h"

#define CRC32_POLYNOMIAL	0xEDB88320

uint32_t crc32_update(uint32_t crc, const void *data, uint32_t size) {
	const uint8_t *bytes = data;

	for (uint32_t i = 0; i < size; i++) {
		crc ^= bytes[i];
		for (uint32_t bit = 0; bit < 8; bit++) {
			uint32_t low_bit = (crc & 1);
			crc = (crc >> 1);
			if (low_bit != 0)
				crc ^= CRC32_POLYNOMIAL;
		}
	}

	return crc;
}
