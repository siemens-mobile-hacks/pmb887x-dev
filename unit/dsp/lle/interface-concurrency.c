#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp/dsp-hw.h"
#ifdef PMB8875
#include "interface-concurrency-8875.inc"
#define DSP_INTERFACE_IMAGE DSP_INTERFACE_IMAGE_8875
#else
#include "interface-concurrency-8876.inc"
#define DSP_INTERFACE_IMAGE DSP_INTERFACE_IMAGE_8876
#endif
#include "test.h"

#define READY_OFFSET 0x0000
#define IRQ_REQUEST_OFFSET 0x0001
#define IRQ_ACK_OFFSET 0x0002
#define BUFFER_OFFSET 0x0100
#define READY_MARKER 0xC101
#define INTERFACE_FLAG BIT(15)
#define INTERFACE_SEMAPHORE BIT(15)
#define LATENCY_SAMPLES 256
#define LATENCY_P95_LIMIT_NS 10000
#define IRQ_TIMEOUT_MS 100

static void check_latency(const char *name, uint32_t samples[LATENCY_SAMPLES]) {
	uint64_t total = 0;

	for (size_t i = 1; i < LATENCY_SAMPLES; i++) {
		uint32_t value = samples[i];
		size_t position = i;

		while (position != 0 && samples[position - 1] > value) {
			samples[position] = samples[position - 1];
			position--;
		}
		samples[position] = value;
	}
	for (size_t i = 0; i < LATENCY_SAMPLES; i++)
		total += samples[i];

	uint32_t minimum = samples[0];
	uint32_t average = (uint32_t) (total / LATENCY_SAMPLES);
	uint32_t median = samples[LATENCY_SAMPLES / 2];
	uint32_t percentile95 = samples[(LATENCY_SAMPLES * 95 + 99) / 100 - 1];
	uint32_t maximum = samples[LATENCY_SAMPLES - 1];
	uint32_t ticks_per_second = stopwatch_ticks_per_s();
	uint32_t percentile95_ns = (uint32_t) ((uint64_t) percentile95 * 1000000000 / ticks_per_second);

	printf("# ARM-DSP-ACCESS,%s,samples=%u,ticks-min=%lu,ticks-avg=%lu,ticks-p50=%lu,ticks-p95=%lu,ticks-max=%lu\n",
		name, LATENCY_SAMPLES, minimum, average, median, percentile95, maximum);
	printf("# ARM-DSP-ACCESS-NS,%s,min=%lu,avg=%lu,p50=%lu,p95=%lu,max=%lu\n",
		name,
		(uint32_t) ((uint64_t) minimum * 1000000000 / ticks_per_second),
		(uint32_t) ((uint64_t) average * 1000000000 / ticks_per_second),
		(uint32_t) ((uint64_t) median * 1000000000 / ticks_per_second),
		percentile95_ns,
		(uint32_t) ((uint64_t) maximum * 1000000000 / ticks_per_second));

	char assertion[96];
	sprintf(assertion, "%s p95 remains below %lu ns", name, (uint32_t) LATENCY_P95_LIMIT_NS);
	test_check(assertion, percentile95_ns <= LATENCY_P95_LIMIT_NS);
}

static bool wait_irq_ack(uint16_t sequence, stopwatch_t start) {
	while (dsp_hw_shared_memory[IRQ_ACK_OFFSET] != sequence && stopwatch_elapsed_ms(start) < IRQ_TIMEOUT_MS)
		test_watchdog_serve();

	return dsp_hw_shared_memory[IRQ_ACK_OFFSET] == sequence;
}

static void test_shared_memory_latency(void) {
	uint32_t samples[LATENCY_SAMPLES];
	uint32_t checksum = 0;

	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		stopwatch_t start = stopwatch_get();

		dsp_hw_shared_memory[BUFFER_OFFSET] = (uint16_t) sample;
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
	}
	check_latency("Shared-write16", samples);
	test_eq_u32("timed Shared RAM writes publish the final value", LATENCY_SAMPLES - 1,
		dsp_hw_shared_memory[BUFFER_OFFSET]);

	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		stopwatch_t start = stopwatch_get();

		checksum += dsp_hw_shared_memory[BUFFER_OFFSET];
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
	}
	check_latency("Shared-read16", samples);
	test_check("timed Shared RAM reads consume the published value", checksum != 0);
}

