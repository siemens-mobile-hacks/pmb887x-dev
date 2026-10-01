#include <pmb887x.h>
#include <stopwatch.h>

#include "test.h"

#define DSP_BOOT_PLOAD 0
#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_PREAD 3
#define DSP_BOOT_DREAD 4
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_FIRMWARE_RECORD_WORDS 509
#define DSP_BOOT_TIMING_SAMPLES 64
// SL98 waits this many empty loop iterations after a DSP reset pulse and runtime startup.
#define DSP_FIRMWARE_DELAY_ITERATIONS 2000
// SL98 dsp_submit_command() permits this many DSP_COM_STATUS reads before asserting.
#define DSP_FIRMWARE_POLL_LIMIT 1000
#define DSP_WAIT_ITERATIONS 1000000
#define DSP_PENDING_OBSERVATION_US 100
#define DSP_PROGRAM_ADDRESS 0x0400
#define DSP_PATTERN_A_BASE 0x1000
#define DSP_PATTERN_B_BASE 0x8000
#define DSP_RESULT_SENTINEL 0xA55A
#define DSP_SHARED_TARGET_OFFSET 0x0300

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

struct timing_stats {
	uint32_t minimum;
	uint32_t maximum;
	uint64_t total;
	uint32_t samples;
	size_t maximum_polls;
};

static void timing_stats_add(struct timing_stats *stats, uint32_t elapsed, size_t polls) {
	if (stats->samples == 0 || elapsed < stats->minimum)
		stats->minimum = elapsed;
	if (elapsed > stats->maximum)
		stats->maximum = elapsed;
	stats->total += elapsed;
	stats->samples++;
	if (polls > stats->maximum_polls)
		stats->maximum_polls = polls;
}

static void timing_stats_print(const char *operation, size_t words, const struct timing_stats *stats) {
	if (stats->samples == 0) {
		printf("# %s words=%lu: no completed samples\n", operation, (uint32_t) words);
		return;
	}

	printf("# %s words=%lu: samples=%lu ticks min=%lu mean=%lu max=%lu polls_max=%lu\n",
		operation, (uint32_t) words, stats->samples, stats->minimum,
		(uint32_t) (stats->total / stats->samples), stats->maximum, (uint32_t) stats->maximum_polls);
}

static bool is_mask_rom_ready(void) {
	uint16_t mask_family = DSP_SHARED_MEMORY[0] & 0xFF00;

	return (mask_family == 0x0600 || mask_family == 0x0800) && (DSP_COM_STATUS & BIT(0)) == 0;
}

static bool wait_for_boot_ready(
	stopwatch_t start,
	bool wait_for_mask_id,
	stopwatch_t *elapsed,
	size_t *polls
) {
	for (size_t poll = 0; poll < DSP_WAIT_ITERATIONS; poll++) {
		bool ready = wait_for_mask_id ? is_mask_rom_ready() : (DSP_COM_STATUS & BIT(0)) == 0;

		if (ready) {
			*elapsed = stopwatch_elapsed(start);
			*polls = poll + 1;
			return true;
		}
		if ((poll & 0x3FFF) == 0x3FFF)
			test_watchdog_serve();
	}

	*elapsed = stopwatch_elapsed(start);
	*polls = DSP_WAIT_ITERATIONS;
	return false;
}

static bool pulse_dsp_reset(stopwatch_t *elapsed, size_t *polls) {
	DSP_SHARED_MEMORY[0] = 0;
	stopwatch_t start = stopwatch_get();
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	SCU_RST_REQ = 0;

	return wait_for_boot_ready(start, true, elapsed, polls);
}

static bool reset_dsp(stopwatch_t *elapsed, size_t *polls) {
	DSP_COM_CLEAR = UINT16_MAX;
	SCU_DSP_INT = 0;

	return pulse_dsp_reset(elapsed, polls);
}

static bool submit_boot_command(
	uint32_t interrupt_delay_us,
	bool *pending_before_interrupt,
	stopwatch_t *elapsed,
	size_t *polls
) {
	DSP_COM_SET = BIT(0);
	if (interrupt_delay_us != 0) {
		stopwatch_usleep_wd(interrupt_delay_us);
		*pending_before_interrupt &= (DSP_COM_STATUS & BIT(0)) != 0;
	}
	stopwatch_t start = stopwatch_get();
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;

	return wait_for_boot_ready(start, false, elapsed, polls);
}

static uint16_t pattern_word(uint16_t base, size_t index) {
	return base ^ (uint16_t) (index * 0x31U);
}

static void write_payload(uint16_t base, size_t words) {
	volatile uint16_t *payload = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET + 3;

	for (size_t i = 0; i < words; i++)
		payload[i] = pattern_word(base, i);
}

