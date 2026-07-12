#include <pmb887x.h>

#include "test.h"

#define RF_SSC_TIMEOUT_US 1000

static bool wait_rf_ssc_request(void) {
	stopwatch_t start = stopwatch_get();

	while ((TPU_RFSSC_SRC & MOD_SRC_SRR) == 0 && stopwatch_elapsed_us(start) < RF_SSC_TIMEOUT_US)
		test_watchdog_serve();

	return (TPU_RFSSC_SRC & MOD_SRC_SRR) != 0;
}

static bool wait_rf_ssc_complete(void) {
	stopwatch_t start = stopwatch_get();

	while ((TPU_RFCON2 & TPU_RFCON2_SSCEN) != 0 && stopwatch_elapsed_us(start) < RF_SSC_TIMEOUT_US)
		test_watchdog_serve();

	return (TPU_RFCON2 & TPU_RFCON2_SSCEN) == 0;
}

static void test_transfer(const char *name, uint32_t control, uint16_t data) {
	TPU_RFSSC_SRC = MOD_SRC_CLRR;
	TPU_RFSSC_SRC = MOD_SRC_SRE;
	TPU_RFCON2 = control | TPU_RFCON2_SSCEN;
	TPU_RFSSCTB = data;
	test_check(name, wait_rf_ssc_request());
	test_check("SSCEN clears after transfer", wait_rf_ssc_complete());
	TPU_RFSSC_SRC = MOD_SRC_CLRR;
}

static void test_registers(void) {
	test_category("RF SSC registers");
	TPU_RFCON1 = TPU_RFCON1_STBSEL | TPU_RFCON1_RFISSCP;
	test_eq_u32("RFCON1 implemented fields", TPU_RFCON1_STBSEL | TPU_RFCON1_RFISSCP, TPU_RFCON1);

	TPU_RFCON2 = (
		TPU_RFCON2_SSCBM_16 |
		TPU_RFCON2_SSCHB_MSB |
		TPU_RFCON2_SSCPB_TRAILING_EDGE |
		TPU_RFCON2_SSCSB_RFSTR3 |
		TPU_RFCON2_SSCFB_3_25MHZ
	);
	test_eq_u32("RFCON2 implemented fields", (
		TPU_RFCON2_SSCBM_16 |
		TPU_RFCON2_SSCHB_MSB |
		TPU_RFCON2_SSCPB_TRAILING_EDGE |
		TPU_RFCON2_SSCSB_RFSTR3 |
		TPU_RFCON2_SSCFB_3_25MHZ
	), TPU_RFCON2);
}

static void test_disabled_transfer(void) {
	test_category("RF SSC enable control");
	TPU_RFSSC_SRC = MOD_SRC_CLRR;
	TPU_RFSSC_SRC = MOD_SRC_SRE;
	TPU_RFCON2 = TPU_RFCON2_SSCBM_8;
	TPU_RFSSCTB = 0x5A;
	stopwatch_usleep_wd(100);
	test_eq_u32("buffer write without SSCEN does not request service", 0, TPU_RFSSC_SRC & MOD_SRC_SRR);
	test_eq_u32("buffer write without SSCEN stays disabled", 0, TPU_RFCON2 & TPU_RFCON2_SSCEN);

	TPU_RFCON2 = TPU_RFCON2_SSCBM_8 | TPU_RFCON2_SSCEN;
	test_check("setting SSCEN after buffer write starts transfer", wait_rf_ssc_request());
	test_check("deferred transfer completes", wait_rf_ssc_complete());
	TPU_RFSSC_SRC = MOD_SRC_CLRR;
}

static void test_transfers(void) {
	test_category("RF SSC direct transfers");
	TPU_RFCON1 = 0;
	test_transfer("2-bit LSB transfer", TPU_RFCON2_SSCBM_2, 0x0002);
	test_transfer("8-bit MSB transfer", TPU_RFCON2_SSCBM_8 | TPU_RFCON2_SSCHB_MSB, 0x00A5);
	test_transfer("16-bit trailing-edge transfer", (
		TPU_RFCON2_SSCBM_16 |
		TPU_RFCON2_SSCPB_TRAILING_EDGE
	), 0xA55A);
	test_transfer("RFSTR3 transfer", TPU_RFCON2_SSCBM_8 | TPU_RFCON2_SSCSB_RFSTR3, 0x005A);
	test_transfer("3.25 MHz transfer", (
		TPU_RFCON2_SSCBM_16 |
		TPU_RFCON2_SSCFB_3_25MHZ
	), 0x5AA5);
}

int main(void) {
	test_start("TPU RF SSC test");
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_id("module ID", 0xF021C000, TPU_ID);
	test_module_clock("module clock is enabled", TPU_CLC);
	test_registers();
	test_disabled_transfer();
	test_transfers();

	return test_finish();
}