static void test_communication_flag_latency(void) {
	uint32_t samples[LATENCY_SAMPLES];
	uint32_t checksum = 0;
	bool valid = true;

	DSP_COM_CLEAR = INTERFACE_FLAG;
	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		stopwatch_t start = stopwatch_get();

		DSP_COM_SET = INTERFACE_FLAG;
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
		valid &= (DSP_COM_STATUS & INTERFACE_FLAG) != 0;
		DSP_COM_CLEAR = INTERFACE_FLAG;
	}
	check_latency("COM_SET", samples);
	test_check("COM_SET is visible before any DSP action", valid);

	valid = true;
	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		DSP_COM_SET = INTERFACE_FLAG;
		stopwatch_t start = stopwatch_get();

		DSP_COM_CLEAR = INTERFACE_FLAG;
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
		valid &= (DSP_COM_STATUS & INTERFACE_FLAG) == 0;
	}
	check_latency("COM_CLEAR", samples);
	test_check("COM_CLEAR is visible without DSP progress", valid);

	DSP_COM_SET = INTERFACE_FLAG;
	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		stopwatch_t start = stopwatch_get();

		checksum += DSP_COM_STATUS;
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
	}
	check_latency("COM_STATUS", samples);
	test_check("COM_STATUS returns the current latch while DSP is busy", checksum != 0);
	DSP_COM_CLEAR = INTERFACE_FLAG;
}

static void test_semaphore_latency(void) {
	uint32_t samples[LATENCY_SAMPLES];
	uint32_t checksum = 0;
	bool valid = true;

	DSP_SEM_CLEAR = INTERFACE_SEMAPHORE;
	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		stopwatch_t start = stopwatch_get();

		DSP_SEM_SET = INTERFACE_SEMAPHORE;
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
		valid &= (DSP_SEM_STATUS & INTERFACE_SEMAPHORE) == 0;
		DSP_SEM_CLEAR = INTERFACE_SEMAPHORE;
	}
	check_latency("SEM_SET", samples);
	test_check("SEM_SET acquires a free semaphore without DSP progress", valid);

	valid = true;
	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		DSP_SEM_SET = INTERFACE_SEMAPHORE;
		stopwatch_t start = stopwatch_get();

		DSP_SEM_CLEAR = INTERFACE_SEMAPHORE;
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
		valid &= (DSP_SEM_STATUS & INTERFACE_SEMAPHORE) != 0;
	}
	check_latency("SEM_CLEAR", samples);
	test_check("SEM_CLEAR releases an owned semaphore without DSP progress", valid);

	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		stopwatch_t start = stopwatch_get();

		checksum += DSP_SEM_STATUS;
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
	}
	check_latency("SEM_STATUS", samples);
	test_check("SEM_STATUS returns the current latch while DSP is busy", checksum != 0);
}

static void test_interrupt_assert_latency(void) {
	uint32_t samples[LATENCY_SAMPLES];
	uint16_t sequence = 0x4000;
	bool completed = true;

	for (size_t sample = 0; sample < LATENCY_SAMPLES; sample++) {
		dsp_hw_shared_memory[IRQ_REQUEST_OFFSET] = ++sequence;
		stopwatch_t start = stopwatch_get();

		SCU_DSP_INT = BIT(0);
		samples[sample] = (uint32_t) stopwatch_elapsed(start);
		SCU_DSP_INT = 0;
		completed &= wait_irq_ack(sequence, start);
	}
	check_latency("MCU0-assert", samples);
	test_check("every MCU0 assertion returns before its later DSP acknowledgement", completed);
}

int main(void) {
	test_start("DSP asynchronous interface concurrency test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	DSP_SEM_CLEAR = UINT16_MAX;
	dsp_hw_shared_memory[READY_OFFSET] = 0;
	dsp_hw_shared_memory[IRQ_REQUEST_OFFSET] = 0;
	dsp_hw_shared_memory[IRQ_ACK_OFFSET] = 0;
	if (!test_check("boot commands load the interface workload",
		dsp_hw_load_image(DSP_INTERFACE_IMAGE, sizeof(DSP_INTERFACE_IMAGE)))) {
		return test_finish();
	}
	if (!test_check("BRANCH starts the interface workload", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("DSP interface workload becomes ready",
		dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, 100))) {
		return test_finish();
	}

	test_category("Shared RAM access while DSP executes");
	test_shared_memory_latency();
	test_category("Communication flags while DSP executes");
	test_communication_flag_latency();
	test_category("Semaphores while DSP executes");
	test_semaphore_latency();
	test_category("MCU-to-DSP interrupt publication while DSP executes");
	test_interrupt_assert_latency();

	SCU_DSP_INT = 0;
	DSP_COM_CLEAR = UINT16_MAX;
	DSP_SEM_CLEAR = UINT16_MAX;
	return test_finish();
}