static bool read_program(size_t words) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	stopwatch_t elapsed;
	size_t polls;
	bool pending_before_interrupt = true;

	boot_data[0] = DSP_BOOT_PREAD;
	boot_data[1] = DSP_PROGRAM_ADDRESS;
	boot_data[2] = (uint16_t) words;
	return submit_boot_command(0, &pending_before_interrupt, &elapsed, &polls);
}

static void test_firmware_poll_budget(const char *operation, size_t words, const struct timing_stats *stats) {
	char name[104];

	sprintf(name, "%s of %lu words completes within firmware's 1000-poll wait",
		operation, (uint32_t) words);
	test_check(name, stats->samples != 0 && stats->maximum_polls <= DSP_FIRMWARE_POLL_LIMIT);
}

static void firmware_delay(void) {
	for (size_t i = 0; i < DSP_FIRMWARE_DELAY_ITERATIONS; i++)
		__asm__ volatile("" : : : "memory");
}

static uint16_t shared_data_address(void) {
	uint16_t mask_family = DSP_SHARED_MEMORY[0] & 0xFF00;

	return (mask_family == 0x0600 ? 0xD800 : 0xD000) + DSP_SHARED_TARGET_OFFSET;
}

static void test_pload_ack(size_t words, uint32_t interrupt_delay_us, size_t samples) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	volatile uint16_t *result = boot_data + 3;
	struct timing_stats stats = { 0 };
	bool pending_before_interrupt = true;
	bool completed = true;
	bool source_consumed = true;

	for (size_t sample = 0; sample < samples; sample++) {
		uint16_t pattern_a = DSP_PATTERN_A_BASE ^ (uint16_t) sample;
		uint16_t pattern_b = DSP_PATTERN_B_BASE ^ (uint16_t) sample;
		stopwatch_t elapsed;
		size_t polls;

		boot_data[0] = DSP_BOOT_PLOAD;
		boot_data[1] = DSP_PROGRAM_ADDRESS;
		boot_data[2] = (uint16_t) words;
		write_payload(pattern_a, words);
		bool load_complete = submit_boot_command(interrupt_delay_us, &pending_before_interrupt, &elapsed, &polls);
		completed &= load_complete;
		timing_stats_add(&stats, (uint32_t) elapsed, polls);
		if (!load_complete)
			break;

		write_payload(pattern_b, words);
		if (!read_program(words)) {
			completed = false;
			continue;
		}
		for (size_t i = 0; i < words; i++)
			source_consumed &= result[i] == pattern_word(pattern_a, i);
	}

	char name[96];
	sprintf(name, "PLOAD of %lu words completes every sample", (uint32_t) words);
	test_check(name, completed);
	sprintf(name, "PLOAD of %lu words has consumed its complete source at ACK", (uint32_t) words);
	test_check(name, source_consumed);
	if (interrupt_delay_us != 0) {
		sprintf(name, "PLOAD remains pending for %lu us without an interrupt", interrupt_delay_us);
		test_check(name, pending_before_interrupt);
	}
	timing_stats_print("PLOAD IRQ-to-ACK", words, &stats);
	test_firmware_poll_budget("PLOAD", words, &stats);
}

static void test_dload_ack(size_t words) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	volatile uint16_t *target = DSP_SHARED_MEMORY + DSP_SHARED_TARGET_OFFSET;
	uint16_t mask_id = DSP_SHARED_MEMORY[0];
	uint16_t shared_base = (mask_id & 0xFF00) == 0x0600 ? 0xD800 : 0xD000;
	struct timing_stats stats = { 0 };
	bool completed = true;
	bool destination_complete = true;
	bool pending_before_interrupt = true;

	for (size_t sample = 0; sample < DSP_BOOT_TIMING_SAMPLES; sample++) {
		uint16_t pattern = DSP_PATTERN_A_BASE ^ (uint16_t) sample;
		stopwatch_t elapsed;
		size_t polls;

		for (size_t i = 0; i < words; i++)
			target[i] = 0;
		boot_data[0] = DSP_BOOT_DLOAD;
		boot_data[1] = shared_base + DSP_SHARED_TARGET_OFFSET;
		boot_data[2] = (uint16_t) words;
		write_payload(pattern, words);
		bool load_complete = submit_boot_command(0, &pending_before_interrupt, &elapsed, &polls);
		completed &= load_complete;
		timing_stats_add(&stats, (uint32_t) elapsed, polls);
		if (!load_complete)
			break;

		for (size_t i = 0; i < words; i++)
			destination_complete &= target[i] == pattern_word(pattern, i);
	}

	char name[96];
	sprintf(name, "DLOAD of %lu words completes every sample", (uint32_t) words);
	test_check(name, completed);
	sprintf(name, "DLOAD of %lu words publishes its complete destination at ACK", (uint32_t) words);
	test_check(name, destination_complete);
	timing_stats_print("DLOAD IRQ-to-ACK", words, &stats);
	test_firmware_poll_budget("DLOAD", words, &stats);
}

