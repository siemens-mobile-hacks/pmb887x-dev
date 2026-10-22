#include <pmb887x.h>

#include "bus-timing.h"
#include "test.h"

#ifdef PMB8876

#define EL71_WRITE_TIM1 0x00000400
#define EL71_WRITE_TIM2 0x02000400

static struct dif_bus_timing measure(const char *name, const struct dif_lane_config *lane,
	enum dif_blit_source source, uint32_t rmc, uint32_t tim1, uint32_t tim2) {
	uint32_t clc = rmc << MOD_CLC_RMC_SHIFT;
	struct dif_bus_timing timing = dif_bus_timing_measure(lane, source, clc, tim1, tim2);

	/* DIF clock cycles per byte, in 1/100 cycle, assuming the module clock is FPI1 / RMC. */
	uint32_t cycles100 = (uint32_t) ((uint64_t) timing.byte_ns100 * cpu_get_fpi1_freq() / rmc / 1000000000);
	printf("# %s, %s, %s source: %u.%02u ns = %u.%02u DIF cycles per byte\n",
		name, lane->name, source == DIF_BLIT_SRAM ? "SRAM" : "EXTRAM",
		(unsigned int) (timing.byte_ns100 / 100), (unsigned int) (timing.byte_ns100 % 100),
		(unsigned int) (cycles100 / 100), (unsigned int) (cycles100 % 100));
	printf("# timing: CLC=%08X LCDTIM1=%08X LCDTIM2=%08X CSREG=%08X byte_ns100=%u long_ns=%u short_ns=%u\n",
		(unsigned int) clc, (unsigned int) tim1, (unsigned int) tim2, (unsigned int) lane->bsconf,
		(unsigned int) timing.byte_ns100, (unsigned int) timing.long_ns, (unsigned int) timing.short_ns);
	dif_bus_timing_print_failure(&timing);
	if (!timing.ok)
		test_check(name, false);

	return timing;
}

/* Assert that one byte takes the given number of DIF module clock cycles, within 1%. */
static void check_cycles(const char *name, const struct dif_lane_config *lane, uint32_t rmc,
	uint32_t tim1, uint32_t tim2, uint32_t cycles) {
	struct dif_bus_timing timing = measure(name, lane, DIF_BLIT_SRAM, rmc, tim1, tim2);
	uint32_t expected100 = (uint32_t) ((uint64_t) cycles * rmc * 100000000000ULL / cpu_get_fpi1_freq());
	uint32_t error100 = timing.byte_ns100 > expected100 ?
		timing.byte_ns100 - expected100 : expected100 - timing.byte_ns100;
	char check[96];

	snprintf(check, sizeof(check), "%s takes %u DIF cycles per byte", name, (unsigned int) cycles);
	test_check(check, timing.ok && error100 * 100 <= expected100);
}

/*
 * One parallel byte lasts ((ACCESSCYCLE - 1) mod 32) + 2 module clock cycles: ACCESSCYCLE is
 * effectively 5 bits wide and 0 behaves like 32.
 */
static uint32_t access_cycles(uint32_t access_cycle) {
	return ((access_cycle - 1) & 0x1F) + 2;
}

static void test_access_cycle(void) {
	/*
	 * At RMC=1 periods below 6 cycles are within 5% of the SRAM DMA feed limit; they are checked with
	 * RMC=2 in later categories and reported by test_dma_feed_limit().
	 */
	static const uint32_t VALUES[] = { 4, 5, 6, 7, 8, 16, 31, 32, 0, 33, 48, 63, 64, 127 };
	char name[64];

	test_category("ACCESSCYCLE sets the byte period");
	for (uint32_t v = 0; v < ARRAY_SIZE(VALUES); v++) {
		uint32_t cycles = access_cycles(VALUES[v]);

		snprintf(name, sizeof(name), "ACCESSCYCLE=%u", (unsigned int) VALUES[v]);
		if (cycles < 6) {
			measure(name, &DIF_LANE_2x8, DIF_BLIT_SRAM, 1, dif_lcdtim1(0, VALUES[v], 0), 0);
			continue;
		}
		check_cycles(name, &DIF_LANE_2x8, 1, dif_lcdtim1(0, VALUES[v], 0), 0, cycles);
	}
}

static void test_other_fields(void) {
	static const uint32_t VALUES[] = { 1, 4, 16, 127 };
	static const struct {
		const char *name;
		bool tim2;
		uint32_t shift;
	} FIELDS[] = {
		{ "ADDRDELAY", false, DIF_LCDTIM1_ADDRDELAY_SHIFT },
		{ "DATADELAY", false, DIF_LCDTIM1_DATADELAY_SHIFT },
		{ "CSACT", true, DIF_LCDTIM2_CSACT_SHIFT },
		{ "CSDEACT", true, DIF_LCDTIM2_CSDEACT_SHIFT },
		{ "WRRDACT", true, DIF_LCDTIM2_WRRDACT_SHIFT },
		{ "WRRDDEACT", true, DIF_LCDTIM2_WRRDDEACT_SHIFT },
	};
	char name[64];

	test_category("Edge delays do not change the byte period");
	for (uint32_t f = 0; f < ARRAY_SIZE(FIELDS); f++) {
		for (uint32_t v = 0; v < ARRAY_SIZE(VALUES); v++) {
			uint32_t value = VALUES[v] << FIELDS[f].shift;

			snprintf(name, sizeof(name), "ACCESSCYCLE=8 %s=%u", FIELDS[f].name, (unsigned int) VALUES[v]);
			check_cycles(name, &DIF_LANE_2x8, 1, dif_lcdtim1(0, 8, 0) | (FIELDS[f].tim2 ? 0 : value),
				FIELDS[f].tim2 ? value : 0, 9);
		}
	}
	check_cycles("ACCESSCYCLE=8 with all edge delays 16", &DIF_LANE_2x8, 1,
		dif_lcdtim1(16, 8, 16), dif_lcdtim2(16, 16, 16, 16), 9);
}

