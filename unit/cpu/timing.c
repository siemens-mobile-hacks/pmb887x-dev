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
	X(arm_drain_write_buffer, "ARM MCR drain write buffer", 1, 6000) \
	X(arm_ldr_use_page_cross, "ARM LDR + dependent ADD across a 4 KiB boundary", 2, 3000) \
	X(arm_ldrh_use_second_page_cross, "ARM LDRH + ADD + dependent ADD across a 4 KiB boundary", 3, 4000) \
	X(thumb_ldr_use_page_cross, "Thumb LDR + dependent ADDS across a 4 KiB boundary", 2, 3000)

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
void timing_arm_code_loop_256(uint32_t iterations, uint32_t *data);
void timing_arm_code_loop_288(uint32_t iterations, uint32_t *data);
void timing_arm_code_loop_1k(uint32_t iterations, uint32_t *data);
void timing_arm_code_loop_16k(uint32_t iterations, uint32_t *data);
void timing_arm_code_loop_32k(uint32_t iterations, uint32_t *data);
void timing_arm_line_ldr(uint32_t iterations, uint32_t *data);
void timing_arm_line_ldr_str_clean(uint32_t iterations, uint32_t *data);

#define ENTRY(fn, name, insns, mcycles) { name, timing_##fn, insns, mcycles },
static const struct workload workloads[] = { WORKLOADS(ENTRY) };

/* EBU runs from AHB or, with SCU_EBUCLC2.FLAG1 set, from the PLL through a synchronizer. */
struct clock_mode {
	const char *name;
	uint32_t ahb_clksel;
	uint32_t ebu_clksel;
	uint32_t cpu_divider;
	uint32_t cpu_hz;
	uint32_t ebu_hz;
	/* Cycles x1000 per 64 KiB blit in the firmware clock modes, 0 in the others. */
	uint32_t blit_mcycles;
};

static const struct clock_mode CLOCK_MODES[] = {
	{
		"ARM 104 MHz, AHB PLL, EBU AHB", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_AHB, 1, 104000000, 104000000,
		468683166,
	},
	{ "ARM 104 MHz, AHB PLL, EBU PLL", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_PLL, 1, 104000000, 104000000, 0 },
	{ "ARM 52 MHz, AHB PLL / 2, EBU AHB", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_AHB, 2, 52000000, 104000000, 0 },
	{
		"ARM 52 MHz, AHB PLL / 2, EBU PLL", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_PLL, 2, 52000000, 104000000,
		440751951,
	},
	{
		"ARM 26 MHz, AHB PLL / 4, EBU AHB", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_AHB, 4, 26000000, 104000000,
		419878199,
	},
	{ "ARM 26 MHz, AHB PLL / 4, EBU PLL", CGU_CON1_AHB_CLKSEL_PLL, CGU_CON2_EBU_CLKSEL_PLL, 4, 26000000, 104000000, 0 },
	{
		"ARM 208 MHz, AHB phase 2, EBU PLL", CGU_CON1_AHB_CLKSEL_PHASE2, CGU_CON2_EBU_CLKSEL_PLL, 1, 208000000,
		104000000, 936321119,
	},
	{
		"ARM 104 MHz, AHB phase 2 / 2, EBU PLL", CGU_CON1_AHB_CLKSEL_PHASE2, CGU_CON2_EBU_CLKSEL_PLL, 2, 104000000,
		104000000, 0,
	},
	{
		"ARM 52 MHz, AHB phase 2 / 4, EBU PLL", CGU_CON1_AHB_CLKSEL_PHASE2, CGU_CON2_EBU_CLKSEL_PLL, 4, 52000000,
		104000000, 0,
	},
	{
		"ARM 6.5 MHz, AHB oscillator / 4, EBU AHB", CGU_CON1_AHB_CLKSEL_BYPASS, CGU_CON2_EBU_CLKSEL_AHB, 4, 6500000,
		26000000, 419878849,
	},
};

/*
 * Straight-line NOP loops from the I-cache: running on into a code line costs a cycle unless the line is one of the
 * CODE_RECENT_LINES latest; a loop longer than that pays every line it runs on into.
 */
#define CODE_LOOP_START_BYTES 12
#define CODE_LOOP_END_BYTES 8
#define CODE_RECENT_LINES 9
#define CODE_LOOP_32K_MCYCLES 4092

