#include <pmb887x.h>
#include <printf/printf.h>
#include <stdarg.h>
#include <string.h>

#include "cgu/clock.h"

#if defined(BOARD_HAS_PMIC_D1094XX)
#include <gen/peripheral/pmic/D1094XX.h>
#elif defined(BOARD_HAS_PMIC_PMB6812)
#include <gen/peripheral/pmic/PMB6812.h>
#include <pmb681x.h>
#endif

#define USB_VID 0x1209
#define USB_PID 0x0001

#define EP0_OUT_SLOT 0
#define EP0_IN_SLOT 1
#define EP0_SIZE 32
#define EP0_FIFO_START 0x50

#define CDC_DATA_IN_EP 1
#define CDC_DATA_OUT_EP 2
#define CDC_NOTIFY_EP 3
#define CDC_DATA_SIZE 64
#define CDC_NOTIFY_SIZE 16
#define CDC_DATA_IN_FIFO_START 0x00
#define CDC_DATA_OUT_FIFO_START 0x08
#define CDC_NOTIFY_FIFO_START 0x04

#define USB_CONNECT_TIMEOUT_MS 60000
#define USB_DETACH_MS 90
#define USB_STATUS_TIMEOUT_MS 50
#define CDC_WRITE_TIMEOUT_MS 100
#define HELLO_INTERVAL_MS 1000

#define USB_TYPE_MASK 0x60
#define USB_ENDPOINT_IN 0x80
#define USB_CONFIG_SELF_POWERED 0xC0
#define CDC_CONTROL_LINE_DTR BIT(0)
#define CDC_ACM_LINE_CODING BIT(1)
#define USB_LANGID_EN_US 0x0409
#define USB_STRING_MAX_CHARS 31

enum usb_request_type {
	USB_TYPE_STANDARD = 0x00,
	USB_TYPE_CLASS = 0x20,
};

enum usb_transfer_type {
	USB_ENDPOINT_BULK = 0x02,
	USB_ENDPOINT_INTERRUPT = 0x03,
};

enum usb_request {
	USB_REQ_GET_STATUS = 0,
	USB_REQ_CLEAR_FEATURE = 1,
	USB_REQ_SET_FEATURE = 3,
	USB_REQ_SET_ADDRESS = 5,
	USB_REQ_GET_DESCRIPTOR = 6,
	USB_REQ_GET_CONFIGURATION = 8,
	USB_REQ_SET_CONFIGURATION = 9,
	USB_REQ_GET_INTERFACE = 10,
	USB_REQ_SET_INTERFACE = 11,
};

enum cdc_request {
	CDC_REQ_SET_LINE_CODING = 0x20,
	CDC_REQ_GET_LINE_CODING = 0x21,
	CDC_REQ_SET_CONTROL_LINE_STATE = 0x22,
	CDC_REQ_SEND_BREAK = 0x23,
};

enum usb_descriptor_type {
	USB_DT_DEVICE = 1,
	USB_DT_CONFIGURATION = 2,
	USB_DT_STRING = 3,
	USB_DT_INTERFACE = 4,
	USB_DT_ENDPOINT = 5,
	USB_DT_CS_INTERFACE = 0x24,
};

enum usb_class {
	USB_CLASS_CDC = 0x02,
	USB_CLASS_CDC_DATA = 0x0A,
};

enum cdc_subclass {
	CDC_SUBCLASS_ACM = 0x02,
};

enum cdc_protocol {
	CDC_PROTOCOL_AT = 0x01,
};

enum cdc_descriptor_subtype {
	CDC_DST_HEADER = 0x00,
	CDC_DST_CALL_MANAGEMENT = 0x01,
	CDC_DST_ACM = 0x02,
	CDC_DST_UNION = 0x06,
};

enum usb_string_index {
	USB_STRING_LANGID,
	USB_STRING_MANUFACTURER,
	USB_STRING_PRODUCT,
};

enum cdc_interface {
	CDC_INTERFACE_CONTROL,
	CDC_INTERFACE_DATA,
};

