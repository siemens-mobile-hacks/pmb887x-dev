#include <pmb887x.h>

#include "test.h"

#define RTC_INTERRUPT_ENABLES \
	(RTC_ISNC_T14IE | RTC_ISNC_RTC0IE | RTC_ISNC_RTC1IE | RTC_ISNC_RTC2IE | RTC_ISNC_RTC3IE | RTC_ISNC_ALARMIE)
#define RTC_INTERRUPT_FLAGS \
	(RTC_ISNC_T14IR | RTC_ISNC_RTC0IR | RTC_ISNC_RTC1IR | RTC_ISNC_RTC2IR | RTC_ISNC_RTC3IR | RTC_ISNC_ALARMIR)
#define RTC_CLEAR_REQUESTS \
	(RTC_ISNRC_T14 | RTC_ISNRC_RTC0 | RTC_ISNRC_RTC1 | RTC_ISNRC_RTC2 | RTC_ISNRC_RTC3 | RTC_ISNRC_ALARM)
#define RTC_CLOCK_CONTROL \
	(RTC_CTRL_RTCOUTEN | RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN)
#define RTC_SYNC_CONTROL \
	(RTC_CLOCK_CONTROL | RTC_CTRL_CLK_SEL)

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;
static volatile uint32_t irq_isnc;
static volatile uint32_t irq_ctrl;
static volatile uint32_t irq_src;
static volatile uint32_t irq_cnt;
static volatile bool keep_irq_requests;

static bool wait_for_access(void) {
	stopwatch_t start = stopwatch_get();

	while ((RTC_CON & RTC_CON_ACCPOS) == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return (RTC_CON & RTC_CON_ACCPOS) != 0;
}

static bool wait_for_irq(void) {
	stopwatch_t start = stopwatch_get();

	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 500)
		test_watchdog_serve();

	return irq_count != 0;
}

static void stop_and_clear(void) {
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_CLR_RTCINT | RTC_CTRL_CLR_RTCBAD;
	wait_for_access();
	RTC_CON = RTC_CON_PRE;
	stopwatch_usleep_wd(100);
	RTC_ISNC = 0;
	RTC_ISNRC = RTC_CLEAR_REQUESTS;
	RTC_SRC = MOD_SRC_CLRR;
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_CLR_RTCINT | RTC_CTRL_CLR_RTCBAD;
	stopwatch_usleep_wd(100);
	RTC_ISNC = 0;
	RTC_ISNRC = RTC_CLEAR_REQUESTS;
}

static void test_registers(void) {
	test_module_id("module ID", 0xF049C000, RTC_ID);
	test_module_clock("module clock", RTC_CLC);
	test_check("synchronous registers are accessible", wait_for_access());

	stop_and_clear();
	RTC_T14 = (0xA55A << RTC_T14_CNT_SHIFT) | (0x5AA5 << RTC_T14_REL_SHIFT);
	RTC_CNT = 0x12345678;
	RTC_REL = 0x89ABCDEF;
	RTC_ALARM = 0x13579BDF;
	test_eq_u32("T14 count and reload read back", 0xA55A5AA5, RTC_T14);
	test_eq_u32("counter reads back while stopped", 0x12345678, RTC_CNT);
	test_eq_u32("counter reload reads back", 0x89ABCDEF, RTC_REL);
	test_eq_u32("alarm reads back", 0x13579BDF, RTC_ALARM);

	RTC_ISNC = RTC_INTERRUPT_ENABLES;
	stopwatch_usleep_wd(100);
	RTC_ISNC = RTC_INTERRUPT_ENABLES;
	RTC_ISNRC = RTC_CLEAR_REQUESTS;
	test_eq_u32("interrupt enables read back", RTC_INTERRUPT_ENABLES, RTC_ISNC & RTC_INTERRUPT_ENABLES);
	test_eq_u32("interrupt requests start inactive", 0, RTC_ISNC & RTC_INTERRUPT_FLAGS);
	stop_and_clear();
}

