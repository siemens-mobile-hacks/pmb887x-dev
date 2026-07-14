#include <pmb887x.h>

#include "test.h"
#include "watchdog.h"

int main(void) {
	test_start("SCU reset request test");

	uint32_t reload = (SCU_WDTCON0 & SCU_WDTCON0_WDTREL) >> SCU_WDTCON0_WDTREL_SHIFT;
	struct scu_watchdog_access_result disable = scu_watchdog_configure(SCU_WDTCON1_WDTDR, reload, true);
	test_check("watchdog is disabled", (
		disable.unlocked &&
		disable.modified &&
		(SCU_WDT_SR & SCU_WDT_SR_WDTDS) != 0
	));

	struct scu_watchdog_access_result unlock = scu_watchdog_set_endinit(false);
	test_check("reset registers are unlocked", unlock.unlocked && unlock.modified);
	printf("# Requesting software reset; the CPU must reset before the next message.\n");
	SCU_BOOT_FLAG = 0;
	SCU_RST_CON = 0;
	scu_watchdog_set_endinit(true);

	stopwatch_msleep_wd(250);
	printf("# ERROR: execution continued after the software reset request.\n");
	test_check("software reset request resets the CPU", false);

	return test_finish();
}