struct usb_setup {
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __attribute__((packed));
_Static_assert(sizeof(struct usb_setup) == 8, "SETUP packet size");

struct usb_device_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} __attribute__((packed));
_Static_assert(sizeof(struct usb_device_descriptor) == 18, "device descriptor size");

struct usb_configuration_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} __attribute__((packed));
_Static_assert(sizeof(struct usb_configuration_descriptor) == 9, "configuration descriptor size");

struct usb_interface_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} __attribute__((packed));
_Static_assert(sizeof(struct usb_interface_descriptor) == 9, "interface descriptor size");

struct usb_endpoint_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} __attribute__((packed));
_Static_assert(sizeof(struct usb_endpoint_descriptor) == 7, "endpoint descriptor size");

struct cdc_header_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint16_t bcdCDC;
} __attribute__((packed));
_Static_assert(sizeof(struct cdc_header_descriptor) == 5, "CDC header descriptor size");

struct cdc_call_management_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bmCapabilities;
	uint8_t bDataInterface;
} __attribute__((packed));
_Static_assert(sizeof(struct cdc_call_management_descriptor) == 5, "CDC call management descriptor size");

struct cdc_acm_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bmCapabilities;
} __attribute__((packed));
_Static_assert(sizeof(struct cdc_acm_descriptor) == 4, "CDC ACM descriptor size");

struct cdc_union_descriptor {
	uint8_t bFunctionLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bControlInterface;
	uint8_t bSubordinateInterface0;
} __attribute__((packed));
_Static_assert(sizeof(struct cdc_union_descriptor) == 5, "CDC union descriptor size");

struct cdc_configuration {
	struct usb_configuration_descriptor configuration;
	struct usb_interface_descriptor control_interface;
	struct cdc_header_descriptor header;
	struct cdc_call_management_descriptor call_management;
	struct cdc_acm_descriptor acm;
	struct cdc_union_descriptor cdc_union;
	struct usb_endpoint_descriptor notify_endpoint;
	struct usb_interface_descriptor data_interface;
	struct usb_endpoint_descriptor data_out_endpoint;
	struct usb_endpoint_descriptor data_in_endpoint;
} __attribute__((packed));
_Static_assert(sizeof(struct cdc_configuration) == 67, "CDC configuration size");

struct cdc_line_coding {
	uint32_t dwDTERate;
	uint8_t bCharFormat;
	uint8_t bParityType;
	uint8_t bDataBits;
} __attribute__((packed));
_Static_assert(sizeof(struct cdc_line_coding) == 7, "CDC line coding size");

struct usb_string_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wData[USB_STRING_MAX_CHARS];
} __attribute__((packed));

struct cdc_tx_buffer {
	char data[CDC_DATA_SIZE];
	size_t length;
};

struct usb_cdc {
	const uint8_t *ep0_tx_data;
	size_t ep0_tx_left;
	bool ep0_tx_active;
	bool ep0_tx_zlp;
	bool line_coding_pending;
	bool configured;
	bool dtr;
	struct cdc_line_coding line_coding;
	struct usb_string_descriptor string;
};

static const struct usb_device_descriptor DEVICE_DESCRIPTOR = {
	.bLength = sizeof(struct usb_device_descriptor),
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0110,
	.bDeviceClass = USB_CLASS_CDC,
	.bMaxPacketSize0 = EP0_SIZE,
	.idVendor = USB_VID,
	.idProduct = USB_PID,
	.bcdDevice = 0x0100,
	.iManufacturer = USB_STRING_MANUFACTURER,
	.iProduct = USB_STRING_PRODUCT,
	.bNumConfigurations = 1,
};

