#include <pmb887x.h>
#include <cpu.h>
#include <gpio.h>
#include <stopwatch.h>

#include "dsp/audio/board.h"
#include "dsp/audio/board/siemens/commands.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/board/siemens/siemens.h"
#include "dsp/audio/pcm.h"
#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

const char *audio_board_get_name(void) {
	return SIEMENS_AUDIO_BOARD.name;
}

bool audio_board_configure_clocks(void) {
#ifdef PMB8875
	CGU_OSC |= CGU_OSC_PLL_POWER_UP;
	stopwatch_t start = stopwatch_get();
	while ((CGU_STAT & CGU_STAT_LOCK) == 0) {
		if (stopwatch_elapsed_ms(start) >= 100)
			return false;
		test_watchdog_serve();
	}
	CGU_OSC |= CGU_OSC_PLL_BYPASS_N;
	CGU_CON2 = (CGU_CON2 & ~(CGU_CON2_DSP_CLKSEL | CGU_CON2_MS_CLKSEL)) |
		CGU_CON2_DSP_CLKSEL_PLL | CGU_CON2_MS_CLKSEL_PLL;
	stopwatch_update();

	printf("# Audio clocks: PLL=%lu Hz, DSP=%lu Hz, mixed-signal=%lu Hz\n",
		cpu_get_pll_freq(), cpu_get_dsp_freq(), cpu_get_ms_freq());
	return cpu_get_pll_freq() == 104000000 &&
		cpu_get_dsp_freq() == 104000000 && cpu_get_ms_freq() == 104000000;
#else
	uint32_t pll_mask = CGU_OSC_NDIV | CGU_OSC_MDIV |
		CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
	uint32_t osc = (CGU_OSC & ~pll_mask) | (3 << CGU_OSC_NDIV_SHIFT);

	CGU_CON1 &= ~(CGU_CON1_AHB_CLKSEL | CGU_CON1_FSYS_CLKSEL);
	CGU_OSC = osc;
	CGU_OSC = osc | CGU_OSC_PLL_POWER_UP;
	stopwatch_t start = stopwatch_get();
	while ((CGU_STAT & CGU_STAT_LOCK) == 0) {
		if (stopwatch_elapsed_ms(start) >= 100)
			return false;
		test_watchdog_serve();
	}
	CGU_OSC = osc | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
	if (cpu_get_pll_freq() != 104000000)
		return false;

	CGU_CON0 = (CGU_CON0 & ~CGU_CON0_PHASE1_CONFIG) |
		(0x0B << CGU_CON0_PHASE1_CONFIG_SHIFT);
	CGU_OSC |= CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE1_BYPASS_N;
	CGU_CON2 = (CGU_CON2 & ~(CGU_CON2_MS_CLKSEL | CGU_CON2_DSP_CLKSEL)) |
		CGU_CON2_MS_CLKSEL_PLL | CGU_CON2_DSP_CLKSEL_PHASE1;
	stopwatch_update();

	printf("# Audio clocks: PLL=%lu Hz, fSYS=%lu Hz, DSP=%lu Hz\n",
		cpu_get_pll_freq(), cpu_get_sys_freq(), cpu_get_dsp_freq());
	return cpu_get_dsp_freq() == 138666666;
#endif
}

bool audio_board_has_mask_rom(uint16_t mask_id) {
	for (size_t i = 0; i < SIEMENS_AUDIO_BOARD.mask_id_count; i++)
		if (SIEMENS_AUDIO_BOARD.mask_ids[i] == mask_id)
			return true;
	return false;
}

static const struct siemens_audio_output *output_get(enum audio_output output) {
	switch (output) {
		case AUDIO_OUTPUT_I2S:
			return &SIEMENS_AUDIO_BOARD.i2s;
		case AUDIO_OUTPUT_AFE:
			return &SIEMENS_AUDIO_BOARD.afe;
	}
	return NULL;
}

uint32_t audio_output_get_rate_hz(enum audio_output output) {
	const struct siemens_audio_output *configuration = output_get(output);

	return configuration != NULL ? configuration->sample_rate_hz : 0;
}

static bool shared_ring_play(
	const struct siemens_audio_output *output,
	const struct dsp_audio_pcm *pcm,
	size_t total_frames,
	size_t *frames_played
) {
	const struct pcm_shared_ring RING = {
		.buffer = dsp_hw_shared_memory + SIEMENS_AUDIO_BOARD.buffer_word_offset,
		.buffer_word_count = SIEMENS_AUDIO_BOARD.block_words,
		.channel_count = SIEMENS_AUDIO_BOARD.buffer_channels,
		.source_frame_step = 1,
		.response_flag = SIEMENS_AUDIO_BOARD.response_flag,
		.request_timeout_ms = 1000,
		.start = output->start,
	};
	if (!pcm_shared_ring_play(&RING, pcm, total_frames))
		return false;

	*frames_played = total_frames;
	return true;
}

static void i2s_pins_configure(void) {
	gpio_init_output(GPIO_I2S2_CLK0, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
#ifdef PMB8875
	gpio_init_input(GPIO_I2S2_RX, GPIO_IS_ALT0, GPIO_PS_ALT, GPIO_PDPU_PULLUP, GPIO_ENAQ_OFF);
#else
	gpio_init_input(GPIO_I2S2_RX, GPIO_IS_ALT0, GPIO_PS_ALT, GPIO_PDPU_PULLUP, GPIO_ENAQ_ON);
#endif
	gpio_init_output(GPIO_I2S2_TX, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
	gpio_init_output(GPIO_I2S2_WA0, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
#ifndef PMB8875
	gpio_init_output(GPIO_I2S2_CLK1, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
	gpio_init_output(GPIO_I2S2_WA1, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
#endif
}

bool audio_output_configure(enum audio_output output) {
	const struct siemens_audio_output *configuration = output_get(output);
	if (configuration == NULL)
		return false;
	if (configuration->configure == NULL)
		return false;

	if (output == AUDIO_OUTPUT_I2S)
		i2s_pins_configure();
	i2c_init();
	if (!pasic_detect())
		return false;
	return configuration->configure();
}

bool audio_output_play(
	enum audio_output output,
	const struct dsp_audio_pcm *pcm,
	size_t total_frames,
	size_t *frames_played
) {
	const struct siemens_audio_output *configuration = output_get(output);

	*frames_played = 0;
	if (configuration == NULL)
		return false;
	if (configuration->start == NULL)
		return false;
	if (pcm->sample_count == 0)
		return false;
	if (pcm->sample_rate_hz != configuration->sample_rate_hz)
		return false;
	if (total_frames == 0)
		return false;
	return shared_ring_play(configuration, pcm, total_frames, frames_played);
}

bool audio_output_stop(enum audio_output output) {
	const struct siemens_audio_output *configuration = output_get(output);

	if (configuration == NULL)
		return false;
	if (configuration->stop == NULL)
		return false;
	return configuration->stop();
}
