#include <pmb887x.h>
#include <gen/dsp.h>
#include <stopwatch.h>

#include "dsp-hw.h"
#include "test.h"

#define BENCHMARK_READY 0xB001
#define BENCHMARK_RUN 0xB002
#define BENCHMARK_DONE 0xB003
#define BENCHMARK_RUNS 3
#define BENCHMARK_ITERATIONS 4096
#define MATRIX_DIMENSION 16
#define MACS_PER_ITERATION (MATRIX_DIMENSION * MATRIX_DIMENSION)
#define BENCHMARK_MACS (BENCHMARK_ITERATIONS * MACS_PER_ITERATION)
#define COMMAND_OFFSET 0x0001
#define OUTPUT_OFFSET 0x0010
#define BENCHMARK_TIMEOUT_MS 2500

#ifdef PMB8875
#include "benchmark-8875.inc"
#define DSP_BENCHMARK_IMAGE DSP_BENCHMARK_8875
#else
#include "benchmark-8876.inc"
#define DSP_BENCHMARK_IMAGE DSP_BENCHMARK_8876
#endif

static const uint16_t EXPECTED_OUTPUT[MATRIX_DIMENSION] = {
	0x0044, 0x0088, 0x00CC, 0x0110, 0x0154, 0x0198, 0x01DC, 0x0220,
	0x0264, 0x02A8, 0x02EC, 0x0330, 0x0374, 0x03B8, 0x03FC, 0x0440,
};

static const uint16_t EXPECTED_VECTOR[MATRIX_DIMENSION] = {
	0x0080, 0x0100, 0x0180, 0x0200, 0x0280, 0x0300, 0x0380, 0x0400,
	0x0480, 0x0500, 0x0580, 0x0600, 0x0680, 0x0700, 0x0780, 0x0800,
};

static const uint16_t EXPECTED_MATRIX_ROW[MATRIX_DIMENSION] = {
	0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
};

static const char *const COMPLETION_NAMES[BENCHMARK_RUNS] = {
	"dense benchmark run 1 completes",
	"dense benchmark run 2 completes",
	"dense benchmark run 3 completes",
};

static const char *const OUTPUT_NAMES[BENCHMARK_RUNS] = {
	"dense benchmark run 1 produces the expected vector",
	"dense benchmark run 2 produces the expected vector",
	"dense benchmark run 3 produces the expected vector",
};

static const char *const REARM_NAMES[BENCHMARK_RUNS] = {
	"dense benchmark runner rearms after run 1",
	"dense benchmark runner rearms after run 2",
	"dense benchmark runner rearms after run 3",
};

static const char *const SCORE_NAMES[BENCHMARK_RUNS] = {
	"run 1",
	"run 2",
	"run 3",
};

static void print_score(const char *name, uint32_t elapsed_us) {
	uint32_t mmac_milli = (uint32_t) ((uint64_t) BENCHMARK_MACS * 1000 / elapsed_us);
	uint32_t mops_milli = mmac_milli * 2;

	printf("# %s: %u us, %u.%03u MMAC/s, %u.%03u MOPS\n", name, elapsed_us,
		mmac_milli / 1000, mmac_milli % 1000, mops_milli / 1000, mops_milli % 1000);
}

int main(void) {
	test_start("DSP dense fixed-point benchmark");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("16x16 matrix-vector multiply, 4096 iterations");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load benchmark runner",
		dsp_hw_load_image(DSP_BENCHMARK_IMAGE, sizeof(DSP_BENCHMARK_IMAGE))))
		return test_finish();
	dsp_hw_shared_memory[0] = 0;
	dsp_hw_shared_memory[COMMAND_OFFSET] = 0;
	if (!test_check("BRANCH starts benchmark runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("benchmark runner becomes ready", dsp_hw_wait_shared(0, BENCHMARK_READY, BENCHMARK_TIMEOUT_MS)))
		return test_finish();
	test_eq_memory("runner reads the loaded X vector", EXPECTED_VECTOR, dsp_hw_shared_memory + 0x0040,
		sizeof(EXPECTED_VECTOR));
	test_eq_memory("runner reads the loaded first Y-matrix row", EXPECTED_MATRIX_ROW,
		dsp_hw_shared_memory + 0x0060, sizeof(EXPECTED_MATRIX_ROW));

	uint32_t best_us = UINT32_MAX;
	for (size_t run = 0; run < BENCHMARK_RUNS; run++) {
		stopwatch_t start = stopwatch_get();
		dsp_hw_shared_memory[COMMAND_OFFSET] = BENCHMARK_RUN;
		bool completed = dsp_hw_wait_shared(0, BENCHMARK_DONE, BENCHMARK_TIMEOUT_MS);
		uint32_t elapsed_us = stopwatch_elapsed_us(start);

		test_check(COMPLETION_NAMES[run], completed);
		test_eq_memory(OUTPUT_NAMES[run], EXPECTED_OUTPUT, dsp_hw_shared_memory + OUTPUT_OFFSET,
			sizeof(EXPECTED_OUTPUT));
		if (completed && elapsed_us != 0) {
			print_score(SCORE_NAMES[run], elapsed_us);
			if (elapsed_us < best_us)
				best_us = elapsed_us;
		}

		dsp_hw_shared_memory[COMMAND_OFFSET] = 0;
		test_check(REARM_NAMES[run], dsp_hw_wait_shared(0, BENCHMARK_READY, 500));
	}

	if (best_us != UINT32_MAX)
		print_score("best of 3", best_us);

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
