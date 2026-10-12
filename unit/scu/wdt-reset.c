#include <pmb887x.h>

#include "test.h"
#include "watchdog.h"

#define WDT_RESET_RELOAD 0xF000

int main(void) {
	test_start("SCU watchdog reset test");

	scu_watchdog_configure(SCU_WDTCON1_WDTIR, WDT_RESET_RELOAD, true);
	test_check("watchdog enters normal mode", (SCU_WDT_SR & SCU_WDT_SR_WDTDS) == 0);
	test_expect_reset("watchdog reset stops current execution");

	stopwatch_msleep(250);
	printf("# ERROR: execution continued after the watchdog timeout.\n");
	test_check("watchdog timeout resets the CPU", false);

	return test_finish();
}