static void test_counter(void) {
	stop_and_clear();
	RTC_T14 = (0xFFFF << RTC_T14_CNT_SHIFT) | (0xFFFF << RTC_T14_REL_SHIFT);
	RTC_CNT = 0x12340000;
	RTC_REL = 0;
	RTC_CON = RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL;
	stopwatch_usleep_wd(1000);
	RTC_CTRL = RTC_SYNC_CONTROL;
	test_check("counter is accessible after returning to synchronous mode", wait_for_access());
	test_check("counter advances while running", RTC_CNT != 0x12340000);

	RTC_CON = RTC_CON_PRE;
	uint32_t stopped = RTC_CNT;
	stopwatch_usleep_wd(1000);
	test_eq_u32("counter stops when RUN is clear", stopped, RTC_CNT);
}

static void test_t14_adjustment(void) {
	stop_and_clear();
	RTC_T14 = (0x4000 << RTC_T14_CNT_SHIFT) | (0x8000 << RTC_T14_REL_SHIFT);
	RTC_CON = RTC_CON_RUN | RTC_CON_T14INC;
	stopwatch_usleep_wd(100);
	RTC_CON = 0;
	test_check("T14 increment advances the counter", ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) > 0x4000);
	test_check("T14 increment command clears itself", (RTC_CON & RTC_CON_T14INC) == 0);

	RTC_T14 = (0x4000 << RTC_T14_CNT_SHIFT) | (0x8000 << RTC_T14_REL_SHIFT);
	RTC_CON = RTC_CON_RUN | RTC_CON_T14DEC;
	stopwatch_usleep_wd(100);
	RTC_CON = 0;
	test_check("T14 decrement is applied before counting", ((RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT) >= 0x4000);
	test_check("T14 decrement command clears itself", (RTC_CON & RTC_CON_T14DEC) == 0);
}

