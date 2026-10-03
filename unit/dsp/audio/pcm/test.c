#include <pmb887x.h>

#include "dsp/audio/pcm.h"
#include "test.h"

static bool words_equal(const int16_t *actual, const int16_t *expected, size_t count) {
	for (size_t i = 0; i < count; i++) {
		if (actual[i] != expected[i]) {
			printf("# word %lu: actual=%d, expected=%d\n",
				(uint32_t) i, actual[i], expected[i]);
			return false;
		}
	}
	return true;
}

static void test_triangle_generation(void) {
	static const int16_t EXPECTED[] = { 0, 4, 8, 4, 0, -4, -8, -4 };
	int16_t samples[ARRAY_SIZE(EXPECTED)];

	pcm_triangle_generate(samples, ARRAY_SIZE(samples), 8, 1, 8);
	test_check("triangle generator produces one exact period", words_equal(
		samples, EXPECTED, ARRAY_SIZE(EXPECTED)));
}

static void test_buffer_fill(void) {
	static const int16_t SOURCE[] = { 1, 2, 3, 4 };
	static const int16_t EXPECTED[] = { 3, 3, 1, 1, 0, 0 };
	uint16_t buffer[ARRAY_SIZE(EXPECTED)];
	const struct dsp_audio_pcm pcm = {
		.samples = SOURCE,
		.sample_count = ARRAY_SIZE(SOURCE),
		.sample_rate_hz = 1,
	};

	pcm_buffer_fill(buffer, ARRAY_SIZE(buffer), &pcm, 1, 2, 2, 2);
	test_check("PCM fill applies source stride, interleave, and tail padding", words_equal(
		(const int16_t *) buffer, EXPECTED, ARRAY_SIZE(EXPECTED)));
}

int main(void) {
	test_start("DSP audio PCM core test");
	test_category("Exact PCM fixtures");

	test_triangle_generation();
	test_buffer_fill();
	return test_finish();
}
