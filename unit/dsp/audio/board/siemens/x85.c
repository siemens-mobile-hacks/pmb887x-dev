#include <pmb887x.h>
#include <cpu.h>
#include <gpio.h>
#include <stopwatch.h>

#include "dsp/audio/board.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/i2s.h"
#include "dsp/audio/pcm.h"
#include "dsp/dsp-hw.h"
#include "test.h"
#include "dsp/audio/board/siemens/x85.h"

static bool i2s_output_validate(const struct dsp_audio_pcm *pcm) {
	const struct siemens_x85_audio_route *route = siemens_x85_audio_board.route;
	const struct i2s_output_probe probe = {
		.result_offset = 0x0200,
		.max_large_steps = route->i2s_max_large_steps,
	};

	return i2s2_output_validate(pcm, &probe);
}

const char *dsp_audio_board_get_name(void) {
	return siemens_x85_audio_board.name;
}

uint32_t dsp_audio_board_get_sample_rate_hz(void) {
	return siemens_x85_audio_board.route->source_sample_rate_hz;
}

bool dsp_audio_board_clocks_configure(void) {
	uint32_t pll_mask = CGU_OSC_NDIV | CGU_OSC_MDIV |
		CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
	uint32_t osc = (CGU_OSC & ~pll_mask) |
		(3 << CGU_OSC_NDIV_SHIFT);

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

	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_MS_CLKSEL) | CGU_CON2_MS_CLKSEL_PLL;
	CGU_CON0 = (CGU_CON0 & ~CGU_CON0_PHASE1_CONFIG) |
		(0x0B << CGU_CON0_PHASE1_CONFIG_SHIFT);
	CGU_OSC |= CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE1_BYPASS_N;
	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_DSP_CLKSEL) | CGU_CON2_DSP_CLKSEL_PHASE1;

	printf("# Audio clocks: PLL=%lu Hz, fSYS=%lu Hz, DSP=%lu Hz\n",
		cpu_get_pll_freq(), cpu_get_sys_freq(), cpu_get_dsp_freq());
	return cpu_get_dsp_freq() == 138666666;
}

bool dsp_audio_board_supports_mask_rom(uint16_t mask_id) {
	const struct siemens_x85_audio_route *route = siemens_x85_audio_board.route;

	return dsp_audio_mask_is_supported(route->mask_ids, route->mask_id_count, mask_id);
}

static void i2s_configure(void) {
	gpio_init_output(GPIO_I2S2_CLK0, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
	gpio_init_input(GPIO_I2S2_RX, GPIO_IS_ALT0, GPIO_PS_ALT, GPIO_PDPU_PULLUP, GPIO_ENAQ_ON);
	gpio_init_output(GPIO_I2S2_TX, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
	gpio_init_output(GPIO_I2S2_WA0, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
	gpio_init_output(GPIO_I2S2_CLK1, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
	gpio_init_output(GPIO_I2S2_WA1, GPIO_OS_ALT0, GPIO_PS_ALT, false,
		GPIO_PPEN_PUSHPULL, GPIO_PDPU_PULLUP, GPIO_ENAQ_OUTPUT_ENABLED);
}

bool dsp_audio_board_play(const struct dsp_audio_pcm *pcm, size_t total_frames, size_t *frames_played) {
	const struct siemens_x85_audio_route *route = siemens_x85_audio_board.route;

	*frames_played = 0;
	if (pcm->sample_count == 0)
		return false;
	if (pcm->sample_rate_hz != route->source_sample_rate_hz)
		return false;
	if (total_frames == 0)
		return false;

	i2s_configure();
	if (!route->configure())
		return false;
	const struct pcm_shared_ring ring = {
		.buffer = dsp_hw_shared_memory + route->buffer_offset,
		.buffer_word_count = route->block_words,
		.channel_count = route->buffer_channels,
		.source_frame_step = 1,
		.response_flag = route->response_flag,
		.request_timeout_ms = 1000,
		.output_probe_block_count = 4,
		.start = route->start,
		.output_validate = i2s_output_validate,
	};
	if (!pcm_shared_ring_play(&ring, pcm, total_frames))
		return false;

	*frames_played = total_frames;
	return true;
}

bool dsp_audio_board_stop(void) {
	return siemens_x85_audio_board.route->stop();
}

bool dsp_audio_board_output_is_muted(void) {
	return pasic_output_is_muted();
}