static uint32_t measure_t14(bool prescaler) {
	stop_and_clear();
	RTC_T14 = 0;
	RTC_CON = RTC_CON_RUN | (prescaler ? RTC_CON_PRE : 0);
	RTC_CTRL = RTC_CLOCK_CONTROL;
	stopwatch_usleep_wd(10000);
	RTC_CTRL = RTC_SYNC_CONTROL;
	wait_for_access();
	RTC_CON = RTC_CON_PRE;

	return (RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT;
}

static void test_prescaler(void) {
	uint32_t divided = measure_t14(true);
	uint32_t direct = measure_t14(false);

	test_check("T14 advances with prescaler enabled", divided != 0);
	test_check("PRE divides the T14 clock by approximately eight", direct > divided * 4 && direct < divided * 12);
}

static void test_section_reload(void) {
	stop_and_clear();
	RTC_T14 = (0xFFFE << RTC_T14_CNT_SHIFT) | (0xFFFE << RTC_T14_REL_SHIFT);
	RTC_CNT = 0xFFFFFFFF;
	RTC_REL = (0x155 << 22) | (0x15 << 16) | (0x2A << 10) | 0x155;
	RTC_CON = RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL;
	stopwatch_t start = stopwatch_get();
	while (RTC_CNT == 0xFFFFFFFF && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();
	RTC_CTRL = RTC_SYNC_CONTROL;
	test_check("counter is accessible after section reload", wait_for_access());
	RTC_CON = RTC_CON_PRE;
	uint32_t reloaded = RTC_CNT;
	test_eq_u32("upper counter sections reload from RTC_REL", RTC_REL & ~0x3FF, reloaded & ~0x3FF);
	test_check(
		"low counter section continues from its reload value",
		(reloaded & 0x3FF) >= (RTC_REL & 0x3FF) && (reloaded & 0x3FF) < 0x3FF
	);
}

static void test_section_flags(void) {
	stop_and_clear();
	RTC_T14 = (0xFFF0 << RTC_T14_CNT_SHIFT) | (0xFFF0 << RTC_T14_REL_SHIFT);
	RTC_CNT = 0xFFFFFFFF;
	RTC_REL = 0;
	uint32_t enables = (
		RTC_ISNC_T14IE | RTC_ISNC_RTC0IE | RTC_ISNC_RTC1IE | RTC_ISNC_RTC2IE | RTC_ISNC_RTC3IE
	);
	RTC_ISNRC = (
		RTC_ISNRC_T14 | RTC_ISNRC_RTC0 | RTC_ISNRC_RTC1 | RTC_ISNRC_RTC2 | RTC_ISNRC_RTC3
	);
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_RTCOUTEN | RTC_CTRL_CLR_RTCINT;
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_RTCOUTEN | RTC_CTRL_CLR_RTCINT;
	RTC_ISNC = enables;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_RTC_IRQ) = 1;
	irq_count = 0;
	irq_number = 0;
	irq_isnc = 0;
	irq_ctrl = 0;
	irq_src = 0;
	keep_irq_requests = true;
	cpu_enable_irq(true);
	RTC_CON = RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL;
	stopwatch_t start = stopwatch_get();
	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 500)
		test_watchdog_serve();
	cpu_enable_irq(false);
	test_check("counter section overflow raises an IRQ", irq_count != 0);
	test_eq_u32("counter section IRQ is routed to VIC", VIC_RTC_IRQ, irq_number);
	uint32_t request_mask = (
		RTC_ISNC_T14IR | RTC_ISNC_RTC0IR | RTC_ISNC_RTC1IR | RTC_ISNC_RTC2IR | RTC_ISNC_RTC3IR
	);
	uint32_t requests = request_mask;
	test_eq_u32(
		"timer and all counter section requests are pending",
		requests,
		irq_isnc & request_mask
	);

	RTC_CTRL = RTC_SYNC_CONTROL;
	wait_for_access();
	RTC_CON = RTC_CON_PRE;
	test_check("counter wraps after all sections overflow", RTC_CNT < 0x10000);
	keep_irq_requests = false;
	test_eq_u32("hardware request flags remain pending", request_mask, RTC_ISNC & request_mask);
	RTC_ISNC = enables | request_mask;
	test_eq_u32("interrupt enables are restored with requests pending", enables, RTC_ISNC & RTC_INTERRUPT_ENABLES);
	RTC_ISNRC = RTC_INTERRUPT_FLAGS;
	test_eq_u32("odd clear register bits are ignored", requests, RTC_ISNC & request_mask);
	test_eq_u32("odd clear register bits preserve interrupt enables", enables, RTC_ISNC & RTC_INTERRUPT_ENABLES);

	RTC_ISNRC = RTC_ISNRC_T14;
	requests &= ~RTC_ISNC_T14IR;
	test_eq_u32("T14 clear command clears only T14IR", requests, RTC_ISNC & request_mask);
	RTC_ISNRC = RTC_ISNRC_RTC0;
	requests &= ~RTC_ISNC_RTC0IR;
	test_eq_u32("RTC0 clear command clears only RTC0IR", requests, RTC_ISNC & request_mask);
	RTC_ISNRC = RTC_ISNRC_RTC1;
	requests &= ~RTC_ISNC_RTC1IR;
	test_eq_u32("RTC1 clear command clears only RTC1IR", requests, RTC_ISNC & request_mask);
	RTC_ISNRC = RTC_ISNRC_RTC2;
	requests &= ~RTC_ISNC_RTC2IR;
	test_eq_u32("RTC2 clear command clears only RTC2IR", requests, RTC_ISNC & request_mask);
	RTC_ISNRC = RTC_ISNRC_RTC3;
	test_eq_u32("RTC3 clear command clears RTC3IR", 0, RTC_ISNC & request_mask);
	test_eq_u32("request clear commands preserve interrupt enables", enables, RTC_ISNC & RTC_INTERRUPT_ENABLES);
	VIC_CON(VIC_RTC_IRQ) = 0;
	stop_and_clear();
}

