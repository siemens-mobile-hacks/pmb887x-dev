#pragma once

#include <pmb887x.h>

static inline void gpio_init_input(uint32_t gpio, uint32_t is, uint32_t ps, uint32_t pdpu, uint32_t enaq) {
	GPIO_PIN(gpio) = is | ps | GPIO_DIR_IN | pdpu | enaq;
}

static inline void gpio_init_output(uint32_t gpio, uint32_t os, uint32_t ps, bool data, uint32_t ppen, uint32_t pdpu, uint32_t enaq) {
	GPIO_PIN(gpio) = os | (data ? GPIO_DATA_HIGH : GPIO_DATA_LOW) | ps | GPIO_DIR_OUT | ppen | pdpu | enaq;
}

static inline void gpio_set(uint32_t gpio, bool value) {
	GPIO_PIN(gpio) = value ? GPIO_PIN(gpio) | GPIO_DATA_HIGH : GPIO_PIN(gpio) & ~GPIO_DATA_HIGH;
}

static inline bool gpio_get(uint32_t gpio) {
	return (GPIO_PIN(gpio) & GPIO_DATA_HIGH) != 0;
}

static inline void gpio_toggle(uint32_t gpio) {
	gpio_set(gpio, !gpio_get(gpio));
}
