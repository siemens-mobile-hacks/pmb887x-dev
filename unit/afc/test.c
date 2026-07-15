#include <pmb887x.h>

#include "test.h"

#define AFC_FIRMWARE_NOMINAL_VALUE 0x4800

static void test_reset_values(void) {
	test_category("Reset values");
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, AFC_CLC);
	AFC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_id("ID", 0xF004C000, AFC_ID);
	test_eq_u32("AFCVAL reset value", 0, AFC_AFCVAL);
}

static void test_reference_value(void) {
	test_category("Reference value");
	AFC_AFCVAL = 0x1234;
	test_eq_u32("AFCVAL stores special-mode low bits", 0x1234, AFC_AFCVAL);

	AFC_AFCVAL = AFC_AFCVAL_AFC;
	test_eq_u32("AFCVAL stores all 15 reference bits", AFC_AFCVAL_AFC, AFC_AFCVAL);

	AFC_AFCVAL = 0xFFFF0000 | AFC_AFCVAL_ENAFC | AFC_FIRMWARE_NOMINAL_VALUE;
	test_eq_u32(
		"AFCVAL ignores bits above the documented 16-bit register",
		AFC_AFCVAL_ENAFC | AFC_FIRMWARE_NOMINAL_VALUE,
		AFC_AFCVAL
	);
	test_check("ENAFC enables the output", (AFC_AFCVAL & AFC_AFCVAL_ENAFC) != 0);
	test_eq_u32(
		"ENAFC preserves the nominal reference value",
		AFC_FIRMWARE_NOMINAL_VALUE,
		AFC_AFCVAL & AFC_AFCVAL_AFC
	);

	AFC_AFCVAL = AFC_FIRMWARE_NOMINAL_VALUE;
	test_eq_u32("clearing ENAFC disables the output", 0, AFC_AFCVAL & AFC_AFCVAL_ENAFC);
	test_eq_u32(
		"disabling preserves the reference value",
		AFC_FIRMWARE_NOMINAL_VALUE,
		AFC_AFCVAL & AFC_AFCVAL_AFC
	);

	AFC_AFCVAL = 0;
}

int main(void) {
	test_start("AFC");
	test_reset_values();
	test_reference_value();

	return test_finish();
}
