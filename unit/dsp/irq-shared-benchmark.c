#include <pmb887x.h>
#include <gen/dsp.h>
#include <stopwatch.h>

#include "dsp-hw.h"
#include "irq-shared-benchmark-8876.inc"
#include "test.h"

#define READY_OFFSET 0x0000
#define COMMAND_OFFSET 0x0001
#define DONE_OFFSET 0x0002
#define IRQ_REQUEST_OFFSET 0x0003
#define IRQ_ACK_OFFSET 0x0004
#define IRQ_ENTRY_BASE 0x0010
#define IRQ_ROUND_TRIP_TICKS_OFFSET 0x0018
#define IRQ_ROUND_TRIP_COUNT_OFFSET 0x0019
#define MEMORY_RESULTS_BASE 0x0020
#define BUFFER_OFFSET 0x0100
#define BUFFER_WORDS 1024
#define DSP_MEMORY_OPERATIONS 65536
#define ARM_MEMORY_OPERATIONS 262144
#define ARM_IRQ_SAMPLES 128
#define ARM_DSP_IRQ_CHANNELS 3
#define DSP_IRQ_ROUND_TRIPS 4096
#define TIMER2_DIVIDER 96
#define DSP_CLOCK_HZ 104000000
#define READY_MARKER 0xB101
#define BENCHMARK_SEQUENCE 1
#define BENCHMARK_TIMEOUT_MS 1000

static const char *const IRQ_NAMES[] = { "INT0/A0", "INT0/B0", "INT1", "INT2" };

static uint16_t initial_pattern(size_t index) {
	return 0xA5A5 ^ (uint16_t) (index * 0x31U);
}

static void print_irq_entry_results(void) {
	for (size_t i = 0; i < ARRAY_SIZE(IRQ_NAMES); i++) {
		uint16_t retired = dsp_hw_shared_memory[IRQ_ENTRY_BASE + i * 2];
		uint16_t handlers = dsp_hw_shared_memory[IRQ_ENTRY_BASE + i * 2 + 1];

		printf("# DSP-IRQ-ENTRY,%s,instructions-after-SINT=%u,handlers=%u\n",
			IRQ_NAMES[i], (uint32_t) retired, (uint32_t) handlers);
		char name[80];
		tfp_sprintf(name, "%s reaches its vector inside the probe window", IRQ_NAMES[i]);
		test_check(name, retired <= 16);
		tfp_sprintf(name, "%s enters exactly one handler", IRQ_NAMES[i]);
		test_eq_u32(name, 1, handlers);
	}
}

static void print_dsp_irq_round_trip(void) {
	uint32_t ticks = dsp_hw_shared_memory[IRQ_ROUND_TRIP_TICKS_OFFSET];
	uint32_t count = dsp_hw_shared_memory[IRQ_ROUND_TRIP_COUNT_OFFSET];

	if (!test_check("Timer2 observes the sustained IRQ benchmark", ticks != 0))
		return;

	uint32_t cycles = ticks * TIMER2_DIVIDER;
	uint32_t cycles_milli = cycles * 1000 / DSP_IRQ_ROUND_TRIPS;

	printf("# DSP-IRQ-ROUNDTRIP,requests=%u,timer2-ticks=%u,cycles-per-request=%u.%03u\n",
		DSP_IRQ_ROUND_TRIPS, ticks, cycles_milli / 1000, cycles_milli % 1000);
	test_eq_u32("every sustained synthetic IRQ enters the handler", DSP_IRQ_ROUND_TRIPS, count);
}

static void print_dsp_memory_score(const char *name, uint16_t ticks) {
	if (ticks == 0) {
		printf("# DSP-MEMORY,%s,words=%u,timer2-ticks=0\n", name, DSP_MEMORY_OPERATIONS);
		return;
	}

	uint32_t cycles = (uint32_t) ticks * TIMER2_DIVIDER;
	uint32_t cycles_milli = cycles * 1000 / DSP_MEMORY_OPERATIONS;
	uint32_t mwords_milli = (uint32_t) ((uint64_t) DSP_MEMORY_OPERATIONS * (DSP_CLOCK_HZ / 1000) / cycles);

	printf("# DSP-MEMORY,%s,words=%u,timer2-ticks=%u,cycles-per-word=%u.%03u,Mword/s=%u.%03u\n",
		name, DSP_MEMORY_OPERATIONS, (uint32_t) ticks, cycles_milli / 1000, cycles_milli % 1000,
		mwords_milli / 1000, mwords_milli % 1000);
}

