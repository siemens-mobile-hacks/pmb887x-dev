#include <pmb887x.h>

#include "test.h"

static void pulse_usb_reset(void) {
	SCU_RST_REQ = SCU_RST_REQ_USB;
	uint32_t asserted_requests = SCU_RST_REQ;
	SCU_RST_REQ = 0;
	test_eq_u32("USB reset request asserts", SCU_RST_REQ_USB, asserted_requests);
}

static void test_reset_values(void) {
	test_eq_u32("clock control reset value", MOD_CLC_DISR | MOD_CLC_DISS, USB_CLC);
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("module clock is enabled", USB_CLC);
#ifdef PMB8875
	test_module_id("module ID", 0xF047C000, USB_ID);
#else
	test_module_id("module ID", 0xF047C012, USB_ID);
#endif
	pulse_usb_reset();
	test_eq_u32("wrapper configuration reset value", 0, USB_CFG);
#ifdef PMB8876
	test_eq_u32("endpoint enable low reset value", 0, USB_EP_ENABLE_LOW);
	test_eq_u32("endpoint enable high reset value", 0, USB_EP_ENABLE_HIGH);
	test_eq_u32("device address reset value", 0, USB_DEVICE_ADDRESS);
	test_eq_u32("frame number low reset value", 0, USB_FRAME_NUMBER_LOW);
	test_eq_u32("frame number high reset value", 0, USB_FRAME_NUMBER_HIGH);
	test_eq_u32("core control reset value", USB_CONTROL_ENABLE, USB_CONTROL);
	for (uint32_t index = 0; index < 8; index++)
		test_eq_u32("setup packet byte reset value", 0, USB_SETUP_PACKET(index));
	test_eq_u32("endpoint 0 status reset value", 0x24, USB_EP0_STATUS);
	test_eq_u32("global interrupt status reset value", 0, USB_GLOBAL_INT_STATUS);
	test_eq_u32("global interrupt enable reset value", 0, USB_GLOBAL_INT_ENABLE);
	test_eq_u32("DMA interrupt group 0 status reset value", 0, USB_DMA0_INT_STATUS);
	test_eq_u32("DMA interrupt group 0 enable reset value", 0, USB_DMA0_INT_ENABLE);
	test_eq_u32("DMA interrupt group 1 status reset value", 0, USB_DMA1_INT_STATUS);
	test_eq_u32("DMA interrupt group 1 enable reset value", 0, USB_DMA1_INT_ENABLE);
	test_eq_u32("USB event interrupt status reset value", BIT(3), USB_EVENT_INT_STATUS);
	test_eq_u32("USB event interrupt enable reset value", 0, USB_EVENT_INT_ENABLE);
	test_eq_u32("endpoint group A low status reset value", 0, USB_EP_A_INT_STATUS_LOW);
	test_eq_u32("endpoint group A low enable reset value", 0, USB_EP_A_INT_ENABLE_LOW);
	test_eq_u32("endpoint group A high status reset value", 0, USB_EP_A_INT_STATUS_HIGH);
	test_eq_u32("endpoint group A high enable reset value", 0, USB_EP_A_INT_ENABLE_HIGH);
	test_eq_u32("endpoint group B low status reset value", 0, USB_EP_B_INT_STATUS_LOW);
	test_eq_u32("endpoint group B low enable reset value", 0, USB_EP_B_INT_ENABLE_LOW);
	test_eq_u32("endpoint group B high status reset value", 0, USB_EP_B_INT_STATUS_HIGH);
	test_eq_u32("endpoint group B high enable reset value", 0, USB_EP_B_INT_ENABLE_HIGH);
	test_eq_u32("PHY control reset value", 0, USB_PHY_CONTROL);

	static const uint8_t ENDPOINT_CONFIG_RESET[] = {
		0x2C, 0x28, 0x20, 0x28, 0x20, 0x28, 0x20, 0x28, 0x20, 0x28, 0x20,
	};
	static const uint8_t ENDPOINT_COUNT_LOW_RESET[] = {
		0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	static const uint8_t ENDPOINT_COUNT_HIGH_RESET[] = {
		0x50, 0x50, 0xA0, 0x50, 0xA0, 0x50, 0xA0, 0x50, 0xA0, 0x50, 0xA0,
	};
	for (uint32_t endpoint = 0; endpoint < 11; endpoint++) {
		test_eq_u32("endpoint configuration reset value", ENDPOINT_CONFIG_RESET[endpoint], USB_EP_CONFIG(endpoint));
		test_eq_u32("endpoint FIFO control reset value", 0, USB_EP_CONTROL(endpoint));
		test_eq_u32("endpoint count low reset value", ENDPOINT_COUNT_LOW_RESET[endpoint], USB_EP_COUNT_LOW(endpoint));
		test_eq_u32(
			"endpoint count high reset value",
			ENDPOINT_COUNT_HIGH_RESET[endpoint],
			USB_EP_COUNT_HIGH(endpoint)
		);
	}
#endif
}

static void test_wrapper_reset(void) {
	USB_CONTROL = 0;
	USB_CFG = 3;
	pulse_usb_reset();
	test_eq_u32("wrapper returns to reset value", 0, USB_CFG);
	test_eq_u32("core control returns to reset value", USB_CONTROL_ENABLE, USB_CONTROL);
}

static void test_interrupt_enable_reset(void) {
	USB_GLOBAL_INT_ENABLE = 0xFE;
	USB_DMA0_INT_ENABLE = UINT8_MAX;
	USB_DMA1_INT_ENABLE = UINT8_MAX;
	USB_EVENT_INT_ENABLE = UINT8_MAX;
	USB_EP_A_INT_ENABLE_LOW = UINT8_MAX;
	USB_EP_A_INT_ENABLE_HIGH = 7;
	USB_EP_B_INT_ENABLE_LOW = UINT8_MAX;
	USB_EP_B_INT_ENABLE_HIGH = 7;
	pulse_usb_reset();
	test_eq_u32("global interrupt enable returns to reset value", 0, USB_GLOBAL_INT_ENABLE);
	test_eq_u32("DMA interrupt group 0 enable returns to reset value", 0, USB_DMA0_INT_ENABLE);
	test_eq_u32("DMA interrupt group 1 enable returns to reset value", 0, USB_DMA1_INT_ENABLE);
	test_eq_u32("USB event interrupt enable returns to reset value", 0, USB_EVENT_INT_ENABLE);
	test_eq_u32("endpoint group A low enable returns to reset value", 0, USB_EP_A_INT_ENABLE_LOW);
	test_eq_u32("endpoint group A high enable returns to reset value", 0, USB_EP_A_INT_ENABLE_HIGH);
	test_eq_u32("endpoint group B low enable returns to reset value", 0, USB_EP_B_INT_ENABLE_LOW);
	test_eq_u32("endpoint group B high enable returns to reset value", 0, USB_EP_B_INT_ENABLE_HIGH);
}

static void test_register_layout(void) {
	test_eq_u32("endpoint low bitmap occupies bits 7:0", UINT8_MAX, USB_EP_ENABLE_LOW_ENDPOINTS);
	test_eq_u32("endpoint high bitmap occupies bits 2:0", GENMASK(2, 0), USB_EP_ENABLE_HIGH_ENDPOINTS);
	test_eq_u32("device address occupies bits 6:0", GENMASK(6, 0), USB_DEVICE_ADDRESS_ADDRESS);
	test_eq_u32("device address activation is bit 7", BIT(7), USB_DEVICE_ADDRESS_ENABLE);
	test_eq_u32("frame number high part occupies bits 2:0", GENMASK(2, 0), USB_FRAME_NUMBER_HIGH_VALUE);
	test_eq_u32("core enable is bit 0", BIT(0), USB_CONTROL_ENABLE);
	test_eq_u32("global interrupt bitmap occupies bits 7:0", UINT8_MAX, USB_GLOBAL_INT_ENABLE_SOURCES);
	test_eq_u32("DMA interrupt group 0 occupies bits 7:0", UINT8_MAX, USB_DMA0_INT_ENABLE_SOURCES);
	test_eq_u32("DMA interrupt group 1 occupies bits 1:0", GENMASK(1, 0), USB_DMA1_INT_ENABLE_SOURCES);
	test_eq_u32("USB event bitmap occupies bits 7:0", UINT8_MAX, USB_EVENT_INT_ENABLE_EVENTS);
	test_eq_u32("endpoint low interrupt bitmap occupies bits 7:0", UINT8_MAX, USB_EP_A_INT_ENABLE_LOW_ENDPOINTS);
	test_eq_u32("endpoint high interrupt bitmap occupies bits 2:0", GENMASK(2, 0), USB_EP_A_INT_ENABLE_HIGH_ENDPOINTS);
}

int main(void) {
	test_start("USB peripheral test");

	test_category("Reset values");
	test_reset_values();
	test_category("Wrapper reset");
	test_wrapper_reset();
	test_category("Interrupt enable reset");
	test_interrupt_enable_reset();
	test_category("Register layout");
	test_register_layout();

	return test_finish();
}