static const struct {
	const char *name;
	void (*fn)(uint32_t iterations, uint32_t *data);
	uint32_t bytes;
} CODE_LOOPS[] = {
	{ "256 B code loop", timing_arm_code_loop_256, 256 },
	{ "288 B code loop", timing_arm_code_loop_288, 288 },
	{ "1 KiB code loop", timing_arm_code_loop_1k, 1024 },
	{ "16 KiB code loop", timing_arm_code_loop_16k, 16384 },
};

enum memory_kind {
	MEMORY_SDRAM,
	MEMORY_FLASH,
	MEMORY_SRAM,
	MEMORY_ITCM,
};

/* A cost of the path through EBU in 1/1000 CPU cycles: CPU cycles, EBU cycles and synchronizer CPU cycles. */
struct ebu_cost {
	uint32_t cpu_mcycles;
	uint32_t ebu_mcycles;
	uint32_t sync_mcycles;
};

enum access_law {
	/* The loop takes fixed CPU cycles at any clocks. */
	LAW_FIXED,
	/* Every line stalls the loop until its first word. */
	LAW_FIRST_WORD,
	/* Every line stalls the loop until the whole line, the rest of it a word per EBU cycle. */
	LAW_LINE,
	/* Writes drain behind the instructions until the bus limits the loop. */
	LAW_BUS,
};

struct memory_access {
	const char *name;
	void (*fn)(uint32_t iterations, uint32_t *data);
	enum memory_kind kind;
	uint32_t bytes_per_insn;
	/* Cycles x1000 of one loop iteration from the caches. */
	uint32_t loop_mcycles;
	enum access_law law;
	struct ebu_cost cost;
	uint32_t tolerance_percent;
};

/* Fitted over the CPU, AHB and EBU clock combinations of CLOCK_MODES; the tolerances bound the fit. */
#define SDRAM_FIRST_WORD { 6960, 17200, 2240 }
#define SDRAM_LINE { 5040, 24610, 1690 }
#define SDRAM_WRITE { 1750, 8600, 1650 }
#define SDRAM_WRITE_LINE { 7200, 10750, 3550 }
#define SDRAM_COPY { 20100, 58880, 2440 }
#define FLASH_FIRST_WORD { 6160, 26880, 2240 }
#define FLASH_LINE { 4290, 33250, 2200 }
#define FLASH_CODE_FIRST_WORD { 10080, 27700, 2040 }
#define LINE_BYTES 32
#define LINE_WORDS (LINE_BYTES / 4)
#define COPY_LOOP_MCYCLES 20000
#define NOP_MCYCLES 1000

static const struct memory_access ACCESSES[] = {
	{ "SDRAM LDM 8 words", timing_arm_region_ldm, MEMORY_SDRAM, 32, 12000, LAW_LINE, SDRAM_LINE, 8 },
	{ "SDRAM STM 8 words", timing_arm_region_stm, MEMORY_SDRAM, 32, 12000, LAW_BUS, SDRAM_WRITE_LINE, 10 },
	{ "SDRAM LDR", timing_arm_region_ldr, MEMORY_SDRAM, 4, 5000, LAW_FIRST_WORD, SDRAM_FIRST_WORD, 3 },
	{ "SDRAM STR", timing_arm_region_str, MEMORY_SDRAM, 4, 5000, LAW_BUS, SDRAM_WRITE, 10 },
	{ "SDRAM line LDR", timing_arm_line_ldr, MEMORY_SDRAM, 32, 6000, LAW_LINE, SDRAM_LINE, 12 },
	{ "Flash LDM 8 words", timing_arm_region_ldm, MEMORY_FLASH, 32, 12000, LAW_LINE, FLASH_LINE, 7 },
	{ "Flash LDR", timing_arm_region_ldr, MEMORY_FLASH, 4, 5000, LAW_FIRST_WORD, FLASH_FIRST_WORD, 3 },
	{ "SRAM LDM 8 words", timing_arm_region_ldm, MEMORY_SRAM, 32, 20000, LAW_FIXED, { 0, 0, 0 }, 3 },
	{ "SRAM STM 8 words", timing_arm_region_stm, MEMORY_SRAM, 32, 12000, LAW_FIXED, { 0, 0, 0 }, 3 },
	{ "SRAM LDR", timing_arm_region_ldr, MEMORY_SRAM, 4, 6000, LAW_FIXED, { 0, 0, 0 }, 3 },
	{ "SRAM STR", timing_arm_region_str, MEMORY_SRAM, 4, 5000, LAW_FIXED, { 0, 0, 0 }, 3 },
	{ "ITCM LDM 8 words", timing_arm_region_ldm, MEMORY_ITCM, 32, 28000, LAW_FIXED, { 0, 0, 0 }, 3 },
	{ "ITCM STM 8 words", timing_arm_region_stm, MEMORY_ITCM, 32, 28000, LAW_FIXED, { 0, 0, 0 }, 3 },
	{ "ITCM LDR", timing_arm_region_ldr, MEMORY_ITCM, 4, 7000, LAW_FIXED, { 0, 0, 0 }, 3 },
	{ "ITCM STR", timing_arm_region_str, MEMORY_ITCM, 4, 7000, LAW_FIXED, { 0, 0, 0 }, 3 },
};