static const struct cdc_configuration CONFIGURATION = {
	.configuration = {
		.bLength = sizeof(struct usb_configuration_descriptor),
		.bDescriptorType = USB_DT_CONFIGURATION,
		.wTotalLength = sizeof(struct cdc_configuration),
		.bNumInterfaces = 2,
		.bConfigurationValue = 1,
		.bmAttributes = USB_CONFIG_SELF_POWERED,
		.bMaxPower = 50,
	},
	.control_interface = {
		.bLength = sizeof(struct usb_interface_descriptor),
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = CDC_INTERFACE_CONTROL,
		.bNumEndpoints = 1,
		.bInterfaceClass = USB_CLASS_CDC,
		.bInterfaceSubClass = CDC_SUBCLASS_ACM,
		.bInterfaceProtocol = CDC_PROTOCOL_AT,
	},
	.header = {
		.bFunctionLength = sizeof(struct cdc_header_descriptor),
		.bDescriptorType = USB_DT_CS_INTERFACE,
		.bDescriptorSubtype = CDC_DST_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_management = {
		.bFunctionLength = sizeof(struct cdc_call_management_descriptor),
		.bDescriptorType = USB_DT_CS_INTERFACE,
		.bDescriptorSubtype = CDC_DST_CALL_MANAGEMENT,
		.bDataInterface = CDC_INTERFACE_DATA,
	},
	.acm = {
		.bFunctionLength = sizeof(struct cdc_acm_descriptor),
		.bDescriptorType = USB_DT_CS_INTERFACE,
		.bDescriptorSubtype = CDC_DST_ACM,
		.bmCapabilities = CDC_ACM_LINE_CODING,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct cdc_union_descriptor),
		.bDescriptorType = USB_DT_CS_INTERFACE,
		.bDescriptorSubtype = CDC_DST_UNION,
		.bControlInterface = CDC_INTERFACE_CONTROL,
		.bSubordinateInterface0 = CDC_INTERFACE_DATA,
	},
	.notify_endpoint = {
		.bLength = sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = USB_ENDPOINT_IN | CDC_NOTIFY_EP,
		.bmAttributes = USB_ENDPOINT_INTERRUPT,
		.wMaxPacketSize = CDC_NOTIFY_SIZE,
		.bInterval = 255,
	},
	.data_interface = {
		.bLength = sizeof(struct usb_interface_descriptor),
		.bDescriptorType = USB_DT_INTERFACE,
		.bInterfaceNumber = CDC_INTERFACE_DATA,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_CLASS_CDC_DATA,
	},
	.data_out_endpoint = {
		.bLength = sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = CDC_DATA_OUT_EP,
		.bmAttributes = USB_ENDPOINT_BULK,
		.wMaxPacketSize = CDC_DATA_SIZE,
	},
	.data_in_endpoint = {
		.bLength = sizeof(struct usb_endpoint_descriptor),
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = USB_ENDPOINT_IN | CDC_DATA_IN_EP,
		.bmAttributes = USB_ENDPOINT_BULK,
		.wMaxPacketSize = CDC_DATA_SIZE,
	},
};

static const char *const STRINGS[] = {
	[USB_STRING_MANUFACTURER] = "pmb887x",
	[USB_STRING_PRODUCT] = "USB CDC ACM example",
};

static struct usb_cdc usb = {
	.line_coding = { .dwDTERate = 115200, .bDataBits = 8 },
};

static uint32_t usb_slot_get_count(uint32_t slot) {
	return USB_EP_COUNT_LOW(slot) | ((USB_EP_COUNT_HIGH(slot) & USB_EP_COUNT_HIGH_VALUE) << 8);
}

static void usb_endpoint_set_fifo(uint32_t endpoint, uint32_t start, uint32_t size_code, uint32_t max_packet) {
	USB_EP_FIFO_START(endpoint) = start;
	USB_EP_FIFO_SIZE(endpoint) = size_code << USB_EP_FIFO_SIZE_SIZE_SHIFT;
	USB_EP_MAX_PACKET_LOW(endpoint) = max_packet & USB_EP_MAX_PACKET_LOW_VALUE;
	USB_EP_MAX_PACKET_HIGH(endpoint) = max_packet >> 8;
}

