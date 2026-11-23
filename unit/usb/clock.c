#include <pmb887x.h>
#include <sio.h>

#include "cgu/clock.h"
#include "test.h"

#ifdef BOARD_HAS_PMIC_PMB6812
#include <gen/peripheral/pmic/PMB6812.h>
#include <pmb681x.h>
#endif

#define USB_SUSPEND_CYCLES 144000
#define USB_IDLE_EVENT_CYCLES 240000
#define USB_REMOTE_WAKEUP_K_CYCLES 256000
#define USB_TIMING_TIMEOUT_MS 200

/*
 * HW: the largest timer deviation measured is 1.8 % on PMB8876 and 4.1 % on PMB8875.
 * PMB8875 raises the suspend event 2-13 % late with run-to-run variance, so only its earliest time is checked.
 */
#define USB_TIMING_TOLERANCE_PERCENT 5

static void usb_start(void) {
	SCU_RST_REQ |= SCU_RST_REQ_USB;
	SCU_RST_REQ &= ~SCU_RST_REQ_USB;
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;
	stopwatch_usleep_wd(100);
	USB_EVENT_INT_STATUS = UINT8_MAX;
}

static void clk48m_select(uint32_t source) {
	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_CLK48M_CLKSEL) | source;
}

#ifdef PMB8876
static void clk48m_set_divider(uint32_t divider) {
	CGU_CON3 = (CGU_CON3 & ~CGU_CON3_CLK48M_CLKDIV) | divider;
}
#endif

static bool usb_read_faults(volatile uint32_t *reg) {
	if (test_fault_guard()) {
		test_fault_guard_end();
		return true;
	}
	(void) *reg;
	test_fault_guard_end();
	return false;
}

static bool wait_bits(volatile uint32_t *reg, uint32_t mask, uint32_t value, stopwatch_t start, uint32_t *elapsed_us) {
	while ((*reg & mask) != value) {
		if (stopwatch_elapsed_ms(start) >= USB_TIMING_TIMEOUT_MS)
			return false;
		test_watchdog_serve();
	}
	*elapsed_us = stopwatch_elapsed_us(start);
	return true;
}

static uint32_t cycles_to_us(uint32_t cycles, uint32_t frequency_hz) {
	return (uint64_t) cycles * 1000000 / frequency_hz;
}

static bool in_tolerance(uint32_t measured_us, uint32_t expected_us) {
	uint32_t margin_us = expected_us * USB_TIMING_TOLERANCE_PERCENT / 100;
	return test_u32_in_interval(measured_us, expected_us - margin_us, expected_us + margin_us);
}

static void test_clock_gating(void) {
	clk48m_select(CGU_CON2_CLK48M_CLKSEL_PHASE4);
	usb_start();
	USB_DEVICE_ADDRESS = USB_DEVICE_ADDRESS_ENABLE | 5;
	USB_CFG = USB_CFG_PHY_ENABLE;

	USB_CLC = MOD_CLC_DISR;
	test_check("core register access faults while CLC disables the module", usb_read_faults(&USB_DEVICE_ADDRESS));
	test_check("wrapper register access faults while CLC disables the module", usb_read_faults(&USB_CFG));
	test_check("clock control stays readable while CLC disables the module", !usb_read_faults(&USB_CLC));
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;

	clk48m_select(CGU_CON2_CLK48M_CLKSEL_DISABLE);
	test_check("core register access faults without CLK48M", usb_read_faults(&USB_DEVICE_ADDRESS));
	test_check("wrapper register access faults without CLK48M", usb_read_faults(&USB_CFG));
	test_check("clock control access faults without CLK48M", usb_read_faults(&USB_CLC));
	test_check("module ID access faults without CLK48M", usb_read_faults(&USB_ID));
	clk48m_select(CGU_CON2_CLK48M_CLKSEL_PHASE4);
	test_eq_u32("core registers keep their state without CLK48M", USB_DEVICE_ADDRESS_ENABLE | 5, USB_DEVICE_ADDRESS);
	test_eq_u32("wrapper keeps its state without CLK48M", USB_CFG_PHY_ENABLE, USB_CFG);
}

