#include <pmb887x.h>

#include "test.h"

#define PAGE_BYTES 4096
#define LINE_BYTES 32
#define REMAP_VIRTUAL_PAGE 0xAFF00000
#define LINK_CALLS 4
#define REWRITTEN_RETURN_VALUE 0x55

#define ARM_MOV_R0 0xE3A00000		/* mov r0, #<imm8> */
#define ARM_BX_LR 0xE12FFF1E		/* bx lr */
#define ARM_PUSH_LR 0xE52DE004		/* push {lr} */
#define ARM_BL 0xEB000000			/* bl <offset> */
#define ARM_POP_PC 0xE49DF004		/* pop {pc} */

struct tlb_invalidation {
	const char *name;
	const char *indirect_name;
	void (*invalidate)(void);
};

static void tlb_invalidate_all(void);
static void tlb_invalidate_entry(void);
static void itlb_invalidate_all(void);
static void itlb_invalidate_entry(void);

static const struct tlb_invalidation TLB_INVALIDATIONS[] = {
	{ "Remap after invalidating the whole TLB", "Indirect call after invalidating the whole TLB",
		tlb_invalidate_all },
	{ "Remap after invalidating the TLB entry", "Indirect call after invalidating the TLB entry",
		tlb_invalidate_entry },
	{ "Remap after invalidating the whole I-TLB", "Indirect call after invalidating the whole I-TLB",
		itlb_invalidate_all },
	{ "Remap after invalidating the I-TLB entry", "Indirect call after invalidating the I-TLB entry",
		itlb_invalidate_entry },
};

static uint32_t translation_table[4096] __attribute__((aligned(16384)));
static uint32_t coarse_table[256] __attribute__((aligned(1024)));
static uint32_t return_pages[ARRAY_SIZE(TLB_INVALIDATIONS) + 1][PAGE_BYTES / 4] __attribute__((aligned(PAGE_BYTES)));
static uint32_t caller_page[PAGE_BYTES / 4] __attribute__((aligned(PAGE_BYTES)));

static void dcache_clean_range(const void *start, size_t bytes) {
	uint32_t line = (uint32_t) start & ~(LINE_BYTES - 1);
	uint32_t end = (uint32_t) start + bytes;

	for (; line < end; line += LINE_BYTES)
		__asm__ volatile("mcr p15, 0, %0, c7, c10, 1" : : "r" (line) : "memory");
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (0) : "memory");
}

static void icache_invalidate(void) {
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r" (0) : "memory");
}

static void tlb_invalidate_all(void) {
	__asm__ volatile("mcr p15, 0, %0, c8, c7, 0" : : "r" (0) : "memory");
}

static void tlb_invalidate_entry(void) {
	__asm__ volatile("mcr p15, 0, %0, c8, c7, 1" : : "r" (REMAP_VIRTUAL_PAGE) : "memory");
}

static void itlb_invalidate_all(void) {
	__asm__ volatile("mcr p15, 0, %0, c8, c5, 0" : : "r" (0) : "memory");
}

static void itlb_invalidate_entry(void) {
	__asm__ volatile("mcr p15, 0, %0, c8, c5, 1" : : "r" (REMAP_VIRTUAL_PAGE) : "memory");
}

static void remap_page_write(const uint32_t *physical_page) {
	coarse_table[0] = (uint32_t) physical_page | (0xFF << 4) | (1 << 3) | (1 << 2) | 2;
	dcache_clean_range(&coarse_table[0], sizeof(coarse_table[0]));
}

/* Flat cacheable sections, except the remapped page, which has its own coarse table. */
static void mmu_enable(void) {
	uint32_t remap_section = REMAP_VIRTUAL_PAGE >> 20;

	for (uint32_t section = 0; section < 4096; section++) {
		bool cacheable = section == 0 || (section >= 0xA00 && section < 0xC00);
		translation_table[section] = (section << 20) | (3 << 10) | (1 << 4) | (cacheable ? (1 << 3) | (1 << 2) : 0) | 2;
	}
	translation_table[remap_section] = (uint32_t) coarse_table | (1 << 4) | 1;
	remap_page_write(return_pages[0]);

	__asm__ volatile(
		"mcr p15, 0, %0, c7, c10, 4\n"
		"mcr p15, 0, %0, c7, c7, 0\n"
		"mcr p15, 0, %0, c8, c7, 0\n"
		"mcr p15, 0, %1, c2, c0, 0\n"
		"mcr p15, 0, %2, c3, c0, 0\n"
		: : "r" (0), "r" (translation_table), "r" (3) : "memory");
	uint32_t control;
	__asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r" (control));
	control |= (1 << 12) | (1 << 2) | 1;
	__asm__ volatile("mcr p15, 0, %0, c1, c0, 0\n" "nop\n" "nop\n" : : "r" (control) : "memory");
}

static void code_pages_write(void) {
	uint32_t bl_address = (uint32_t) &caller_page[1];

	for (size_t page = 0; page < ARRAY_SIZE(return_pages); page++) {
		return_pages[page][0] = ARM_MOV_R0 | (page + 1);
		return_pages[page][1] = ARM_BX_LR;
		dcache_clean_range(return_pages[page], 2 * sizeof(uint32_t));
	}
	caller_page[0] = ARM_PUSH_LR;
	caller_page[1] = ARM_BL | (((REMAP_VIRTUAL_PAGE - (bl_address + 8)) >> 2) & 0x00FFFFFF);
	caller_page[2] = ARM_POP_PC;
	dcache_clean_range(caller_page, 3 * sizeof(uint32_t));
	icache_invalidate();
}

/* Repeated calls let the emulator link the caller directly to the remapped page. */
static uint32_t remapped_page_call(void) {
	uint32_t (*caller)(void) = (uint32_t (*)(void)) caller_page;
	uint32_t first = caller();

	for (int i = 1; i < LINK_CALLS; i++)
		caller();
	return first;
}

static uint32_t remapped_page_indirect_call(void) {
	uint32_t (*function)(void) = (uint32_t (*)(void)) REMAP_VIRTUAL_PAGE;
	uint32_t first = function();

	for (int i = 1; i < LINK_CALLS; i++)
		function();
	return first;
}

static void remapped_code_rewrite(uint32_t *physical_page, uint32_t value) {
	physical_page[0] = ARM_MOV_R0 | value;
	dcache_clean_range(physical_page, sizeof(physical_page[0]));
	icache_invalidate();
}

int main(void) {
	test_start("ARM926 code page remap");

	mmu_enable();
	code_pages_write();
	test_eq_u32("Call through the initial mapping", 1, remapped_page_call());
	test_eq_u32("Indirect call through the initial mapping", 1, remapped_page_indirect_call());

	for (size_t i = 0; i < ARRAY_SIZE(TLB_INVALIDATIONS); i++) {
		size_t page = i + 1;

		remap_page_write(return_pages[page]);
		TLB_INVALIDATIONS[i].invalidate();
		icache_invalidate();
		test_eq_u32(TLB_INVALIDATIONS[i].name, page + 1, remapped_page_call());
		test_eq_u32(TLB_INVALIDATIONS[i].indirect_name, page + 1, remapped_page_indirect_call());
	}

	remapped_code_rewrite(return_pages[ARRAY_SIZE(TLB_INVALIDATIONS)], REWRITTEN_RETURN_VALUE);
	test_eq_u32("Indirect call after rewriting the mapped code", REWRITTEN_RETURN_VALUE, remapped_page_indirect_call());

	return test_finish();
}