static void usb_ep0_write_chunk(void) {
	uint32_t free_bytes = usb_slot_get_count(EP0_IN_SLOT);
	size_t size = MIN(usb.ep0_tx_left, free_bytes);

	for (size_t index = 0; index < size; index++)
		USB_EP_DATA(EP0_IN_SLOT) = *usb.ep0_tx_data++;
	usb.ep0_tx_left -= size;
	USB_EP_MAX_PACKET_LOW(0) = size;
	USB_EP_MAX_PACKET_HIGH(0) = 0;
	USB_EP_CONFIG(EP0_IN_SLOT) &= ~(USB_EP_CONFIG_ZLP | USB_EP_CONFIG_NAK);
	if (usb.ep0_tx_left != 0)
		return;

	if (size == EP0_SIZE && usb.ep0_tx_zlp) {
		usb.ep0_tx_zlp = false;
		return;
	}
	USB_EP_CONTROL(EP0_IN_SLOT) |= USB_EP_CONTROL_COMMIT;
	usb.ep0_tx_active = false;
}

static void usb_ep0_send(const void *data, size_t length, uint16_t requested) {
	usb.ep0_tx_data = data;
	usb.ep0_tx_left = MIN(length, requested);
	usb.ep0_tx_zlp = length < requested;
	usb.ep0_tx_active = true;
	usb_ep0_write_chunk();
}

static void usb_ep0_ack(void) {
	USB_EP_CONFIG(EP0_IN_SLOT) |= USB_EP_CONFIG_ZLP;
	USB_EP_CONFIG(EP0_IN_SLOT) &= ~(USB_EP_CONFIG_NAK | USB_EP_CONFIG_STALL);
}

static void usb_ep0_stall(void) {
	USB_EP_CONFIG(EP0_IN_SLOT) |= USB_EP_CONFIG_STALL;
}

static void usb_set_address(uint32_t address) {
	usb_ep0_ack();
	stopwatch_t start = stopwatch_get();
	while (!(USB_EP_CONFIG(EP0_IN_SLOT) & USB_EP_CONFIG_NAK)) {
		if (stopwatch_elapsed_ms(start) >= USB_STATUS_TIMEOUT_MS)
			break;
	}
	USB_DEVICE_ADDRESS = USB_DEVICE_ADDRESS_ENABLE | address;
}

static void usb_configure_in_endpoint(uint32_t endpoint) {
	uint32_t slot = endpoint + 1;
	USB_EP_CONFIG(slot) = (USB_EP_CONFIG(slot) & ~BIT(4)) | USB_EP_CONFIG_NAK;
	USB_EP_CONTROL(slot) |= USB_EP_CONTROL_FLUSH | BIT(2) | USB_EP_CONTROL_IN_ENABLE;
	USB_EP_CONTROL(slot) &= ~USB_EP_CONTROL_COMMIT;
	USB_EP_CONTROL(slot) |= USB_EP_CONTROL_ARM;
}

static void usb_configure_out_endpoint(uint32_t endpoint) {
	uint32_t slot = endpoint + 1;
	USB_EP_CONFIG(slot) |= BIT(4) | USB_EP_CONFIG_NAK;
	USB_EP_CONTROL(slot) |= USB_EP_CONTROL_OUT_ENABLE;
	USB_EP_CONTROL(slot) &= ~USB_EP_CONTROL_COMMIT;
	USB_EP_CONTROL(slot) |= USB_EP_CONTROL_ARM;
}

static void usb_set_configuration(void) {
	USB_EP_ENABLE_LOW |= BIT(CDC_DATA_IN_EP) | BIT(CDC_DATA_OUT_EP) | BIT(CDC_NOTIFY_EP);
	usb_endpoint_set_fifo(CDC_DATA_IN_EP, CDC_DATA_IN_FIFO_START, 2, CDC_DATA_SIZE);
	usb_endpoint_set_fifo(CDC_DATA_OUT_EP, CDC_DATA_OUT_FIFO_START, 2, CDC_DATA_SIZE);
	usb_endpoint_set_fifo(CDC_NOTIFY_EP, CDC_NOTIFY_FIFO_START, 1, CDC_NOTIFY_SIZE);
	usb_configure_in_endpoint(CDC_DATA_IN_EP);
	usb_configure_out_endpoint(CDC_DATA_OUT_EP);
	usb_configure_in_endpoint(CDC_NOTIFY_EP);
	usb.configured = true;
}

