#include <pmb887x.h>
#include <stopwatch.h>

#include "cgu/clock.h"
#include "test.h"

#define LOOP_ITERATIONS 100000
#define BODY_REPEATS 8
#define LOOP_OVERHEAD_CYCLES 4
#define STREAM_BYTES (64 * 1024)

struct workload {
	const char *name;
	void (*fn)(uint32_t iterations, uint32_t *data);
	uint32_t body_instructions;
	uint32_t expected_mcycles;
};

#define WORKLOADS(X) \
	X(arm_empty, "ARM NOP", 1, 1000) \
	X(arm_alu, "ARM ADD imm", 1, 1000) \
	X(arm_alu_shift_imm, "ARM ADD LSL imm", 1, 1000) \
	X(arm_alu_shift_reg, "ARM ADD LSL reg", 1, 2000) \
	X(arm_cond_skipped, "ARM ADDEQ not executed", 1, 1000) \
	X(arm_mul, "ARM MUL", 1, 2000) \
	X(arm_muls, "ARM MULS", 1, 4000) \
	X(arm_mla, "ARM MLA", 1, 2000) \
	X(arm_umull, "ARM UMULL", 1, 3000) \
	X(arm_smlal, "ARM SMLAL", 1, 3000) \
	X(arm_smulbb, "ARM SMULBB", 1, 1000) \
	X(arm_clz, "ARM CLZ", 1, 1000) \
	X(arm_qadd, "ARM QADD", 1, 1000) \
	X(arm_mul_use, "ARM MUL + dependent ADD", 2, 4000) \
	X(arm_b, "ARM B taken", 1, 3000) \
	X(arm_b_not_taken, "ARM BEQ not taken", 1, 1000) \
	X(arm_bl, "ARM BL", 1, 3000) \
	X(arm_bx, "ARM ADR + BX", 2, 4000) \
	X(arm_mov_pc, "ARM ADD PC", 1, 3000) \
	X(arm_ldr, "ARM LDR", 1, 1000) \
	X(arm_ldr_use, "ARM LDR + dependent ADD", 2, 3000) \
	X(arm_ldr_use_second, "ARM LDR + ADD + dependent ADD", 3, 3000) \
	X(arm_ldr_use_address, "ARM LDR + dependent LDR address", 2, 3000) \
	X(arm_ldrb_use, "ARM LDRB + dependent ADD", 2, 4000) \
	X(arm_ldrh_use, "ARM LDRH + dependent ADD", 2, 4000) \
	X(arm_ldr_pc, "ARM LDR PC", 1, 5000) \
	X(arm_str, "ARM STR", 1, 1000) \
	X(arm_strb, "ARM STRB", 1, 1000) \
	X(arm_ldm1, "ARM LDM 1 reg", 1, 2000) \
	X(arm_ldm2, "ARM LDM 2 regs", 1, 2000) \
	X(arm_ldm4, "ARM LDM 4 regs", 1, 4000) \
	X(arm_ldm8, "ARM LDM 8 regs", 1, 8000) \
	X(arm_stm1, "ARM STM 1 reg", 1, 2000) \
	X(arm_stm2, "ARM STM 2 regs", 1, 2000) \
	X(arm_stm4, "ARM STM 4 regs", 1, 4000) \
	X(arm_stm8, "ARM STM 8 regs", 1, 8000) \
	X(arm_ldm_pc, "ARM ADR + LDM {r2, pc}", 2, 7000) \
	X(arm_mrs, "ARM MRS", 1, 2000) \
	X(arm_msr_flags, "ARM MSR cpsr_f", 1, 1000) \
	X(arm_msr_control, "ARM MSR cpsr_c", 1, 3250) \
	X(thumb_alu, "Thumb ADDS imm", 1, 1000) \
	X(thumb_b, "Thumb B taken", 1, 3000) \
	X(thumb_b_not_taken, "Thumb BEQ not taken", 1, 1000) \
	X(thumb_bl, "Thumb BL", 1, 4000) \
	X(thumb_ldr_use, "Thumb LDR + dependent ADDS", 2, 3000) \
	X(thumb_ldrh_use, "Thumb LDRH + dependent ADDS", 2, 4000) \
	X(thumb_ldr_literal, "Thumb LDR literal", 1, 1000) \
	X(thumb_push_pop, "Thumb PUSH + POP 2 regs", 2, 4000) \
	X(arm_ldr_str_data, "ARM LDR + STR of the loaded reg", 2, 3000) \
	X(arm_ldr_shift_reg, "ARM LDR + dependent shift reg", 2, 4000) \
	X(arm_ldrh_use_second, "ARM LDRH + ADD + dependent ADD", 3, 4000) \
	X(arm_ldr_ldm_base, "ARM LDR + dependent LDM base", 2, 4000) \
	X(arm_ldm_use, "ARM LDM 2 + use of the last reg", 2, 4000) \
	X(arm_ldm_use_first, "ARM LDM 2 + use of the first reg", 2, 3000) \
	X(arm_mul_mla_acc, "ARM MUL + dependent MLA acc", 2, 4000) \
	X(arm_mul_use_second, "ARM MUL + ADD + dependent ADD", 3, 4000) \
	X(arm_ldr_writeback_use, "ARM LDR post-index + base use", 2, 2000) \
	X(arm_bne_taken, "ARM BNE taken", 1, 3000) \
	X(arm_icache_line_invalidate, "ARM MCR I-cache line invalidate", 1, 12000) \
	X(arm_dcache_line_invalidate, "ARM MCR D-cache line invalidate", 1, 11125) \
	X(arm_drain_write_buffer, "ARM MCR drain write buffer", 1, 6000)

