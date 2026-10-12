#include <pmb887x.h>

#include "test.h"
#include "watchdog.h"

#define WDT_ERROR_STATUS (SCU_WDT_SR_WDTAE | SCU_WDT_SR_WDTOE | SCU_WDT_SR_WDTPR)
#define WDT_CONFIG (SCU_WDTCON0_WDTREL | SCU_WDTCON0_WDTPW)
#define WDT_SLOW_FREQUENCY_WINDOW_US 50000
#define WDT_FAST_FREQUENCY_WINDOW_US 1000
#define WDT_FREQUENCY_TOLERANCE_PERCENT 10

typedef struct {
	uint32_t frequency;
	uint32_t status;
	bool counter_advanced;
	bool unlocked;
	bool modified;
} watchdog_measurement_t;

typedef struct {
	volatile uint32_t *source;
	uint32_t irq;
} scu_irq_source_t;

static const scu_irq_source_t IRQ_SOURCES[] = {
	{ &SCU_EXTI0_SRC, VIC_SCU_EXTI0_IRQ },
	{ &SCU_EXTI1_SRC, VIC_SCU_EXTI1_IRQ },
	{ &SCU_EXTI2_SRC, VIC_SCU_EXTI2_IRQ },
	{ &SCU_EXTI3_SRC, VIC_SCU_EXTI3_IRQ },
	{ &SCU_EXTI4_SRC, VIC_SCU_EXTI4_IRQ },
#ifdef PMB8876
	{ &SCU_EXTI5_SRC, VIC_SCU_EXTI5_IRQ },
	{ &SCU_EXTI6_SRC, VIC_SCU_EXTI6_IRQ },
	{ &SCU_EXTI7_SRC, VIC_SCU_EXTI7_IRQ },
	{ &SCU_PM_INT_SRC, VIC_SCU_PM_INT_IRQ },
#endif
	{ &SCU_DSP_SRC(0), VIC_SCU_DSP0_IRQ },
	{ &SCU_DSP_SRC(1), VIC_SCU_DSP1_IRQ },
	{ &SCU_DSP_SRC(2), VIC_SCU_DSP2_IRQ },
	{ &SCU_DSP_SRC(3), VIC_SCU_DSP3_IRQ },
#ifdef PMB8876
	{ &SCU_UNK0_SRC, VIC_SCU_UNK0_IRQ },
	{ &SCU_UNK1_SRC, VIC_SCU_UNK1_IRQ },
	{ &SCU_UNK2_SRC, VIC_SCU_UNK2_IRQ },
#endif
};

static void test_reset_values(void) {
	uint32_t reset_status = SCU_RST_SR;
	uint32_t reset_control = SCU_RST_CON;
	uint32_t peripheral_reset_requests = SCU_RST_REQ;
	uint32_t sleep_request = SCU_SLEEP_REQ;
	uint32_t dsp_interrupt = SCU_DSP_INT;
	uint32_t interrupt_filter = SCU_EXTI_FILTER;
	uint32_t interrupt_edge = SCU_EXTI_EDGE;
	uint32_t rtc_interface = SCU_RTCIF;
#ifdef PMB8876
	uint32_t redesign_tracing = SCU_RTID;
	uint32_t dma_request_select = SCU_DMARS;
#endif
	uint32_t emulator_id = SCU_EMU_ID;

	test_module_id("module ID", 0xF040C000, SCU_ID);
	test_module_clock("module clock is enabled", SCU_CLC);
	test_eq_u32("peripheral reset requests are inactive", 0, peripheral_reset_requests);
	test_eq_u32("sleep request is inactive", 0, sleep_request & SCU_SLEEP_REQ_REQ);
	test_eq_u32("DSP interrupt requests are inactive", 0, dsp_interrupt & SCU_DSP_INT_REQ);
	test_eq_u32("RTC interface starts disabled", 0, rtc_interface & SCU_RTCIF_RTCIFEN);
#ifdef PMB8876
	test_eq_u32("RTID reserved bits are zero", 0, redesign_tracing & GENMASK(31, 16));
#endif
	test_eq_u32(
		"emulator ID matches the execution environment",
		test_is_qemu() ? SCU_EMU_ID_VALUE_QEMU : 0,
		emulator_id
	);
	printf(
		"# SCU: RST_SR=%08X RST_CON=%08X RST_REQ=%08X SLEEP=%08X DSP_INT=%08X\n",
		(unsigned int) reset_status,
		(unsigned int) reset_control,
		(unsigned int) peripheral_reset_requests,
		(unsigned int) sleep_request,
		(unsigned int) dsp_interrupt
	);
	printf("# SCU: EXTI_FILTER=%08X EXTI_EDGE=%08X RTCIF=%08X EMU_ID=%08X\n",
		(unsigned int) interrupt_filter,
		(unsigned int) interrupt_edge,
		(unsigned int) rtc_interface,
		(unsigned int) emulator_id
	);
#ifdef PMB8876
	printf(
		"# SCU: RTID=%08X DMARS=%08X\n",
		(unsigned int) redesign_tracing,
		(unsigned int) dma_request_select
	);
#endif
}

