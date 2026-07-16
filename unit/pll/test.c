#include <pmb887x.h>

#include "test.h"

#define ITCM_BASE 0x01000000
#define TCM_REGION_SIZE_8K (4 << 2)
#define TCM_REGION_ENABLE BIT(0)
#define PLL_MEASURE_NOPS 512
#define PLL_MEASURE_ITERATIONS 1000
#define PLL_DIVIDER_MEASURE_ITERATIONS 1000
#define RTC_T14_RELOAD 61440
#define RTC_T14_PERIOD (65536 - RTC_T14_RELOAD)
#define RTC_T14_MEASURE_TICKS 512
#define CLOCK_MANAGER_WAIT_ITERATIONS 100000
#define USART_IRQ_MASK 0xFF

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;
static volatile uint32_t tpu_frame_count;

uint32_t pll_execute_itcm(uint32_t address, uint32_t iterations);
void pll_execute_itcm_source(uint32_t address, uint32_t iterations, uint32_t selected_con1, uint32_t restore_con1);
void pll_execute_itcm_cpu_div(uint32_t address, uint32_t iterations, uint32_t selected_con2, uint32_t restore_con2);
extern const uint32_t pll_cpu_div_itcm_template_start[];
extern const uint32_t pll_cpu_div_itcm_template_end[];
static void test_measured_frequency(const char *name, uint32_t expected_khz, uint32_t measured_khz);

static uint32_t read_itcm(void) {
	uint32_t value;
	__asm__ volatile("mrc p15, 0, %0, c9, c1, 1" : "=r" (value));
	return value;
}

static void write_itcm(uint32_t value) {
	__asm__ volatile("mcr p15, 0, %0, c9, c1, 1" : : "r" (value) : "memory");
}

static void sync_code(void) {
	uint32_t value = 0;
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (value) : "memory");
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r" (value) : "memory");
}

static bool wait_for_irq(void) {
	stopwatch_t start = stopwatch_get();

	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 100) {
		test_watchdog_serve();
	}

	return irq_count != 0;
}

static bool wait_for_pll_lock(void) {
	stopwatch_t start = stopwatch_get();

	while ((PLL_STAT & PLL_STAT_LOCK) == 0 && stopwatch_elapsed_ms(start) < 20) {
		test_watchdog_serve();
	}

	return (PLL_STAT & PLL_STAT_LOCK) != 0;
}

static bool wait_for_pll_unlock(void) {
	stopwatch_t start = stopwatch_get();

	while ((PLL_STAT & PLL_STAT_LOCK) != 0 && stopwatch_elapsed_ms(start) < 20)
		test_watchdog_serve();

	return (PLL_STAT & PLL_STAT_LOCK) == 0;
}

static bool apply_pll_osc(uint32_t value) {
	PLL_OSC = value & ~(PLL_OSC_PLL_POWER_UP | PLL_OSC_PLL_BYPASS_N);
	if ((value & PLL_OSC_PLL_POWER_UP) == 0) {
		return true;
	}

	PLL_OSC = (value | PLL_OSC_PLL_POWER_UP) & ~PLL_OSC_PLL_BYPASS_N;
	if (!wait_for_pll_lock()) {
		return false;
	}

	PLL_OSC = value;
	return true;
}

static void test_reset_values(void) {
	/* PLL clock registers are live boot configuration and cannot be reset while executing from this clock tree. */
	/* PLL_SRC is omitted because its lock request is already pending asynchronously after boot. */
	test_check("PLL is powered after boot", (PLL_OSC & PLL_OSC_PLL_POWER_UP) != 0);
	test_eq_u32("PLL is locked after boot", PLL_STAT_LOCK, PLL_STAT & PLL_STAT_LOCK);
	printf(
		"# PLL boot configuration: OSC=%08X CON0=%08X CON1=%08X CON2=%08X CON3=%08X\n",
		(unsigned int) PLL_OSC,
		(unsigned int) PLL_CON0,
		(unsigned int) PLL_CON1,
		(unsigned int) PLL_CON2,
		(unsigned int) PLL_CON3
	);
}

static void test_boot_clock_calculation(void) {
	if ((PLL_CON1 & PLL_CON1_AHB_CLKSEL) == PLL_CON1_AHB_CLKSEL_BYPASS) {
		test_eq_u32("AHB bypass clock matches the oscillator frequency", CPU_OSC_FREQ, cpu_get_ahb_freq());
	} else {
		test_skip("AHB bypass clock matches the CPU-specific frequency", "boot selects another AHB source");
	}

	if (
		(PLL_CON1 & PLL_CON1_AHB_CLKSEL) == PLL_CON1_AHB_CLKSEL_BYPASS &&
		(PLL_CON2 & PLL_CON2_CPU_DIV_EN) == 0
	) {
		test_eq_u32("undivided CPU follows the AHB bypass clock", CPU_OSC_FREQ, cpu_get_freq());
	} else {
		test_skip("undivided CPU follows the AHB bypass clock", "boot uses another source or divider");
	}
}

static void init_example_rtc(void) {
	SCU_RTCIF = 0xAA;
	RTC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	RTC_CTRL |= RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN;
	RTC_CON |= RTC_CON_PRE;
	RTC_T14 = (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT);
	RTC_REL = 0;
	RTC_ALARM = 0;
	RTC_SRC = 0;
	RTC_ISNC = 0;
	RTC_CTRL |= RTC_CTRL_CLK_SEL | RTC_CTRL_CLR_RTCBAD | RTC_CTRL_CLR_RTCINT;
	RTC_CON |= RTC_CON_RUN;
}

static void init_example_tpu(void) {
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_GSMCLK1 = 1 << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = 1 << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_INIT;
	for (uint32_t index = 0; index < 512; index++)
		TPU_RAM(index) = 0;
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 0;
	TPU_INT(0) = 0;
	TPU_INT(1) = 30000;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	TPU_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_CLRR;
}

static void benchmark_example_clocks(uint32_t *tpu_frames, uint32_t *stm_ticks) {
	test_watchdog_serve();
	uint32_t start = RTC_CNT + 1;
	while (RTC_CNT < start)
		test_watchdog_serve();
	uint32_t first_tpu = tpu_frame_count;
	stopwatch_t first_stm = stopwatch_get();
	while (RTC_CNT - start < 1)
		test_watchdog_serve();
	*tpu_frames = tpu_frame_count - first_tpu;
	*stm_ticks = (uint32_t) stopwatch_elapsed(first_stm);
	test_watchdog_serve();
}