enum register_bus {
	BUS_AHB,
	BUS_FPI1,
	BUS_FPI2,
	BUS_FSYS,
	BUS_AHB_PER,
};

struct register_read {
	const char *name;
	uint32_t address;
	enum register_bus bus;
	uint32_t ahb_mcycles;
};

/* Peripheral reads in the address blocks of each bus: 0xF1 FPI1, 0xF4 FPI2, 0xF6 FSYS, 0xF7-0xF8 AHB_PER. */
static const struct register_read REGISTERS[] = {
	{ "VIC_ID", (uint32_t) &VIC_ID, BUS_AHB, 5000 },
	{ "DMAC_CONFIG", (uint32_t) &DMAC_CONFIG, BUS_AHB, 6000 },
	{ "USART0 ID", (uint32_t) &USART_ID(USART0), BUS_FPI1, 0 },
	{ "SSC_ID", (uint32_t) &SSC_ID, BUS_FPI1, 0 },
	{ "SCU_CHIPID", (uint32_t) &SCU_CHIPID, BUS_FPI2, 0 },
	{ "CGU_STAT", (uint32_t) &CGU_STAT, BUS_FPI2, 0 },
	{ "RTC_ID", (uint32_t) &RTC_ID, BUS_FPI2, 0 },
	{ "STM_TIM0", (uint32_t) &STM_TIM0, BUS_FPI2, 0 },
	{ "TPU_ID", (uint32_t) &TPU_ID, BUS_FSYS, 0 },
	{ "DSP_ID", (uint32_t) &DSP_ID, BUS_FSYS, 0 },
	{ "DIF_ID", (uint32_t) &DIF_ID, BUS_AHB_PER, 0 },
	{ "I2C_ID", (uint32_t) &I2C_ID, BUS_AHB_PER, 0 },
	{ "MMICIF_ID", (uint32_t) &MMICIF_ID, BUS_AHB_PER, 0 },
};

/* A read through a bus bridge, beyond a cached LDR, in CPU cycles plus cycles of the bus clock, x1000. */
#define FPI_BRIDGE_CPU_MCYCLES 5410
#define FPI_BRIDGE_BUS_MCYCLES 4620
#define AHB_PER_BRIDGE_CPU_MCYCLES 8670
#define AHB_PER_BRIDGE_BUS_MCYCLES 7330
#define IO_LOOP_MCYCLES 5000
#define REGISTER_TOLERANCE_MCYCLES 1500

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
#define ITCM_BASE 0x01000000
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

static void check_cycles_within(
	const char *name,
	uint32_t mcycles,
	uint32_t expected_mcycles,
	uint32_t instructions,
	uint32_t tolerance_percent
) {
	uint32_t tolerance = MAX(expected_mcycles / 100 * tolerance_percent, 30);

	printf("# %s: %u.%03u cycles for %u instructions, expected %u.%03u\n", name,
		(unsigned int) (mcycles / 1000), (unsigned int) (mcycles % 1000), (unsigned int) instructions,
		(unsigned int) (expected_mcycles / 1000), (unsigned int) (expected_mcycles % 1000));
	test_check(name, test_u32_in_interval(mcycles, expected_mcycles - tolerance, expected_mcycles + tolerance));
}