#define DECLARE(fn, name, insns, mcycles) void timing_##fn(uint32_t iterations, uint32_t *data);
WORKLOADS(DECLARE)
void timing_thumb_call(uint32_t iterations, uint32_t *data);
void timing_thumb_pop_pc_call(uint32_t iterations, uint32_t *data);
void timing_arm_stream_copy(uint32_t iterations, uint32_t *data);
void timing_thumb_blit(uint32_t iterations, uint32_t *data);
void timing_arm_region_ldm(uint32_t iterations, uint32_t *data);
void timing_arm_region_stm(uint32_t iterations, uint32_t *data);
void timing_arm_region_ldr(uint32_t iterations, uint32_t *data);
void timing_arm_region_str(uint32_t iterations, uint32_t *data);
void timing_arm_io_ldr(uint32_t iterations, uint32_t *data);
void timing_arm_code_stream(uint32_t iterations, uint32_t *data);

#define ENTRY(fn, name, insns, mcycles) { name, timing_##fn, insns, mcycles },
static const struct workload workloads[] = { WORKLOADS(ENTRY) };

#define CLOCK_MODE_COUNT 5

struct clock_mode {
	const char *name;
	uint32_t ahb_clksel;
	uint32_t ebu_clksel;
	uint32_t cpu_divider;
	uint32_t cpu_hz;
	uint32_t copy_mcycles;
	uint32_t blit_mcycles;
	uint32_t code_fetch_mcycles;
};

static const struct clock_mode CLOCK_MODES[CLOCK_MODE_COUNT] = {
	{
		"ARM 104 MHz, AHB PLL, EBU AHB", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_AHB, 1, 104000000,
		163038842, 468683166, 4728,
	},
	{
		"ARM 52 MHz, AHB PLL / 2, EBU PLL", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_PLL, 2, 52000000,
		104559621, 440751951, 3295,
	},
	{
		"ARM 26 MHz, AHB PLL / 4, EBU AHB", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_AHB, 4, 26000000,
		71538899, 419878199, 2399,
	},
	{
		"ARM 6.5 MHz, AHB oscillator / 4, EBU AHB", CGU_CON1_AHB_CLKSEL_BYPASS, CGU_CON2_EBU_CLKSEL_AHB, 4, 6500000,
		71539599, 419878849, 2399,
	},
	{
		"ARM 208 MHz, AHB phase 2, EBU PLL", CGU_CON1_AHB_CLKSEL_PHASE2, CGU_CON2_EBU_CLKSEL_PLL, 1, 208000000,
		299570392, 936321119, 8957,
	},
};