static uint32_t benchmark_stm_ticks(void) {
	test_watchdog_serve();
	uint32_t first_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
	stopwatch_t first_stm = stopwatch_get();
	uint32_t elapsed_t14;
	do {
		uint32_t current_t14 = ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) - RTC_T14_RELOAD;
		elapsed_t14 = (current_t14 - first_t14) & (RTC_T14_PERIOD - 1);
		test_watchdog_serve();
	} while (elapsed_t14 < RTC_T14_MEASURE_TICKS);
	return (uint32_t) stopwatch_elapsed(first_stm);
}

static uint32_t stm_ticks_to_khz(uint32_t stm_ticks) {
	return (uint64_t) stm_ticks * RTC_T14_PERIOD / RTC_T14_MEASURE_TICKS / 1000;
}

static void configure_clock_control_usart(void) {
	uint32_t control = USART_CON_M_ASYNC_8BIT | USART_CON_FDE | USART_CON_LB;
	USART_CLC(USART1) = 1 << MOD_CLC_RMC_SHIFT;
	USART_CON(USART1) = control;
	USART_BG(USART1) = 0x0C;
	USART_FDV(USART1) = 0x1D8;
	USART_TMO(USART1) = 0;
	USART_DMAE(USART1) = 0;
	USART_IMSC(USART1) = 0;
	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_RXFCON(USART1) = 0;
	USART_TXFCON(USART1) = 0;
	USART_WHBCON(USART1) = USART_WHBCON_CLRPE | USART_WHBCON_CLRFE | USART_WHBCON_CLROE;
	USART_CON(USART1) = control | USART_CON_CON_R;
	USART_WHBCON(USART1) = USART_WHBCON_SETREN;
}

static bool clock_control_usart_loopback(uint8_t value) {
	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_TXB(USART1) = value;
	for (uint32_t index = 0;
		index < CLOCK_MANAGER_WAIT_ITERATIONS && (USART_RIS(USART1) & USART_RIS_RX) == 0;
		index++)
		__asm__ volatile("nop");
	if ((USART_RIS(USART1) & USART_RIS_RX) == 0)
		return false;
	uint32_t actual = USART_RXB(USART1);
	USART_ICR(USART1) = USART_ICR_RX;
	return actual == value;
}

static void test_clock_manager_control(void) {
	uint32_t initial_con1 = PLL_CON1;
	uint32_t initial_control = initial_con1 & PLL_CON1_SYSTEM_OUT_CTRL;
	uint32_t bypass_base =
		(initial_con1 & ~(PLL_CON1_SYSTEM_OUT_CTRL | PLL_CON1_FSYS_CLKSEL)) |
		PLL_CON1_FSYS_CLKSEL_BYPASS;
	uint32_t off_bypass_con1 = bypass_base | PLL_CON1_SYSTEM_OUT_CTRL_OFF;
	uint32_t on_bypass_con1 = bypass_base | PLL_CON1_SYSTEM_OUT_CTRL_ON;
	uint32_t on_pll_con1 =
		(bypass_base | PLL_CON1_FSYS_CLKSEL_PLL) | PLL_CON1_SYSTEM_OUT_CTRL_ON;
	configure_clock_control_usart();
	/* USART TX IRQ means that TXB is empty, not that the last frame has left the shifter. */
	stopwatch_usleep_wd(1000);

	/* Firmware only disables an unselected branch and enables it before selecting it. */
	PLL_CON1 = off_bypass_con1;
	uint32_t off_bypass_readback = PLL_CON1;

	PLL_CON1 = on_bypass_con1;
	uint32_t on_bypass_readback = PLL_CON1;
	bool on_bypass_loopback = clock_control_usart_loopback(0x5A);

	PLL_CON1 = on_pll_con1;
	uint32_t on_pll_readback = PLL_CON1;
	bool on_pll_loopback = clock_control_usart_loopback(0xC3);
	PLL_CON1 = on_bypass_con1;
	USART_CLC(USART1) = MOD_CLC_DISR;
	PLL_CON1 = initial_con1;
	uint32_t restored_readback = PLL_CON1;

	printf("# PLL CON1[1:0] Clock Manager control: initial=%u OFF/BYPASS=%08X ON/BYPASS=%08X ON/PLL=%08X restored=%08X\n",
		(unsigned int) initial_control, (unsigned int) off_bypass_readback,
		(unsigned int) on_bypass_readback, (unsigned int) on_pll_readback,
		(unsigned int) restored_readback);
	test_eq_u32("SYSTEM_OUT OFF state reads back as 3", PLL_CON1_SYSTEM_OUT_CTRL_OFF,
		off_bypass_readback & PLL_CON1_SYSTEM_OUT_CTRL);
	test_eq_u32("SYSTEM_OUT ON state reads back as 2", PLL_CON1_SYSTEM_OUT_CTRL_ON,
		on_bypass_readback & PLL_CON1_SYSTEM_OUT_CTRL);
	test_check("bypass keeps USART running while SYSTEM_OUT is ON", on_bypass_loopback);
	test_eq_u32("SYSTEM_OUT ON permits selecting PLL for fSYS", on_pll_con1, on_pll_readback);
	test_check("USART loopback runs with SYSTEM_OUT ON and PLL selected", on_pll_loopback);
	test_eq_u32("Clock Manager sequence restores PLL CON1", initial_con1, restored_readback);
}