static void check_cycles(const char *name, uint32_t mcycles, uint32_t expected_mcycles, uint32_t instructions) {
	check_cycles_within(name, mcycles, expected_mcycles, instructions, 3);
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

/* Writes back and drops every D-cache line with the ARM926 test, clean and invalidate loop. */
static void dcache_clean_invalidate(void) {
	__asm__ volatile(
		"1: mrc p15, 0, r15, c7, c14, 3\n"
		"bne 1b\n"
		"mcr p15, 0, %0, c7, c10, 4\n"
		: : "r" (0) : "memory", "cc");
}

/* Workload time per iteration in ps; both runs start with a cold D-cache, their difference cancels it. */
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
		dcache_clean_invalidate();
		stopwatch_t start = stopwatch_get();
		clock_mode_run(mode->ahb_clksel, mode->ebu_clksel, mode->cpu_divider, fn, iterations * (run + 1), data);
		ticks[run] = stopwatch_elapsed(start);
		cpu_enable_irq(!irq_was_disabled);
	}
	return (ticks[1] - ticks[0]) * 1000000 / iterations * 1000000 / stopwatch_ticks_per_s();
}

/* The console divider follows the new FPI1 clock once the last character has left the shift register. */
static void fpi1_select_pll(uint32_t divider) {
	uint32_t old_khz = cpu_get_fpi1_freq() / 1000;
	uint32_t console_divider = USART_BG(USART0) + 1;

	usart_flush(USART0);
	stopwatch_usleep(100);
	cgu_fpi1_select(CGU_FPI1_PLL_DIV_2, divider);
	USART_BG(USART0) = console_divider * (cpu_get_fpi1_freq() / 1000) / old_khz - 1;
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

static uint32_t ebu_cost_mcycles(const struct ebu_cost *cost, const struct clock_mode *mode) {
	bool is_synchronized = mode->ebu_clksel == CGU_CON2_EBU_CLKSEL_PLL;

	return cost->cpu_mcycles +
		(uint64_t) cost->ebu_mcycles * mode->cpu_hz / mode->ebu_hz +
		(is_synchronized ? cost->sync_mcycles : 0);
}

/* After the first word, the rest of a line arrives a word per EBU cycle while the CPU takes a word per cycle. */
static uint32_t line_rest_mcycles(const struct clock_mode *mode) {
	uint32_t rest_mcycles = (LINE_WORDS - 1) * 1000;
	uint32_t rest = (uint64_t) rest_mcycles * mode->cpu_hz / mode->ebu_hz;
	return rest > rest_mcycles ? rest - rest_mcycles : 0;
}

/* Cycles x1000 of one measured instruction with the loop overhead it carries. */
static uint32_t memory_access_mcycles(const struct memory_access *access, const struct clock_mode *mode) {
	uint32_t cost = ebu_cost_mcycles(&access->cost, mode);

	switch (access->law) {
		case LAW_FIXED:
			return access->loop_mcycles;
		case LAW_FIRST_WORD:
			return access->loop_mcycles + cost * access->bytes_per_insn / LINE_BYTES;
		case LAW_LINE:
			return access->loop_mcycles + cost + line_rest_mcycles(mode);
		case LAW_BUS:
			return MAX(access->loop_mcycles, cost);
	}
	return 0;
}

/* Calls a Thumb BX LR found in flash, its I-cache line invalidated first when data[1] is set. */
static void flash_call_loop(uint32_t iterations, uint32_t *data) {
	void (*fn)(void) = (void (*)(void)) (data[0] | 1);
	uint32_t invalidated = data[1] ? data[0] : ITCM_BASE;

	for (uint32_t i = 0; i < iterations; i++) {
		__asm__ volatile("mcr p15, 0, %0, c7, c5, 1" : : "r" (invalidated) : "memory");
		fn();
	}
}

static uint32_t flash_find_bx_lr(void) {
	for (uint32_t addr = FLASH_BASE + 0x1000; addr < FLASH_BASE + 0x400000; addr += 2) {
		if (*(volatile uint16_t *) addr == 0x4770)
			return addr;
	}
	return 0;
}

static uint32_t flash_call_mcycles(const struct clock_mode *mode, uint32_t cpu_hz, uint32_t bx_lr, bool is_miss) {
	uint32_t data[2] = { bx_lr, is_miss };
	return ps_to_mcycles(clock_mode_iteration_ps(mode, flash_call_loop, 1000, data), cpu_hz);
}

static void test_clock_mode_accesses(const struct clock_mode *mode, uint32_t cpu_hz) {
	static const struct ebu_cost COPY = SDRAM_COPY;
	static const struct ebu_cost LINE = SDRAM_LINE;
	static const struct ebu_cost CODE_FIRST_WORD = FLASH_CODE_FIRST_WORD;

	uint32_t copy = ps_to_mcycles(clock_mode_iteration_ps(mode, timing_arm_stream_copy, 10, buffer), cpu_hz) / 2048;
	uint32_t copy_expected = MAX(COPY_LOOP_MCYCLES, ebu_cost_mcycles(&COPY, mode));
	check_cycles_within("SDRAM LDMIA/STMIA copy per 32 bytes", copy, copy_expected, 4, 6);

	uint32_t code = ps_to_mcycles(clock_mode_iteration_ps(mode, timing_arm_code_stream, 4, buffer), cpu_hz) / 16384;
	uint32_t code_expected = NOP_MCYCLES + (ebu_cost_mcycles(&LINE, mode) + line_rest_mcycles(mode)) / LINE_WORDS;
	check_cycles_within("SDRAM code fetch per NOP", code, code_expected, 1, 9);

	uint32_t bx_lr = flash_find_bx_lr();
	if (test_is_qemu()) {
		test_skip("Flash code fetch miss", "QEMU does not model I-cache maintenance");
	} else if (bx_lr) {
		uint32_t miss = flash_call_mcycles(mode, cpu_hz, bx_lr, true) - flash_call_mcycles(mode, cpu_hz, bx_lr, false);
		check_cycles_within("Flash code fetch miss", miss, ebu_cost_mcycles(&CODE_FIRST_WORD, mode), 1, 10);
	} else {
		test_skip("Flash code fetch miss", "no BX LR in the first 4 MiB of flash");
	}
	if (test_is_qemu()) {
		test_skip("SDRAM line LDR + STR + clean of the dirty line", "QEMU does not model the D-cache");
	} else {
		struct memory_region region = { (uint32_t) buffer, REGION_BYTES };
		uint32_t lines = REGION_BYTES / LINE_BYTES;
		uint64_t line_ps = clock_mode_iteration_ps(mode, timing_arm_line_ldr, 4, (uint32_t *) &region);
		uint64_t cleaned_ps = clock_mode_iteration_ps(mode, timing_arm_line_ldr_str_clean, 4, (uint32_t *) &region);
		uint32_t line = ps_to_mcycles(line_ps, cpu_hz) / lines;
		uint32_t cleaned = ps_to_mcycles(cleaned_ps, cpu_hz) / lines;
		check_cycles_within("SDRAM line LDR + STR + clean of the dirty line", cleaned, line, 1, 3);
	}
	if (mode->blit_mcycles) {
		uint64_t blit_ps = clock_mode_iteration_ps(mode, timing_thumb_blit, 10, buffer);
		check_cycles("Thumb 64 KiB color-key blit", ps_to_mcycles(blit_ps, cpu_hz), mode->blit_mcycles, 32768 * 8 + 8);
	}

	for (size_t i = 0; i < ARRAY_SIZE(ACCESSES); i++) {
		const struct memory_access *access = &ACCESSES[i];
		if (test_is_qemu() && access->kind != MEMORY_SDRAM) {
			test_skip(access->name, "QEMU prices streaming data as SDRAM");
			continue;
		}

		struct memory_region region = memory_region_get(access->kind);
		uint32_t instructions = region.bytes / access->bytes_per_insn;
		uint64_t ps = clock_mode_iteration_ps(mode, access->fn, 4, (uint32_t *) &region);
		check_cycles_within(access->name, ps_to_mcycles(ps, cpu_hz) / instructions,
			memory_access_mcycles(access, mode), 1, access->tolerance_percent);
	}
}

static uint32_t register_bus_hz(enum register_bus bus) {
	switch (bus) {
		case BUS_FPI1:
			return cpu_get_fpi1_freq();
		case BUS_FPI2:
			return cpu_get_fpi2_freq();
		case BUS_FSYS:
			return cpu_get_sys_freq();
		case BUS_AHB_PER:
			return cpu_get_ahb_per_freq();
		case BUS_AHB:
			break;
	}
	return 0;
}

/* Cycles x1000 of one timing_arm_io_ldr iteration: the loop, the cached LDR and the bridge to the bus. */
static uint32_t register_read_mcycles(const struct register_read *reg, uint32_t cpu_hz) {
	if (reg->bus == BUS_AHB)
		return IO_LOOP_MCYCLES + reg->ahb_mcycles;

	bool is_ahb_per = reg->bus == BUS_AHB_PER;
	uint32_t cpu_mcycles = is_ahb_per ? AHB_PER_BRIDGE_CPU_MCYCLES : FPI_BRIDGE_CPU_MCYCLES;
	uint32_t bus_mcycles = is_ahb_per ? AHB_PER_BRIDGE_BUS_MCYCLES : FPI_BRIDGE_BUS_MCYCLES;
	return IO_LOOP_MCYCLES + cpu_mcycles + bus_mcycles * (cpu_hz / 1000) / (register_bus_hz(reg->bus) / 1000);
}

static void test_register_reads(const struct clock_mode *mode, uint32_t cpu_hz) {
	for (size_t i = 0; i < ARRAY_SIZE(REGISTERS); i++) {
		const struct register_read *reg = &REGISTERS[i];
		struct memory_region region = { reg->address, IO_READS * 4 };
		uint64_t ps = clock_mode_iteration_ps(mode, timing_arm_io_ldr, 4, (uint32_t *) &region);
		uint32_t mcycles = ps_to_mcycles(ps, cpu_hz) / IO_READS;
		uint32_t expected_mcycles = register_read_mcycles(reg, cpu_hz);
		char name[64];

		snprintf(name, sizeof(name), "%s LDR", reg->name);
		printf("# %s: %u.%03u cycles, expected %u.%03u\n", name,
			(unsigned int) (mcycles / 1000), (unsigned int) (mcycles % 1000),
			(unsigned int) (expected_mcycles / 1000), (unsigned int) (expected_mcycles % 1000));
		test_check(name, test_u32_in_interval(mcycles, expected_mcycles - REGISTER_TOLERANCE_MCYCLES,
			expected_mcycles + REGISTER_TOLERANCE_MCYCLES));
	}
}

static void test_clock_modes(void) {
	bool irq_was_disabled = cpu_enable_irq(false);
	flash_burst_enable();
	cpu_enable_irq(!irq_was_disabled);
	fpi1_select_pll(0);
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
		if (mode->ahb_clksel == CGU_CON1_AHB_CLKSEL_BYPASS) {
			test_skip("Register reads", "peripherals are unreachable with the oscillator AHB");
		} else {
			test_register_reads(mode, cpu_hz);
		}
		test_clock_mode_accesses(mode, cpu_hz);
	}
}

