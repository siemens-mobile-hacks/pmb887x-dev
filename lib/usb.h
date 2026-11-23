#pragma once

#include <stdbool.h>
#include <stdint.h>

bool usb_is_configured(void);
uint32_t usb_read_packet(void *data, uint32_t capacity);
bool usb_write_packet(const void *data, uint32_t size);
void usb_write(const void *data, uint32_t size);
