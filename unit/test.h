#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void test_start(const char *name);
void test_category(const char *name);
int test_finish(void);
void test_skip(const char *name, const char *reason);
bool test_check(const char *name, bool condition);
bool test_eq_u32(const char *name, uint32_t expected, uint32_t actual);
bool test_id_u32(const char *name, uint32_t expected, uint32_t actual);
bool test_module_id(const char *name, uint32_t expected, uint32_t actual);
bool test_module_clock(const char *name, uint32_t clc);
bool test_amba_part_id(const char *name, uint16_t expected, uint32_t id0, uint32_t id1);
bool test_eq_memory(const char *name, const void *expected, const volatile void *actual, size_t size);
bool test_wait_for_flag(const volatile uint32_t *reg, uint32_t flag, uint32_t timeout_ms);

bool test_u32_in_interval(uint32_t value, uint32_t first, uint32_t last);
bool test_is_qemu(void);
uint32_t test_stm_ticks_per_ms(void);
bool test_elapsed_bound_ms(uint64_t start, uint32_t ms);
void test_rtc_init(void);
bool test_rtc_second_elapsed(void);
void test_rtc_wait_second(void);
void test_heartbeat(void);
void test_heartbeat_end(void);
void test_watchdog_serve(void);
void test_watchdog_reset(void);
void test_fault_decode(uint32_t pc, uint32_t far);
/* Arm fault recovery around a guarded body; 1 if a fault was recovered at this call. */
int test_fault_guard(void);
void test_fault_guard_end(void);
void test_spin(unsigned int iterations);