static void test_pread_ack(size_t words) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	volatile uint16_t *result = boot_data + 3;
	struct timing_stats stats = { 0 };
	bool completed = true;
	bool result_complete = true;
	bool pending_before_interrupt = true;

	for (size_t sample = 0; sample < DSP_BOOT_TIMING_SAMPLES; sample++) {
		uint16_t pattern = DSP_PATTERN_A_BASE ^ (uint16_t) sample;
		stopwatch_t elapsed;
		size_t polls;

		boot_data[0] = DSP_BOOT_PLOAD;
		boot_data[1] = DSP_PROGRAM_ADDRESS;
		boot_data[2] = (uint16_t) words;
		write_payload(pattern, words);
		if (!submit_boot_command(0, &pending_before_interrupt, &elapsed, &polls)) {
			completed = false;
			break;
		}

		boot_data[0] = DSP_BOOT_PREAD;
		boot_data[1] = DSP_PROGRAM_ADDRESS;
		boot_data[2] = (uint16_t) words;
		for (size_t i = 0; i < words; i++)
			result[i] = DSP_RESULT_SENTINEL;
		bool read_complete = submit_boot_command(0, &pending_before_interrupt, &elapsed, &polls);
		completed &= read_complete;
		timing_stats_add(&stats, (uint32_t) elapsed, polls);
		if (!read_complete)
			break;

		for (size_t i = 0; i < words; i++)
			result_complete &= result[i] == pattern_word(pattern, i);
	}

	char name[96];
	sprintf(name, "PREAD of %lu words completes every sample", (uint32_t) words);
	test_check(name, completed);
	sprintf(name, "PREAD of %lu words publishes its complete result at ACK", (uint32_t) words);
	test_check(name, result_complete);
	timing_stats_print("PREAD IRQ-to-ACK", words, &stats);
	test_firmware_poll_budget("PREAD", words, &stats);
}

static void test_dread_ack(size_t words) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	volatile uint16_t *result = boot_data + 3;
	volatile uint16_t *source = DSP_SHARED_MEMORY + DSP_SHARED_TARGET_OFFSET;
	struct timing_stats stats = { 0 };
	bool completed = true;
	bool result_complete = true;
	bool pending_before_interrupt = true;

	for (size_t sample = 0; sample < DSP_BOOT_TIMING_SAMPLES; sample++) {
		uint16_t pattern = DSP_PATTERN_B_BASE ^ (uint16_t) sample;
		stopwatch_t elapsed;
		size_t polls;

		for (size_t i = 0; i < words; i++)
			source[i] = pattern_word(pattern, i);
		boot_data[0] = DSP_BOOT_DREAD;
		boot_data[1] = shared_data_address();
		boot_data[2] = (uint16_t) words;
		for (size_t i = 0; i < words; i++)
			result[i] = DSP_RESULT_SENTINEL;
		bool read_complete = submit_boot_command(0, &pending_before_interrupt, &elapsed, &polls);
		completed &= read_complete;
		timing_stats_add(&stats, (uint32_t) elapsed, polls);
		if (!read_complete)
			break;

		for (size_t i = 0; i < words; i++)
			result_complete &= result[i] == pattern_word(pattern, i);
	}

	char name[96];
	sprintf(name, "DREAD of %lu words completes every sample", (uint32_t) words);
	test_check(name, completed);
	sprintf(name, "DREAD of %lu words publishes its complete result at ACK", (uint32_t) words);
	test_check(name, result_complete);
	timing_stats_print("DREAD IRQ-to-ACK", words, &stats);
	test_firmware_poll_budget("DREAD", words, &stats);
}

static void test_rejected_boot_command(uint16_t command) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	uint16_t expected[] = { command, 0x1357, 0x2468, 0x369C, 0x48AD, 0x5ABE };
	stopwatch_t elapsed;
	size_t polls;
	bool pending_before_interrupt = true;

	for (size_t i = 0; i < ARRAY_SIZE(expected); i++)
		boot_data[i] = expected[i];
	bool completed = submit_boot_command(0, &pending_before_interrupt, &elapsed, &polls);

	char name[96];
	sprintf(name, "rejected boot command %lu returns to the dispatcher", (uint32_t) command);
	test_check(name, completed);
	sprintf(name, "rejected boot command %lu completes within firmware's 1000-poll wait", (uint32_t) command);
	test_check(name, completed && polls <= DSP_FIRMWARE_POLL_LIMIT);
	sprintf(name, "rejected boot command %lu leaves its record unchanged", (uint32_t) command);
	test_eq_memory(name, expected, boot_data, sizeof(expected));
}

