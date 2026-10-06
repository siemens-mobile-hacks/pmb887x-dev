#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct dsp_user_rom {
	const char *name;
	uint16_t mask_id;
	const uint8_t *container;
	size_t container_size;
	bool accepts_i2s_swap;
	bool supports_fc_notifications;
	bool uses_stm_command_timeout;
};

const struct dsp_user_rom *dsp_user_rom_find(uint16_t mask_id);
bool dsp_user_rom_start(const struct dsp_user_rom **selected_rom);
