#pragma once

#include <stdbool.h>
#include <stdint.h>

bool lz4_decompress(const void *input, uint32_t input_size, void *output, uint32_t output_size);
