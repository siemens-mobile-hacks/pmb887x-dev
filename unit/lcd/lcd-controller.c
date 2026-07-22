#include <pmb887x.h>

#include "lcd-controller.h"
#include "lcd-transport.h"

static const struct lcd_controller * const LCD_CONTROLLERS[] = {
	&lcd_controller_l5f30539p00,
	&lcd_controller_jbt6k71,
};

void lcd_controller_reset(const struct lcd_controller *lcd) {
	lcd_transport_reset_controller();
	stopwatch_usleep_wd(lcd->reset_settle_ms * 1000);
}

const struct lcd_controller *lcd_controller_detect(uint32_t *detected_id) {
	for (uint32_t i = 0; i < ARRAY_SIZE(LCD_CONTROLLERS); i++) {
		lcd_controller_reset(LCD_CONTROLLERS[i]);
		if (LCD_CONTROLLERS[i]->probe(detected_id) && *detected_id == LCD_CONTROLLERS[i]->id)
			return LCD_CONTROLLERS[i];
	}

	return NULL;
}
