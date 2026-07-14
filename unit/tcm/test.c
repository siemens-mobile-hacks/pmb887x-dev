#include <pmb887x.h>

#include "test.h"

#define TCM_SIZE (8 * 1024)
#define TCM_REGION_SIZE_8K (4 << 2)
#define TCM_REGION_ENABLE BIT(0)
#define ITCM_BASE 0x01000000
#define DTCM_BASE 0x01002000
#define ITCM_REMAP_BASE 0x01004000
#define DTCM_REMAP_BASE 0x01006000
#define DTCM_OVERLAY_BASE 0xA8100000
#define ITCM_OVERLAY_BASE 0xA8120000
#define OVERLAY_DATA_OFFSET 0x100

uint32_t tcm_execute_with_resume(uint32_t address);

static uint32_t read_main_id(void) {
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c0, c0, 0" : "=r" (value));
	return value;
}

static uint32_t read_tcm_type(void) {
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c0, c0, 2" : "=r" (value));
	return value;
}

static uint32_t read_itcm(void) {
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c9, c1, 1" : "=r" (value));
	return value;
}

static uint32_t read_dtcm(void) {
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c9, c1, 0" : "=r" (value));
	return value;
}

static void write_itcm(uint32_t value) {
	__asm__ volatile("mcr p15, 0, %0, c9, c1, 1" : : "r" (value) : "memory");
}

static void write_dtcm(uint32_t value) {
	__asm__ volatile("mcr p15, 0, %0, c9, c1, 0" : : "r" (value) : "memory");
}

static void sync_code(void) {
	uint32_t value = 0;
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (value) : "memory");
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r" (value) : "memory");
}

static void fill_tcm(uint32_t base, uint32_t seed) {
	for (uint32_t offset = 0; offset < TCM_SIZE; offset += sizeof(uint32_t)) {
		MMIO32(base + offset) = seed ^ offset;
		test_watchdog_serve();
	}
}

static bool check_tcm(uint32_t base, uint32_t seed) {
	for (uint32_t offset = 0; offset < TCM_SIZE; offset += sizeof(uint32_t)) {
		if (MMIO32(base + offset) != (seed ^ offset)) {
			printf("# mismatch at %08X: expected=%08X actual=%08X\n", base + offset, seed ^ offset,
				MMIO32(base + offset));
			return false;
		}
		test_watchdog_serve();
	}
	return true;
}

static void test_access_widths(uint32_t base) {
	MMIO32(base) = 0x12345678;
	test_eq_u32("word access", 0x12345678, MMIO32(base));
	MMIO16(base + 2) = 0xA5C3;
	test_eq_u32("half-word access", 0xA5C35678, MMIO32(base));
	MMIO8(base + 1) = 0x9A;
	test_eq_u32("byte access", 0xA5C39A78, MMIO32(base));
	MMIO32(base + TCM_SIZE - sizeof(uint32_t)) = 0x5AA59669;
	test_eq_u32("last word access", 0x5AA59669, MMIO32(base + TCM_SIZE - sizeof(uint32_t)));
}

static void write_test_code(uint32_t base, uint32_t value) {
	MMIO32(base) = 0xE3A00000 | value;
	MMIO32(base + 4) = 0xE12FFF14;
}