static void test_bus_timers(void) {
	static const struct {
		const char *name;
		uint32_t source;
		uint32_t phase_k1;
		uint32_t phase_k2;
		uint32_t divider;
		uint32_t frequency_hz;
	} CLOCKS[] = {
#ifdef PMB8876
		{ "PHASE4/2", CGU_CON2_CLK48M_CLKSEL_PHASE4, 2, 1, CGU_CON3_CLK48M_CLKDIV_DIV2, 48000000 },
		{ "PHASE4/4", CGU_CON2_CLK48M_CLKSEL_PHASE4, 2, 1, CGU_CON3_CLK48M_CLKDIV_DIV4, 24000000 },
		{ "PHASE4/8", CGU_CON2_CLK48M_CLKSEL_PHASE4, 2, 1, CGU_CON3_CLK48M_CLKDIV_DIV8, 12000000 },
		{ "OSC/1", CGU_CON2_CLK48M_CLKSEL_OSC, 2, 1, CGU_CON3_CLK48M_CLKDIV_DIV1, CPU_OSC_FREQ },
		{ "OSC/2", CGU_CON2_CLK48M_CLKSEL_OSC, 2, 1, CGU_CON3_CLK48M_CLKDIV_DIV2, CPU_OSC_FREQ / 2 },
		{ "OSC/4", CGU_CON2_CLK48M_CLKSEL_OSC, 2, 1, CGU_CON3_CLK48M_CLKDIV_DIV4, CPU_OSC_FREQ / 4 },
		{ "OSC/8", CGU_CON2_CLK48M_CLKSEL_OSC, 2, 1, CGU_CON3_CLK48M_CLKDIV_DIV8, CPU_OSC_FREQ / 8 },
#else
		{ "PHASE4 4/2", CGU_CON2_CLK48M_CLKSEL_PHASE4, 4, 2, CGU_CON3_CLK48M_CLKDIV_DIV1, 48000000 },
		{ "PHASE4 8/4", CGU_CON2_CLK48M_CLKSEL_PHASE4, 8, 4, CGU_CON3_CLK48M_CLKDIV_DIV1, 24000000 },
		{ "PHASE4 13/0", CGU_CON2_CLK48M_CLKSEL_PHASE4, 13, 0, CGU_CON3_CLK48M_CLKDIV_DIV1, 16000000 },
		{ "OSC", CGU_CON2_CLK48M_CLKSEL_OSC, 4, 2, CGU_CON3_CLK48M_CLKDIV_DIV1, CPU_OSC_FREQ },
#endif
	};
	bool suspend_matches = true;
	bool idle_matches = true;
	bool remote_wakeup_matches = true;
	for (uint32_t index = 0; index < ARRAY_SIZE(CLOCKS); index++) {
		cgu_phase_set_divider(4, CLOCKS[index].phase_k1, CLOCKS[index].phase_k2);
		clk48m_select(CLOCKS[index].source);
#ifdef PMB8876
		clk48m_set_divider(CLOCKS[index].divider);
#endif
		usb_start();
		uint32_t suspend_us = 0;
		uint32_t idle_us = 0;
		uint32_t k_start_us = 0;
		uint32_t k_end_us = 0;
		USB_CFG = USB_CFG_PHY_ENABLE | USB_CFG_DRIVE_ENABLE;
		stopwatch_t start = stopwatch_get();
		bool suspended = wait_bits(
			&USB_EVENT_INT_STATUS,
			USB_EVENT_INT_STATUS_SUSPEND,
			USB_EVENT_INT_STATUS_SUSPEND,
			start,
			&suspend_us
		);
		bool idle = suspended && wait_bits(
			&USB_EVENT_INT_STATUS,
			USB_EVENT_INT_STATUS_IDLE,
			USB_EVENT_INT_STATUS_IDLE,
			start,
			&idle_us
		);
		USB_PHY_CONTROL = USB_PHY_CONTROL_RESUME;
		bool k_started = idle && wait_bits(
			&USB_PHY_CONTROL,
			USB_PHY_CONTROL_LINE_STATE,
			USB_PHY_CONTROL_LINE_STATE_K,
			start,
			&k_start_us
		);
		bool k_ended = k_started && wait_bits(
			&USB_PHY_CONTROL,
			USB_PHY_CONTROL_LINE_STATE,
			USB_PHY_CONTROL_LINE_STATE_J,
			start,
			&k_end_us
		);

		uint32_t k_duration_us = k_ended ? k_end_us - k_start_us : 0;
		uint32_t suspend_expected_us = cycles_to_us(USB_SUSPEND_CYCLES, CLOCKS[index].frequency_hz);
		uint32_t idle_expected_us = cycles_to_us(USB_IDLE_EVENT_CYCLES, CLOCKS[index].frequency_hz);
		uint32_t k_expected_us = cycles_to_us(USB_REMOTE_WAKEUP_K_CYCLES, CLOCKS[index].frequency_hz);
		printf("# CLK48M %s (%lu kHz): ", CLOCKS[index].name, CLOCKS[index].frequency_hz / 1000);
		printf("suspend after %lu us (expected %lu), ", suspend_us, suspend_expected_us);
		printf("idle event after %lu us (%lu), ", idle_us, idle_expected_us);
		printf("remote wakeup K %lu us (%lu)\n", k_duration_us, k_expected_us);
		uint32_t suspend_earliest_us = suspend_expected_us - suspend_expected_us * USB_TIMING_TOLERANCE_PERCENT / 100;
		suspend_matches = suspend_matches && idle && suspend_us >= suspend_earliest_us && suspend_us <= idle_us;
		idle_matches = idle_matches && idle && in_tolerance(idle_us, idle_expected_us);
		remote_wakeup_matches = remote_wakeup_matches && k_ended && in_tolerance(k_duration_us, k_expected_us);
	}
	test_check("suspend event comes after 144000 CLK48M cycles, before the idle event", suspend_matches);
	test_check("idle event comes after 240000 CLK48M cycles of bus idle", idle_matches);
	test_check("remote wakeup K lasts 256000 CLK48M cycles", remote_wakeup_matches);
}

int main(void) {
	test_start("USB clock test");
	if (sio_is_usb()) {
		test_skip("USB clock test", "reprograms the clock of the USB controller that carries the console");
		return test_finish();
	}
	cgu_pll_set(7, 1);
	cgu_phases_enable(CGU_PHASE4);
	cgu_phase_set_divider(4, 2, 1);
#ifdef PMB8876
	clk48m_set_divider(CGU_CON3_CLK48M_CLKDIV_DIV2);
#endif
#ifdef BOARD_HAS_PMIC_PMB6812
	i2c_init();
	test_check(
		"PMIC enables the USB transceiver supply",
		pmb681x_reg_write(PMB6812_USBCTRL, PMB6812_USBCTRL_LUSBMD_ON | PMB6812_USBCTRL_PULLUP_PULLUP)
	);
#endif

	test_category("Clock gating");
	test_clock_gating();
	test_category("Bus timers");
	test_bus_timers();
	return test_finish();
}