static void test_alarm_irq(void) {
	stop_and_clear();
	RTC_T14 = (0xFFF0 << RTC_T14_CNT_SHIFT) | (0xFFF0 << RTC_T14_REL_SHIFT);
	RTC_CNT = 0x10203040;
	RTC_REL = 0;
	RTC_ALARM = 0x10203041;
	RTC_ISNRC = RTC_ISNRC_ALARM;
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_RTCOUTEN | RTC_CTRL_CLR_RTCINT;
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_RTCOUTEN | RTC_CTRL_CLR_RTCINT;
	RTC_ISNC = RTC_ISNC_ALARMIE;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_RTC_IRQ) = 1;
	irq_count = 0;
	irq_number = 0;
	irq_isnc = 0;
	irq_ctrl = 0;
	irq_src = 0;
	keep_irq_requests = true;
	cpu_enable_irq(true);
	RTC_CON = RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL;
	test_check("alarm raises an IRQ", wait_for_irq());
	cpu_enable_irq(false);
	test_eq_u32("alarm IRQ is routed to VIC", VIC_RTC_IRQ, irq_number);
	test_check("alarm IRQ sets ALARMIR", (irq_isnc & RTC_ISNC_ALARMIR) != 0);
	test_check("alarm IRQ sets shell RTCINT", (irq_ctrl & RTC_CTRL_RTCINT) != 0);
	test_check("alarm IRQ sets SRC request", (irq_src & MOD_SRC_SRR) != 0);

	RTC_CTRL = RTC_SYNC_CONTROL;
	wait_for_access();
	RTC_CON = RTC_CON_PRE;
	keep_irq_requests = false;
	RTC_ISNC |= RTC_ISNC_ALARMIE;
	test_check("hardware ALARMIR remains pending", (RTC_ISNC & RTC_ISNC_ALARMIR) != 0);
	RTC_ISNRC = RTC_ISNRC_ALARM;
	test_check("alarm clear command clears ALARMIR", (RTC_ISNC & RTC_ISNC_ALARMIR) == 0);
	test_check("alarm clear command preserves ALARMIE", (RTC_ISNC & RTC_ISNC_ALARMIE) != 0);
	VIC_CON(VIC_RTC_IRQ) = 0;
	stop_and_clear();
}

static void test_repeated_irq(void) {
	stop_and_clear();
	RTC_T14 = (0xFFF0 << RTC_T14_CNT_SHIFT) | (0xFFF0 << RTC_T14_REL_SHIFT);
	RTC_CNT = 0x3FF;
	RTC_REL = 0;
	RTC_ISNRC = RTC_ISNRC_RTC0;
	RTC_ISNC = RTC_ISNC_RTC0IE;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_RTC_IRQ) = 1;
	irq_count = 0;
	cpu_enable_irq(true);
	RTC_CON = RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL;
	test_check("first RTC0 overflow raises an IRQ", wait_for_irq());
	cpu_enable_irq(false);

	RTC_CTRL = RTC_SYNC_CONTROL;
	wait_for_access();
	RTC_CON = RTC_CON_PRE;
	RTC_CNT = 0x3FF;
	RTC_ISNRC = RTC_ISNRC_RTC0;
	RTC_ISNC = RTC_ISNC_RTC0IE;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	cpu_enable_irq(true);
	RTC_CON = RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL;
	stopwatch_t start = stopwatch_get();
	while (irq_count < 2 && stopwatch_elapsed_ms(start) < 500)
		test_watchdog_serve();
	cpu_enable_irq(false);
	test_eq_u32("RTC0 request rearms after ISNRC clear", 2, irq_count);

	RTC_CTRL = RTC_SYNC_CONTROL;
	wait_for_access();
	RTC_CON = RTC_CON_PRE;
	VIC_CON(VIC_RTC_IRQ) = 0;
	stop_and_clear();
}

static void test_alarm_wrap(void) {
	stop_and_clear();
	RTC_T14 = (0xFFF0 << RTC_T14_CNT_SHIFT) | (0xFFF0 << RTC_T14_REL_SHIFT);
	RTC_CNT = 0xFFFFFFFE;
	RTC_REL = 0;
	RTC_ALARM = 0xFFFFFFFF;
	RTC_ISNRC = RTC_ISNRC_ALARM;
	RTC_ISNC = RTC_ISNC_ALARMIE;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_RTC_IRQ) = 1;
	irq_count = 0;
	irq_cnt = 0xFFFFFFFF;
	cpu_enable_irq(true);
	RTC_CON = RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL;
	test_check("alarm raises an IRQ across counter wrap", wait_for_irq());
	cpu_enable_irq(false);
	test_check("wrapped alarm fires after counter equality", irq_cnt < 0x100);
	test_eq_u32(
		"wrapped alarm and counter sections become pending together",
		RTC_INTERRUPT_FLAGS,
		irq_isnc & RTC_INTERRUPT_FLAGS
	);

	RTC_ISNRC = RTC_ISNRC_ALARM;
	RTC_ISNC = RTC_ISNC_ALARMIE;
	cpu_enable_irq(true);
	stopwatch_usleep_wd(100000);
	cpu_enable_irq(false);
	test_eq_u32("alarm does not repeat without another equality", 1, irq_count);

	RTC_CTRL = RTC_SYNC_CONTROL;
	wait_for_access();
	RTC_CON = RTC_CON_PRE;
	VIC_CON(VIC_RTC_IRQ) = 0;
	stop_and_clear();
}

