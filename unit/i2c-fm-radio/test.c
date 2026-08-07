#include <pmb887x.h>
#include <fm/TEA5761UK.h>

#include "i2c-v2.h"
#include "test.h"

#define FM_RADIO_READ_SIZE 16
#define FM_RADIO_WRITE_SIZE 7

#ifdef BOARD_SIEMENS_E71
static void enable_fm_radio(void) {
	GPIO_PIN(GPIO_CLK32) = GPIO_OS_ALT1 | GPIO_PS_ALT;
	CGU_CON2 |= CGU_CON2_CLK32K_EN;
	gpio_init_output(
		GPIO_A_FM_STANDBY,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_PULLUP,
		GPIO_ENAQ_OFF
	);
	stopwatch_usleep_wd(20);
}

static void test_tea5761uk(void) {
	static const uint8_t expected_id[] = { 0x40, 0x2B, 0x57, 0x61 };
	uint8_t before[FM_RADIO_READ_SIZE] = { 0 };
	uint8_t after[FM_RADIO_READ_SIZE] = { 0 };
	uint8_t config[FM_RADIO_WRITE_SIZE];
	uint8_t interrupt_regs[2];
	uint8_t signal_level;
	bool completed;

	completed = i2c_v2_transfer(TEA5761UK_I2C_ADDR, NULL, before, sizeof(before));
	if (!test_check("TEA5761UK register read completes", completed))
		return;
	if (!test_eq_memory("TEA5761UK ID", expected_id, &before[TEA5761UK_MANID_HIGH], sizeof(expected_id)))
		return;
	printf("# TEA5761UK registers:");
	for (unsigned int i = 0; i < ARRAY_SIZE(before); i++)
		printf(" %02X", before[i]);
	printf("\n");
	signal_level = before[TEA5761UK_TUNCHK_STATUS] & TEA5761UK_TUNCHK_STATUS_LEV_3_0;
	test_check("TEA5761UK idle signal level is nonzero", signal_level != 0);

	config[0] = before[TEA5761UK_INTREG_MASK] ^ TEA5761UK_INTREG_MASK_LEVMSK;
	config[1] = before[TEA5761UK_FRQSET_HIGH];
	config[2] = before[TEA5761UK_FRQSET_LOW];
	config[3] = before[TEA5761UK_TNCTRL_CONTROL];
	config[4] = before[TEA5761UK_TNCTRL_AUDIO];
	config[5] = before[TEA5761UK_TESTREG_CONTROL];
	config[6] = before[TEA5761UK_TESTREG_CONFIG];
	completed = i2c_v2_transfer(TEA5761UK_I2C_ADDR, config, NULL, sizeof(config));
	if (!test_check("TEA5761UK register write completes", completed))
		return;
	completed = i2c_v2_transfer(TEA5761UK_I2C_ADDR, NULL, after, sizeof(after));
	if (!test_check("TEA5761UK readback completes", completed))
		return;
	test_eq_u32("TEA5761UK interrupt mask readback", config[0], after[TEA5761UK_INTREG_MASK]);
	test_eq_u32("TEA5761UK FRQSET high readback", config[1], after[TEA5761UK_FRQSET_HIGH]);
	test_eq_u32("TEA5761UK FRQSET low readback", config[2], after[TEA5761UK_FRQSET_LOW]);
	test_eq_u32("TEA5761UK TNCTRL control readback", config[3], after[TEA5761UK_TNCTRL_CONTROL]);
	test_eq_u32("TEA5761UK TNCTRL audio readback", config[4], after[TEA5761UK_TNCTRL_AUDIO]);
	test_eq_u32("TEA5761UK TESTREG control readback", config[5], after[TEA5761UK_TESTREG_CONTROL]);
	test_eq_u32("TEA5761UK TESTREG config readback", config[6], after[TEA5761UK_TESTREG_CONFIG]);

	completed = i2c_v2_transfer(TEA5761UK_I2C_ADDR, NULL, interrupt_regs, sizeof(interrupt_regs));
	if (!test_check("TEA5761UK interrupt register reread completes", completed))
		return;
	test_eq_u32("TEA5761UK interrupt mask clears after read", 0, interrupt_regs[TEA5761UK_INTREG_MASK]);
}
#endif

int main(void) {
	test_start("I2C FM radio test");

#ifdef BOARD_SIEMENS_E71
	enable_fm_radio();
	i2c_v2_init();
	test_tea5761uk();
#else
	test_skip("TEA5761UK", "unsupported board");
#endif

	return test_finish();
}
