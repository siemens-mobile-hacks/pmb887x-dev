#include "dsp/audio/board.h"

bool dsp_audio_mask_is_supported(
	const uint16_t *mask_ids,
	size_t mask_id_count,
	uint16_t mask_id
) {
	for (size_t i = 0; i < mask_id_count; i++)
		if (mask_ids[i] == mask_id)
			return true;
	return false;
}