static void test_masked_alarm(void) {
	stop_and_clear();
	RTC_T14 = (0xFFF0 << RTC_T14_CNT_SHIFT) | (0xFFF0 << RTC_T14_REL_SHIFT);
	RTC_CNT = 0x50607080;
	RTC_REL = 0;
	RTC_ALARM = 0x50607081;
	RTC_ISNRC = RTC_ISNRC_ALARM;
	RTC_ISNC = 0;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_RTC_IRQ) = 1;
	irq_count = 0;
	irq_number = 0;
	cpu_enable_irq(true);
	RTC_CON = RTC_CON_RUN;
	RTC_CTRL = RTC_CLOCK_CONTROL;
	stopwatch_usleep_wd(100000);
	cpu_enable_irq(false);

	RTC_CTRL = RTC_SYNC_CONTROL;
	wait_for_access();
	RTC_CON = RTC_CON_PRE;
	test_check("masked alarm keeps ALARMIR pending", (RTC_ISNC & RTC_ISNC_ALARMIR) != 0);
	test_eq_u32("masked alarm does not raise an IRQ", 0, irq_count);

	irq_isnc = 0;
	irq_ctrl = 0;
	irq_src = 0;
	cpu_enable_irq(true);
	RTC_ISNC |= RTC_ISNC_ALARMIE;
	stopwatch_usleep_wd(1000);
	cpu_enable_irq(false);
	test_eq_u32("enabling a pending alarm does not create a new edge", 0, irq_count);
	test_check("enabling a pending alarm keeps ALARMIR visible", (RTC_ISNC & RTC_ISNC_ALARMIR) != 0);

	VIC_CON(VIC_RTC_IRQ) = 0;
	stop_and_clear();
}

int main(void) {
	test_start("RTC peripheral test");
	SCU_RTCIF = 0xAA;
	RTC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_CLR_RTCINT | RTC_CTRL_CLR_RTCBAD;

	test_category("Registers");
	test_registers();

	test_category("Counter");
	test_counter();
	test_t14_adjustment();
	test_prescaler();
	test_section_reload();

	test_category("Interrupts");
	test_section_flags();
	test_alarm_irq();
	test_repeated_irq();
	test_alarm_wrap();
	test_masked_alarm();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	irq_isnc = RTC_ISNC;
	irq_ctrl = RTC_CTRL;
	irq_src = RTC_SRC;
	irq_cnt = RTC_CNT;
	irq_count++;
	RTC_CTRL = RTC_SYNC_CONTROL | RTC_CTRL_CLR_RTCINT;
	for (unsigned int i = 0; i < 1000 && (RTC_CON & RTC_CON_ACCPOS) == 0; i++) {
	}
	if (keep_irq_requests)
		RTC_ISNC = irq_isnc & RTC_INTERRUPT_FLAGS;
	else {
		RTC_ISNC = 0;
		RTC_ISNRC = RTC_CLEAR_REQUESTS;
	}
	RTC_SRC = MOD_SRC_CLRR;
	for (unsigned int i = 0; i < 1000; i++) {
		if ((RTC_ISNC & RTC_INTERRUPT_FLAGS) == 0 && (RTC_SRC & MOD_SRC_SRR) == 0)
			break;
	}
	RTC_CTRL = RTC_CLOCK_CONTROL | RTC_CTRL_CLR_RTCINT;
	VIC_IRQ_ACK = 1;
}
