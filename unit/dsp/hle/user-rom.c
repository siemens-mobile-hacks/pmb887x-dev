#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp/dsp-hw.h"
#include "dsp/hle/user-rom.h"

#if defined(PMB8875) && defined(BOARD_PLATFORM_SIEMENS)
#include "user-rom-siemens-0602.inc"
#include "user-rom-siemens-0605.inc"
static const struct dsp_user_rom DSP_USER_ROMS[] = {
	{
		.name = "Siemens 0602",
		.mask_id = 0x0602,
		.container = DSP_USER_ROM_SIEMENS_0602,
		.container_size = sizeof(DSP_USER_ROM_SIEMENS_0602),
	},
	{
		.name = "Siemens 0605",
		.mask_id = 0x0605,
		.container = DSP_USER_ROM_SIEMENS_0605,
		.container_size = sizeof(DSP_USER_ROM_SIEMENS_0605),
	},
};
#elif defined(PMB8875) && defined(BOARD_PLATFORM_APOXI)
#include "user-rom-apoxi-0603.inc"
#include "user-rom-apoxi-0604.inc"
static const struct dsp_user_rom DSP_USER_ROMS[] = {
	{
		.name = "APOXI 0603",
		.mask_id = 0x0603,
		.container = DSP_USER_ROM_APOXI_0603,
		.container_size = sizeof(DSP_USER_ROM_APOXI_0603),
	},
	{
		.name = "APOXI 0604",
		.mask_id = 0x0604,
		.container = DSP_USER_ROM_APOXI_0604,
		.container_size = sizeof(DSP_USER_ROM_APOXI_0604),
	},
};
#elif defined(PMB8876) && defined(BOARD_PLATFORM_SIEMENS)
#include "user-rom-siemens-0800.inc"
#include "user-rom-siemens-0801.inc"
static const struct dsp_user_rom DSP_USER_ROMS[] = {
	{
		.name = "Siemens 0800",
		.mask_id = 0x0800,
		.container = DSP_USER_ROM_SIEMENS_0800,
		.container_size = sizeof(DSP_USER_ROM_SIEMENS_0800),
		.accepts_i2s_swap = true,
	},
	{
		.name = "Siemens 0801",
		.mask_id = 0x0801,
		.container = DSP_USER_ROM_SIEMENS_0801,
		.container_size = sizeof(DSP_USER_ROM_SIEMENS_0801),
		.accepts_i2s_swap = true,
		.supports_fc_notifications = true,
		.uses_stm_command_timeout = true,
	},
};
#elif defined(PMB8876) && defined(BOARD_PLATFORM_APOXI)
#include "user-rom-apoxi-0800.inc"
#include "user-rom-apoxi-0801.inc"
static const struct dsp_user_rom DSP_USER_ROMS[] = {
	{
		.name = "APOXI 0800",
		.mask_id = 0x0800,
		.container = DSP_USER_ROM_APOXI_0800,
		.container_size = sizeof(DSP_USER_ROM_APOXI_0800),
	},
	{
		.name = "APOXI 0801",
		.mask_id = 0x0801,
		.container = DSP_USER_ROM_APOXI_0801,
		.container_size = sizeof(DSP_USER_ROM_APOXI_0801),
	},
};
#else
#error Unsupported DSP User ROM platform
#endif

const struct dsp_user_rom *dsp_user_rom_find(uint16_t mask_id) {
	for (size_t i = 0; i < ARRAY_SIZE(DSP_USER_ROMS); i++)
		if (DSP_USER_ROMS[i].mask_id == mask_id)
			return &DSP_USER_ROMS[i];

	return NULL;
}

bool dsp_user_rom_start(const struct dsp_user_rom **selected_rom) {
	if (!dsp_hw_reset())
		return false;

	uint16_t mask_id = dsp_hw_shared_memory[0];
	const struct dsp_user_rom *rom = dsp_user_rom_find(mask_id);
	if (rom == NULL) {
		printf("# No original User ROM for Mask ROM %04lX and board %s\n",
			(uint32_t) mask_id, BOARD_NAME);
		return false;
	}

	printf("# DSP User ROM: %s (%lu bytes)\n", rom->name, (uint32_t) rom->container_size);
	for (size_t i = 0; i < TEAK_SHARED_RAM_SIZE / sizeof(*dsp_hw_shared_memory); i++)
		dsp_hw_shared_memory[i] = 0;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!dsp_hw_load_container(rom->container, rom->container_size))
		return false;
	if (selected_rom != NULL)
		*selected_rom = rom;
	return true;
}