static bool frequency_matches(uint32_t actual, uint32_t expected) {
	uint32_t tolerance = expected * WDT_FREQUENCY_TOLERANCE_PERCENT / 100;

	return actual >= expected - tolerance && actual <= expected + tolerance;
}

static watchdog_measurement_t measure_watchdog_frequency(uint32_t con1) {
	struct scu_watchdog_access_result enable = scu_watchdog_configure(con1, 0, true);
	uint32_t status = SCU_WDT_SR;
	uint32_t before = (status & SCU_WDT_SR_WDTTIM) >> SCU_WDT_SR_WDTTIM_SHIFT;
	uint32_t window_us = (con1 & SCU_WDTCON1_WDTIR) != 0 ?
		WDT_FAST_FREQUENCY_WINDOW_US : WDT_SLOW_FREQUENCY_WINDOW_US;
	stopwatch_t start = stopwatch_get();
	stopwatch_usleep_wd(window_us);
	uint32_t elapsed_us = stopwatch_elapsed_us(start);
	uint32_t after = (SCU_WDT_SR & SCU_WDT_SR_WDTTIM) >> SCU_WDT_SR_WDTTIM_SHIFT;
	struct scu_watchdog_access_result disable = scu_watchdog_configure(con1 | SCU_WDTCON1_WDTDR, 0, true);
	uint32_t ticks = (after - before) & 0xFFFF;

	return (watchdog_measurement_t) {
		.frequency = (uint32_t) ((uint64_t) ticks * 1000000 / elapsed_us),
		.status = status,
		.counter_advanced = after != before,
		.unlocked = enable.unlocked && disable.unlocked,
		.modified = enable.modified && disable.modified,
	};
}

static void pulse_reset_request(const char *name, uint32_t request) {
	SCU_RST_REQ |= request;
	/* Firmware uses the same readback before deasserting reset; back-to-back writes are too short. */
	uint32_t asserted_requests = SCU_RST_REQ;
	SCU_RST_REQ &= ~request;
	test_eq_u32(name, request, asserted_requests & request);
}

static void test_reset_requests(void) {
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("DSP clock is enabled for the reset probe", DSP_CLC);
	DSP_COM_CLEAR = UINT16_MAX;
	DSP_COM_SET = BIT(15);
	test_eq_u32("DSP communication flag is prepared set", BIT(15), DSP_COM_STATUS & BIT(15));
	pulse_reset_request("DSP reset request asserts", SCU_RST_REQ_DSP);
	test_eq_u32("DSP reset does not affect clock control", 1 << MOD_CLC_RMC_SHIFT, DSP_CLC);
	test_eq_u32("DSP communication flags return to reset value", 0, DSP_COM_STATUS);
	DSP_CLC = MOD_CLC_DISR;

	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("USB is prepared in a non-reset state", USB_CLC);
	USB_CFG = 3;
	pulse_reset_request("USB reset request asserts", SCU_RST_REQ_USB);
	test_eq_u32("USB reset disables its clock", MOD_CLC_DISR | MOD_CLC_DISS, USB_CLC);
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("USB wrapper returns to reset value", 0, USB_CFG);
	USB_CLC = MOD_CLC_DISR;

	DMAC_CH_SRC_ADDR(7) = 0x12345678;
	pulse_reset_request("DMA1 reset request asserts", SCU_RST_REQ_DMA1);
	test_eq_u32("DMAC channel returns to reset value", 0, DMAC_CH_SRC_ADDR(7));

#ifdef PMB8876
	I2C_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("I2C is prepared in a non-reset state", I2C_CLC);
	I2C_FDIVCFG = 0x0004003D;
	pulse_reset_request("I2C reset request asserts", SCU_RST_REQ_I2C);
	test_eq_u32("I2C clock control returns to reset value", MOD_CLC_DISR | MOD_CLC_DISS, I2C_CLC);
	I2C_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("I2C divider returns to reset value", 0, I2C_FDIVCFG);
#endif

	test_eq_u32("reset requests finish deasserted", 0, SCU_RST_REQ);
}