/* Each bridge follows its own bus clock; FPI1 goes last, the console keeps its new divider. */
static void test_bus_clocks(void) {
	const struct clock_mode *base = &CLOCK_MODES[0];

	test_category("Register reads with FPI2 from the PLL at 52 MHz");
	CGU_CON1 = (CGU_CON1 & ~CGU_CON1_FPI2_CLKDIV) | CGU_CON1_FPI2_CLKSEL;
	stopwatch_update();
	test_register_reads(base, base->cpu_hz);

	test_category("Register reads with FSYS from the PLL at 52 MHz");
	cgu_fsys_select(CGU_FSYS_PLL);
	test_register_reads(base, base->cpu_hz);

	test_category("Register reads with AHB_PER from the PLL at 52 MHz");
	CGU_CON3 = (CGU_CON3 & ~(CGU_CON3_AHB_PER_CLKSEL | CGU_CON3_AHB_PER_CLKDIV)) |
		CGU_CON3_AHB_PER_CLKSEL_PLL_DIV_2 |
		CGU_CON3_AHB_PER_CLKDIV_DIV1;
	test_register_reads(base, base->cpu_hz);

	test_category("Register reads with FPI1 at 26 MHz");
	fpi1_select_pll(1);
	test_register_reads(base, base->cpu_hz);
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

	test_category("I-cache capacity, cycles per NOP");
	for (size_t i = 0; i < ARRAY_SIZE(CODE_LOOPS); i++) {
		uint32_t nops = CODE_LOOPS[i].bytes / 4;
		uint32_t lines = (CODE_LOOP_START_BYTES + CODE_LOOPS[i].bytes + CODE_LOOP_END_BYTES - 1) / LINE_BYTES + 1;
		uint32_t line_cycles = lines > CODE_RECENT_LINES ? lines - 1 : 0;
		uint32_t expected = (nops + LOOP_OVERHEAD_CYCLES + line_cycles) * 1000 / nops;
		uint32_t loop = measure_loop_mcycles(CODE_LOOPS[i].fn, 4000000 / nops);
		check_cycles(CODE_LOOPS[i].name, loop / nops, expected, 1);
	}
	uint32_t nops_32k = 32768 / 4;
	uint32_t loop_32k = measure_loop_mcycles(timing_arm_code_loop_32k, 4000000 / nops_32k);
	check_cycles("32 KiB code loop", loop_32k / nops_32k, CODE_LOOP_32K_MCYCLES, 1);

	test_clock_modes();
	test_bus_clocks();
	return test_finish();
}