static void usb_send_string(uint32_t index, uint16_t requested) {
	if (index == USB_STRING_LANGID) {
		usb.string.bLength = 4;
		usb.string.wData[0] = USB_LANGID_EN_US;
	} else if (index < ARRAY_SIZE(STRINGS)) {
		size_t length = MIN(strlen(STRINGS[index]), USB_STRING_MAX_CHARS);
		for (size_t i = 0; i < length; i++)
			usb.string.wData[i] = STRINGS[index][i];
		usb.string.bLength = 2 + length * 2;
	} else {
		usb_ep0_stall();
		return;
	}
	usb.string.bDescriptorType = USB_DT_STRING;
	usb_ep0_send(&usb.string, usb.string.bLength, requested);
}

static void usb_handle_get_descriptor(const struct usb_setup *setup) {
	switch (setup->wValue >> 8) {
		case USB_DT_DEVICE:
			usb_ep0_send(&DEVICE_DESCRIPTOR, sizeof(DEVICE_DESCRIPTOR), setup->wLength);
			break;

		case USB_DT_CONFIGURATION:
			usb_ep0_send(&CONFIGURATION, sizeof(CONFIGURATION), setup->wLength);
			break;

		case USB_DT_STRING:
			usb_send_string(setup->wValue & 0xFF, setup->wLength);
			break;

		default:
			usb_ep0_stall();
			break;
	}
}

static void usb_handle_standard_request(const struct usb_setup *setup) {
	static const uint8_t ZERO_STATUS[2] = { 0, 0 };
	static const uint8_t CONFIGURATION_VALUES[2] = { 0, 1 };

	switch (setup->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			usb_handle_get_descriptor(setup);
			break;

		case USB_REQ_SET_ADDRESS:
			usb_set_address(setup->wValue & USB_DEVICE_ADDRESS_ADDRESS);
			break;

		case USB_REQ_SET_CONFIGURATION:
			if (setup->wValue == CONFIGURATION.configuration.bConfigurationValue) {
				usb_set_configuration();
			} else {
				usb.configured = false;
			}
			usb_ep0_ack();
			break;

		case USB_REQ_GET_CONFIGURATION:
			usb_ep0_send(&CONFIGURATION_VALUES[usb.configured], 1, setup->wLength);
			break;

		case USB_REQ_GET_STATUS:
		case USB_REQ_GET_INTERFACE:
			usb_ep0_send(ZERO_STATUS, setup->bRequest == USB_REQ_GET_STATUS ? 2 : 1, setup->wLength);
			break;

		case USB_REQ_CLEAR_FEATURE:
		case USB_REQ_SET_FEATURE:
		case USB_REQ_SET_INTERFACE:
			usb_ep0_ack();
			break;

		default:
			usb_ep0_stall();
			break;
	}
}

static void usb_handle_class_request(const struct usb_setup *setup) {
	switch (setup->bRequest) {
		case CDC_REQ_SET_LINE_CODING:
			usb.line_coding_pending = true;
			break;

		case CDC_REQ_GET_LINE_CODING:
			usb_ep0_send(&usb.line_coding, sizeof(usb.line_coding), setup->wLength);
			break;

		case CDC_REQ_SET_CONTROL_LINE_STATE:
			usb.dtr = (setup->wValue & CDC_CONTROL_LINE_DTR) != 0;
			usb_ep0_ack();
			break;

		case CDC_REQ_SEND_BREAK:
			usb_ep0_ack();
			break;

		default:
			usb_ep0_stall();
			break;
	}
}