static void test_interrupt_routing(void) {
	bool routed = true;

	for (size_t i = 0; i < ARRAY_SIZE(IRQ_SOURCES); i++) {
		*IRQ_SOURCES[i].source = MOD_SRC_CLRR;
		VIC_CON(IRQ_SOURCES[i].irq) = 1;
	}

	for (size_t i = 0; i < ARRAY_SIZE(IRQ_SOURCES); i++) {
		const scu_irq_source_t *irq_source = &IRQ_SOURCES[i];

		*irq_source->source = MOD_SRC_SRE | MOD_SRC_SETR;

		uint32_t source_status = *irq_source->source;
		uint32_t selected_irq = VIC_IRQ_CON & VIC_IRQ_CON_NUM;
		if (selected_irq != irq_source->irq) {
			printf(
				"# SCU source %u status %08X selected VIC IRQ %u instead of %u\n",
				(unsigned int) i,
				(unsigned int) source_status,
				(unsigned int) selected_irq,
				(unsigned int) irq_source->irq
			);
			routed = false;
		}

		*irq_source->source = MOD_SRC_CLRR;
	}

	for (size_t i = 0; i < ARRAY_SIZE(IRQ_SOURCES); i++)
		VIC_CON(IRQ_SOURCES[i].irq) = 0;

	test_check("all SCU service requests route to their documented VIC IRQ", routed);
}

static void test_watchdog(void) {
	uint32_t initial_con0 = SCU_WDTCON0;
	uint32_t initial_con1 = SCU_WDTCON1;
	uint32_t initial_status = SCU_WDT_SR;

	test_check("watchdog control is locked", (initial_con0 & SCU_WDTCON0_WDTLCK) != 0);
	test_check("ENDINIT starts set", (initial_con0 & SCU_WDTCON0_ENDINIT) != 0);
	test_eq_u32("watchdog starts without errors", 0, initial_status & WDT_ERROR_STATUS);

	watchdog_measurement_t slow = measure_watchdog_frequency(0);
	watchdog_measurement_t fast = measure_watchdog_frequency(SCU_WDTCON1_WDTIR);
	uint32_t reload = (initial_con0 & SCU_WDTCON0_WDTREL) >> SCU_WDTCON0_WDTREL_SHIFT;
	struct scu_watchdog_access_result restore = scu_watchdog_configure(initial_con1, reload, true);
	uint32_t restored_con0 = SCU_WDTCON0;
	uint32_t restored_con1 = SCU_WDTCON1;
	uint32_t restored_status = SCU_WDT_SR;
	uint32_t expected_slow = CPU_OSC_FREQ / 16384;
	uint32_t expected_fast = CPU_OSC_FREQ / 256;
	printf(
		"# Watchdog: WDTIR=0 %lu Hz, WDTIR=1 %lu Hz, OSC=%lu Hz\n",
		slow.frequency,
		fast.frequency,
		(uint32_t) CPU_OSC_FREQ
	);

	test_check("watchdog password access unlocks WDTCON0", (
		slow.unlocked && fast.unlocked && restore.unlocked
	));
	test_check("watchdog modify access updates ENDINIT", (
		slow.modified && fast.modified && restore.modified
	));
	test_eq_u32("watchdog enters normal mode", 0, slow.status & SCU_WDT_SR_WDTDS);
	test_check("watchdog counter advances", slow.counter_advanced && fast.counter_advanced);
	test_eq_u32("WDTIR selects slow clock", 0, slow.status & SCU_WDT_SR_WDTIS);
	test_eq_u32("WDTIR selects fast clock", SCU_WDT_SR_WDTIS, fast.status & SCU_WDT_SR_WDTIS);
	test_check("slow watchdog frequency is OSC / 16384", frequency_matches(slow.frequency, expected_slow));
	test_check("fast watchdog frequency is OSC / 256", frequency_matches(fast.frequency, expected_fast));
	test_eq_u32(
		"watchdog mode is restored",
		initial_status & SCU_WDT_SR_WDTDS,
		restored_status & SCU_WDT_SR_WDTDS
	);
	test_eq_u32("watchdog configuration is restored", initial_con0 & WDT_CONFIG, restored_con0 & WDT_CONFIG);
	test_eq_u32(
		"watchdog mode requests are restored",
		initial_con1 & (SCU_WDTCON1_WDTIR | SCU_WDTCON1_WDTDR),
		restored_con1 & (SCU_WDTCON1_WDTIR | SCU_WDTCON1_WDTDR)
	);
	test_check("watchdog finishes locked with ENDINIT", (
		(restored_con0 & (SCU_WDTCON0_WDTLCK | SCU_WDTCON0_ENDINIT)) ==
		(SCU_WDTCON0_WDTLCK | SCU_WDTCON0_ENDINIT)
	));
	test_eq_u32("watchdog access sequence has no errors", 0, restored_status & WDT_ERROR_STATUS);
}

int main(void) {
	test_start("SCU peripheral test");

	test_category("Reset values");
	test_reset_values();
	test_category("Reset requests");
	test_reset_requests();
	test_category("Interrupt routing");
	test_interrupt_routing();
	test_category("Watchdog");
	test_watchdog();

	return test_finish();
}