static void test_reset_cancels_pending_command(void) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	volatile uint16_t *source = DSP_SHARED_MEMORY + DSP_SHARED_TARGET_OFFSET;
	stopwatch_t elapsed;
	size_t polls;

	boot_data[0] = DSP_BOOT_DREAD;
	boot_data[1] = shared_data_address();
	boot_data[2] = 1;
	boot_data[3] = DSP_RESULT_SENTINEL;
	SCU_DSP_INT = 0;
	DSP_COM_SET = BIT(0);
	stopwatch_usleep_wd(DSP_PENDING_OBSERVATION_US);
	test_eq_u32("boot command is pending before reset", BIT(0), DSP_COM_STATUS & BIT(0));

	bool reset_ready = pulse_dsp_reset(&elapsed, &polls);
	test_check("reset cancels a pending boot command and restarts the dispatcher", reset_ready);
	test_eq_u32("reset does not execute the pending boot command", DSP_RESULT_SENTINEL, boot_data[3]);
	if (!reset_ready)
		return;

	source[0] = DSP_PATTERN_A_BASE;
	boot_data[0] = DSP_BOOT_DREAD;
	boot_data[1] = shared_data_address();
	boot_data[2] = 1;
	boot_data[3] = DSP_RESULT_SENTINEL;
	bool pending_before_interrupt = true;
	bool command_complete = submit_boot_command(0, &pending_before_interrupt, &elapsed, &polls);
	test_check("dispatcher accepts a new command after pending-command reset", command_complete);
	test_check("new command after reset completes within firmware's 1000-poll wait",
		command_complete && polls <= DSP_FIRMWARE_POLL_LIMIT);
	test_eq_u32("new command after reset returns its complete result", DSP_PATTERN_A_BASE, boot_data[3]);
}

static void characterize_reset(void) {
	struct timing_stats stats = { 0 };
	bool ready = true;

	for (size_t i = 0; i < DSP_BOOT_TIMING_SAMPLES; i++) {
		stopwatch_t elapsed;
		size_t polls;

		bool reset_ready = reset_dsp(&elapsed, &polls);
		ready &= reset_ready;
		timing_stats_add(&stats, (uint32_t) elapsed, polls);
		if (!reset_ready)
			break;
	}

	test_check("Mask ROM boot dispatcher becomes ready after every reset pulse", ready);
	timing_stats_print("reset-to-ready", 0, &stats);
}

static void test_firmware_reset_delay(void) {
	bool ready = true;

	for (size_t sample = 0; sample < DSP_BOOT_TIMING_SAMPLES; sample++) {
		DSP_COM_CLEAR = UINT16_MAX;
		SCU_DSP_INT = 0;
		DSP_SHARED_MEMORY[0] = 0;
		SCU_RST_REQ = SCU_RST_REQ_DSP;
		SCU_RST_REQ = 0;
		firmware_delay();
		ready &= is_mask_rom_ready();
	}

	test_check("Mask ROM is ready after firmware's 2000-iteration reset delay", ready);
}

int main(void) {
	test_start("DSP Mask ROM boot timing test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("Reset timing");
	characterize_reset();
	test_firmware_reset_delay();

	test_category("PLOAD ACK and source ownership");
	test_pload_ack(1, 0, DSP_BOOT_TIMING_SAMPLES);
	test_pload_ack(8, 0, DSP_BOOT_TIMING_SAMPLES);
	test_pload_ack(DSP_BOOT_FIRMWARE_RECORD_WORDS, 0, DSP_BOOT_TIMING_SAMPLES);
	test_pload_ack(DSP_BOOT_FIRMWARE_RECORD_WORDS, 10, 1);
	test_pload_ack(DSP_BOOT_FIRMWARE_RECORD_WORDS, 100, 1);

	test_category("DLOAD ACK and destination visibility");
	test_dload_ack(1);
	test_dload_ack(8);
	test_dload_ack(DSP_BOOT_FIRMWARE_RECORD_WORDS);

	test_category("PREAD ACK and result visibility");
	test_pread_ack(1);
	test_pread_ack(8);
	test_pread_ack(DSP_BOOT_FIRMWARE_RECORD_WORDS);

	test_category("DREAD ACK and result visibility");
	test_dread_ack(1);
	test_dread_ack(8);
	test_dread_ack(DSP_BOOT_FIRMWARE_RECORD_WORDS);

	test_category("Rejected commands and dispatcher aliases");
	test_rejected_boot_command(6);
	test_rejected_boot_command(7);
	test_rejected_boot_command(14);
	test_rejected_boot_command(15);

	test_category("Reset with a pending boot command");
	test_reset_cancels_pending_command();

	return test_finish();
}
