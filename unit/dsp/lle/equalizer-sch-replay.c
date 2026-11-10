#include <pmb887x.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define STREAM_OFFSET 0x0100
#define OUTPUT_OFFSET 0x0A00

#include "equalizer-sch-replay-stream.inc"

#ifdef PMB8875
#include "equalizer-sch-replay-8875.inc"
#define DSP_EQUALIZER_SCH_REPLAY_IMAGE DSP_EQUALIZER_SCH_REPLAY_8875
#else
#include "equalizer-sch-replay-8876.inc"
#define DSP_EQUALIZER_SCH_REPLAY_IMAGE DSP_EQUALIZER_SCH_REPLAY_8876
#endif

_Static_assert(STREAM_OFFSET + ARRAY_SIZE(EQUALIZER_SCH_REPLAY_STREAM) <= OUTPUT_OFFSET, "stream overlaps the output");
_Static_assert(STREAM_OFFSET + ARRAY_SIZE(EQUALIZER_RESET_REPLAY_STREAM) <= OUTPUT_OFFSET, "stream overlaps the output");
_Static_assert(STREAM_OFFSET + ARRAY_SIZE(EQUALIZER_CCH_REPLAY_STREAM) <= OUTPUT_OFFSET, "stream overlaps the output");
_Static_assert(STREAM_OFFSET + ARRAY_SIZE(EQUALIZER_SQUAL_REPLAY_STREAM) <= OUTPUT_OFFSET, "stream overlaps the output");

static bool replay_stream(const uint16_t *stream, size_t stream_words) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load the replay runner",
		dsp_hw_load_image(DSP_EQUALIZER_SCH_REPLAY_IMAGE, sizeof(DSP_EQUALIZER_SCH_REPLAY_IMAGE))))
		return false;
	for (size_t i = 0; i < stream_words; i++)
		dsp_hw_shared_memory[STREAM_OFFSET + i] = stream[i];
	if (!test_check("BRANCH starts the replay runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	return test_check("replay runner completes", dsp_hw_wait_shared(0, COMPLETE_MARKER, 1000));
}

int main(void) {
	test_start("DSP equalizer SCH replay test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("equalizer outputs for a firmware SCH command stream");
	if (replay_stream(EQUALIZER_SCH_REPLAY_STREAM, ARRAY_SIZE(EQUALIZER_SCH_REPLAY_STREAM))) {
		test_eq_memory("equalizer outputs match the hardware", EQUALIZER_SCH_REPLAY_EXPECTED,
			dsp_hw_shared_memory + OUTPUT_OFFSET, sizeof(EQUALIZER_SCH_REPLAY_EXPECTED));
	}

	test_category("combined output state across RES_EQ and RES_ALL");
	if (replay_stream(EQUALIZER_RESET_REPLAY_STREAM, ARRAY_SIZE(EQUALIZER_RESET_REPLAY_STREAM))) {
		test_eq_memory("combined outputs match the hardware", EQUALIZER_RESET_REPLAY_EXPECTED,
			dsp_hw_shared_memory + OUTPUT_OFFSET, sizeof(EQUALIZER_RESET_REPLAY_EXPECTED));
	}

	test_category("equalizer outputs for a firmware BCCH command stream");
	if (replay_stream(EQUALIZER_CCH_REPLAY_STREAM, ARRAY_SIZE(EQUALIZER_CCH_REPLAY_STREAM))) {
		test_eq_memory("equalizer outputs match the hardware", EQUALIZER_CCH_REPLAY_EXPECTED,
			dsp_hw_shared_memory + OUTPUT_OFFSET, sizeof(EQUALIZER_CCH_REPLAY_EXPECTED));
	}

	test_category("soft-output histogram across RES_EQ");
	if (replay_stream(EQUALIZER_SQUAL_REPLAY_STREAM, ARRAY_SIZE(EQUALIZER_SQUAL_REPLAY_STREAM))) {
		test_eq_memory("equalizer outputs and SQUAL match the hardware", EQUALIZER_SQUAL_REPLAY_EXPECTED,
			dsp_hw_shared_memory + OUTPUT_OFFSET, sizeof(EQUALIZER_SQUAL_REPLAY_EXPECTED));
	}

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
