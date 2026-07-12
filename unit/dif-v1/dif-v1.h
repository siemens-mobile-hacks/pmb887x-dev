#pragma once

#include <stdint.h>

int dif_v1_test(void);
int dif_v1_dma_test(void);
void dif_v1_configure(uint32_t width, uint32_t rxfcon, uint32_t txfcon, uint32_t format);
