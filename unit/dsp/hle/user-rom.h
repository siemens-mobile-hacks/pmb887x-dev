#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum dsp_user_rom_platform {
	DSP_USER_ROM_PLATFORM_SIEMENS,
	DSP_USER_ROM_PLATFORM_APOXI,
};

struct dsp_user_rom {
	const char *name;
	uint16_t mask_id;
	enum dsp_user_rom_platform platform;
	const uint8_t *container;
	size_t container_size;
};

const struct dsp_user_rom *dsp_user_rom_find(uint16_t mask_id);
bool dsp_user_rom_start(const struct dsp_user_rom **selected_rom);
