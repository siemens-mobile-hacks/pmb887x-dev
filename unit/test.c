#include "test.h"

#include <pmb887x.h>
#include <stopwatch.h>
#include <string.h>

#define PHONE_INFO_STRING_SIZE 16
#define TEST_TIMEOUT_MS 3000
#define TEST_HEARTBEAT_MS 100
#define RTC_T14_RELOAD 61440
#define RTC_SECOND_COUNTER 0x3FF
#define RTC_CLOCK_CONTROL (RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN)
#define RTC_SYNC_CONTROL (RTC_CLOCK_CONTROL | RTC_CTRL_CLK_SEL)

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

typedef struct test_fault_jmp {
	uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
	uint32_t sp, lr, cpsr;
} test_fault_jmp_t;

static test_fault_jmp_t fault_recovery;
static volatile bool fault_recovery_armed;
static volatile uint32_t fault_nesting;

/* r0 = ctx; returns 0 on the direct call, 1 when the fault handler longjmps back here. */
__attribute__((naked)) int test_fault_setjmp(test_fault_jmp_t *) {
	__asm__("stmia r0!, {r4-r11}\n\t"
		"str sp, [r0], #4\n\t"
		"str lr, [r0], #4\n\t"
		"mrs r1, cpsr\n\t"
		"str r1, [r0]\n\t"
		"mov r0, #0\n\t"
		"bx lr");
}

/* r0 = ctx; switches back to the mode that armed the recovery and returns 1 there. */
__attribute__((naked, noreturn)) void test_fault_longjmp(test_fault_jmp_t *) {
	__asm__("ldr r1, [r0, #40]\n\t"
		"msr cpsr_cxsf, r1\n\t"
		"ldmia r0!, {r4-r11}\n\t"
		"ldr sp, [r0], #4\n\t"
		"ldr lr, [r0], #4\n\t"
		"mov r0, #1\n\t"
		"bx lr");
}

/*
 * Arm recovery around a guarded body.  Returns 0 when the arm itself completed; if a fault is
 * taken before test_fault_guard_end(), the handler prints the fields and longjmps back, so
 * this call returns 1 at the same point and the body's writes must not be trusted.  Both
 * paths must call test_fault_guard_end().
 */
int test_fault_guard(void) {
	fault_nesting = 0;
	fault_recovery_armed = true;
	return test_fault_setjmp(&fault_recovery) != 0;
}

void test_fault_guard_end(void) {
	fault_recovery_armed = false;
}

static void fault_putc(char c) {
	USART_TXB(USART0) = c;
	while ((USART_RIS(USART0) & USART_RIS_TX) == 0)
		;
	USART_ICR(USART0) |= USART_ICR_TX;
}

static void fault_put_hex(uint32_t value) {
	fault_putc('0');
	fault_putc('x');
	for (int shift = 28; shift >= 0; shift -= 4) {
		uint32_t nibble = (value >> shift) & 0xF;

		fault_putc(nibble < 10 ? (char) ('0' + nibble) : (char) ('A' + nibble - 10));
	}
}

/* One line: "\n# EXC <type> <label>=<hex>", label up to four immediate characters.  The
   leading newline terminates whatever was on the line before; the caller ends the run of
   fields with one newline. */
static void fault_field(char type, char a, char b, char c, char d, uint32_t value) {
	fault_putc('\n');
	fault_putc('#');
	fault_putc(' ');
	fault_putc('E');
	fault_putc('X');
	fault_putc('C');
	fault_putc(' ');
	fault_putc(type);
	fault_putc(' ');
	fault_putc(a);
	if (b != 0) {
		fault_putc(b);
		if (c != 0) {
			fault_putc(c);
			if (d != 0)
				fault_putc(d);
		}
	}
	fault_putc('=');
	fault_put_hex(value);
}