enum memory_kind {
	MEMORY_SDRAM,
	MEMORY_FLASH,
	MEMORY_SRAM,
	MEMORY_ITCM,
	MEMORY_FPI_REGISTER,
	MEMORY_AHB_REGISTER,
};

struct memory_access {
	const char *name;
	void (*fn)(uint32_t iterations, uint32_t *data);
	enum memory_kind kind;
	uint32_t bytes_per_insn;
	uint32_t expected_mcycles[CLOCK_MODE_COUNT];
};

static const struct memory_access ACCESSES[] = {
	{ "SDRAM LDM 8 words", timing_arm_region_ldm, MEMORY_SDRAM, 32, { 41759, 30390, 23214, 23213, 75662 } },
	{ "SDRAM STM 8 words", timing_arm_region_stm, MEMORY_SDRAM, 32, { 17014, 15211, 11997, 11999, 30343 } },
	{ "SDRAM LDR", timing_arm_region_ldr, MEMORY_SDRAM, 4, { 7975, 7286, 6405, 6405, 10493 } },
	{ "SDRAM STR", timing_arm_region_str, MEMORY_SDRAM, 4, { 10231, 7505, 4999, 4999, 20110 } },
	{ "Flash LDM 8 words", timing_arm_region_ldm, MEMORY_FLASH, 32, { 49500, 33718, 24889, 24888, 91034 } },
	{ "Flash LDR", timing_arm_region_ldr, MEMORY_FLASH, 4, { 9031, 7759, 6631, 6631, 12817 } },
	{ "SRAM LDM 8 words", timing_arm_region_ldm, MEMORY_SRAM, 32, { 20026, 20017, 20011, 20011, 20053 } },
	{ "SRAM STM 8 words", timing_arm_region_stm, MEMORY_SRAM, 32, { 11992, 11998, 11999, 12002, 11986 } },
	{ "SRAM LDR", timing_arm_region_ldr, MEMORY_SRAM, 4, { 6004, 6003, 6001, 6001, 6010 } },
	{ "SRAM STR", timing_arm_region_str, MEMORY_SRAM, 4, { 4999, 4999, 5000, 5000, 4997 } },
	{ "ITCM LDM 8 words", timing_arm_region_ldm, MEMORY_ITCM, 32, { 28034, 27911, 27976, 27976, 28054 } },
	{ "ITCM STM 8 words", timing_arm_region_stm, MEMORY_ITCM, 32, { 28049, 28051, 28044, 28044, 28069 } },
	{ "ITCM LDR", timing_arm_region_ldr, MEMORY_ITCM, 4, { 7006, 7006, 7005, 7005, 7009 } },
	{ "ITCM STR", timing_arm_region_str, MEMORY_ITCM, 4, { 6998, 7006, 7004, 7005, 7007 } },
	{ "FPI register LDR", timing_arm_io_ldr, MEMORY_FPI_REGISTER, 4, { 28001, 20001, 15001, 0, 47995 } },
	{ "AHB register LDR", timing_arm_io_ldr, MEMORY_AHB_REGISTER, 4, { 11000, 11003, 11007, 0, 10965 } },
};

#define BASE_AHB_CLKSEL CGU_CON1_AHB_CLKSEL_PLL
#define BASE_EBU_CLKSEL CGU_CON2_EBU_CLKSEL_AHB
#define EBUCLC2_EBU_PLL (1 << SCU_EBUCLC2_FLAG1_SHIFT)
#define EBUCLC2_EBU_PLL_READY (1 << SCU_EBUCLC2_READY_SHIFT)