static void check_dsp_memory_results(void) {
	uint16_t xram_read_ticks = dsp_hw_shared_memory[MEMORY_RESULTS_BASE];
	uint16_t shared_read_ticks = dsp_hw_shared_memory[MEMORY_RESULTS_BASE + 2];
	uint16_t xram_write_ticks = dsp_hw_shared_memory[MEMORY_RESULTS_BASE + 4];
	uint16_t shared_write_ticks = dsp_hw_shared_memory[MEMORY_RESULTS_BASE + 6];

	print_dsp_memory_score("XRAM-read16", xram_read_ticks);
	print_dsp_memory_score("Shared-read16", shared_read_ticks);
	print_dsp_memory_score("XRAM-write16", xram_write_ticks);
	print_dsp_memory_score("Shared-write16", shared_write_ticks);
	test_check("XRAM read benchmark advances Timer2", xram_read_ticks != 0);
	test_check("Shared RAM read benchmark advances Timer2", shared_read_ticks != 0);
	test_check("XRAM write benchmark advances Timer2", xram_write_ticks != 0);
	test_check("Shared RAM write benchmark advances Timer2", shared_write_ticks != 0);
	test_eq_u32("DSP reads the final initialized Shared RAM word", initial_pattern(BUFFER_WORDS - 1),
		dsp_hw_shared_memory[MEMORY_RESULTS_BASE + 3]);
	test_eq_u32("DSP reads back the final Shared RAM write", 0x5A5A,
		dsp_hw_shared_memory[MEMORY_RESULTS_BASE + 7]);

	bool written = true;
	for (size_t i = 0; i < BUFFER_WORDS; i++)
		written &= dsp_hw_shared_memory[BUFFER_OFFSET + i] == 0x5A5A;
	test_check("DSP writes the complete Shared RAM benchmark window", written);
}

