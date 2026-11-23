#pragma once

#include <stdbool.h>
#include <stdint.h>

void sio_init(void);
bool sio_is_usb(void);
void sio_flush(void);
void sio_putc(char character);
void sio_write(const void *data, uint32_t size);
