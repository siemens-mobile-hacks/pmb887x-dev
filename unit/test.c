#include "test.h"

#include <pmb887x.h>

#define TEST_TIMEOUT_MS 3000

#if TEST_COLOR
#define COLOR_RESET "\x1B[0m"
#define COLOR_RED "\x1B[31m"
#define COLOR_GREEN "\x1B[32m"
#define COLOR_YELLOW "\x1B[33m"
#define COLOR_CYAN "\x1B[36m"
#else
#define COLOR_RESET ""
#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_YELLOW ""
#define COLOR_CYAN ""
#endif

static struct test_state {
	unsigned int assertions;
	unsigned int failures;
} state;

static void reset_timeout(void) {
	wdt_set_max_execution_time(TEST_TIMEOUT_MS);
	wdt_serve();
}

static bool report(const char *name, bool passed) {
	state.assertions++;
	printf(
		"%s %u - %s%s%s\n",
		passed ? "ok" : "not ok",
		state.assertions,
		passed ? COLOR_GREEN : COLOR_RED,
		name,
		COLOR_RESET
	);

	if (!passed)
		state.failures++;

	return passed;
}

void test_start(const char *name) {
	state = (struct test_state) {0};
	wdt_init();
	reset_timeout();

	printf("TAP version 13\n");
	printf("# %s%s%s\n", COLOR_CYAN, name, COLOR_RESET);
}

void test_category(const char *name) {
	printf("# %s--- %s ---%s\n", COLOR_CYAN, name, COLOR_RESET);
	reset_timeout();
}

int test_finish(void) {
	printf("1..%u\n", state.assertions);
	printf(
		"# result: %s%s%s (%u failed)\n",
		state.failures ? COLOR_RED : COLOR_GREEN,
		state.failures ? "FAIL" : "PASS",
		COLOR_RESET,
		state.failures
	);

	usart_putc(USART0, 0);

	return state.failures ? 1 : 0;
}

void test_skip(const char *name, const char *reason) {
	state.assertions++;
	printf("ok %u - %s%s # SKIP %s%s\n", state.assertions, COLOR_YELLOW, name, reason, COLOR_RESET);
	reset_timeout();
}

bool test_check(const char *name, bool condition) {
	bool passed = report(name, condition);
	reset_timeout();

	return passed;
}

bool test_eq_u32(const char *name, uint32_t expected, uint32_t actual) {
	bool passed = report(name, actual == expected);

	if (!passed) {
		printf("# %sexpected: %08X%s\n", COLOR_YELLOW, expected, COLOR_RESET);
		printf("# %sactual:   %08X%s\n", COLOR_RED, actual, COLOR_RESET);
	}

	reset_timeout();

	return passed;
}

bool test_module_id(const char *name, uint32_t expected, uint32_t actual) {
	return test_eq_u32(name, expected & ~MOD_ID_REV, actual & ~MOD_ID_REV);
}

bool test_module_clock(const char *name, uint32_t clc) {
	bool enabled = (clc & (MOD_CLC_DISR | MOD_CLC_DISS)) == 0 && (clc & MOD_CLC_RMC) != 0;

	report(name, enabled);
	if (!enabled)
		printf("# %sCLC=%08X%s\n", COLOR_RED, clc, COLOR_RESET);

	reset_timeout();

	return enabled;
}

bool test_amba_part_id(const char *name, uint16_t expected, uint32_t id0, uint32_t id1) {
	uint16_t actual = (id0 & 0xFF) | ((id1 & 0x0F) << 8);

	return test_eq_u32(name, expected, actual);
}

bool test_eq_memory(const char *name, const void *expected, const volatile void *actual, size_t size) {
	const uint8_t *expected_bytes = expected;
	const volatile uint8_t *actual_bytes = actual;
	size_t offset = 0;

	while (offset < size && expected_bytes[offset] == actual_bytes[offset])
		offset++;

	bool passed = report(name, offset == size);
	if (!passed) {
		printf("# %soffset:   %u%s\n", COLOR_YELLOW, (unsigned int) offset, COLOR_RESET);
		printf("# %sexpected: %02X%s\n", COLOR_YELLOW, expected_bytes[offset], COLOR_RESET);
		printf("# %sactual:   %02X%s\n", COLOR_RED, actual_bytes[offset], COLOR_RESET);
	}

	reset_timeout();

	return passed;
}

bool test_u32_in_interval(uint32_t value, uint32_t first, uint32_t last) {
	return (uint32_t) (value - first) <= (uint32_t) (last - first);
}

void test_watchdog_serve(void) {
	wdt_serve();
}

void test_watchdog_reset(void) {
	reset_timeout();
}

void test_spin(unsigned int iterations) {
	for (volatile unsigned int i = 0; i < iterations; i++) {
	}
}
