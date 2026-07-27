#include <pmb887x.h>

#include "lcd/lcd-controller-test.h"
#include "lcd/lcd-controller.h"
#include "lcd/lcd-transport.h"
#include "lcd-board.h"
#include "s1d13732.h"
#include "test.h"

#if !defined(BOARD_SIEMENS_CX75)
#error The DIFv1 S1D13732 LCD test requires BOARD=siemens-cx75
#endif

static const struct lcd_controller *select_detected_controller(enum lcd_controller_type type) {
	switch (type) {
	case LCD_CONTROLLER_SSD1286:
		return &lcd_controller_ssd1286;
	case LCD_CONTROLLER_PCF8882:
		return &lcd_controller_pcf8882;
	case LCD_CONTROLLER_LS020:
	case LCD_CONTROLLER_L5F30539P00:
	case LCD_CONTROLLER_JBT6K71:
	case LCD_CONTROLLER_UNKNOWN:
		return NULL;
	}

	return NULL;
}

int main(void) {
	test_start("DIFv1 S1D13732 LCD controller");
	lcd_board_initialize_light();
	lcd_transport_init();
	test_eq_u32(
		"CX75 DIF is enabled as master",
		DIF_CON_MS_MASTER | DIF_CON_EN,
		DIF_CON & (DIF_CON_MS | DIF_CON_EN)
	);

	test_category("S1D13732 bridge");
	test_id_u32("bridge ID", 0x706B, s1d13732_read_register(0x0000));

	test_category("Display detection");
	struct s1d13732_display_detect_result detection = s1d13732_detect_display();
	printf(
		"# display detect: C0=%c/%u E0=%c/%u C1=%c/%u\n",
		(detection.sampled_patterns & BIT(0)) != 0 ? 'S' : '-',
		(detection.high_patterns & BIT(0)) != 0,
		(detection.sampled_patterns & BIT(1)) != 0 ? 'S' : '-',
		(detection.high_patterns & BIT(1)) != 0,
		(detection.sampled_patterns & BIT(2)) != 0 ? 'S' : '-',
		(detection.high_patterns & BIT(2)) != 0
	);
	uint32_t expected_sampled_patterns = BIT(0);
	uint32_t expected_high_patterns = BIT(0);
	if (detection.controller_type == LCD_CONTROLLER_LS020) {
		expected_sampled_patterns = BIT(0) | BIT(1);
		expected_high_patterns = BIT(1);
	} else if (detection.controller_type == LCD_CONTROLLER_PCF8882) {
		expected_sampled_patterns = BIT(0) | BIT(1) | BIT(2);
		expected_high_patterns = BIT(2);
	} else if (detection.controller_type == LCD_CONTROLLER_UNKNOWN) {
		expected_sampled_patterns = BIT(0) | BIT(1) | BIT(2);
		expected_high_patterns = 0;
	}
	test_eq_u32("probe stops after the first high response", expected_sampled_patterns, detection.sampled_patterns);
	test_id_u32("display detect response bits", expected_high_patterns, detection.high_patterns);

	const struct lcd_controller *lcd = select_detected_controller(detection.controller_type);
	test_check("supported LCD controller detected", lcd != NULL);
	if (lcd == NULL)
		return test_finish();
	printf("# detected controller: %s, type=%u, nominal ID=%08X\n", lcd->name, lcd->type, lcd->id);
	test_eq_u32("controller type matches selected backend", detection.controller_type, lcd->type);

	test_category("Display power and initialization");
	lcd_board_enable_vboost();
	stopwatch_usleep_wd(10000);
	lcd_board_enable_backlight();
	lcd_transport_reset_controller();
	test_check("LCD controller initializes", lcd->initialize());

	lcd_test_controller(lcd);

	return test_finish();
}
