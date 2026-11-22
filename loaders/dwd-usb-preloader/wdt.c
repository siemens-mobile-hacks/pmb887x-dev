#include "wdt.h"

#include <pmb887x.h>
#include <stdint.h>
#include <stdbool.h>

#define WDT_CONFIG (SCU_WDTCON0_WDTREL | SCU_WDTCON0_WDTPW)

static void wdt_modify(uint32_t reload, bool endinit) {
	SCU_WDTCON0 = (
		(SCU_WDTCON0 & (WDT_CONFIG | SCU_WDTCON0_ENDINIT)) |
		SCU_WDTCON0_WDTHPW1 |
		(SCU_WDTCON1 & (SCU_WDTCON1_WDTIR | SCU_WDTCON1_WDTDR))
	);
	uint32_t modify = (
		(reload << SCU_WDTCON0_WDTREL_SHIFT) |
		(SCU_WDTCON0 & SCU_WDTCON0_WDTPW) |
		SCU_WDTCON0_WDTHPW1 |
		SCU_WDTCON0_WDTLCK
	);
	if (endinit)
		modify |= SCU_WDTCON0_ENDINIT;
	SCU_WDTCON0 = modify;
}

void wdt_set_endinit(bool endinit) {
	uint32_t reload = (SCU_WDTCON0 & SCU_WDTCON0_WDTREL) >> SCU_WDTCON0_WDTREL_SHIFT;
	wdt_modify(reload, endinit);
}
