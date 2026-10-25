#include <pmb887x.h>
#include <stopwatch.h>

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

#define ENTRY(fn, name, insns, mcycles) { name, timing_##fn, insns, mcycles },
static const struct workload workloads[] = { WORKLOADS(ENTRY) };

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

/*
 * The firmware PLL (104 MHz) with ARM, AHB and EBU on it, and the firmware SDRAM timings (CAS 3). The boot
 * SDRAM geometry stays. The firmware's 208 MHz ARM/AHB clock does not run from SDRAM here.
 */
static __SRAM void clocks_set_pll(void) {
	uint32_t timings = EBU_SDRMCON_CRP | EBU_SDRMCON_CRCD | EBU_SDRMCON_CRC | EBU_SDRMCON_CRCE;
	EBU_SDRMCON(0) = (EBU_SDRMCON(0) & ~timings) | (0x00D7A870 & timings);
	EBU_SDRMREF(0) = (EBU_SDRMREF(0) & ~EBU_SDRMREF_REFRESHC) | (0x00000A19 & EBU_SDRMREF_REFRESHC);
	EBU_SDRMOD(0) = (EBU_SDRMOD(0) & ~EBU_SDRMOD_CASLAT) | (0x00000033 & EBU_SDRMOD_CASLAT);

	uint32_t osc = (CGU_OSC & ~(CGU_OSC_NDIV | CGU_OSC_MDIV | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N)) |
		(3 << CGU_OSC_NDIV_SHIFT);
	CGU_CON1 &= ~(CGU_CON1_AHB_CLKSEL | CGU_CON1_FSYS_CLKSEL);
	CGU_OSC = osc;
	CGU_OSC = osc | CGU_OSC_PLL_POWER_UP;
	while ((CGU_STAT & CGU_STAT_LOCK) == 0);
	CGU_OSC = osc | CGU_OSC_PLL_POWER_UP | CGU_OSC_PLL_BYPASS_N;
	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_EBU_CLKSEL) | CGU_CON2_EBU_CLKSEL_PLL;
	CGU_CON1 |= CGU_CON1_AHB_CLKSEL_PLL;
}

/* CPU cycles per loop iteration, x1000. */
static uint32_t measure_loop_mcycles(void (*fn)(uint32_t iterations, uint32_t *data), uint32_t iterations) {
	buffer[0] = (uint32_t) buffer;
	fn(1, buffer);
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	stopwatch_t start = stopwatch_get();
	fn(iterations, buffer);
	stopwatch_t ticks = stopwatch_elapsed(start);
	cpu_enable_irq(!irq_was_disabled);
	return (uint32_t) (ticks * (cpu_get_freq() / 1000) / (stopwatch_ticks_per_s() / 1000) * 1000 / iterations);
}

/* Expectations come from EL71 at 104 MHz. */
static void check_cycles(const char *name, uint32_t mcycles, uint32_t expected_mcycles, uint32_t instructions) {
	uint32_t tolerance = MAX(expected_mcycles * 3 / 100, 30);

	printf("# %s: %u.%03u cycles for %u instructions, expected %u.%03u\n", name,
		(unsigned int) (mcycles / 1000), (unsigned int) (mcycles % 1000), (unsigned int) instructions,
		(unsigned int) (expected_mcycles / 1000), (unsigned int) (expected_mcycles % 1000));
	test_check(name, test_u32_in_interval(mcycles, expected_mcycles - tolerance, expected_mcycles + tolerance));
}

int main(void) {
	test_start("ARM926 instruction timing");

	bool irq_was_disabled = cpu_enable_irq(false);
	clocks_set_pll();
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
	return test_finish();
}