struct memory_region {
	uint32_t base;
	uint32_t bytes;
};

#define FLASH_BASE 0xA0000000
#define SRAM_END 0x00098000
#define ITCM_END 0x01002000
#define REGION_BYTES (64 * 1024)
#define ITCM_REGION_BYTES (4 * 1024)
#define IO_READS 1024

extern uint32_t _sram_end;
extern uint32_t _tcm_end;

static uint32_t buffer[2 * STREAM_BYTES / 4] __attribute__((aligned(32)));
static uint32_t translation_table[4096] __attribute__((aligned(16384)));

static uint32_t cp15_control(void) {
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c1, c0, 0" : "=r" (value));
	return value;
}

/* Flat sections; internal SRAM (stacks) and EBU memory are write-back cacheable as in the firmware. */
static void mmu_enable(void) {
	for (uint32_t section = 0; section < 4096; section++) {
		bool cacheable = section == 0 || (section >= 0xA00 && section < 0xC00);
		translation_table[section] = (section << 20) | (3 << 10) | (1 << 4) | (cacheable ? (1 << 3) | (1 << 2) : 0) | 2;
	}

	__asm__ volatile(
		"mcr p15, 0, %0, c7, c10, 4\n"
		"mcr p15, 0, %0, c7, c7, 0\n"
		"mcr p15, 0, %0, c8, c7, 0\n"
		"mcr p15, 0, %1, c2, c0, 0\n"
		"mcr p15, 0, %2, c3, c0, 0\n"
		: : "r" (0), "r" (translation_table), "r" (3) : "memory");
	uint32_t control = cp15_control() | (1 << 12) | (1 << 2) | 1;
	__asm__ volatile("mcr p15, 0, %0, c1, c0, 0\n" "nop\n" "nop\n" : : "r" (control) : "memory");
}

static __SRAM void ebu_pll_flag_write(bool is_pll) {
	if (is_pll) {
		SCU_EBUCLC2 |= EBUCLC2_EBU_PLL;
	} else {
		SCU_EBUCLC2 &= ~EBUCLC2_EBU_PLL;
	}
	while ((SCU_EBUCLC2 & EBUCLC2_EBU_PLL_READY) == 0);
}

/* Writes in the firmware's order. SCU is unreadable with the oscillator AHB, so the caller passes the old sources. */
static __SRAM void clocks_set_mode(
	uint32_t ahb_clksel,
	uint32_t ebu_clksel,
	uint32_t cpu_divider,
	uint32_t old_ahb_clksel,
	uint32_t old_ebu_clksel
) {
	uint32_t con2_mask = CGU_CON2_EBU_CLKSEL | CGU_CON2_CPU_DIV_EN | CGU_CON2_CPU_DIV;
	uint32_t cpu_div = cpu_divider > 1 ?
		CGU_CON2_CPU_DIV_EN | ((cpu_divider - 1) << CGU_CON2_CPU_DIV_SHIFT) :
		0;
	uint32_t con2 = (CGU_CON2 & ~con2_mask) | ebu_clksel | cpu_div;
	uint32_t con1 = (CGU_CON1 & ~CGU_CON1_AHB_CLKSEL) | ahb_clksel;
	bool is_ebu_pll = ebu_clksel == CGU_CON2_EBU_CLKSEL_PLL;
	bool was_ebu_pll = old_ebu_clksel == CGU_CON2_EBU_CLKSEL_PLL;
	/* Only the oscillator AHB moves the EBU clock away from 104 MHz; the firmware does that around WFI. */
	bool is_ebu_clock_moving = ahb_clksel == CGU_CON1_AHB_CLKSEL_BYPASS || old_ahb_clksel == CGU_CON1_AHB_CLKSEL_BYPASS;

	if (is_ebu_clock_moving) {
		EBU_SDRMREF(0) |= EBU_SDRMREF_SELFREN;
		EBU_SDRMREF(0) &= ~EBU_SDRMREF_SELFREN;
	}
	if (is_ebu_pll) {
		CGU_CON2 = con2;
		if (!was_ebu_pll)
			ebu_pll_flag_write(true);
		CGU_CON1 = con1;
	} else {
		CGU_CON1 = con1;
		if (was_ebu_pll)
			ebu_pll_flag_write(false);
		CGU_CON2 = con2;
	}
	if (is_ebu_clock_moving) {
		EBU_SDRMREF(0) |= EBU_SDRMREF_SELFREX;
		EBU_SDRMREF(0) &= ~EBU_SDRMREF_SELFREX;
	}
}

