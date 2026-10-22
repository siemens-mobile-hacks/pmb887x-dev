#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#define BENCHMARK_READY 0xB001
#define BENCHMARK_RUN 0xB002
#define BENCHMARK_DONE 0xB003
#define BENCHMARK_COMMAND_OFFSET 0x0001
#define BENCHMARK_TIMEOUT_MS 2500
#define GSM_COPY_WORDS 0x70
#define LATENCY_SAMPLES 32
/* INT1=9000 to the following frame's INT0=0 at 13 MHz / 6. */
#define GSM_IRQ_WINDOW_US 462

#ifdef PMB8875
#include "lle/benchmark-8875.inc"
#define DSP_BENCHMARK_IMAGE DSP_BENCHMARK_8875
#else
#include "lle/benchmark-8876.inc"
#define DSP_BENCHMARK_IMAGE DSP_BENCHMARK_8876
#endif

int main(void) {
	test_start("ARM DSP Shared RAM latency test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	if (!test_check("boot commands load the DSP workload",
		dsp_hw_load_image(DSP_BENCHMARK_IMAGE, sizeof(DSP_BENCHMARK_IMAGE))))
		return test_finish();
	dsp_hw_shared_memory[0] = 0;
	dsp_hw_shared_memory[BENCHMARK_COMMAND_OFFSET] = 0;
	if (!test_check("BRANCH starts the DSP workload", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("DSP workload becomes ready",
		dsp_hw_wait_shared(0, BENCHMARK_READY, BENCHMARK_TIMEOUT_MS)))
		return test_finish();

	dsp_hw_shared_memory[BENCHMARK_COMMAND_OFFSET] = BENCHMARK_RUN;

	uint32_t maximum_us = 0;
	uint32_t checksum = 0;
	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		stopwatch_t start = stopwatch_get();

		for (size_t word = 0; word < GSM_COPY_WORDS; word++)
			checksum += dsp_hw_shared_memory[word];

		uint32_t elapsed_us = stopwatch_elapsed_us(start);
		if (elapsed_us > maximum_us)
			maximum_us = elapsed_us;
	}

	printf("# %u reads: maximum %u us across %u samples, checksum %08X\n",
		(unsigned int) GSM_COPY_WORDS, (unsigned int) maximum_us,
		(unsigned int) LATENCY_SAMPLES, (unsigned int) checksum);
	test_check("the GSM Shared RAM copy fits between TPU INT1 and the next INT0",
		maximum_us <= GSM_IRQ_WINDOW_US);
	test_check("concurrent DSP workload completes",
		dsp_hw_wait_shared(0, BENCHMARK_DONE, BENCHMARK_TIMEOUT_MS));

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