static void usb_handle_setup(void) {
	uint8_t bytes[sizeof(struct usb_setup)];
	for (size_t index = 0; index < sizeof(bytes); index++)
		bytes[index] = USB_SETUP_PACKET(index);
	struct usb_setup setup;
	memcpy(&setup, bytes, sizeof(setup));

	USB_EP_CONTROL(EP0_OUT_SLOT) |= USB_EP_CONTROL_FLUSH;
	USB_EP_CONFIG(EP0_OUT_SLOT) &= ~(USB_EP_CONFIG_NAK | USB_EP_CONFIG_STALL);
	USB_EP_CONTROL(EP0_IN_SLOT) |= USB_EP_CONTROL_FLUSH;
	USB_EP_CONFIG(EP0_IN_SLOT) &= ~(USB_EP_CONFIG_ZLP | USB_EP_CONFIG_STALL);
	usb.ep0_tx_active = false;
	usb.line_coding_pending = false;

	switch (setup.bmRequestType & USB_TYPE_MASK) {
		case USB_TYPE_STANDARD:
			usb_handle_standard_request(&setup);
			break;

		case USB_TYPE_CLASS:
			usb_handle_class_request(&setup);
			break;

		default:
			usb_ep0_stall();
			break;
	}
}

static void usb_handle_ep0_out_data(void) {
	if (!usb.line_coding_pending || usb_slot_get_count(EP0_OUT_SLOT) < sizeof(usb.line_coding))
		return;

	uint8_t bytes[sizeof(usb.line_coding)];
	for (size_t index = 0; index < sizeof(bytes); index++)
		bytes[index] = USB_EP_DATA(EP0_OUT_SLOT);
	memcpy(&usb.line_coding, bytes, sizeof(bytes));
	usb.line_coding_pending = false;

	USB_EP_CONTROL(EP0_OUT_SLOT) |= USB_EP_CONTROL_FLUSH;
	USB_EP_CONFIG(EP0_OUT_SLOT) &= ~(USB_EP_CONFIG_NAK | USB_EP_CONFIG_STALL);
	usb_ep0_ack();
}

static void usb_handle_ep0_out(void) {
	uint32_t config = USB_EP_CONFIG(EP0_OUT_SLOT);
	if (!(config & BIT(7))) {
		usb_handle_ep0_out_data();
		return;
	}
	if (config & USB_EP_CONFIG_STALL) {
		USB_EP_CONFIG(EP0_OUT_SLOT) &= ~USB_EP_CONFIG_STALL;
		return;
	}
	usb_handle_setup();
}

static void cdc_drain_out(void) {
	uint32_t slot = CDC_DATA_OUT_EP + 1;
	uint32_t count = usb_slot_get_count(slot);
	for (uint32_t index = 0; index < count; index++)
		(void) USB_EP_DATA(slot);
	USB_EP_CONFIG(slot) &= ~(USB_EP_CONFIG_ZLP | USB_EP_CONFIG_NAK | USB_EP_CONFIG_STALL);
	USB_EP_CONTROL(slot) |= USB_EP_CONTROL_ARM;
}

static void usb_bus_reset(void) {
	usb.configured = false;
	usb.dtr = false;
	usb.ep0_tx_active = false;
	usb.line_coding_pending = false;
}