static bool wait_irq_ack(uint16_t sequence, stopwatch_t start) {
	while (dsp_hw_shared_memory[IRQ_ACK_OFFSET] != sequence && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return dsp_hw_shared_memory[IRQ_ACK_OFFSET] == sequence;
}

static void benchmark_arm_to_dsp_irq(size_t channel) {
	uint32_t samples[ARM_IRQ_SAMPLES];
	uint64_t total = 0;
	uint16_t sequence = 0x1000 + (uint16_t) (channel * ARM_IRQ_SAMPLES);
	bool completed = true;

	for (size_t sample = 0; sample < ARM_IRQ_SAMPLES; sample++) {
		dsp_hw_shared_memory[IRQ_REQUEST_OFFSET] = ++sequence;
		stopwatch_t start = stopwatch_get();
		SCU_DSP_INT = BIT(channel);
		SCU_DSP_INT = 0;
		completed &= wait_irq_ack(sequence, start);
		uint32_t elapsed = (uint32_t) stopwatch_elapsed(start);

		samples[sample] = elapsed;
		total += elapsed;
		if (!completed)
			break;
	}

	char name[72];
	tfp_sprintf(name, "MCU%u completes every ARM-to-DSP latency sample", (uint32_t) channel);
	test_check(name, completed);
	if (!completed)
		return;

	for (size_t i = 1; i < ARM_IRQ_SAMPLES; i++) {
		uint32_t value = samples[i];
		size_t position = i;

		while (position != 0 && samples[position - 1] > value) {
			samples[position] = samples[position - 1];
			position--;
		}
		samples[position] = value;
	}

	uint32_t minimum = samples[0];
	uint32_t average = (uint32_t) (total / ARM_IRQ_SAMPLES);
	uint32_t median = samples[ARM_IRQ_SAMPLES / 2];
	uint32_t percentile95 = samples[(ARM_IRQ_SAMPLES * 95 + 99) / 100 - 1];
	uint32_t maximum = samples[ARM_IRQ_SAMPLES - 1];
	uint32_t ticks_per_second = stopwatch_ticks_per_s();
	uint32_t minimum_ns = (uint32_t) ((uint64_t) minimum * 1000000000 / ticks_per_second);
	uint32_t average_ns = (uint32_t) ((uint64_t) average * 1000000000 / ticks_per_second);
	uint32_t median_ns = (uint32_t) ((uint64_t) median * 1000000000 / ticks_per_second);
	uint32_t percentile95_ns = (uint32_t) ((uint64_t) percentile95 * 1000000000 / ticks_per_second);
	uint32_t maximum_ns = (uint32_t) ((uint64_t) maximum * 1000000000 / ticks_per_second);

	printf("# ARM-DSP-IRQ,MCU%u,samples=%u,ticks-min=%u,ticks-avg=%u,ticks-p50=%u,ticks-p95=%u,ticks-max=%u\n",
		(uint32_t) channel, ARM_IRQ_SAMPLES, minimum, average, median, percentile95, maximum);
	printf("# ARM-DSP-IRQ-NS,MCU%u,min=%u,avg=%u,p50=%u,p95=%u,max=%u\n",
		(uint32_t) channel, minimum_ns, average_ns, median_ns, percentile95_ns, maximum_ns);
}

static void print_arm_memory_score(const char *name, size_t operations, size_t bytes, stopwatch_t elapsed, uint32_t checksum) {
	if (elapsed == 0) {
		printf("# ARM-MEMORY,%s,accesses=%u,ticks=0,checksum=%08X\n",
			name, (uint32_t) operations, checksum);
		return;
	}

	uint64_t bytes_per_second = (uint64_t) operations * bytes * stopwatch_ticks_per_s() / elapsed;
	uint32_t mib_milli = (uint32_t) (bytes_per_second * 1000 / (1024 * 1024));
	uint32_t ns_per_access = (uint32_t) ((uint64_t) elapsed * 1000000000 / stopwatch_ticks_per_s() / operations);

	printf("# ARM-MEMORY,%s,accesses=%u,ticks=%u,MiB/s=%u.%03u,ns/access=%u,checksum=%08X\n",
		name, (uint32_t) operations, (uint32_t) elapsed, mib_milli / 1000, mib_milli % 1000,
		ns_per_access, checksum);
}

static void benchmark_arm_shared_memory(void) {
	volatile uint16_t *words16 = dsp_hw_shared_memory + BUFFER_OFFSET;
	volatile uint32_t *words32 = (volatile uint32_t *) words16;
	const size_t dwords = BUFFER_WORDS / 2;
	uint32_t checksum = 0;

	stopwatch_t start = stopwatch_get();
	for (size_t i = 0; i < ARM_MEMORY_OPERATIONS; i++)
		checksum += words16[i & (BUFFER_WORDS - 1)];
	stopwatch_t elapsed = stopwatch_elapsed(start);
	print_arm_memory_score("Shared-read16", ARM_MEMORY_OPERATIONS, sizeof(uint16_t), elapsed, checksum);

	start = stopwatch_get();
	for (size_t i = 0; i < ARM_MEMORY_OPERATIONS; i++)
		words16[i & (BUFFER_WORDS - 1)] = (uint16_t) i;
	elapsed = stopwatch_elapsed(start);
	print_arm_memory_score("Shared-write16", ARM_MEMORY_OPERATIONS, sizeof(uint16_t), elapsed, 0);

	bool write16_valid = true;
	for (size_t i = 0; i < BUFFER_WORDS; i++)
		write16_valid &= words16[i] == (uint16_t) (ARM_MEMORY_OPERATIONS - BUFFER_WORDS + i);
	test_check("ARM 16-bit benchmark leaves the expected Shared RAM pattern", write16_valid);

	checksum = 0;
	start = stopwatch_get();
	for (size_t i = 0; i < ARM_MEMORY_OPERATIONS; i++)
		checksum += words32[i & (dwords - 1)];
	elapsed = stopwatch_elapsed(start);
	print_arm_memory_score("Shared-read32", ARM_MEMORY_OPERATIONS, sizeof(uint32_t), elapsed, checksum);

	start = stopwatch_get();
	for (size_t i = 0; i < ARM_MEMORY_OPERATIONS; i++)
		words32[i & (dwords - 1)] = (uint32_t) i ^ 0xA55A5AA5;
	elapsed = stopwatch_elapsed(start);
	print_arm_memory_score("Shared-write32", ARM_MEMORY_OPERATIONS, sizeof(uint32_t), elapsed, 0);

	bool write32_valid = true;
	for (size_t i = 0; i < dwords; i++) {
		uint32_t expected = (uint32_t) (ARM_MEMORY_OPERATIONS - dwords + i) ^ 0xA55A5AA5;

		write32_valid &= words32[i] == expected;
	}
	test_check("ARM 32-bit benchmark leaves the expected Shared RAM pattern", write32_valid);
}

int main(void) {
	test_start("DSP IRQ latency and Shared RAM benchmark");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("DSP benchmark setup");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();

	DSP_COM_CLEAR = 0xFFFF;
	bool loaded = dsp_hw_load_image(DSP_IRQ_SHARED_BENCHMARK_8876, sizeof(DSP_IRQ_SHARED_BENCHMARK_8876));
	if (!test_check("boot commands load the benchmark runner", loaded))
		return test_finish();

	for (size_t i = 0; i < BUFFER_WORDS; i++)
		dsp_hw_shared_memory[BUFFER_OFFSET + i] = initial_pattern(i);
	dsp_hw_shared_memory[COMMAND_OFFSET] = 0;
	dsp_hw_shared_memory[DONE_OFFSET] = 0;
	dsp_hw_shared_memory[IRQ_REQUEST_OFFSET] = 0;
	dsp_hw_shared_memory[IRQ_ACK_OFFSET] = 0;
	if (!test_check("BRANCH starts the benchmark runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();

	bool ready = dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, BENCHMARK_TIMEOUT_MS);
	if (!test_check("benchmark runner becomes ready", ready))
		return test_finish();

	test_category("DSP-side IRQ and memory measurements");
	dsp_hw_shared_memory[COMMAND_OFFSET] = BENCHMARK_SEQUENCE;
	bool completed = dsp_hw_wait_shared(DONE_OFFSET, BENCHMARK_SEQUENCE, BENCHMARK_TIMEOUT_MS);
	if (!test_check("DSP-side benchmark completes", completed))
		return test_finish();

	print_irq_entry_results();
	print_dsp_irq_round_trip();
	check_dsp_memory_results();

	test_category("ARM-to-DSP interrupt notification latency");
	for (size_t channel = 0; channel < ARM_DSP_IRQ_CHANNELS; channel++)
		benchmark_arm_to_dsp_irq(channel);

	test_category("ARM Shared RAM throughput");
	benchmark_arm_shared_memory();

	SCU_DSP_INT = 0;
	DSP_COM_CLEAR = 0xFFFF;
	(void) dsp_hw_reset();
	return test_finish();
}
