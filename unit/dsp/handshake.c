#include <pmb887x.h>
#include <stopwatch.h>

#include "test.h"

#define DSP_BOOT_PLOAD 0
#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_PREAD 3
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_MAX_WORDS 507
#define DSP_WAIT_ITERATIONS 1000000
#define DSP_PROGRAM_ADDRESS 0x0400
#define DSP_PATTERN_A_BASE 0x1000
#define DSP_PATTERN_B_BASE 0x8000
#define DSP_SHARED_TARGET_OFFSET 0x0300

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

static bool wait_for_boot_ready(uint32_t *polls, stopwatch_t *elapsed) {
	stopwatch_t start = stopwatch_get();

	*polls = 0;
	while (*polls < DSP_WAIT_ITERATIONS) {
		(*polls)++;
		if ((DSP_COM_STATUS & BIT(0)) == 0) {
			*elapsed = stopwatch_elapsed(start);
			return true;
		}
		if ((*polls & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	*elapsed = stopwatch_elapsed(start);
	return false;
}

static bool reset_dsp(void) {
	uint32_t polls;
	stopwatch_t elapsed;

	DSP_COM_CLEAR = UINT16_MAX;
	SCU_DSP_INT = 0;
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	uint32_t reset_readback = SCU_RST_REQ;
	SCU_RST_REQ = 0;
	(void) reset_readback;

	return wait_for_boot_ready(&polls, &elapsed);
}

static bool submit_boot_command(uint32_t interrupt_delay_us, uint32_t *polls, stopwatch_t *elapsed) {
	DSP_COM_SET = BIT(0);
	if (interrupt_delay_us != 0)
		stopwatch_usleep_wd(interrupt_delay_us);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;

	return wait_for_boot_ready(polls, elapsed);
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
	uint32_t polls;
	stopwatch_t elapsed;

	boot_data[0] = DSP_BOOT_PREAD;
	boot_data[1] = DSP_PROGRAM_ADDRESS;
	boot_data[2] = (uint16_t) words;
	if (!submit_boot_command(0, &polls, &elapsed))
		return false;
	stopwatch_usleep_wd(1000);

	return true;
}

static void run_race(size_t words, uint32_t interrupt_delay_us) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	volatile uint16_t *result = boot_data + 3;
	uint32_t polls;
	stopwatch_t elapsed;

	boot_data[0] = DSP_BOOT_PLOAD;
	boot_data[1] = DSP_PROGRAM_ADDRESS;
	boot_data[2] = (uint16_t) words;
	write_payload(DSP_PATTERN_A_BASE, words);
	if (!test_check("PLOAD ACK is received", submit_boot_command(interrupt_delay_us, &polls, &elapsed)))
		return;
	uint32_t ack_polls = polls;
	uint32_t ack_ticks = (uint32_t) elapsed;
	uint32_t ack_us = ack_ticks / stopwatch_ticks_per_us();

	write_payload(DSP_PATTERN_B_BASE, words);
	stopwatch_usleep_wd(1000);
	if (!test_check("PREAD after ACK race completes", read_program(words)))
		return;

	size_t pattern_a = 0;
	size_t pattern_b = 0;
	size_t other = 0;
	for (size_t i = 0; i < words; i++) {
		if (result[i] == pattern_word(DSP_PATTERN_A_BASE, i)) {
			pattern_a++;
		} else if (result[i] == pattern_word(DSP_PATTERN_B_BASE, i)) {
			pattern_b++;
		} else {
			other++;
		}
	}

	printf("# words=%u SET-to-IRQ=%u us ACK polls=%u ticks=%u us=%u result: before-ACK=%u after-ACK=%u other=%u\n",
		(uint32_t) words, interrupt_delay_us, ack_polls, ack_ticks, ack_us,
		(uint32_t) pattern_a, (uint32_t) pattern_b, (uint32_t) other);
}

static void observe_dload(void) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;
	volatile uint16_t *target = DSP_SHARED_MEMORY + DSP_SHARED_TARGET_OFFSET;
	uint16_t mask_id = DSP_SHARED_MEMORY[0];
	uint16_t shared_base = (mask_id & 0xFF00) == 0x0600 ? 0xD800 : 0xD000;
	uint16_t first_expected = pattern_word(DSP_PATTERN_A_BASE, 0);
	uint16_t last_expected = pattern_word(DSP_PATTERN_A_BASE, DSP_BOOT_MAX_WORDS - 1);
	bool first_seen_while_busy = false;
	bool last_seen_while_busy = false;
	uint32_t polls = 0;

	for (size_t i = 0; i < DSP_BOOT_MAX_WORDS; i++)
		target[i] = 0;
	boot_data[0] = DSP_BOOT_DLOAD;
	boot_data[1] = shared_base + DSP_SHARED_TARGET_OFFSET;
	boot_data[2] = DSP_BOOT_MAX_WORDS;
	write_payload(DSP_PATTERN_A_BASE, DSP_BOOT_MAX_WORDS);

	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;
	while (polls < DSP_WAIT_ITERATIONS) {
		uint32_t status = DSP_COM_STATUS;
		uint16_t first = target[0];
		uint16_t last = target[DSP_BOOT_MAX_WORDS - 1];

		polls++;
		if ((status & BIT(0)) != 0) {
			first_seen_while_busy |= first == first_expected;
			last_seen_while_busy |= last == last_expected;
		} else {
			break;
		}
	}

	printf("# DLOAD shared: polls=%u first-while-busy=%u last-while-busy=%u final-first=%04X final-last=%04X\n",
		polls, first_seen_while_busy, last_seen_while_busy, target[0], target[DSP_BOOT_MAX_WORDS - 1]);
	test_check("DLOAD destination is complete at ACK", target[0] == first_expected &&
		target[DSP_BOOT_MAX_WORDS - 1] == last_expected);
}

int main(void) {
	test_start("DSP boot handshake timing test");

	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("DSP Mask ROM boot dispatcher becomes ready", reset_dsp()))
		return test_finish();

	test_category("ACK versus shared command buffer consumption");
	run_race(1, 0);
	run_race(8, 0);
	run_race(DSP_BOOT_MAX_WORDS, 0);
	run_race(DSP_BOOT_MAX_WORDS, 10);
	run_race(DSP_BOOT_MAX_WORDS, 100);
	observe_dload();

	return test_finish();
}
