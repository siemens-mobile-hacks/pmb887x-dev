#pragma once

#include <stdbool.h>
#include <stdint.h>

void i2c_v2_init(void);
bool i2c_v2_transfer(uint8_t address, const uint8_t *tx, uint8_t *rx, uint32_t size);
int i2c_v2_test(void);
int i2c_v2_dma_test(void);