static void usb_poll(void) {
	uint32_t status = USB_GLOBAL_INT_STATUS;

	if (status & USB_GLOBAL_INT_STATUS_EP0_OUT) {
		USB_GLOBAL_INT_STATUS = USB_GLOBAL_INT_STATUS_EP0_OUT;
		USB_EVENT_INT_STATUS = USB_EVENT_INT_STATUS_SETUP;
		usb_handle_ep0_out();
	}

	if (status & USB_GLOBAL_INT_STATUS_EP0_IN) {
		USB_GLOBAL_INT_STATUS = USB_GLOBAL_INT_STATUS_EP0_IN;
		if (usb.ep0_tx_active)
			usb_ep0_write_chunk();
	}

	bool out_data = false;
	if (status & USB_GLOBAL_INT_STATUS_EVENT) {
		USB_GLOBAL_INT_STATUS = USB_GLOBAL_INT_STATUS_EVENT;
		uint32_t events = USB_EVENT_INT_STATUS;
		USB_EVENT_INT_STATUS = events;
		if (events & USB_EVENT_INT_STATUS_RESET)
			usb_bus_reset();
		if (events & USB_EVENT_INT_STATUS_EP_B_LOW) {
			uint32_t endpoints = USB_EP_B_INT_STATUS_LOW;
			USB_EP_B_INT_STATUS_LOW = endpoints;
			out_data |= (endpoints & BIT(CDC_DATA_OUT_EP)) != 0;
		}
	}

	if (status & USB_GLOBAL_INT_STATUS_EP_A_LOW) {
		USB_GLOBAL_INT_STATUS = USB_GLOBAL_INT_STATUS_EP_A_LOW;
		uint32_t endpoints = USB_EP_A_INT_STATUS_LOW;
		USB_EP_A_INT_STATUS_LOW = endpoints;
		out_data |= (endpoints & BIT(CDC_DATA_OUT_EP)) != 0;
	}

	if (out_data && usb.configured)
		cdc_drain_out();
}

static void usb_clk48m_enable(void) {
	cgu_pll_set(7, 1);
	cgu_phases_enable(CGU_PHASE4);
	cgu_phase_set_divider(4, 4, 2);
	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_CLK48M_CLKSEL) | CGU_CON2_CLK48M_CLKSEL_PHASE4;
#ifdef PMB8876
	CGU_CON3 = (CGU_CON3 & ~CGU_CON3_CLK48M_CLKDIV) | CGU_CON3_CLK48M_CLKDIV_DIV1;
#endif
}

#if defined(BOARD_HAS_PMIC_D1094XX)
static bool usb_transceiver_enable(void) {
	uint8_t supply = 0;
	i2c_init();
	if (!i2c_smbus_read_byte(D1094XX_I2C_ADDR, D1094XX_SUPPLY_ENABLE_2, &supply))
		return false;
	return i2c_smbus_write_byte(D1094XX_I2C_ADDR, D1094XX_SUPPLY_ENABLE_2, supply | D1094XX_SUPPLY_ENABLE_2_VREGUSB_EN);
}
#elif defined(BOARD_HAS_PMIC_PMB6812)
static bool usb_transceiver_enable(void) {
	i2c_init();
	return pmb681x_reg_write(PMB6812_USBCTRL, PMB6812_USBCTRL_LUSBMD_ON | PMB6812_USBCTRL_PULLUP_PULLUP);
}
#endif

static void usb_start(void) {
	SCU_RST_REQ |= SCU_RST_REQ_USB;
	SCU_RST_REQ &= ~SCU_RST_REQ_USB;
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;

	USB_CONTROL = 0;
	USB_CFG = USB_CFG_DRIVE_ENABLE;
	stopwatch_msleep_wd(USB_DETACH_MS);
	USB_CONTROL = USB_CONTROL_ENABLE;

	usb_endpoint_set_fifo(0, EP0_FIFO_START, 1, EP0_SIZE);
	USB_EP_ENABLE_LOW = BIT(0);
	USB_EP_ENABLE_HIGH = 0;
	USB_EP_CONFIG(EP0_OUT_SLOT) = 0;
	USB_EP_CONTROL(EP0_OUT_SLOT) |= USB_EP_CONTROL_ARM | BIT(2) | BIT(5);
	USB_EP_CONTROL(EP0_IN_SLOT) |= USB_EP_CONTROL_FLUSH | BIT(4);

	USB_GLOBAL_INT_ENABLE = USB_GLOBAL_INT_ENABLE_EP0_OUT |
		USB_GLOBAL_INT_ENABLE_EP0_IN |
		USB_GLOBAL_INT_ENABLE_EVENT |
		USB_GLOBAL_INT_ENABLE_EP_A_LOW;
	USB_EVENT_INT_ENABLE = USB_EVENT_INT_ENABLE_RESET | USB_EVENT_INT_ENABLE_SETUP | USB_EVENT_INT_ENABLE_EP_B_LOW;
	USB_EP_A_INT_ENABLE_LOW = BIT(CDC_DATA_OUT_EP);
	USB_EP_B_INT_ENABLE_LOW = BIT(CDC_DATA_OUT_EP);
	USB_GLOBAL_INT_STATUS = UINT8_MAX;
	USB_EVENT_INT_STATUS = UINT8_MAX;
	USB_DEVICE_ADDRESS = USB_DEVICE_ADDRESS_ENABLE;

#ifdef GPIO_SERIAL_EN
	GPIO_PIN(GPIO_SERIAL_EN) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_LOW;
#endif
	USB_CFG = USB_CFG_PHY_ENABLE | USB_CFG_DRIVE_ENABLE;
}

