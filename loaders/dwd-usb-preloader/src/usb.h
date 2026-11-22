#pragma once

#include <stdint.h>

void usb_send(const uint8_t *data, uint32_t size);
static inline void usb_send_byte(uint8_t byte) {
	usb_send(&byte, 1);
}

void usb_recv_reset(void);
uint8_t usb_recv(void);
void usb_recv_buffer(void *buf, uint32_t len);

static inline uint32_t usb_recv_uint32_le() {
	uint32_t v;
	usb_recv_buffer(&v, 4);
	return v;
}
