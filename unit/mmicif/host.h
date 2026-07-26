#pragma once

#include <stdbool.h>
#include <stdint.h>

bool mmicif_host_read32(uint32_t address, uint32_t *value);
void mmicif_host_write32(uint32_t address, uint32_t value);
