#include "sio.h"

#include <pmb887x.h>

#include "usb.h"

enum sio_channel {
	SIO_CHANNEL_UNKNOWN,
	SIO_CHANNEL_USART,
	SIO_CHANNEL_USB,
};

static enum sio_channel sio_channel;

void sio_init(void) {
	sio_channel = usb_is_configured() ? SIO_CHANNEL_USB : SIO_CHANNEL_USART;
}

void sio_flush(void) {
	if (!sio_is_usb())
		usart_flush(USART0);
}

void sio_putc(char character) {
	sio_write(&character, sizeof(character));
}

bool sio_is_usb(void) {
	if (sio_channel == SIO_CHANNEL_UNKNOWN)
		sio_init();
	return sio_channel == SIO_CHANNEL_USB;
}

void sio_write(const void *data, uint32_t size) {
	if (sio_is_usb()) {
		usb_write(data, size);
	} else {
		usart_write(USART0, data, size);
	}
}