static void test_dtcm_overlay(void) {
	write_test_code(DTCM_OVERLAY_BASE, 0xA5);
	MMIO32(DTCM_OVERLAY_BASE + OVERLAY_DATA_OFFSET) = 0x11223344;
	write_dtcm(DTCM_OVERLAY_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_test_code(DTCM_OVERLAY_BASE, 0x5A);
	MMIO32(DTCM_OVERLAY_BASE + OVERLAY_DATA_OFFSET) = 0xA5C39669;
	sync_code();
	test_eq_u32("DTCM data access uses TCM", 0xA5C39669, MMIO32(DTCM_OVERLAY_BASE + OVERLAY_DATA_OFFSET));
	test_eq_u32("DTCM instruction fetch uses system memory", 0xA5, tcm_execute_with_resume(DTCM_OVERLAY_BASE));
	write_dtcm(TCM_REGION_SIZE_8K);
	test_eq_u32("DTCM disable restores system data", 0x11223344,
		MMIO32(DTCM_OVERLAY_BASE + OVERLAY_DATA_OFFSET));
}

static void test_itcm_overlay(void) {
	write_test_code(ITCM_OVERLAY_BASE, 0xA5);
	MMIO32(ITCM_OVERLAY_BASE + OVERLAY_DATA_OFFSET) = 0x11223344;
	write_itcm(ITCM_OVERLAY_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_test_code(ITCM_OVERLAY_BASE, 0x5A);
	MMIO32(ITCM_OVERLAY_BASE + OVERLAY_DATA_OFFSET) = 0xA5C39669;
	sync_code();
	test_eq_u32("ITCM data access uses TCM", 0xA5C39669, MMIO32(ITCM_OVERLAY_BASE + OVERLAY_DATA_OFFSET));
	test_eq_u32("ITCM instruction fetch uses TCM", 0x5A, tcm_execute_with_resume(ITCM_OVERLAY_BASE));
	write_itcm(TCM_REGION_SIZE_8K);
	sync_code();
	test_eq_u32("ITCM disable restores system data", 0x11223344,
		MMIO32(ITCM_OVERLAY_BASE + OVERLAY_DATA_OFFSET));
	test_eq_u32("ITCM disable restores system instructions", 0xA5, tcm_execute_with_resume(ITCM_OVERLAY_BASE));
}

int main(void) {
	test_start("ARM TCM test");
	test_category("Reset values");
	test_eq_u32("ITCM region reset value", TCM_REGION_SIZE_8K, read_itcm());
	test_eq_u32("DTCM region reset value", TCM_REGION_SIZE_8K, read_dtcm());

	test_category("Identification");
	uint32_t main_id = read_main_id();
	uint32_t tcm_type = read_tcm_type();
	printf("# MIDR=%08X TCMTR=%08X\n", main_id, tcm_type);
	test_eq_u32("CPU implementer is Arm", 0x41, main_id >> 24);
	test_eq_u32("CPU core is ARM926", 0x926, (main_id >> 4) & 0xFFF);
	test_eq_u32("one ITCM is present", 1, tcm_type & 0x7);
	test_eq_u32("one DTCM is present", 1, (tcm_type >> 16) & 0x7);

	test_category("Region registers");
	uint32_t itcm_region = ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE;
	uint32_t dtcm_region = DTCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE;
	write_itcm(itcm_region);
	write_dtcm(dtcm_region);
	test_eq_u32("ITCM region readback", itcm_region, read_itcm());
	test_eq_u32("DTCM region readback", dtcm_region, read_dtcm());

	test_category("Memory");
	fill_tcm(ITCM_BASE, 0xA5A50000);
	fill_tcm(DTCM_BASE, 0x5A5A0000);
	test_check("ITCM complete range", check_tcm(ITCM_BASE, 0xA5A50000));
	test_check("DTCM complete range", check_tcm(DTCM_BASE, 0x5A5A0000));
	test_check("ITCM remains independent", check_tcm(ITCM_BASE, 0xA5A50000));
	test_access_widths(ITCM_BASE);
	test_access_widths(DTCM_BASE);
	fill_tcm(ITCM_BASE, 0xA5A50000);
	fill_tcm(DTCM_BASE, 0x5A5A0000);

	test_category("Remap");
	write_itcm(ITCM_REMAP_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_dtcm(DTCM_REMAP_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	test_check("ITCM data survives remap", check_tcm(ITCM_REMAP_BASE, 0xA5A50000));
	test_check("DTCM data survives remap", check_tcm(DTCM_REMAP_BASE, 0x5A5A0000));

	test_category("Overlay behavior");
	test_dtcm_overlay();
	test_itcm_overlay();

	return test_finish();
}