static void __attribute__((noreturn)) exception_report(char type, uint32_t lr, uint32_t spsr, uint32_t fsr,
	uint32_t far) {
	/* One entry prints the fields; a re-fault while printing sends nothing further and
	   goes straight to the recovery or the spin, so the reporter cannot recurse forever. */
	if (fault_nesting++ == 0) {
		fault_field(type, 'L', 'R', 0, 0, lr);
		fault_field(type, 'S', 'P', 'S', 'R', spsr);
		fault_field(type, 'F', 'S', 'R', 0, fsr);
		fault_field(type, 'F', 'A', 'R', 0, far);
		fault_putc('\n');
	}

	if (fault_recovery_armed)
		test_fault_longjmp(&fault_recovery);

	test_fault_decode(lr, far);
	while (true)
		__asm__ volatile("nop");
}

/* A suite that knows its own register map names the block here (see test.h); the decode runs
   only after the fields have reached the log. */
__attribute__((weak)) void test_fault_decode(uint32_t pc, uint32_t far) {
	(void) pc;
	(void) far;
}

__IRQ void data_abort_handler(void) {
	uint32_t link;
	uint32_t spsr;
	uint32_t fault_status;
	uint32_t fault_address;
	__asm__ volatile("mov %0, lr" : "=r" (link));
	__asm__ volatile("mrs %0, spsr" : "=r" (spsr));
	__asm__ volatile("mrc p15, 0, %0, c5, c0, 0" : "=r" (fault_status));
	__asm__ volatile("mrc p15, 0, %0, c6, c0, 0" : "=r" (fault_address));
	exception_report('D', link - 8, spsr, fault_status, fault_address);
}

__IRQ void prefetch_abort_handler(void) {
	uint32_t link;
	uint32_t spsr;
	uint32_t fault_status;
	uint32_t fault_address;
	__asm__ volatile("mov %0, lr" : "=r" (link));
	__asm__ volatile("mrs %0, spsr" : "=r" (spsr));
	__asm__ volatile("mrc p15, 0, %0, c5, c0, 1" : "=r" (fault_status));
	__asm__ volatile("mrc p15, 0, %0, c6, c0, 2" : "=r" (fault_address));
	exception_report('P', link - 4, spsr, fault_status, fault_address);
}

__IRQ void undef_handler(void) {
	uint32_t link;
	uint32_t spsr;
	__asm__ volatile("mov %0, lr" : "=r" (link));
	__asm__ volatile("mrs %0, spsr" : "=r" (spsr));
	exception_report('U', link - 4, spsr, 0, 0);
}

static bool read_flash_string(char *destination, size_t size, uint32_t address) {
	const volatile char *source = (const volatile char *) address;

	destination[0] = 0;
	for (size_t i = 0; i < size; i++) {
		char value = source[i];

		if (value == 0) {
			destination[i] = 0;
			return i != 0;
		}
		if (value < 0x20 || value > 0x7E) {
			destination[0] = 0;
			return false;
		}
		destination[i] = value;
	}
	destination[0] = 0;

	return false;
}

static bool read_phone_info(char *vendor, char *model, uint32_t model_address, uint32_t vendor_address) {
	char found_vendor[PHONE_INFO_STRING_SIZE];
	char found_model[PHONE_INFO_STRING_SIZE];

	if (!read_flash_string(found_vendor, sizeof(found_vendor), vendor_address))
		return false;
	if (!read_flash_string(found_model, sizeof(found_model), model_address))
		return false;

	memcpy(vendor, found_vendor, sizeof(found_vendor));
	memcpy(model, found_model, sizeof(found_model));

	return true;
}

static void print_hardware(void) {
	char vendor[PHONE_INFO_STRING_SIZE] = "unknown";
	char model[PHONE_INFO_STRING_SIZE] = "unknown";
	uint32_t chip = (SCU_CHIPID & SCU_CHIPID_CHIPD) >> SCU_CHIPID_CHIPD_SHIFT;
	uint32_t revision = (SCU_CHIPID & SCU_CHIPID_CHREV) >> SCU_CHIPID_CHREV_SHIFT;
	const char *cpu = "unknown";

	if (!read_phone_info(vendor, model, 0xA003E000, 0xA003E010))
		read_phone_info(vendor, model, 0xA0000210, 0xA0000220);
	if (chip == 0x1A)
		cpu = "PMB8875";
	else if (chip == 0x1B)
		cpu = "PMB8876";
	printf("# Hardware: vendor=%s, phone=%s, CPU=%s rev=%02X\n", vendor, model, cpu, (unsigned int) revision);
}

