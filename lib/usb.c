#include "usb.h"

#include <pmb887x.h>

enum usb_endpoint_t {
	USB_IN_ENDPOINT = 3,
	USB_OUT_ENDPOINT = 4,
};

enum usb_slot_t {
	USB_IN_SLOT = 4,
	USB_OUT_SLOT = 5,
};

static bool usb_in_packet_pending;

static uint32_t usb_slot_get_byte_count(uint32_t slot) {
	return USB_EP_COUNT_LOW(slot) |
		((USB_EP_COUNT_HIGH(slot) & USB_EP_COUNT_HIGH_VALUE) << 8);
}

static uint32_t usb_in_get_max_packet_size(void) {
	return USB_EP_MAX_PACKET_LOW(USB_IN_ENDPOINT) |
		((USB_EP_MAX_PACKET_HIGH(USB_IN_ENDPOINT) & USB_EP_MAX_PACKET_HIGH_VALUE) << 8);
}

bool usb_is_configured(void) {
	uint32_t clk48m_source = CGU_CON2 & CGU_CON2_CLK48M_CLKSEL;
	if (clk48m_source != CGU_CON2_CLK48M_CLKSEL_OSC && clk48m_source != CGU_CON2_CLK48M_CLKSEL_PHASE4)
		return false;
	if ((USB_CLC & MOD_CLC_DISS) != 0)
		return false;
	if ((USB_DEVICE_ADDRESS & USB_DEVICE_ADDRESS_ADDRESS) == 0)
		return false;
	return usb_in_get_max_packet_size() != 0;
}

uint32_t usb_read_packet(void *data, uint32_t capacity) {
	uint8_t *bytes = data;
	uint32_t endpoint_mask = BIT(USB_OUT_ENDPOINT);

	USB_GLOBAL_INT_ENABLE = 0;
	while (((USB_EP_READY_INT_STATUS_LOW | USB_EP_DONE_INT_STATUS_LOW) & endpoint_mask) == 0 &&
		usb_slot_get_byte_count(USB_OUT_SLOT) == 0);

	uint32_t size = 0;
	if (!(USB_EP_CONFIG(USB_OUT_SLOT) & USB_EP_CONFIG_ZLP))
		size = usb_slot_get_byte_count(USB_OUT_SLOT);
	for (uint32_t index = 0; index < size; index++) {
		uint8_t byte = USB_EP_DATA(USB_OUT_SLOT);
		if (index < capacity)
			bytes[index] = byte;
	}

	USB_EP_READY_INT_STATUS_LOW = endpoint_mask;
	USB_EP_DONE_INT_STATUS_LOW = endpoint_mask;
	USB_EP_CONFIG(USB_OUT_SLOT) &= ~(USB_EP_CONFIG_ZLP | USB_EP_CONFIG_NAK | USB_EP_CONFIG_STALL);
	USB_EP_CONTROL(USB_OUT_SLOT) &= ~USB_EP_CONTROL_COMMIT;
	USB_EP_CONTROL(USB_OUT_SLOT) |= USB_EP_CONTROL_FLUSH | USB_EP_CONTROL_ARM;
	return size;
}

bool usb_write_packet(const void *data, uint32_t size) {
	const uint8_t *bytes = data;
	uint32_t endpoint_mask = BIT(USB_IN_ENDPOINT);
	uint32_t max_packet_size = usb_in_get_max_packet_size();

	if (max_packet_size == 0 || size > max_packet_size)
		return false;

	USB_GLOBAL_INT_ENABLE = 0;
	if (usb_in_packet_pending)
		while (!(USB_EP_DONE_INT_STATUS_LOW & endpoint_mask));

	USB_EP_READY_INT_STATUS_LOW = endpoint_mask;
	USB_EP_DONE_INT_STATUS_LOW = endpoint_mask;
	USB_EP_CONTROL(USB_IN_SLOT) &= ~USB_EP_CONTROL_COMMIT;
	for (uint32_t index = 0; index < size; index++)
		USB_EP_DATA(USB_IN_SLOT) = bytes[index];

	if (size == 0) {
		USB_EP_CONFIG(USB_IN_SLOT) |= USB_EP_CONFIG_ZLP;
		USB_EP_CONTROL(USB_IN_SLOT) |= USB_EP_CONTROL_COMMIT;
		USB_EP_CONFIG(USB_IN_SLOT) &= ~(USB_EP_CONFIG_NAK | USB_EP_CONFIG_STALL);
	} else {
		USB_EP_CONTROL(USB_IN_SLOT) |= USB_EP_CONTROL_COMMIT;
		USB_EP_CONFIG(USB_IN_SLOT) &= ~(USB_EP_CONFIG_ZLP | USB_EP_CONFIG_NAK | USB_EP_CONFIG_STALL);
	}
	usb_in_packet_pending = true;
	return true;
}

void usb_write(const void *data, uint32_t size) {
	const uint8_t *bytes = data;
	uint32_t max_packet_size = usb_in_get_max_packet_size();

	for (uint32_t offset = 0; offset < size; offset += max_packet_size)
		usb_write_packet(bytes + offset, MIN(size - offset, max_packet_size));
}