/* CPU cycles per loop iteration, x1000. */
static uint32_t measure_loop_mcycles(void (*fn)(uint32_t iterations, uint32_t *data), uint32_t iterations) {
	buffer[0] = (uint32_t) buffer;
	fn(1, buffer);
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	/* Stores do not allocate cache lines: load the data line so cache-hit stores do not depend on the layout. */
	(void) *(volatile uint32_t *) buffer;
	stopwatch_t start = stopwatch_get();
	fn(iterations, buffer);
	stopwatch_t ticks = stopwatch_elapsed(start);
	cpu_enable_irq(!irq_was_disabled);
	return (uint32_t) (ticks * (cpu_get_freq() / 1000) / (stopwatch_ticks_per_s() / 1000) * 1000 / iterations);
}

static void check_cycles(const char *name, uint32_t mcycles, uint32_t expected_mcycles, uint32_t instructions) {
	uint32_t tolerance = MAX(expected_mcycles * 3 / 100, 30);

	printf("# %s: %u.%03u cycles for %u instructions, expected %u.%03u\n", name,
		(unsigned int) (mcycles / 1000), (unsigned int) (mcycles % 1000), (unsigned int) instructions,
		(unsigned int) (expected_mcycles / 1000), (unsigned int) (expected_mcycles % 1000));
	test_check(name, test_u32_in_interval(mcycles, expected_mcycles - tolerance, expected_mcycles + tolerance));
}

/* Runs from ITCM so that the clock switches fetch no code over EBU. */
static __attribute__((section(".tcm"), noinline)) void clock_mode_run(
	uint32_t ahb_clksel,
	uint32_t ebu_clksel,
	uint32_t cpu_divider,
	void (*fn)(uint32_t iterations, uint32_t *data),
	uint32_t iterations,
	uint32_t *data
) {
	clocks_set_mode(ahb_clksel, ebu_clksel, cpu_divider, BASE_AHB_CLKSEL, BASE_EBU_CLKSEL);
	fn(iterations, data);
	clocks_set_mode(BASE_AHB_CLKSEL, BASE_EBU_CLKSEL, 1, ahb_clksel, ebu_clksel);
}

/* Workload time per iteration in ps; the two runs cancel the mode switches and cold caches. */
static uint64_t clock_mode_iteration_ps(
	const struct clock_mode *mode,
	void (*fn)(uint32_t iterations, uint32_t *data),
	uint32_t iterations,
	uint32_t *data
) {
	uint64_t ticks[2];

	buffer[0] = (uint32_t) buffer;
	for (uint32_t run = 0; run < 2; run++) {
		test_watchdog_serve();
		bool irq_was_disabled = cpu_enable_irq(false);
		stopwatch_t start = stopwatch_get();
		clock_mode_run(mode->ahb_clksel, mode->ebu_clksel, mode->cpu_divider, fn, iterations * (run + 1), data);
		ticks[run] = stopwatch_elapsed(start);
		cpu_enable_irq(!irq_was_disabled);
	}
	return (ticks[1] - ticks[0]) * 1000000 / iterations * 1000000 / stopwatch_ticks_per_s();
}

