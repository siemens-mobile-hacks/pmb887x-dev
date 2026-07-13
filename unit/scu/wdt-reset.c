#include <pmb887x.h>

#include "test.h"
#include "watchdog.h"

#define WDT_RESET_RELOAD 0xF000

int main(void) {
	test_start("SCU watchdog reset test");

	printf("# Enabling watchdog; the CPU must reset before the next message.\n");
	scu_watchdog_configure(SCU_WDTCON1_WDTIR, WDT_RESET_RELOAD, true);
	test_check("watchdog enters normal mode", (SCU_WDT_SR & SCU_WDT_SR_WDTDS) == 0);

	stopwatch_msleep_wd(250);
	printf("# ERROR: execution continued after the watchdog timeout.\n");
	test_check("watchdog timeout resets the CPU", false);

	return test_finish();
}
