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
bool test_module_id(const char *name, uint32_t expected, uint32_t actual);
bool test_module_clock(const char *name, uint32_t clc);
bool test_amba_part_id(const char *name, uint16_t expected, uint32_t id0, uint32_t id1);
bool test_eq_memory(const char *name, const void *expected, const volatile void *actual, size_t size);

bool test_u32_in_interval(uint32_t value, uint32_t first, uint32_t last);
void test_watchdog_serve(void);
void test_watchdog_reset(void);
void test_spin(unsigned int iterations);