/* The firmware keeps FPI1 at PLL / 2 in every clock mode. */
static void fpi1_select_pll(void) {
	uint32_t old_hz = cpu_get_fpi1_freq();
	uint32_t console_divider = USART_BG(USART0) + 1;

	usart_flush(USART0);
	cgu_fpi1_select(CGU_FPI1_PLL_DIV_2, 0);
	USART_BG(USART0) = console_divider * (cpu_get_fpi1_freq() / old_hz) - 1;
}

static struct memory_region memory_region_get(enum memory_kind kind) {
	switch (kind) {
		case MEMORY_SDRAM:
			return (struct memory_region) { (uint32_t) buffer, REGION_BYTES };
		case MEMORY_FLASH:
			return (struct memory_region) { FLASH_BASE, REGION_BYTES };
		case MEMORY_SRAM: {
			uint32_t base = ((uint32_t) &_sram_end + 31) & ~31U;
			return (struct memory_region) { base, MIN(REGION_BYTES, (SRAM_END - base) & ~31U) };
		}
		case MEMORY_ITCM: {
			uint32_t base = ((uint32_t) &_tcm_end + 31) & ~31U;
			return (struct memory_region) { base, MIN(ITCM_REGION_BYTES, (ITCM_END - base) & ~31U) };
		}
		case MEMORY_FPI_REGISTER:
			return (struct memory_region) { (uint32_t) &SCU_CHIPID, IO_READS * 4 };
		case MEMORY_AHB_REGISTER:
			return (struct memory_region) { (uint32_t) &DMAC_CONFIG, IO_READS * 4 };
	}
	return (struct memory_region) { 0, 0 };
}

static uint32_t ps_to_mcycles(uint64_t ps, uint32_t cpu_hz) {
	return (uint32_t) (ps * (cpu_hz / 1000) / 1000000);
}

/* The firmware's synchronous burst flash setup. */
static __SRAM void flash_burst_enable(void) {
	volatile uint16_t *rcr = (volatile uint16_t *) (FLASH_BASE + (0x5003 << 1));
	*rcr = 0x60;
	*rcr = 0x03;
	*rcr = 0xFF;
	EBU_BUSCON(0) = 0xA0522E00;
	EBU_BFCON = 0x00000403;
	EBU_BUSCON(0) = 0xA2520E00;
	EBU_BUSAP(0) = 0x8CE11222;
	EBU_BFCON = 0x00004D03;
}

static void test_clock_mode_accesses(const struct clock_mode *mode, size_t mode_index, uint32_t cpu_hz) {
	uint64_t copy_ps = clock_mode_iteration_ps(mode, timing_arm_stream_copy, 10, buffer);
	uint64_t blit_ps = clock_mode_iteration_ps(mode, timing_thumb_blit, 10, buffer);
	uint64_t code_ps = clock_mode_iteration_ps(mode, timing_arm_code_stream, 4, buffer);

	check_cycles("ARM 64 KiB LDMIA/STMIA copy", ps_to_mcycles(copy_ps, cpu_hz), mode->copy_mcycles, 2048 * 4 + 5);
	check_cycles("Thumb 64 KiB color-key blit", ps_to_mcycles(blit_ps, cpu_hz), mode->blit_mcycles, 32768 * 8 + 8);
	check_cycles("SDRAM code fetch per NOP", ps_to_mcycles(code_ps, cpu_hz) / 16384, mode->code_fetch_mcycles, 1);

	for (size_t i = 0; i < ARRAY_SIZE(ACCESSES); i++) {
		const struct memory_access *access = &ACCESSES[i];
		bool is_register = access->kind == MEMORY_FPI_REGISTER || access->kind == MEMORY_AHB_REGISTER;
		if (is_register && mode->ahb_clksel == CGU_CON1_AHB_CLKSEL_BYPASS) {
			test_skip(access->name, "registers are unreachable with the oscillator AHB");
			continue;
		}

		struct memory_region region = memory_region_get(access->kind);
		uint32_t instructions = region.bytes / access->bytes_per_insn;
		uint64_t ps = clock_mode_iteration_ps(mode, access->fn, 4, (uint32_t *) &region);
		check_cycles(access->name, ps_to_mcycles(ps, cpu_hz) / instructions, access->expected_mcycles[mode_index], 1);
	}
}