static void reset_timeout(void) {
	stopwatch_update();
	wdt_set_max_execution_time(TEST_TIMEOUT_MS);
	wdt_serve();
}

void test_rtc_init(void) {
	reset_timeout();

	SCU_RTCIF = 0xAA;
	RTC_CLC = (1 << MOD_CLC_RMC_SHIFT);
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_CLR_RTCBAD | RTC_CTRL_CLR_RTCINT;
	while ((RTC_CON & RTC_CON_ACCPOS) == 0)
		;
	RTC_CON = RTC_CON_PRE;
	RTC_T14 = (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT);
	RTC_CNT = RTC_SECOND_COUNTER;
	RTC_REL = 0;
	RTC_ALARM = 0;
	RTC_SRC = 0;
	RTC_ISNC = RTC_ISNC_RTC0IE;
	RTC_ISNRC = RTC_ISNRC_RTC0;
	RTC_CON |= RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL | RTC_CTRL_CLR_RTCINT;
}

bool test_rtc_second_elapsed(void) {
	return (RTC_CTRL & RTC_CTRL_RTCINT) != 0;
}

void test_rtc_wait_second(void) {
	while (!test_rtc_second_elapsed())
		test_spin(1000);

	test_watchdog_serve();
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
	print_hardware();
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
		printf("# %sexpected: %08lX%s\n", COLOR_YELLOW, expected, COLOR_RESET);
		printf("# %sactual:   %08lX%s\n", COLOR_RED, actual, COLOR_RESET);
	}

	reset_timeout();

	return passed;
}

bool test_id_u32(const char *name, uint32_t expected, uint32_t actual) {
	printf("# %s: %08lX\n", name, actual);
	return test_eq_u32(name, expected, actual);
}

bool test_module_id(const char *name, uint32_t expected, uint32_t actual) {
	printf("# %s: %08lX\n", name, actual);
	return test_eq_u32(name, expected & ~MOD_ID_REV, actual & ~MOD_ID_REV);
}

bool test_module_clock(const char *name, uint32_t clc) {
	bool enabled = (clc & (MOD_CLC_DISR | MOD_CLC_DISS)) == 0 && (clc & MOD_CLC_RMC) != 0;

	report(name, enabled);
	if (!enabled)
		printf("# %sCLC=%08lX%s\n", COLOR_RED, clc, COLOR_RESET);

	reset_timeout();

	return enabled;
}

bool test_amba_part_id(const char *name, uint16_t expected, uint32_t id0, uint32_t id1) {
	uint16_t actual = (id0 & 0xFF) | ((id1 & 0x0F) << 8);

	return test_id_u32(name, expected, actual);
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

bool test_wait_for_flag(const volatile uint32_t *reg, uint32_t flag, uint32_t timeout_ms) {
	stopwatch_t start = stopwatch_get();

	while ((*reg & flag) == 0 && stopwatch_elapsed_ms(start) < timeout_ms)
		test_watchdog_serve();
	return (*reg & flag) != 0;
}

bool test_u32_in_interval(uint32_t value, uint32_t first, uint32_t last) {
	return (uint32_t) (value - first) <= (uint32_t) (last - first);
}

bool test_is_qemu(void) {
	return SCU_EMU_ID == SCU_EMU_ID_VALUE_QEMU;
}

uint32_t test_stm_ticks_per_ms(void) {
	uint32_t hz = cpu_get_stm_freq();

	if (hz == 0)
		hz = 26000000;

	return hz / 1000;
}

bool test_elapsed_bound_ms(uint64_t start, uint32_t ms) {
	uint64_t ticks = (uint64_t) ms * test_stm_ticks_per_ms();

	return stopwatch_elapsed(start) >= (ticks ? ticks : 1);
}

void test_heartbeat(void) {
	static uint64_t last;
	uint64_t now = stopwatch_get();

	if (now - last < (uint64_t) TEST_HEARTBEAT_MS * test_stm_ticks_per_ms())
		return;
	last = now;
	usart_putc(USART0, '.');
}

/* Ends a heartbeat line a caller started with "# ". */
void test_heartbeat_end(void) {
	usart_putc(USART0, '\n');
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