static void test_module_clock_divider(void) {
	static const uint32_t RMC[] = { 1, 2, 4 };
	char name[64];

	test_category("Module clock divider");
	printf("# CPU %u Hz, AHB %u Hz, FPI1 %u Hz, DMA %u Hz, STM %u Hz\n",
		(unsigned int) cpu_get_freq(), (unsigned int) cpu_get_ahb_freq(), (unsigned int) cpu_get_fpi1_freq(),
		(unsigned int) cpu_get_dma_freq(), (unsigned int) cpu_get_stm_freq());
	for (uint32_t i = 0; i < ARRAY_SIZE(RMC); i++) {
		snprintf(name, sizeof(name), "ACCESSCYCLE=16 with RMC=%u", (unsigned int) RMC[i]);
		check_cycles(name, &DIF_LANE_2x8, RMC[i], dif_lcdtim1(0, 16, 0), 0, 17);
		snprintf(name, sizeof(name), "EL71 write profile with RMC=%u", (unsigned int) RMC[i]);
		if (RMC[i] == 1) {
			measure(name, &DIF_LANE_2x8, DIF_BLIT_SRAM, RMC[i], EL71_WRITE_TIM1, EL71_WRITE_TIM2);
			continue;
		}
		check_cycles(name, &DIF_LANE_2x8, RMC[i], EL71_WRITE_TIM1, EL71_WRITE_TIM2, 5);
	}
}

static void test_lane_formats(void) {
	const struct dif_lane_config *LANES[] = { &DIF_LANE_1x8, &DIF_LANE_2x8, &DIF_LANE_4x8 };

	test_category("BSCONF byte lanes share the byte period");
	for (uint32_t i = 0; i < ARRAY_SIZE(LANES); i++) {
		check_cycles("ACCESSCYCLE=8", LANES[i], 1, dif_lcdtim1(0, 8, 0), 0, 9);
		check_cycles("EL71 write profile with RMC=2", LANES[i], 2, EL71_WRITE_TIM1, EL71_WRITE_TIM2, 5);
	}
}

static void test_firmware_profiles(void) {
	test_category("EL71 firmware profiles");
	/*
	 * Runtime write profile, boot profile, and GRAM read profile written by the EL71 firmware. RMC=2
	 * keeps the short profiles away from the DMA feed limit of the 26 MHz test clock tree.
	 */
	check_cycles("EL71 write profile with RMC=2", &DIF_LANE_2x8, 2, EL71_WRITE_TIM1, EL71_WRITE_TIM2, 5);
	check_cycles("EL71 boot profile with RMC=2", &DIF_LANE_2x8, 2, 0x00030500, 0x04020501, 6);
	check_cycles("EL71 read profile", &DIF_LANE_2x8, 1, 0x00030F00, 0x05020601, 16);
	measure("EL71 write profile", &DIF_LANE_2x8, DIF_BLIT_EXTRAM, 1, EL71_WRITE_TIM1, EL71_WRITE_TIM2);
}

static void test_dma_feed_limit(void) {
	static const uint32_t VALUES[] = { 1, 2, 3, 4, 5, 6, 33 };
	static const enum dif_blit_source SOURCES[] = { DIF_BLIT_SRAM, DIF_BLIT_EXTRAM };
	char name[64];

	/*
	 * Short access cycles drain the FIFO faster than DMAC can refill it. The floor depends on the
	 * source memory and bus clocks, so it is reported rather than asserted.
	 */
	test_category("DMA feed limit for short access cycles");
	for (uint32_t source = 0; source < ARRAY_SIZE(SOURCES); source++) {
		for (uint32_t v = 0; v < ARRAY_SIZE(VALUES); v++) {
			snprintf(name, sizeof(name), "ACCESSCYCLE=%u", (unsigned int) VALUES[v]);
			struct dif_bus_timing timing = measure(name, &DIF_LANE_2x8, SOURCES[source], 1,
				dif_lcdtim1(0, VALUES[v], 0), 0);
			uint32_t expected100 = (uint32_t) ((uint64_t) access_cycles(VALUES[v]) * 100000000000ULL /
				cpu_get_fpi1_freq());

			snprintf(name, sizeof(name), "ACCESSCYCLE=%u is not faster than LCDTIM", (unsigned int) VALUES[v]);
			test_check(name, timing.ok && timing.byte_ns100 * 100 >= expected100 * 99);
		}
	}
}

int main(void) {
	test_start("DIFv2 parallel bus timings");

	cpu_enable_irq(false);
	if (!test_check("SRAM source buffer fits", dif_bus_timing_init()))
		return test_finish();

	test_access_cycle();
	test_other_fields();
	test_module_clock_divider();
	test_lane_formats();
	test_firmware_profiles();
	test_dma_feed_limit();

	return test_finish();
}

#else

int main(void) {
	test_start("DIFv2 parallel bus timings");
	test_skip("DIFv2 is available", "unsupported");

	return test_finish();
}

#endif
