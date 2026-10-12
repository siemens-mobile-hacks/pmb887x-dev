#include <pmb887x.h>

#include "test.h"

#ifdef PMB8875
#define EXPECTED_CHIP 0x1A
#elif defined(PMB8876)
#define EXPECTED_CHIP 0x1B
#else
#error Unsupported CPU
#endif

int main(void) {
	test_start("SCU identification test");

	uint32_t manid = SCU_MANID;
	uint32_t chipid = SCU_CHIPID;
	uint32_t uid0 = SCU_UID0;
	uint32_t uid1 = SCU_UID1;
	uint32_t uid2 = SCU_UID2;
	uint32_t manufacturer = (manid & SCU_MANID_MANUF) >> SCU_MANID_MANUF_SHIFT;
	uint32_t department = (manid & SCU_MANID_DEPT) >> SCU_MANID_DEPT_SHIFT;
	uint32_t chip = (chipid & SCU_CHIPID_CHIPD) >> SCU_CHIPID_CHIPD_SHIFT;
	uint32_t revision = (chipid & SCU_CHIPID_CHREV) >> SCU_CHIPID_CHREV_SHIFT;

	test_eq_u32("manufacturer is Infineon", 0x182, manufacturer);
	test_eq_u32("manufacturer department", 3, department);
	test_eq_u32("CPU model", EXPECTED_CHIP, chip);
	test_eq_u32("CHIPID reserved bits are zero", 0, chipid & GENMASK(31, 16));
	test_check("CPU UID is programmed", (uid0 | uid1 | uid2) != 0);
	test_check("CPU UID is not erased", (uid0 & uid1 & uid2) != UINT32_MAX);
	printf(
		"# CPU: CHIPID=%02X rev=%02X, MANID=%04X, UID=%08X-%08X-%08X\n",
		(unsigned int) chip,
		(unsigned int) revision,
		(unsigned int) manid,
		(unsigned int) uid0,
		(unsigned int) uid1,
		(unsigned int) uid2
	);

	return test_finish();
}