static void test_clock_modes(void) {
	bool irq_was_disabled = cpu_enable_irq(false);
	flash_burst_enable();
	cpu_enable_irq(!irq_was_disabled);
	fpi1_select_pll();
	cgu_phase_set_divider(2, 1, 0);
	cgu_phases_enable(CGU_PHASE2);
	clocks_set_mode(BASE_AHB_CLKSEL, BASE_EBU_CLKSEL, 1, CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_PLL);

	for (size_t i = 0; i < ARRAY_SIZE(CLOCK_MODES); i++) {
		const struct clock_mode *mode = &CLOCK_MODES[i];
		uint64_t alu_ps = clock_mode_iteration_ps(mode, timing_arm_alu, LOOP_ITERATIONS, buffer);
		/* One ARM ADD per cycle: 8 body instructions plus the SUBS + BNE loop overhead. */
		uint32_t cpu_hz = (uint32_t) ((BODY_REPEATS + LOOP_OVERHEAD_CYCLES) * 1000000000000ULL / alu_ps);
		char name[64];

		test_category(mode->name);
		snprintf(name, sizeof(name), "ARM runs at %u Hz", (unsigned int) mode->cpu_hz);
		printf("# ARM ADD loop measures %u Hz\n", (unsigned int) cpu_hz);
		test_check(name, test_u32_in_interval(cpu_hz, mode->cpu_hz / 100 * 98, mode->cpu_hz / 100 * 102));
		if (test_is_qemu()) {
			test_skip("Memory and register access cycles", "QEMU does not model memory stalls");
			continue;
		}
		test_clock_mode_accesses(mode, i, cpu_hz);
	}
}

int main(void) {
	test_start("ARM926 instruction timing");

	bool irq_was_disabled = cpu_enable_irq(false);
	cgu_firmware_pll_enable();
	cpu_enable_irq(!irq_was_disabled);
	stopwatch_update();
	mmu_enable();
	printf("# CPU %u Hz, EBU %u Hz, MMU with write-back I/D-caches\n", (unsigned int) cpu_get_freq(),
		(unsigned int) cpu_get_ebu_freq());

	test_category("Cache hits, cycles per measured sequence");
	for (size_t i = 0; i < ARRAY_SIZE(workloads); i++) {
		uint32_t loop = measure_loop_mcycles(workloads[i].fn, LOOP_ITERATIONS);
		uint32_t body = loop > LOOP_OVERHEAD_CYCLES * 1000 ? (loop - LOOP_OVERHEAD_CYCLES * 1000) / BODY_REPEATS : 0;
		check_cycles(workloads[i].name, body, workloads[i].expected_mcycles, workloads[i].body_instructions);
	}

	test_category("Calls, cycles per loop iteration");
	check_cycles("Thumb 8x BL + PUSH {lr} + POP {pc}", measure_loop_mcycles(timing_thumb_call, LOOP_ITERATIONS),
		95000, 8 * 3 + 3);
	check_cycles("Thumb 8x BL + PUSH {r2, lr} + POP {r2, pc}",
		measure_loop_mcycles(timing_thumb_pop_pc_call, LOOP_ITERATIONS), 103000, 8 * 3 + 3);

	test_category("D-cache misses, cycles per loop iteration");
	if (test_is_qemu()) {
		test_skip("64 KiB copy and blit", "QEMU does not model cache misses");
	} else {
		check_cycles("ARM 64 KiB LDMIA/STMIA copy", measure_loop_mcycles(timing_arm_stream_copy, 20), 163040000,
			2048 * 4 + 5);
		check_cycles("Thumb 64 KiB color-key blit", measure_loop_mcycles(timing_thumb_blit, 20), 468630000,
			32768 * 8 + 8);
	}

	test_clock_modes();
	return test_finish();
}
