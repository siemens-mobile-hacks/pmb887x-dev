#pragma once

#include <stdbool.h>
#include <stdint.h>

struct scu_watchdog_access_result {
	bool unlocked;
	bool modified;
};

struct scu_watchdog_access_result scu_watchdog_set_endinit(bool endinit);
struct scu_watchdog_access_result scu_watchdog_configure(uint32_t con1, uint32_t reload, bool endinit);
