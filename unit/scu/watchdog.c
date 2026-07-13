#include <pmb887x.h>

#include "watchdog.h"

#define WDT_CONFIG (SCU_WDTCON0_WDTREL | SCU_WDTCON0_WDTPW)

static struct scu_watchdog_access_result scu_watchdog_modify(uint32_t reload, bool endinit) {
	uint32_t password = (
		(SCU_WDTCON0 & (WDT_CONFIG | SCU_WDTCON0_ENDINIT)) |
		SCU_WDTCON0_WDTHPW1 |
		(SCU_WDTCON1 & (SCU_WDTCON1_WDTIR | SCU_WDTCON1_WDTDR))
	);
	SCU_WDTCON0 = password;
	bool unlocked = (SCU_WDTCON0 & SCU_WDTCON0_WDTLCK) == 0;

	uint32_t modify = (
		(reload << SCU_WDTCON0_WDTREL_SHIFT) |
		(SCU_WDTCON0 & SCU_WDTCON0_WDTPW) |
		SCU_WDTCON0_WDTHPW1 |
		SCU_WDTCON0_WDTLCK
	);
	if (endinit)
		modify |= SCU_WDTCON0_ENDINIT;
	SCU_WDTCON0 = modify;
	bool modified = (
		(SCU_WDTCON0 & (SCU_WDTCON0_WDTLCK | SCU_WDTCON0_ENDINIT)) ==
		(SCU_WDTCON0_WDTLCK | (endinit ? SCU_WDTCON0_ENDINIT : 0))
	);

	return (struct scu_watchdog_access_result) {
		.unlocked = unlocked,
		.modified = modified,
	};
}

struct scu_watchdog_access_result scu_watchdog_set_endinit(bool endinit) {
	uint32_t reload = (SCU_WDTCON0 & SCU_WDTCON0_WDTREL) >> SCU_WDTCON0_WDTREL_SHIFT;

	return scu_watchdog_modify(reload, endinit);
}

struct scu_watchdog_access_result scu_watchdog_configure(uint32_t con1, uint32_t reload, bool endinit) {
	struct scu_watchdog_access_result result = scu_watchdog_set_endinit(false);

	SCU_WDTCON1 = con1;
	struct scu_watchdog_access_result configure = scu_watchdog_modify(reload, endinit);
	result.unlocked &= configure.unlocked;
	result.modified &= configure.modified;

	return result;
}
