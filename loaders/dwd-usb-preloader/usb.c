#include <stdint.h>
#include <stdbool.h>
#include "usb.h"

#define RX_BUFFER	((volatile bcore_usb_rx_buffer_t *) 0x2160u)

#define USB_EP2_TX_STATE			(*(volatile uint8_t *) 0x3952u)
#define USB_EP2_TX_ENQUEUE_INDEX	(*(volatile uint8_t *) 0x39A4u)
#define USB_EP2_TX_DEQUEUE_INDEX	(*(volatile uint8_t *) 0x39A5u)

#define bcore_usb_queue_tx			((int (*)(const uint8_t *, uint16_t, void *)) 0x1214Cu)
#define bcore_usb_set_rx_buffer		((void (*)(volatile bcore_usb_rx_buffer_t *)) 0x12550u)

typedef struct {
	uint16_t busy;
	uint8_t frame[0x80Au];
	uint32_t parse_position;
	uint32_t received_size;
	uint16_t reserved;
	uint16_t payload_position;
} bcore_usb_rx_buffer_t;

enum {
	BCORE_USB_TX_FILL_FIFO	= 1,
	BCORE_USB_TX_COMPLETE	= 2,
	BCORE_USB_TX_IDLE		= 3,
};

void usb_send(const uint8_t *data, uint32_t size) {
	while (size != 0) {
		uint16_t chunk_size = size > 48 ? 48 : size;
		uint8_t tx_complete = 1;
		bcore_usb_queue_tx(data, chunk_size, &tx_complete);
		while (USB_EP2_TX_ENQUEUE_INDEX != USB_EP2_TX_DEQUEUE_INDEX || USB_EP2_TX_STATE != BCORE_USB_TX_IDLE);
		data += chunk_size;
		size -= chunk_size;
	}
}

void usb_recv_reset(void) {
	bcore_usb_set_rx_buffer(RX_BUFFER);
}

uint8_t usb_recv(void) {
	while (RX_BUFFER->parse_position >= RX_BUFFER->received_size);
	return RX_BUFFER->frame[RX_BUFFER->parse_position++];
}

void usb_recv_buffer( void *buf, uint32_t len) {
	uint8_t *buf8 = buf;
	for (uint32_t i = 0; i < len; i++)
		buf8[i] = usb_recv();
}
