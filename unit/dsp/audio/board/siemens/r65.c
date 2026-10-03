#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/audio/board.h"
#include "dsp/audio/board/siemens/pasic.h"
#include "dsp/audio/board/siemens/r65.h"
#include "dsp/audio/pcm.h"
#include "dsp/dsp-hw.h"
#include "dsp/runtime.h"
#include "test.h"

#define DSP_PCM_TRANSFER_OFFSET 0x0400
#define DSP_PCM_PROGRESS_RESULT_OFFSET 0x0580
#define DSP_PCM_PROGRESS_PROBE_INTERVAL_MS 37

static bool pcm_source_upload(const struct dsp_audio_pcm *pcm) {
	pcm_buffer_fill(dsp_hw_shared_memory + DSP_PCM_TRANSFER_OFFSET,
		siemens_r65_audio_board.block_samples, pcm, 0,
		siemens_r65_audio_board.block_samples, 1, 1);

	return dsp_runtime_data_write(DSP_PCM_TRANSFER_OFFSET,
		siemens_r65_audio_board.buffer_address, siemens_r65_audio_board.block_samples);
}

static bool runtime_word_read(uint16_t address, uint16_t *value) {
	return dsp_runtime_data_read(address, DSP_PCM_PROGRESS_RESULT_OFFSET, value, 1);
}

static bool playback_progress_observe(size_t total_frames, uint32_t frame_rate_hz) {
	uint16_t previous_value;
	if (!runtime_word_read(siemens_r65_audio_board.progress_address, &previous_value))
		return false;

	uint32_t duration_ms = total_frames * 1000 / frame_rate_hz;
	stopwatch_t start = stopwatch_get();
	uint32_t next_probe_ms = DSP_PCM_PROGRESS_PROBE_INTERVAL_MS;
	bool advanced = false;
	while (stopwatch_elapsed_ms(start) < duration_ms) {
		uint32_t elapsed_ms = stopwatch_elapsed_ms(start);

		if (elapsed_ms >= next_probe_ms) {
			uint16_t value;
			if (!runtime_word_read(siemens_r65_audio_board.progress_address, &value))
				return false;
			advanced |= value != previous_value;
			previous_value = value;
			next_probe_ms += DSP_PCM_PROGRESS_PROBE_INTERVAL_MS;
		}
		test_watchdog_serve();
	}
	return advanced;
}

const char *dsp_audio_board_get_name(void) {
	return siemens_r65_audio_board.name;
}

uint32_t dsp_audio_board_get_sample_rate_hz(void) {
	return siemens_r65_audio_board.sample_rate_hz;
}

bool dsp_audio_board_clocks_configure(void) {
	return true;
}

bool dsp_audio_board_supports_mask_rom(uint16_t mask_id) {
	return dsp_audio_mask_is_supported(siemens_r65_audio_board.mask_ids,
		siemens_r65_audio_board.mask_id_count, mask_id);
}

bool dsp_audio_board_play(const struct dsp_audio_pcm *pcm, size_t total_frames, size_t *frames_played) {
	*frames_played = 0;
	if (pcm->sample_count == 0)
		return false;
	if (pcm->sample_rate_hz != siemens_r65_audio_board.sample_rate_hz)
		return false;
	if (total_frames == 0)
		return false;

	bool source_ready = pcm_source_upload(pcm);
	bool firmware_ready = siemens_r65_audio_board.prepare();
	if (!source_ready)
		printf("# PCM source upload failed\n");

	bool pasic_ready = pasic_output_enable(
		siemens_r65_audio_board.supply_mode_mask,
		siemens_r65_audio_board.supply_1_mask,
		siemens_r65_audio_board.supply_2_mask,
		siemens_r65_audio_board.supply_settle_ms);
	pasic_ready = pasic_ready && siemens_r65_audio_board.output_configure();
	bool firmware_output_ready = siemens_r65_audio_board.start();
	if (!source_ready || !firmware_ready || !pasic_ready || !firmware_output_ready) {
		printf("# PCM setup: source=%s, firmware=%s, PASIC=%s, output=%s\n",
			source_ready ? "ready" : "failed",
			firmware_ready ? "ready" : "failed",
			pasic_ready ? "ready" : "failed",
			firmware_output_ready ? "ready" : "failed");
		return false;
	}

	if (!playback_progress_observe(total_frames, pcm->sample_rate_hz))
		return false;
	*frames_played = total_frames;
	return true;
}

bool dsp_audio_board_stop(void) {
	bool stopped = siemens_r65_audio_board.stop();
	bool muted = siemens_r65_audio_board.mute();
	return stopped && muted;
}

bool dsp_audio_board_output_is_muted(void) {
	return pasic_output_is_muted();
}
