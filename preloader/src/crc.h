#pragma once

#include <stdint.h>

#define CRC32_INITIAL_VALUE	0xFFFFFFFF

uint32_t crc32_update(uint32_t crc, const void *data, uint32_t size);