static bool cdc_wait_in_empty(void) {
	uint32_t slot = CDC_DATA_IN_EP + 1;
	stopwatch_t start = stopwatch_get();
	while (usb_slot_get_count(slot) != CDC_DATA_SIZE) {
		usb_poll();
		if (!usb.dtr || stopwatch_elapsed_ms(start) >= CDC_WRITE_TIMEOUT_MS)
			return false;
	}
	return true;
}

static void cdc_write(const char *data, size_t length) {
	uint32_t slot = CDC_DATA_IN_EP + 1;

	while (length != 0) {
		if (!cdc_wait_in_empty())
			return;

		size_t size = MIN(length, CDC_DATA_SIZE);
		for (size_t index = 0; index < size; index++)
			USB_EP_DATA(slot) = data[index];
		if (size < CDC_DATA_SIZE)
			USB_EP_CONTROL(slot) |= USB_EP_CONTROL_COMMIT;
		USB_EP_CONFIG(slot) &= ~(USB_EP_CONFIG_ZLP | USB_EP_CONFIG_NAK | USB_EP_CONFIG_STALL);
		data += size;
		length -= size;
	}
}

static void cdc_buffer_putc(char character, void *opaque) {
	struct cdc_tx_buffer *buffer = opaque;

	buffer->data[buffer->length++] = character;
	if (buffer->length == sizeof(buffer->data)) {
		cdc_write(buffer->data, buffer->length);
		buffer->length = 0;
	}
}

static void cdc_printf(const char *format, ...) {
	struct cdc_tx_buffer buffer = { .length = 0 };
	va_list args;

	va_start(args, format);
	vfctprintf(cdc_buffer_putc, &buffer, format, args);
	va_end(args);
	cdc_write(buffer.data, buffer.length);
}

int main(void) {
	wdt_init();
	usb_clk48m_enable();
	printf("USB CDC ACM: waiting up to %u s for a terminal\n", USB_CONNECT_TIMEOUT_MS / 1000);
	usb_start();
#if defined(BOARD_HAS_PMIC_D1094XX) || defined(BOARD_HAS_PMIC_PMB6812)
	if (!usb_transceiver_enable())
		printf("USB CDC ACM: failed to enable the USB transceiver supply\n");
#endif

	stopwatch_t start = stopwatch_get();
	stopwatch_t last_hello = start;
	uint32_t hello_count = 0;
	while (stopwatch_elapsed_ms(start) < USB_CONNECT_TIMEOUT_MS) {
		wdt_serve();
		usb_poll();
		if (!usb.dtr || stopwatch_elapsed_ms(last_hello) < HELLO_INTERVAL_MS)
			continue;

		last_hello = stopwatch_get();
		hello_count++;
		cdc_printf("Hello, world! #%lu (%lu baud)\r\n", hello_count, usb.line_coding.dwDTERate);
	}

	USB_CFG = 0;
	printf("USB CDC ACM: done, %lu hello lines sent\n", hello_count);
	return 0;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}
