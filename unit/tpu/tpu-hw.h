#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TPU_HW_DECODER_CODON_SET 17
#define TPU_HW_DECODER_CODON_CLEAR 18

void tpu_hw_configure_decoders(const uint16_t *decoders, size_t count);
bool tpu_hw_execute_decoders(size_t count);
bool tpu_hw_run_decoder(uint16_t decoder);