static void test_example_fsys_benchmark(void) {
	static const struct {
		uint32_t ndiv;
		uint32_t mdiv;
		uint32_t expected_khz;
	} CONFIGS[] = {
		{ 3, 0, 52000 },
		{ 7, 1, 52000 },
		{ 15, 3, 52000 },
		{ 4, 0, 65000 },
		{ 9, 1, 65000 },
		{ 5, 0, 78000 },
		{ 11, 1, 78000 },
	};
	uint32_t initial_osc = PLL_OSC;
	uint32_t initial_con0 = PLL_CON0;
	uint32_t initial_con1 = PLL_CON1;
	uint32_t initial_con2 = PLL_CON2;
	uint32_t initial_con3 = PLL_CON3;
	uint32_t initial_tpu_vic = VIC_CON(VIC_TPU_INT0_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	init_example_rtc();
	init_example_tpu();
	tpu_frame_count = 0;
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	cpu_enable_irq(true);

	for (uint32_t index = 0; index < ARRAY_SIZE(CONFIGS); index++) {
		PLL_OSC =
			(CONFIGS[index].ndiv << PLL_OSC_NDIV_SHIFT) |
			(CONFIGS[index].mdiv << PLL_OSC_MDIV_SHIFT) |
			PLL_OSC_PLL_POWER_UP |
			PLL_OSC_PHASE0_POWER_UP |
			PLL_OSC_PHASE1_POWER_UP |
			PLL_OSC_PLL_BYPASS_N |
			PLL_OSC_PHASE0_BYPASS_N |
			PLL_OSC_PHASE1_BYPASS_N;
		PLL_CON0 = (1 << PLL_CON0_PLL1_K1_SHIFT) | (1 << PLL_CON0_PLL1_K2_SHIFT);
		PLL_CON1 = PLL_CON1_AHB_CLKSEL_PLL1 | PLL_CON1_FSYS_CLKSEL_PLL;
		PLL_CON2 = (1 << PLL_CON2_CPU_DIV_SHIFT) | PLL_CON2_CPU_DIV_EN;
		bool locked = wait_for_pll_lock();
		uint32_t calculated_hz = cpu_get_sys_freq();
		uint32_t calculated_stm_hz = cpu_get_stm_freq();
		uint32_t tpu_frames = 0;
		uint32_t stm_ticks = 0;
		if (locked)
			benchmark_example_clocks(&tpu_frames, &stm_ticks);

		PLL_CON1 = initial_con1;
		PLL_CON0 = initial_con0;
		PLL_CON2 = initial_con2;
		PLL_CON3 = initial_con3;
		PLL_OSC = initial_osc;
		bool restored = wait_for_pll_lock();
		uint32_t measured_khz = (uint64_t) tpu_frames * 26000 / 4333;

		printf("# example benchmark N=%u M=%u: TPU=%u frames, STM=%u ticks\n",
			(unsigned int) CONFIGS[index].ndiv, (unsigned int) CONFIGS[index].mdiv,
			(unsigned int) tpu_frames, (unsigned int) stm_ticks);
		test_check("working example PLL configuration locks", locked);
		test_eq_u32("cpu_get_sys_freq follows PLL N/M", CONFIGS[index].expected_khz * 1000, calculated_hz);
		test_measured_frequency("TPU benchmark confirms fSYS", CONFIGS[index].expected_khz, measured_khz);
		test_measured_frequency("cpu_get_sys_freq matches TPU/RTC", calculated_hz / 1000, measured_khz);
		test_measured_frequency("STM stays at the 26 MHz oscillator", 26000, stm_ticks / 1000);
		test_eq_u32("cpu_get_stm_freq stays on the oscillator", CPU_OSC_FREQ, calculated_stm_hz);
		test_measured_frequency("cpu_get_stm_freq matches STM/RTC", calculated_stm_hz / 1000,
			stm_ticks / 1000);
		test_check("working example restores the boot PLL configuration",
			restored && PLL_OSC == initial_osc && PLL_CON0 == initial_con0 && PLL_CON1 == initial_con1 &&
			PLL_CON2 == initial_con2 && PLL_CON3 == initial_con3);
	}

	static const struct {
		uint32_t clksel;
		uint32_t expected_khz;
		const char *name;
		bool qemu_only;
	} FSYS_SOURCES[] = {
		{ PLL_CON1_FSYS_CLKSEL_BYPASS, 26000, "fSYS bypass selects the oscillator", false },
		{ PLL_CON1_FSYS_CLKSEL_PLL, 52000, "fSYS PLL source divides PLL by two", false },
		{ PLL_CON1_FSYS_CLKSEL_DISABLE, 0, "disabled fSYS source stops TPU", true },
	};
	for (uint32_t index = 0; index < ARRAY_SIZE(FSYS_SOURCES); index++) {
		if (FSYS_SOURCES[index].qemu_only && !test_is_qemu()) {
			test_skip(FSYS_SOURCES[index].name, "disabling fSYS also stops the hardware watchdog domain");
			continue;
		}
		PLL_OSC =
			(3 << PLL_OSC_NDIV_SHIFT) |
			PLL_OSC_PLL_POWER_UP |
			PLL_OSC_PHASE0_POWER_UP |
			PLL_OSC_PHASE1_POWER_UP |
			PLL_OSC_PLL_BYPASS_N |
			PLL_OSC_PHASE0_BYPASS_N |
			PLL_OSC_PHASE1_BYPASS_N;
		PLL_CON0 = (1 << PLL_CON0_PLL1_K1_SHIFT) | (1 << PLL_CON0_PLL1_K2_SHIFT);
		PLL_CON1 = PLL_CON1_AHB_CLKSEL_PLL1 | FSYS_SOURCES[index].clksel;
		PLL_CON2 = (1 << PLL_CON2_CPU_DIV_SHIFT) | PLL_CON2_CPU_DIV_EN;
		bool locked = wait_for_pll_lock();
		uint32_t calculated_hz = cpu_get_sys_freq();
		uint32_t tpu_frames = 0;
		uint32_t stm_ticks = 0;
		if (locked)
			benchmark_example_clocks(&tpu_frames, &stm_ticks);

		PLL_CON1 = initial_con1;
		PLL_CON0 = initial_con0;
		PLL_CON2 = initial_con2;
		PLL_CON3 = initial_con3;
		PLL_OSC = initial_osc;
		bool restored = wait_for_pll_lock();
		uint32_t measured_khz = (uint64_t) tpu_frames * 26000 / 4333;

		printf("# %s: TPU=%u frames, STM=%u ticks\n", FSYS_SOURCES[index].name,
			(unsigned int) tpu_frames, (unsigned int) stm_ticks);
		test_check("PLL locks for fSYS source selection", locked);
		if (FSYS_SOURCES[index].expected_khz == 0) {
			test_eq_u32("cpu_get_sys_freq reports a stopped source", 0, calculated_hz);
			test_check(FSYS_SOURCES[index].name, tpu_frames <= 1);
		} else {
			test_eq_u32("cpu_get_sys_freq follows source selection",
				FSYS_SOURCES[index].expected_khz * 1000, calculated_hz);
			test_measured_frequency(FSYS_SOURCES[index].name, FSYS_SOURCES[index].expected_khz,
				measured_khz);
			test_measured_frequency("cpu_get_sys_freq matches selected fSYS source", calculated_hz / 1000,
				measured_khz);
		}
		test_measured_frequency("fSYS source selection does not change fSTM", 26000, stm_ticks / 1000);
		test_check("fSYS source test restores the boot PLL configuration", restored);
	}

	uint32_t selected_con1 =
		(initial_con1 & ~(PLL_CON1_FSTM_DIV | PLL_CON1_FSTM_DIV_EN)) |
		(3 << PLL_CON1_FSTM_DIV_SHIFT);
	PLL_CON1 = selected_con1;
	uint32_t calculated_stm_hz = cpu_get_stm_freq();
	uint32_t stm_ticks = benchmark_stm_ticks();
	PLL_CON1 = initial_con1;
	test_eq_u32("cpu_get_stm_freq ignores DIV while disabled", CPU_OSC_FREQ, calculated_stm_hz);
	test_measured_frequency("disabled fSTM divider ignores DIV", 26000, stm_ticks_to_khz(stm_ticks));
	test_measured_frequency("cpu_get_stm_freq matches undivided STM/RTC", calculated_stm_hz / 1000,
		stm_ticks_to_khz(stm_ticks));
	for (uint32_t divider = 0; divider < 4; divider++) {
		uint32_t clock_divider = 4U << divider;
		selected_con1 =
			(initial_con1 & ~(PLL_CON1_FSTM_DIV | PLL_CON1_FSTM_DIV_EN)) |
			PLL_CON1_FSTM_DIV_EN |
			(divider << PLL_CON1_FSTM_DIV_SHIFT);
		PLL_CON1 = selected_con1;
		calculated_stm_hz = cpu_get_stm_freq();
		stm_ticks = benchmark_stm_ticks();
		PLL_CON1 = initial_con1;
		printf("# RTC observes fSTM divider %u: %u ticks\n", (unsigned int) divider,
			(unsigned int) stm_ticks);
		test_measured_frequency("RTC confirms fSTM divider", 26000 / clock_divider,
			stm_ticks_to_khz(stm_ticks));
		test_eq_u32("cpu_get_stm_freq follows divider", CPU_OSC_FREQ / clock_divider, calculated_stm_hz);
		test_measured_frequency("cpu_get_stm_freq matches divided STM/RTC", calculated_stm_hz / 1000,
			stm_ticks_to_khz(stm_ticks));
	}

	cpu_enable_irq(false);
	TPU_PARAM = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	TPU_CLC = MOD_CLC_DISR;
	VIC_CON(VIC_TPU_INT0_IRQ) = initial_tpu_vic;
	cpu_enable_irq(!irq_was_disabled);
}

static void write_itcm_measurement_loop(void) {
	for (uint32_t index = 0; index < PLL_MEASURE_NOPS; index++) {
		MMIO32(ITCM_BASE + index * sizeof(uint32_t)) = 0xE1A00000;
	}

	MMIO32(ITCM_BASE + PLL_MEASURE_NOPS * sizeof(uint32_t)) = 0xE2500001;
	uint32_t branch_offset = (uint32_t) (-(PLL_MEASURE_NOPS + 3)) & 0x00FFFFFF;
	MMIO32(ITCM_BASE + (PLL_MEASURE_NOPS + 1) * sizeof(uint32_t)) = 0x1A000000 | branch_offset;
	MMIO32(ITCM_BASE + (PLL_MEASURE_NOPS + 2) * sizeof(uint32_t)) = 0xE12FFF14;
	sync_code();
}

static void write_itcm_source_measurement_loop(void) {
	MMIO32(ITCM_BASE) = 0xE5831000;
	MMIO32(ITCM_BASE + sizeof(uint32_t)) = 0xE593C000;
	for (uint32_t index = 0; index < PLL_MEASURE_NOPS; index++) {
		MMIO32(ITCM_BASE + (index + 2) * sizeof(uint32_t)) = 0xE1A00000;
	}

	MMIO32(ITCM_BASE + (PLL_MEASURE_NOPS + 2) * sizeof(uint32_t)) = 0xE2500001;
	uint32_t branch_offset = (uint32_t) (-(PLL_MEASURE_NOPS + 3)) & 0x00FFFFFF;
	MMIO32(ITCM_BASE + (PLL_MEASURE_NOPS + 3) * sizeof(uint32_t)) = 0x1A000000 | branch_offset;
	MMIO32(ITCM_BASE + (PLL_MEASURE_NOPS + 4) * sizeof(uint32_t)) = 0xE5832000;
	MMIO32(ITCM_BASE + (PLL_MEASURE_NOPS + 5) * sizeof(uint32_t)) = 0xE593C000;
	MMIO32(ITCM_BASE + (PLL_MEASURE_NOPS + 6) * sizeof(uint32_t)) = 0xE12FFF14;
	sync_code();
}

static void write_itcm_cpu_div_measurement_loop(void) {
	uint32_t words = (uint32_t) (pll_cpu_div_itcm_template_end - pll_cpu_div_itcm_template_start);
	for (uint32_t index = 0; index < words; index++) {
		MMIO32(ITCM_BASE + index * sizeof(uint32_t)) = pll_cpu_div_itcm_template_start[index];
	}
	sync_code();
}

static uint32_t measured_frequency_khz(uint32_t elapsed_ticks, uint32_t iterations) {
	uint64_t work_cycles = (uint64_t) PLL_MEASURE_NOPS * iterations;
	return work_cycles * (CPU_OSC_FREQ / 1000) / elapsed_ticks;
}

static uint32_t measure_ahb_source_khz(uint32_t selected_con1, uint32_t restore_con1) {
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	stopwatch_t start = stopwatch_get();
	pll_execute_itcm_source(ITCM_BASE, PLL_MEASURE_ITERATIONS, selected_con1, restore_con1);
	uint32_t elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	cpu_enable_irq(!irq_was_disabled);

	return measured_frequency_khz(elapsed_ticks, PLL_MEASURE_ITERATIONS);
}

static uint32_t measure_cpu_divider_khz(uint32_t selected_con2, uint32_t restore_con2) {
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	stopwatch_t start = stopwatch_get();
	pll_execute_itcm_cpu_div(ITCM_BASE, PLL_DIVIDER_MEASURE_ITERATIONS, selected_con2, restore_con2);
	uint32_t elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	cpu_enable_irq(!irq_was_disabled);

	return measured_frequency_khz(elapsed_ticks, PLL_DIVIDER_MEASURE_ITERATIONS);
}

static void test_measured_frequency(const char *name, uint32_t expected_khz, uint32_t measured_khz) {
	printf(
		"# %s: expected=%u kHz measured=%u kHz\n",
		name,
		(unsigned int) expected_khz,
		(unsigned int) measured_khz
	);
	test_check(name, test_u32_in_interval(measured_khz, expected_khz * 98 / 100, expected_khz * 102 / 100));
}

static void test_ahb_frequency_measurement(void) {
	if (test_is_qemu()) {
		test_skip("ITCM timing measures the 26 MHz AHB bypass clock", "QEMU does not model fCPU timing");
		return;
	}

	uint32_t initial_itcm = read_itcm();
	bool can_measure =
		(PLL_CON1 & PLL_CON1_AHB_CLKSEL) == PLL_CON1_AHB_CLKSEL_BYPASS &&
		(PLL_CON1 & PLL_CON1_FSTM_DIV_EN) == 0 &&
		(PLL_CON2 & PLL_CON2_CPU_DIV_EN) == 0 &&
		(initial_itcm & TCM_REGION_ENABLE) == 0;

	if (!can_measure) {
		test_skip("ITCM timing measures the 26 MHz AHB clock", "boot clock or ITCM configuration is incompatible");
		return;
	}

	write_itcm(ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_itcm_measurement_loop();
	test_watchdog_serve();
	bool irq_was_disabled = cpu_enable_irq(false);
	stopwatch_t start = stopwatch_get();
	pll_execute_itcm(ITCM_BASE, PLL_MEASURE_ITERATIONS);
	uint32_t elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	cpu_enable_irq(!irq_was_disabled);
	write_itcm(initial_itcm);
	sync_code();

	uint32_t measured_khz = measured_frequency_khz(elapsed_ticks, PLL_MEASURE_ITERATIONS);
	test_measured_frequency("ITCM timing measures the 26 MHz AHB bypass clock", 26000, measured_khz);
}

static void test_pll_source_frequencies(void) {
	if (test_is_qemu()) {
		uint32_t initial_osc = PLL_OSC;
		uint32_t initial_con0 = PLL_CON0;
		uint32_t initial_con1 = PLL_CON1;
		PLL_OSC =
			(7 << PLL_OSC_NDIV_SHIFT) |
			(1 << PLL_OSC_MDIV_SHIFT) |
			PLL_OSC_PLL_POWER_UP |
			PLL_OSC_PHASE0_POWER_UP |
			PLL_OSC_PHASE1_POWER_UP |
			PLL_OSC_PHASE2_POWER_UP |
			PLL_OSC_PHASE3_POWER_UP |
			PLL_OSC_PLL_BYPASS_N |
			PLL_OSC_PHASE0_BYPASS_N |
			PLL_OSC_PHASE1_BYPASS_N |
			PLL_OSC_PHASE2_BYPASS_N |
			PLL_OSC_PHASE3_BYPASS_N;
		bool locked = wait_for_pll_lock();
		PLL_CON1 = (initial_con1 & ~PLL_CON1_AHB_CLKSEL) | PLL_CON1_AHB_CLKSEL_PLL0;
		uint32_t pll0_hz = cpu_get_ahb_freq();
		PLL_CON0 = (PLL_CON0 & ~PLL_CON0_PHASE0_CONFIG) | (0x11 << PLL_CON0_PHASE0_CONFIG_SHIFT);
		PLL_CON1 = (initial_con1 & ~PLL_CON1_AHB_CLKSEL) | PLL_CON1_AHB_CLKSEL_PLL1;
		uint32_t phase0_hz = cpu_get_ahb_freq();
		PLL_CON0 = (PLL_CON0 & ~PLL_CON0_PHASE1_CONFIG) | (0x22 << PLL_CON0_PHASE1_CONFIG_SHIFT);
		PLL_CON1 = (initial_con1 & ~PLL_CON1_AHB_CLKSEL) | PLL_CON1_AHB_CLKSEL_PLL2;
		uint32_t phase1_hz = cpu_get_ahb_freq();
		PLL_CON0 = (PLL_CON0 & ~PLL_CON0_PHASE2_CONFIG) | (0x23 << PLL_CON0_PHASE2_CONFIG_SHIFT);
		PLL_CON1 = (initial_con1 & ~PLL_CON1_AHB_CLKSEL) | PLL_CON1_AHB_CLKSEL_PLL3;
		uint32_t phase2_hz = cpu_get_ahb_freq();
		PLL_CON0 = (PLL_CON0 & ~PLL_CON0_PHASE3_CONFIG) | (0x40 << PLL_CON0_PHASE3_CONFIG_SHIFT);
		PLL_CON1 = (initial_con1 & ~PLL_CON1_AHB_CLKSEL) | PLL_CON1_AHB_CLKSEL_PLL4;
		uint32_t phase3_hz = cpu_get_ahb_freq();
		PLL_CON1 = initial_con1;
		PLL_CON0 = initial_con0;
		PLL_OSC = initial_osc;
		bool restored = wait_for_pll_lock();

		test_check("QEMU PLL locks for AHB source calculations", locked);
		test_eq_u32("cpu_get_ahb_freq reports PLL0", 104000000, pll0_hz);
		test_eq_u32("cpu_get_ahb_freq reports phase 0", 96000000, phase0_hz);
		test_eq_u32("cpu_get_ahb_freq reports phase 1", 48000000, phase1_hz);
		test_eq_u32("cpu_get_ahb_freq reports phase 2", 46222222, phase2_hz);
		test_eq_u32("cpu_get_ahb_freq reports phase 3", 26000000, phase3_hz);
		test_check("QEMU AHB source calculation restores PLL registers", restored);
		test_skip("AHB sources match instruction timing", "QEMU does not model fCPU timing");
		return;
	}

	uint32_t initial_itcm = read_itcm();
	uint32_t initial_osc = PLL_OSC;
	uint32_t initial_con0 = PLL_CON0;
	uint32_t initial_con1 = PLL_CON1;
	bool can_measure =
		(initial_con1 & PLL_CON1_AHB_CLKSEL) == PLL_CON1_AHB_CLKSEL_BYPASS &&
		(initial_con1 & PLL_CON1_FSYS_CLKSEL) == PLL_CON1_FSYS_CLKSEL_BYPASS &&
		(initial_con1 & PLL_CON1_FSTM_DIV_EN) == 0 &&
		(PLL_CON2 & PLL_CON2_CPU_DIV_EN) == 0 &&
		(initial_itcm & TCM_REGION_ENABLE) == 0;

	if (!can_measure) {
		test_skip("PLL0 frequency follows N/M divider", "boot clock configuration is incompatible");
		test_skip("phase 0 frequency follows K1/K2 divider", "boot clock or ITCM configuration is incompatible");
		test_skip("phase 1 frequency follows K1/K2 divider", "boot clock or ITCM configuration is incompatible");
		test_skip("phase 2 frequency follows K1/K2 divider", "boot clock or ITCM configuration is incompatible");
		test_skip("phase 3 frequency follows K1/K2 divider", "boot clock or ITCM configuration is incompatible");
		return;
	}

	write_itcm(ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_itcm_source_measurement_loop();
	uint32_t safe_con1 = initial_con1;
	bool irq_was_disabled = cpu_enable_irq(false);
	PLL_CON0 = 0x22222222;
	uint32_t selected_con1 =
		(safe_con1 & ~PLL_CON1_AHB_CLKSEL) | PLL_CON1_AHB_CLKSEL_PLL0;
	static const uint32_t PLL_DIVIDERS[][2] = {
		{3, 0},
		{7, 1},
		{11, 2},
		{15, 3},
	};
	static const char *const PLL_DIVIDER_TEST_NAMES[] = {
		"N=3 M=0 preserves the 104 MHz PLL0 ratio",
		"N=7 M=1 preserves the 104 MHz PLL0 ratio",
		"N=11 M=2 preserves the 104 MHz PLL0 ratio",
		"N=15 M=3 preserves the 104 MHz PLL0 ratio",
	};
	bool divider_locked[ARRAY_SIZE(PLL_DIVIDERS)];
	uint32_t divider_calculated_hz[ARRAY_SIZE(PLL_DIVIDERS)];
	uint32_t divider_measured_khz[ARRAY_SIZE(PLL_DIVIDERS)];
	for (uint32_t index = 0; index < ARRAY_SIZE(PLL_DIVIDERS); index++) {
		uint32_t ndiv = PLL_DIVIDERS[index][0];
		uint32_t mdiv = PLL_DIVIDERS[index][1];
		uint32_t osc =
			(PLL_OSC & ~(PLL_OSC_NDIV | PLL_OSC_MDIV)) |
			PLL_OSC_PLL_POWER_UP |
			PLL_OSC_PLL_BYPASS_N |
			(ndiv << PLL_OSC_NDIV_SHIFT) |
			(mdiv << PLL_OSC_MDIV_SHIFT);
		divider_locked[index] = apply_pll_osc(osc);
		PLL_CON1 = selected_con1;
		divider_calculated_hz[index] = cpu_get_ahb_freq();
		PLL_CON1 = safe_con1;
		divider_measured_khz[index] = divider_locked[index]
			? measure_ahb_source_khz(selected_con1, safe_con1)
			: 0;
	}

	uint32_t phase_osc =
		(PLL_OSC & ~(PLL_OSC_NDIV | PLL_OSC_MDIV)) |
		PLL_OSC_PLL_POWER_UP |
		PLL_OSC_PLL_BYPASS_N |
		(7 << PLL_OSC_NDIV_SHIFT) |
		(1 << PLL_OSC_MDIV_SHIFT);
	bool phase_pll_locked = apply_pll_osc(phase_osc);

	static const uint32_t PHASE_CONFIGS[] = {
		0x11,
		0x22,
		0x23,
		0x22,
	};
	static const uint32_t PHASE_SOURCES[] = {
		PLL_CON1_AHB_CLKSEL_PLL1,
		PLL_CON1_AHB_CLKSEL_PLL1,
		PLL_CON1_AHB_CLKSEL_PLL1,
		PLL_CON1_AHB_CLKSEL_PLL2,
	};
	static const uint32_t PHASE_EXPECTED_KHZ[] = {
		96000,
		48000,
		46222,
		48000,
	};
	static const uint32_t PHASE_EXPECTED_HZ[] = {
		96000000,
		48000000,
		46222222,
		48000000,
	};
	static const char *const PHASE_TEST_NAMES[] = {
		"phase 0 K1=2 K2=1 produces 96 MHz",
		"phase 0 K1=4 K2=2 produces 48 MHz",
		"phase 0 K1=4 K2=3 produces 46.222 MHz",
		"phase 1 K1=4 K2=2 produces 48 MHz",
	};
	uint32_t phase_calculated_hz[ARRAY_SIZE(PHASE_SOURCES)] = {0};
	uint32_t phase_measured_khz[ARRAY_SIZE(PHASE_SOURCES)] = {0};
	if (phase_pll_locked) {
		PLL_OSC |= PLL_OSC_PHASE0_POWER_UP | PLL_OSC_PHASE1_POWER_UP;
		PLL_OSC |= PLL_OSC_PHASE0_BYPASS_N | PLL_OSC_PHASE1_BYPASS_N;
		for (uint32_t index = 0; index < ARRAY_SIZE(PHASE_SOURCES); index++) {
			uint32_t config_mask = index < 3 ? PLL_CON0_PHASE0_CONFIG : PLL_CON0_PHASE1_CONFIG;
			uint32_t config_shift = index < 3 ? PLL_CON0_PHASE0_CONFIG_SHIFT : PLL_CON0_PHASE1_CONFIG_SHIFT;
			PLL_CON0 = (PLL_CON0 & ~config_mask) | (PHASE_CONFIGS[index] << config_shift);
			selected_con1 = (safe_con1 & ~PLL_CON1_AHB_CLKSEL) | PHASE_SOURCES[index];
			PLL_CON1 = selected_con1;
			phase_calculated_hz[index] = cpu_get_ahb_freq();
			PLL_CON1 = safe_con1;
			phase_measured_khz[index] = measure_ahb_source_khz(selected_con1, safe_con1);
		}
	}

	PLL_CON0 = initial_con0;
	bool restore_locked = apply_pll_osc(initial_osc);
	write_itcm(initial_itcm);
	sync_code();
	cpu_enable_irq(!irq_was_disabled);

	for (uint32_t index = 0; index < ARRAY_SIZE(PLL_DIVIDERS); index++) {
		test_check("PLL locks after changing N/M", divider_locked[index]);
		if (divider_locked[index]) {
			test_eq_u32("register calculation follows N/M divider", 104000000, divider_calculated_hz[index]);
			test_measured_frequency(PLL_DIVIDER_TEST_NAMES[index], 104000, divider_measured_khz[index]);
		} else {
			test_skip("register calculation follows N/M divider", "PLL did not lock");
			test_skip(PLL_DIVIDER_TEST_NAMES[index], "PLL did not lock");
		}
	}
	test_check("PLL locks before phase measurements", phase_pll_locked);
	for (uint32_t index = 0; index < ARRAY_SIZE(PHASE_SOURCES); index++) {
		if (phase_pll_locked) {
			test_eq_u32("register calculation follows K1/K2 divider",
				PHASE_EXPECTED_HZ[index], phase_calculated_hz[index]);
			test_measured_frequency(PHASE_TEST_NAMES[index], PHASE_EXPECTED_KHZ[index], phase_measured_khz[index]);
		} else {
			test_skip("register calculation follows K1/K2 divider", "PLL did not lock");
			test_skip(PHASE_TEST_NAMES[index], "PLL did not lock");
		}
	}
	test_skip("phase 2 frequency follows K1/K2 divider", "C81 source 5 stops AHB and firmware never enables 0x0808");
	test_skip("phase 3 frequency follows K1/K2 divider", "C81 source 6 stops AHB");
	test_check("original PLL configuration is restored and locked", restore_locked);
}

static void test_cpu_divider_frequencies(void) {
	uint32_t initial_itcm = read_itcm();
	uint32_t initial_con1 = PLL_CON1;
	uint32_t initial_con2 = PLL_CON2;
	bool can_calculate =
		(initial_con1 & PLL_CON1_AHB_CLKSEL) == PLL_CON1_AHB_CLKSEL_BYPASS &&
		(initial_con1 & PLL_CON1_FSTM_DIV_EN) == 0;

	if (!can_calculate) {
		test_skip("cpu_get_freq follows disabled CPU divider", "boot AHB clock is incompatible");
		for (uint32_t divider = 0; divider < 4; divider++)
			test_skip("cpu_get_freq follows DIV+1", "boot AHB clock is incompatible");
	} else {
		uint32_t selected_con2 = initial_con2 & ~(PLL_CON2_CPU_DIV | PLL_CON2_CPU_DIV_EN);
		PLL_CON2 = selected_con2;
		uint32_t calculated_hz = cpu_get_freq();
		PLL_CON2 = initial_con2;
		test_eq_u32("cpu_get_freq follows disabled CPU divider", CPU_OSC_FREQ, calculated_hz);
		for (uint32_t divider = 0; divider < 4; divider++) {
			selected_con2 =
				(initial_con2 & ~(PLL_CON2_CPU_DIV | PLL_CON2_CPU_DIV_EN)) |
				PLL_CON2_CPU_DIV_EN |
				(divider << PLL_CON2_CPU_DIV_SHIFT);
			PLL_CON2 = selected_con2;
			calculated_hz = cpu_get_freq();
			PLL_CON2 = initial_con2;
			test_eq_u32("cpu_get_freq follows DIV+1", CPU_OSC_FREQ / (divider + 1), calculated_hz);
		}
	}

	if (test_is_qemu()) {
		test_skip("disabled CPU divider matches instruction timing", "QEMU does not model fCPU timing");
		for (uint32_t divider = 0; divider < 4; divider++)
			test_skip("CPU divider matches instruction timing", "QEMU does not model fCPU timing");
		return;
	}

	bool can_measure = can_calculate && (initial_itcm & TCM_REGION_ENABLE) == 0;
	if (!can_measure) {
		test_skip("disabled CPU divider matches instruction timing", "boot clock or ITCM is incompatible");
		for (uint32_t divider = 0; divider < 4; divider++)
			test_skip("CPU divider matches instruction timing", "boot clock or ITCM is incompatible");
		return;
	}

	write_itcm(ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_itcm_cpu_div_measurement_loop();
	uint32_t selected_con2 = initial_con2 & ~(PLL_CON2_CPU_DIV | PLL_CON2_CPU_DIV_EN);
	uint32_t baseline_khz = measure_cpu_divider_khz(selected_con2, initial_con2);
	test_measured_frequency(
		"disabled CPU divider matches instruction timing",
		CPU_OSC_FREQ / 1000,
		baseline_khz
	);

	for (uint32_t divider = 0; divider < 4; divider++) {
		selected_con2 =
			(initial_con2 & ~(PLL_CON2_CPU_DIV | PLL_CON2_CPU_DIV_EN)) |
			PLL_CON2_CPU_DIV_EN |
			(divider << PLL_CON2_CPU_DIV_SHIFT);
		uint32_t measured_khz = measure_cpu_divider_khz(selected_con2, initial_con2);
		printf("# CPU divider %u selects divide-by-%u\n", (unsigned int) divider,
			(unsigned int) divider + 1);
		test_measured_frequency(
			"cpu_get_freq matches instruction timing",
			CPU_OSC_FREQ / 1000 / (divider + 1),
			measured_khz
		);
	}

	write_itcm(initial_itcm);
	sync_code();
}

static uint32_t measure_stm_frequency_khz(
	uint32_t selected_con1,
	uint32_t restore_con1,
	uint32_t cpu_frequency_khz
) {
	test_watchdog_serve();
	PLL_CON1 = selected_con1;
	bool irq_was_disabled = cpu_enable_irq(false);
	stopwatch_t start = stopwatch_get();
	pll_execute_itcm(ITCM_BASE, PLL_DIVIDER_MEASURE_ITERATIONS);
	stopwatch_t end = stopwatch_get();
	PLL_CON1 = restore_con1;
	cpu_enable_irq(!irq_was_disabled);

	uint32_t elapsed_ticks = (uint32_t) (end - start);
	uint32_t work_cycles = PLL_MEASURE_NOPS * PLL_DIVIDER_MEASURE_ITERATIONS;
	return (uint64_t) elapsed_ticks * cpu_frequency_khz / work_cycles;
}

static void test_stm_divider_frequencies(void) {
	if (test_is_qemu()) {
		test_skip("disabled STM divider leaves STM at oscillator frequency", "QEMU does not model fCPU timing");
		for (uint32_t divider = 0; divider < 4; divider++) {
			test_skip("enabled STM divider follows 4 * 2^DIV", "QEMU does not model fCPU timing");
		}
		return;
	}

	uint32_t initial_itcm = read_itcm();
	uint32_t initial_con1 = PLL_CON1;
	bool can_measure =
		(initial_con1 & PLL_CON1_AHB_CLKSEL) == PLL_CON1_AHB_CLKSEL_BYPASS &&
		(PLL_CON2 & PLL_CON2_CPU_DIV_EN) == 0 &&
		(initial_itcm & TCM_REGION_ENABLE) == 0;

	if (!can_measure) {
		test_skip("disabled STM divider leaves STM at oscillator frequency", "boot clock or ITCM is incompatible");
		for (uint32_t divider = 0; divider < 4; divider++) {
			test_skip("enabled STM divider follows 4 * 2^DIV", "boot clock or ITCM is incompatible");
		}
		return;
	}

	write_itcm(ITCM_BASE | TCM_REGION_SIZE_8K | TCM_REGION_ENABLE);
	write_itcm_measurement_loop();
	uint32_t cpu_frequency_khz = CPU_OSC_FREQ / 1000;
	uint32_t selected_con1 =
		(initial_con1 & ~(PLL_CON1_FSTM_DIV | PLL_CON1_FSTM_DIV_EN)) |
		(3 << PLL_CON1_FSTM_DIV_SHIFT);
	test_measured_frequency(
		"disabled STM divider leaves STM at oscillator frequency",
		CPU_OSC_FREQ / 1000,
		measure_stm_frequency_khz(selected_con1, initial_con1, cpu_frequency_khz)
	);

	for (uint32_t divider = 0; divider < 4; divider++) {
		selected_con1 =
			(initial_con1 & ~(PLL_CON1_FSTM_DIV | PLL_CON1_FSTM_DIV_EN)) |
			PLL_CON1_FSTM_DIV_EN |
			(divider << PLL_CON1_FSTM_DIV_SHIFT);
		uint32_t measured_khz = measure_stm_frequency_khz(
			selected_con1,
			initial_con1,
			cpu_frequency_khz
		);
		printf("# STM divider %u selects divide-by-%u\n", (unsigned int) divider, 4U << divider);
		test_measured_frequency(
			"enabled STM divider follows 4 * 2^DIV",
			CPU_OSC_FREQ / 1000 / (4U << divider),
			measured_khz
		);
	}

	write_itcm(initial_itcm);
	sync_code();
}

static void test_pll_lock_transition(void) {
	uint32_t initial_osc = PLL_OSC;
	uint32_t initial_src = PLL_SRC;
	uint32_t initial_vic_con = VIC_CON(VIC_PLL_IRQ);
	bool safe_clock_tree =
		(PLL_CON1 & PLL_CON1_AHB_CLKSEL) == PLL_CON1_AHB_CLKSEL_BYPASS &&
		(PLL_CON1 & PLL_CON1_FSYS_CLKSEL) == PLL_CON1_FSYS_CLKSEL_BYPASS;
	if (!safe_clock_tree) {
		test_skip("power down clears PLL lock", "boot clock tree depends on the PLL");
		test_skip("power up reacquires PLL lock", "boot clock tree depends on the PLL");
		test_skip("PLL lock transition raises its service request", "boot clock tree depends on the PLL");
		test_skip("PLL lock transition reaches the expected VIC line", "boot clock tree depends on the PLL");
		test_skip("PLL transition restores the boot configuration", "boot clock tree depends on the PLL");
		return;
	}

	bool irq_was_disabled = cpu_enable_irq(false);
	irq_count = 0;
	irq_number = 0;
	PLL_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_PLL_IRQ) = 1;
	PLL_OSC = initial_osc & ~(PLL_OSC_PLL_POWER_UP | PLL_OSC_PLL_BYPASS_N);
	bool unlocked = wait_for_pll_unlock();
	PLL_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	irq_count = 0;
	irq_number = 0;
	cpu_enable_irq(true);
	stopwatch_t lock_start = stopwatch_get();
	PLL_OSC = (initial_osc | PLL_OSC_PLL_POWER_UP) & ~PLL_OSC_PLL_BYPASS_N;
	bool locked = wait_for_pll_lock();
	uint32_t lock_ticks = (uint32_t) stopwatch_elapsed(lock_start);
	bool lock_irq = wait_for_irq();
	cpu_enable_irq(false);

	PLL_OSC = initial_osc;
	bool restored = wait_for_pll_lock();
	VIC_CON(VIC_PLL_IRQ) = initial_vic_con;
	PLL_SRC = MOD_SRC_CLRR | (initial_src & (MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE));
	if ((initial_src & MOD_SRC_SRR) != 0)
		PLL_SRC |= MOD_SRC_SETR;
	cpu_enable_irq(!irq_was_disabled);

	printf("# PLL lock acquisition took %u STM ticks\n", (unsigned int) lock_ticks);
	test_check("power down clears PLL lock", unlocked);
	test_check("power up reacquires PLL lock", locked);
	test_check("PLL lock transition raises its service request", lock_irq);
	test_eq_u32("PLL lock transition reaches the expected VIC line", VIC_PLL_IRQ, irq_number);
	test_check("PLL transition restores the boot configuration", restored && PLL_OSC == initial_osc);
}

static void test_interrupt_routing(void) {
	uint32_t initial_src = PLL_SRC;
	uint32_t initial_vic_con = VIC_CON(VIC_PLL_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	irq_count = 0;
	irq_number = 0;
	PLL_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_PLL_IRQ) = 1;
	cpu_enable_irq(true);
	PLL_SRC |= MOD_SRC_SETR;
	test_check("software request raises PLL IRQ", wait_for_irq());
	cpu_enable_irq(false);
	test_eq_u32("PLL SRC is routed to the expected VIC line", VIC_PLL_IRQ, irq_number);
	test_eq_u32("PLL IRQ handler clears the request", 0, PLL_SRC & MOD_SRC_SRR);

	VIC_CON(VIC_PLL_IRQ) = initial_vic_con;
	PLL_SRC = MOD_SRC_CLRR | (initial_src & (MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE));
	if ((initial_src & MOD_SRC_SRR) != 0) {
		PLL_SRC |= MOD_SRC_SETR;
	}
	cpu_enable_irq(!irq_was_disabled);
}

int main(void) {
	test_start("PLL peripheral test");

	test_category("Reset values");
	test_reset_values();
	test_category("Boot clock calculation");
	test_boot_clock_calculation();
	test_category("PLL CON1 Clock Manager control");
	test_clock_manager_control();
	test_category("Working example fSYS benchmark");
	test_example_fsys_benchmark();
	test_category("AHB frequency measurement");
	test_ahb_frequency_measurement();
	test_category("PLL source frequencies");
	test_pll_source_frequencies();
	test_category("CPU divider frequencies");
	test_cpu_divider_frequencies();
	test_category("STM divider frequencies");
	test_stm_divider_frequencies();
	test_category("PLL lock transition");
	test_pll_lock_transition();
	test_category("Interrupt routing");
	test_interrupt_routing();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	if (irq_number == VIC_TPU_INT0_IRQ) {
		tpu_frame_count++;
		TPU_SRC(0) |= MOD_SRC_CLRR;
		VIC_IRQ_ACK = 1;
		return;
	}
	irq_count++;
	PLL_SRC |= MOD_SRC_CLRR;
	VIC_IRQ_ACK = 1;
}
