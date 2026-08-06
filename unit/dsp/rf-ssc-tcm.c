#include <pmb887x.h>

#include "rf-ssc-tcm.h"

#define ITCM_BASE 0x01000000
#define TCM_REGION_SIZE_8K 0x10
#define TCM_REGION_ENABLE BIT(0)

typedef uint32_t (*rf_ssc_tcm_function_t)(uint32_t control, uint32_t first, uint32_t second, uint32_t has_second);

extern const uint32_t rf_ssc_tcm_template_start[];
extern const uint32_t rf_ssc_tcm_template_end[];

static void itcm_configure(uint32_t value) {
	__asm__ volatile("mcr p15, 0, %0, c9, c1, 1" : : "r" (value) : "memory");
}

static void instruction_cache_sync(void) {
	uint32_t value = 0;

	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (value) : "memory");
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r" (value) : "memory");
}

void rf_ssc_tcm_init(void) {
	itcm_configure(ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	uint32_t words = (uint32_t) (rf_ssc_tcm_template_end - rf_ssc_tcm_template_start);
	for (uint32_t i = 0; i < words; i++)
		MMIO32(ITCM_BASE + i * sizeof(uint32_t)) = rf_ssc_tcm_template_start[i];
	instruction_cache_sync();
}

bool rf_ssc_tcm_transfer(uint32_t control, uint16_t first, uint16_t second, bool has_second) {
	rf_ssc_tcm_function_t transfer = (rf_ssc_tcm_function_t) ITCM_BASE;
	return transfer(control, first, second, has_second) != 0;
}
