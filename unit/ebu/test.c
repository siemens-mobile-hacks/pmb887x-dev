#include <pmb887x.h>

#include "test.h"

#define PROBE_CS 5
#define SDRSTAT_STATUS_MASK (EBU_SDRSTAT_SDRM_BUSY | EBU_SDRSTAT_REFERR)

typedef struct {
	uint32_t addrsel;
	uint32_t buscon;
	uint32_t busap;
} ebu_region_t;

typedef struct {
	uint32_t clc;
	uint32_t con;
	uint32_t bfcon;
	uint32_t sdrmref0;
	uint32_t sdrmcon0;
	uint32_t sdrmod0;
	uint32_t sdrstat0;
	ebu_region_t cs1;
} ebu_cs1_guard_t;

static ebu_region_t read_region(uint32_t chip_select) {
	return (ebu_region_t) {
		.addrsel = EBU_ADDRSEL(chip_select),
		.buscon = EBU_BUSCON(chip_select),
		.busap = EBU_BUSAP(chip_select),
	};
}

static ebu_cs1_guard_t read_cs1_guard(void) {
	return (ebu_cs1_guard_t) {
		.clc = EBU_CLC,
		.con = EBU_CON,
		.bfcon = EBU_BFCON,
		.sdrmref0 = EBU_SDRMREF(0),
		.sdrmcon0 = EBU_SDRMCON(0),
		.sdrmod0 = EBU_SDRMOD(0),
		.sdrstat0 = EBU_SDRSTAT(0),
		.cs1 = read_region(1),
	};
}

static void test_reset_values(const ebu_cs1_guard_t *guard) {
	test_module_id("module ID", 0x0014C000, EBU_ID);
	/* The test executes from CS1, so disabling and re-enabling the EBU here would stop instruction fetches. */
	test_eq_u32("clock control reset value", 0, guard->clc);
	test_eq_u32("SDRAM0 controller is idle", 0, guard->sdrstat0 & SDRSTAT_STATUS_MASK);
	test_eq_u32("SDRAM1 controller is idle", 0, EBU_SDRSTAT(1) & SDRSTAT_STATUS_MASK);
#ifdef PMB8876
	test_eq_u32("SDRAM0 undocumented status bit 8 reads as one", EBU_SDRSTAT_UNK8, guard->sdrstat0);
	test_eq_u32("SDRAM1 undocumented status bit 8 reads as one", EBU_SDRSTAT_UNK8, EBU_SDRSTAT(1));
#endif
	test_check("CS1 memory region is enabled", (guard->cs1.addrsel & EBU_ADDRSEL_REGENAB) != 0);
	test_eq_u32(
		"CS1 uses the SDRAM0 controller",
		3,
		(guard->cs1.buscon & EBU_BUSCON_AGEN) >> EBU_BUSCON_AGEN_SHIFT
	);
	test_eq_u32("CS5 is disabled before the probe", 0, EBU_ADDRSEL(PROBE_CS) & EBU_ADDRSEL_REGENAB);
	test_eq_u32("BFCON DBA1 layout", BIT(26), EBU_BFCON_DBA1);

	printf(
		"# EBU: CLC=%08X CON=%08X BFCON=%08X USERCON=%08X EMUAS=%08X EMUOVL=%08X\n",
		(unsigned int) guard->clc,
		(unsigned int) guard->con,
		(unsigned int) guard->bfcon,
		(unsigned int) EBU_USERCON,
		(unsigned int) EBU_EMUAS,
		(unsigned int) EBU_EMUOVL
	);
	for (uint32_t chip_select = 0; chip_select < 7; chip_select++) {
		ebu_region_t region = read_region(chip_select);
		printf(
			"# EBU CS%u: ADDRSEL=%08X BUSCON=%08X BUSAP=%08X\n",
			(unsigned int) chip_select,
			(unsigned int) region.addrsel,
			(unsigned int) region.buscon,
			(unsigned int) region.busap
		);
	}
	printf(
		"# EBU SDRAM0: REF=%08X CON=%08X MOD=%08X STAT=%08X\n",
		(unsigned int) guard->sdrmref0,
		(unsigned int) guard->sdrmcon0,
		(unsigned int) guard->sdrmod0,
		(unsigned int) guard->sdrstat0
	);
	printf(
		"# EBU SDRAM1: REF=%08X CON=%08X MOD=%08X STAT=%08X\n",
		(unsigned int) EBU_SDRMREF(1),
		(unsigned int) EBU_SDRMCON(1),
		(unsigned int) EBU_SDRMOD(1),
		(unsigned int) EBU_SDRSTAT(1)
	);
}

int main(void) {
	test_start("EBU peripheral test");
	ebu_cs1_guard_t cs1_guard = read_cs1_guard();

	test_category("Reset values");
	test_reset_values(&cs1_guard);

	return test_finish();
}
