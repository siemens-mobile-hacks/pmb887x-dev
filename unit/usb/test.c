#include <pmb887x.h>
#include <sio.h>

#include "cgu/clock.h"
#include "test.h"

#ifdef BOARD_HAS_PMIC_PMB6812
#include <gen/peripheral/pmic/PMB6812.h>
#include <pmb681x.h>
#endif

#define USB_ENDPOINT_COUNT 11
#define USB_SLOT_COUNT 12
#define EP0_OUT_SLOT 0
#define EP0_IN_SLOT 1
#define EP1_IN_SLOT 2
#define EP2_OUT_SLOT 3
#define EP1_FIFO_BYTES 64
#define USB_EVENT_TIMEOUT_MS 50

static void pulse_usb_reset(void) {
	SCU_RST_REQ |= SCU_RST_REQ_USB;
	SCU_RST_REQ &= ~SCU_RST_REQ_USB;
}

static void usb_clear_events(void) {
	USB_EVENT_INT_STATUS = UINT8_MAX;
	USB_GLOBAL_INT_STATUS = UINT8_MAX;
	USB_EP_READY_INT_STATUS_LOW = UINT8_MAX;
	USB_EP_READY_INT_STATUS_HIGH = UINT8_MAX;
	USB_EP_DONE_INT_STATUS_LOW = UINT8_MAX;
	USB_EP_DONE_INT_STATUS_HIGH = UINT8_MAX;
}

static bool usb_wait_event(uint32_t event, uint32_t timeout_ms) {
	stopwatch_t start = stopwatch_get();
	while ((USB_EVENT_INT_STATUS & event) == 0) {
		if (stopwatch_elapsed_ms(start) >= timeout_ms)
			return false;
	}
	return true;
}

static void usb_start(void) {
	pulse_usb_reset();
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;
	usb_wait_event(USB_EVENT_INT_STATUS_RESET, 1);
	usb_clear_events();
}

static void usb_enable_phy(void) {
	USB_CFG = USB_CFG_PHY_ENABLE;
	stopwatch_usleep_wd(200);
}

static void usb_bus_reset(void) {
	usb_enable_phy();
	USB_EVENT_INT_STATUS = UINT8_MAX;
	USB_CFG = 0;
}

static bool usb_irq_pending(void) {
	return (VIC_IRQ_CON & VIC_IRQ_CON_NUM) == VIC_USB_IRQ;
}

static void ep1_configure_fifo(void) {
	USB_EP_FIFO_START(1) = 0;
	USB_EP_FIFO_SIZE(1) = 2 << USB_EP_FIFO_SIZE_SIZE_SHIFT;
	USB_EP_CONTROL(EP1_IN_SLOT) = USB_EP_CONTROL_IN_ENABLE;
}

static void ep1_write(uint32_t count) {
	for (uint32_t index = 0; index < count; index++)
		USB_EP_DATA(EP1_IN_SLOT) = index;
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
#ifdef PMB8875
	test_eq_u32("software reset disables the module clock", MOD_CLC_DISR | MOD_CLC_DISS, USB_CLC);
#else
	test_eq_u32("software reset keeps the module clock", 1 << MOD_CLC_RMC_SHIFT, USB_CLC);
#endif
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("wrapper configuration reset value", 0, USB_CFG);
	test_eq_u32("endpoint enable low reset value", 0, USB_EP_ENABLE_LOW);
	test_eq_u32("endpoint enable high reset value", 0, USB_EP_ENABLE_HIGH);
	test_eq_u32("device address reset value", 0, USB_DEVICE_ADDRESS);
	test_eq_u32("frame number high reset value", 0, USB_FRAME_NUMBER_HIGH);
	test_eq_u32("frame number low reset value", 0, USB_FRAME_NUMBER_LOW);
	test_eq_u32("core control reset value", USB_CONTROL_ENABLE, USB_CONTROL);
	for (uint32_t index = 0; index < 8; index++)
		test_eq_u32("setup packet byte reset value", 0, USB_SETUP_PACKET(index));
	test_eq_u32("global interrupt status reset value", 0, USB_GLOBAL_INT_STATUS);
	test_eq_u32("global interrupt enable reset value", 0, USB_GLOBAL_INT_ENABLE);
	test_eq_u32("DMA interrupt group 0 status reset value", 0, USB_DMA0_INT_STATUS);
	test_eq_u32("DMA interrupt group 0 enable reset value", 0, USB_DMA0_INT_ENABLE);
	test_eq_u32("DMA interrupt group 1 status reset value", 0, USB_DMA1_INT_STATUS);
	test_eq_u32("DMA interrupt group 1 enable reset value", 0, USB_DMA1_INT_ENABLE);
	test_eq_u32("USB event interrupt status reset value", USB_EVENT_INT_STATUS_RESET, USB_EVENT_INT_STATUS);
	test_eq_u32("USB event interrupt enable reset value", 0, USB_EVENT_INT_ENABLE);
	test_eq_u32("endpoint FIFO ready low status reset value", 0, USB_EP_READY_INT_STATUS_LOW);
	test_eq_u32("endpoint FIFO ready low enable reset value", 0, USB_EP_READY_INT_ENABLE_LOW);
	test_eq_u32("endpoint FIFO ready high status reset value", 0, USB_EP_READY_INT_STATUS_HIGH);
	test_eq_u32("endpoint FIFO ready high enable reset value", 0, USB_EP_READY_INT_ENABLE_HIGH);
	test_eq_u32("endpoint transfer done low status reset value", 0, USB_EP_DONE_INT_STATUS_LOW);
	test_eq_u32("endpoint transfer done low enable reset value", 0, USB_EP_DONE_INT_ENABLE_LOW);
	test_eq_u32("endpoint transfer done high status reset value", 0, USB_EP_DONE_INT_STATUS_HIGH);
	test_eq_u32("endpoint transfer done high enable reset value", 0, USB_EP_DONE_INT_ENABLE_HIGH);
	test_eq_u32("PHY control reset value", 0, USB_PHY_CONTROL);
	test_eq_u32("frame number reset value", 0, USB_FRAME_NUMBER);
	test_eq_u32(
		"packed global and event status reset value",
		USB_EVENT_INT_STATUS_RESET << USB_GLOBAL_EVENT_INT_STATUS_EVENT_SHIFT,
		USB_GLOBAL_EVENT_INT_STATUS
	);
	test_eq_u32("packed DMA and FIFO ready status reset value", 0, USB_DMA_EP_READY_INT_STATUS);
	test_eq_u32("packed global, event and transfer done enable reset value", 0, USB_GLOBAL_EVENT_EP_DONE_INT_ENABLE);
	test_eq_u32("packed DMA and FIFO ready enable reset value", 0, USB_DMA_EP_READY_INT_ENABLE);
	test_eq_u32("packed EP0 count reset value", 0x50205000, USB_EP0_COUNT);

	static const uint8_t SLOT_CONFIG_RESET[USB_SLOT_COUNT] = {
		0x24, 0x2C, 0x28, 0x20, 0x28, 0x20, 0x28, 0x20, 0x28, 0x20, 0x28, 0x20,
	};
	static const uint8_t SLOT_COUNT_LOW_RESET[USB_SLOT_COUNT] = {
		0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	static const uint8_t SLOT_COUNT_HIGH_RESET[USB_SLOT_COUNT] = {
		0x50, 0x50, 0xA0, 0x50, 0xA0, 0x50, 0xA0, 0x50, 0xA0, 0x50, 0xA0, 0x50,
	};
	for (uint32_t slot = 0; slot < USB_SLOT_COUNT; slot++) {
		test_eq_u32("endpoint configuration reset value", SLOT_CONFIG_RESET[slot], USB_EP_CONFIG(slot));
		test_eq_u32("endpoint FIFO control reset value", 0, USB_EP_CONTROL(slot));
		test_eq_u32("endpoint count low reset value", SLOT_COUNT_LOW_RESET[slot], USB_EP_COUNT_LOW(slot));
		test_eq_u32("endpoint count high reset value", SLOT_COUNT_HIGH_RESET[slot], USB_EP_COUNT_HIGH(slot));
	}
	for (uint32_t endpoint = 1; endpoint < USB_ENDPOINT_COUNT; endpoint++) {
		uint32_t slot = endpoint + 1;
		test_eq_u32(
			"packed endpoint count reset value",
			(SLOT_COUNT_HIGH_RESET[slot] << 8) | SLOT_COUNT_LOW_RESET[slot],
			USB_EP_COUNT(endpoint - 1)
		);
	}
	for (uint32_t endpoint = 0; endpoint < USB_ENDPOINT_COUNT; endpoint++) {
		test_eq_u32("maximum packet size high reset value", 0, USB_EP_MAX_PACKET_HIGH(endpoint));
		test_eq_u32("maximum packet size low reset value", 0, USB_EP_MAX_PACKET_LOW(endpoint));
		test_eq_u32("FIFO start reset value", 0, USB_EP_FIFO_START(endpoint));
		test_eq_u32(
			"FIFO size reset value",
			endpoint == 0 ? 1 << USB_EP_FIFO_SIZE_SIZE_SHIFT : 0,
			USB_EP_FIFO_SIZE(endpoint)
		);
	}
}

static void test_wrapper_reset(void) {
	USB_CONTROL = 0;
	USB_CFG = USB_CFG_PHY_ENABLE | USB_CFG_DRIVE_ENABLE;
	pulse_usb_reset();
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("wrapper returns to reset value", 0, USB_CFG);
	test_eq_u32("core control returns to reset value", USB_CONTROL_ENABLE, USB_CONTROL);
}

static void test_interrupt_enable_reset(void) {
	USB_GLOBAL_INT_ENABLE = 0xFE;
	USB_DMA0_INT_ENABLE = UINT8_MAX;
	USB_DMA1_INT_ENABLE = UINT8_MAX;
	USB_EVENT_INT_ENABLE = UINT8_MAX;
	USB_EP_READY_INT_ENABLE_LOW = UINT8_MAX;
	USB_EP_READY_INT_ENABLE_HIGH = 7;
	USB_EP_DONE_INT_ENABLE_LOW = UINT8_MAX;
	USB_EP_DONE_INT_ENABLE_HIGH = 7;
	pulse_usb_reset();
	USB_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("global interrupt enable returns to reset value", 0, USB_GLOBAL_INT_ENABLE);
	test_eq_u32("DMA interrupt group 0 enable returns to reset value", 0, USB_DMA0_INT_ENABLE);
	test_eq_u32("DMA interrupt group 1 enable returns to reset value", 0, USB_DMA1_INT_ENABLE);
	test_eq_u32("USB event interrupt enable returns to reset value", 0, USB_EVENT_INT_ENABLE);
	test_eq_u32("endpoint FIFO ready low enable returns to reset value", 0, USB_EP_READY_INT_ENABLE_LOW);
	test_eq_u32("endpoint FIFO ready high enable returns to reset value", 0, USB_EP_READY_INT_ENABLE_HIGH);
	test_eq_u32("endpoint transfer done low enable returns to reset value", 0, USB_EP_DONE_INT_ENABLE_LOW);
	test_eq_u32("endpoint transfer done high enable returns to reset value", 0, USB_EP_DONE_INT_ENABLE_HIGH);
}

static void test_packed_registers(void) {
	usb_start();
	USB_GLOBAL_EVENT_EP_DONE_INT_ENABLE = 0x06884411;
	USB_DMA_EP_READY_INT_ENABLE = 0x05220199;
	test_eq_u32("packed enable writes global enable", 0x11, USB_GLOBAL_INT_ENABLE);
	test_eq_u32("packed enable writes event enable", 0x44, USB_EVENT_INT_ENABLE);
	test_eq_u32("packed enable writes transfer done low enable", 0x88, USB_EP_DONE_INT_ENABLE_LOW);
	test_eq_u32("packed enable writes transfer done high enable", 0x06, USB_EP_DONE_INT_ENABLE_HIGH);
	test_eq_u32("packed enable writes DMA group 0 enable", 0x99, USB_DMA0_INT_ENABLE);
	test_eq_u32("packed enable writes DMA group 1 enable", 0x01, USB_DMA1_INT_ENABLE);
	test_eq_u32("packed enable writes FIFO ready low enable", 0x22, USB_EP_READY_INT_ENABLE_LOW);
	test_eq_u32("packed enable writes FIFO ready high enable", 0x05, USB_EP_READY_INT_ENABLE_HIGH);

	USB_GLOBAL_INT_ENABLE = 0x24;
	USB_EVENT_INT_ENABLE = 0x81;
	USB_EP_DONE_INT_ENABLE_LOW = 0x3C;
	USB_EP_DONE_INT_ENABLE_HIGH = 0x01;
	USB_DMA0_INT_ENABLE = 0x42;
	USB_DMA1_INT_ENABLE = 0x02;
	USB_EP_READY_INT_ENABLE_LOW = 0xA5;
	USB_EP_READY_INT_ENABLE_HIGH = 0x02;
	test_eq_u32("packed enable reads global, event and transfer done", 0x013C8124, USB_GLOBAL_EVENT_EP_DONE_INT_ENABLE);
	test_eq_u32("packed enable reads DMA and FIFO ready", 0x02A50242, USB_DMA_EP_READY_INT_ENABLE);
}

static void test_packed_status(void) {
	usb_start();
	USB_EVENT_INT_ENABLE = USB_EVENT_INT_ENABLE_RESET;
	usb_bus_reset();
	usb_wait_event(USB_EVENT_INT_STATUS_RESET, USB_EVENT_TIMEOUT_MS);
	uint32_t packed_event = USB_EVENT_INT_STATUS_RESET << USB_GLOBAL_EVENT_INT_STATUS_EVENT_SHIFT;
	test_eq_u32(
		"packed status reads global and event status",
		packed_event | USB_GLOBAL_INT_STATUS_EVENT,
		USB_GLOBAL_EVENT_INT_STATUS
	);
	USB_GLOBAL_EVENT_INT_STATUS = USB_GLOBAL_INT_STATUS_EVENT;
	test_eq_u32("packed status write clears global status", 0, USB_GLOBAL_INT_STATUS);
	test_eq_u32("packed status write keeps event status", USB_EVENT_INT_STATUS_RESET, USB_EVENT_INT_STATUS);
	USB_GLOBAL_EVENT_INT_STATUS = packed_event;
	test_eq_u32("packed status write clears event status", 0, USB_EVENT_INT_STATUS);
}

static void test_fifo_size(void) {
	static const uint16_t FREE_BYTES[] = { 0, 32, 64, 128, 256, 512, 1024, 2047 };
	for (uint32_t size = 0; size < ARRAY_SIZE(FREE_BYTES); size++) {
		usb_start();
		USB_EP_FIFO_SIZE(1) = size << USB_EP_FIFO_SIZE_SIZE_SHIFT;
		uint32_t free_bytes = USB_EP_COUNT_LOW(EP1_IN_SLOT) |
			((USB_EP_COUNT_HIGH(EP1_IN_SLOT) & USB_EP_COUNT_HIGH_VALUE) << 8);
		printf("# FIFO size code %lu: %lu free bytes\n", size, free_bytes);
		test_eq_u32("IN FIFO free space follows the size code", FREE_BYTES[size], free_bytes);
	}
	usb_start();
	USB_EP_FIFO_SIZE(1) = 0;
	test_eq_u32(
		"IN endpoint without FIFO reports full",
		USB_EP_COUNT_HIGH_FULL | USB_EP_COUNT_HIGH_HIGH_LEVEL,
		USB_EP_COUNT_HIGH(EP1_IN_SLOT)
	);
}

static void test_fifo_levels(void) {
	static const struct {
		uint32_t written;
		uint32_t count_high;
	} LEVELS[] = {
		{ 0, USB_EP_COUNT_HIGH_EMPTY | USB_EP_COUNT_HIGH_LOW_LEVEL },
		{ 1, USB_EP_COUNT_HIGH_LOW_LEVEL },
		{ 16, USB_EP_COUNT_HIGH_LOW_LEVEL },
		{ 17, 0 },
		{ 47, 0 },
		{ 48, USB_EP_COUNT_HIGH_HIGH_LEVEL },
		{ 63, USB_EP_COUNT_HIGH_HIGH_LEVEL },
		{ 64, USB_EP_COUNT_HIGH_FULL | USB_EP_COUNT_HIGH_HIGH_LEVEL },
		{ 65, USB_EP_COUNT_HIGH_FULL | USB_EP_COUNT_HIGH_HIGH_LEVEL },
	};
	usb_start();
	ep1_configure_fifo();
	uint32_t written = 0;
	for (uint32_t index = 0; index < ARRAY_SIZE(LEVELS); index++) {
		ep1_write(LEVELS[index].written - written);
		written = LEVELS[index].written;
		uint32_t free_bytes = written < EP1_FIFO_BYTES ? EP1_FIFO_BYTES - written : 0;
		uint32_t count_high = USB_EP_COUNT_HIGH(EP1_IN_SLOT);
		uint32_t count_low = USB_EP_COUNT_LOW(EP1_IN_SLOT);
		printf("# %lu bytes written: free %lu, count high %02lX\n", written, count_low, count_high);
		test_check(
			"IN FIFO count and level flags follow the fill level",
			count_low == free_bytes && count_high == LEVELS[index].count_high
		);
		test_eq_u32("packed count mirrors the FIFO count", (count_high << 8) | count_low, USB_EP_COUNT(0));
	}
	test_eq_u32("IN FIFO data reads as 0xFF", 0xFF, USB_EP_DATA(EP1_IN_SLOT));
	test_eq_u32("reading IN FIFO data keeps the count", 0, USB_EP_COUNT_LOW(EP1_IN_SLOT));

	USB_EP_CONTROL(EP1_IN_SLOT) |= USB_EP_CONTROL_FLUSH;
	test_eq_u32("flush empties the IN FIFO", EP1_FIFO_BYTES, USB_EP_COUNT_LOW(EP1_IN_SLOT));
	test_eq_u32(
		"flush restores the empty flags",
		USB_EP_COUNT_HIGH_EMPTY | USB_EP_COUNT_HIGH_LOW_LEVEL,
		USB_EP_COUNT_HIGH(EP1_IN_SLOT)
	);
	test_eq_u32("flush bit clears itself", USB_EP_CONTROL_IN_ENABLE, USB_EP_CONTROL(EP1_IN_SLOT));
	USB_EP_CONTROL(EP1_IN_SLOT) |= USB_EP_CONTROL_ARM;
	test_eq_u32("arm bit clears itself", USB_EP_CONTROL_IN_ENABLE, USB_EP_CONTROL(EP1_IN_SLOT));

	usb_start();
	for (uint32_t index = 0; index < 3; index++)
		USB_EP_DATA(EP0_IN_SLOT) = index;
	test_eq_u32("EP0 IN FIFO counts free bytes", 32 - 3, USB_EP_COUNT_LOW(EP0_IN_SLOT));
	test_eq_u32("packed EP0 count mirrors EP0 IN", 0x101D5000, USB_EP0_COUNT);
	USB_EP_CONTROL(EP0_IN_SLOT) |= USB_EP_CONTROL_FLUSH;
	test_eq_u32("flush empties the EP0 IN FIFO", 32, USB_EP_COUNT_LOW(EP0_IN_SLOT));

	test_eq_u32("empty EP0 OUT FIFO data reads as 0xFF", 0xFF, USB_EP_DATA(EP0_OUT_SLOT));
	test_eq_u32("reading empty EP0 OUT FIFO keeps the count", 0, USB_EP_COUNT_LOW(EP0_OUT_SLOT));
}

static void test_out_fifo_arming(void) {
	for (uint32_t enabled = 0; enabled < 2; enabled++) {
		usb_start();
		USB_EP_FIFO_START(2) = 0;
		USB_EP_FIFO_SIZE(2) = 2 << USB_EP_FIFO_SIZE_SIZE_SHIFT;
		USB_EP_MAX_PACKET_LOW(2) = EP1_FIFO_BYTES;
		USB_EP_ENABLE_LOW = enabled ? BIT(2) : 0;
		USB_EP_CONTROL(EP2_OUT_SLOT) = USB_EP_CONTROL_OUT_ENABLE;
		USB_EP_CONTROL(EP2_OUT_SLOT) |= USB_EP_CONTROL_ARM;
		test_eq_u32("arming an OUT FIFO clears the arm bit", USB_EP_CONTROL_OUT_ENABLE, USB_EP_CONTROL(EP2_OUT_SLOT));
		test_eq_u32(
			enabled ? "armed enabled OUT endpoint sets the armed flag" : "disabled OUT endpoint stays unarmed",
			USB_EP_COUNT_HIGH_EMPTY | USB_EP_COUNT_HIGH_LOW_LEVEL | (enabled ? USB_EP_COUNT_HIGH_ARMED : 0),
			USB_EP_COUNT_HIGH(EP2_OUT_SLOT)
		);
	}
}

static void test_endpoint_interrupts(void) {
	static const struct {
		const char *name;
		bool in_enable;
		uint32_t data;
		uint32_t status;
	} FLUSHES[] = {
		{ "flushing an empty IN FIFO raises no FIFO ready", true, 0, 0 },
		{ "flushing a disabled IN data path raises no FIFO ready", false, 1, 0 },
		{ "flushing queued IN data raises a FIFO ready", true, 1, BIT(1) },
	};
	for (uint32_t index = 0; index < ARRAY_SIZE(FLUSHES); index++) {
		usb_start();
		USB_EP_READY_INT_ENABLE_LOW = UINT8_MAX;
		USB_EP_FIFO_SIZE(1) = 2 << USB_EP_FIFO_SIZE_SIZE_SHIFT;
		USB_EP_CONTROL(EP1_IN_SLOT) = FLUSHES[index].in_enable ? USB_EP_CONTROL_IN_ENABLE : 0;
		ep1_write(FLUSHES[index].data);
		USB_EP_CONTROL(EP1_IN_SLOT) |= USB_EP_CONTROL_FLUSH;
		test_eq_u32(FLUSHES[index].name, FLUSHES[index].status, USB_EP_READY_INT_STATUS_LOW);
	}
	test_eq_u32(
		"enabled FIFO ready latches the global summary",
		USB_GLOBAL_INT_STATUS_EP_READY_LOW,
		USB_GLOBAL_INT_STATUS
	);
	test_eq_u32(
		"packed status reports FIFO ready of endpoints 0-7",
		BIT(1) << USB_DMA_EP_READY_INT_STATUS_EP_READY_LOW_SHIFT,
		USB_DMA_EP_READY_INT_STATUS
	);
	USB_EP_READY_INT_STATUS_LOW = BIT(1);
	test_eq_u32("FIFO ready status is write-one-to-clear", 0, USB_EP_READY_INT_STATUS_LOW);

	usb_start();
	ep1_configure_fifo();
	ep1_write(1);
	USB_EP_CONTROL(EP1_IN_SLOT) |= USB_EP_CONTROL_FLUSH;
	test_eq_u32("FIFO ready status latches without its enable", BIT(1), USB_EP_READY_INT_STATUS_LOW);
	test_eq_u32("disabled FIFO ready leaves the global summary clear", 0, USB_GLOBAL_INT_STATUS);
}

static void test_interrupt_summary(void) {
	cpu_enable_irq(false);
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_USB_IRQ) = 1;

	usb_start();
	USB_EVENT_INT_ENABLE = USB_EVENT_INT_ENABLE_RESET;
	usb_bus_reset();
	usb_wait_event(USB_EVENT_INT_STATUS_RESET, USB_EVENT_TIMEOUT_MS);
	test_eq_u32("enabled event latches the global summary", USB_GLOBAL_INT_STATUS_EVENT, USB_GLOBAL_INT_STATUS);
	test_check("global enable gates the IRQ line", !usb_irq_pending());
	USB_GLOBAL_INT_ENABLE = USB_GLOBAL_INT_ENABLE_EVENT;
	test_check("latched global summary raises the IRQ line", usb_irq_pending());
	USB_EVENT_INT_ENABLE = 0;
	test_check("event enable does not gate a latched summary", usb_irq_pending());
	USB_GLOBAL_INT_STATUS = USB_GLOBAL_INT_STATUS_EVENT;
	test_eq_u32("global summary clears while the event is pending", 0, USB_GLOBAL_INT_STATUS);
	test_check("clearing the global summary drops the IRQ line", !usb_irq_pending());
	USB_EVENT_INT_ENABLE = USB_EVENT_INT_ENABLE_RESET;
	test_eq_u32("enabling a pending event does not latch the summary", 0, USB_GLOBAL_INT_STATUS);

	usb_start();
	usb_bus_reset();
	usb_wait_event(USB_EVENT_INT_STATUS_RESET, USB_EVENT_TIMEOUT_MS);
	test_eq_u32("event status latches without its enable", USB_EVENT_INT_STATUS_RESET, USB_EVENT_INT_STATUS);
	test_eq_u32("disabled event leaves the global summary clear", 0, USB_GLOBAL_INT_STATUS);
	USB_EVENT_INT_STATUS = USB_EVENT_INT_STATUS_RESET;
	test_eq_u32("event status is write-one-to-clear", 0, USB_EVENT_INT_STATUS);

	VIC_CON(VIC_USB_IRQ) = 0;
}

static void test_bus_reset(void) {
	usb_start();
	usb_enable_phy();
	test_eq_u32(
		"enabled transceiver sees the idle J state",
		USB_PHY_CONTROL_LINE_STATE_J,
		USB_PHY_CONTROL & USB_PHY_CONTROL_LINE_STATE
	);
	USB_DEVICE_ADDRESS = USB_DEVICE_ADDRESS_ENABLE | 5;
	USB_EP_ENABLE_LOW = 0x07;
	USB_EP_ENABLE_HIGH = 0x05;
	USB_EP_CONFIG(EP0_OUT_SLOT) = USB_EP_CONFIG_STALL;
	USB_EP_CONFIG(EP1_IN_SLOT) = 0x48;
	USB_EP_MAX_PACKET_LOW(0) = 0x20;
	ep1_configure_fifo();
	ep1_write(2);
	USB_EVENT_INT_STATUS = UINT8_MAX;
	USB_CFG = 0;
	test_check("disabling the transceiver signals a bus reset", usb_wait_event(USB_EVENT_INT_STATUS_RESET, 1));
	test_eq_u32(
		"disabled transceiver sees SE0",
		USB_PHY_CONTROL_LINE_STATE_SE0,
		USB_PHY_CONTROL & USB_PHY_CONTROL_LINE_STATE
	);
	test_eq_u32("bus reset clears the device address", USB_DEVICE_ADDRESS_ENABLE, USB_DEVICE_ADDRESS);
	test_eq_u32("bus reset keeps only EP0 enabled", 1, USB_EP_ENABLE_LOW);
	test_eq_u32("bus reset disables endpoints 8-10", 0, USB_EP_ENABLE_HIGH);
	test_eq_u32(
		"bus reset keeps EP0 OUT configuration",
		USB_EP_CONFIG_STALL | USB_EP_CONFIG_CONTROL,
		USB_EP_CONFIG(EP0_OUT_SLOT)
	);
	test_eq_u32("bus reset keeps EP1 configuration", 0x48, USB_EP_CONFIG(EP1_IN_SLOT));
	test_eq_u32("bus reset keeps the maximum packet size", 0x20, USB_EP_MAX_PACKET_LOW(0));
	test_eq_u32("bus reset keeps FIFO control", USB_EP_CONTROL_IN_ENABLE, USB_EP_CONTROL(EP1_IN_SLOT));
	test_eq_u32("bus reset keeps FIFO data", EP1_FIFO_BYTES - 2, USB_EP_COUNT_LOW(EP1_IN_SLOT));

	static const struct {
		const char *name;
		uint32_t from;
		uint32_t to;
		uint32_t event;
	} TRANSITIONS[] = {
		{ "setting only the drive enable signals no bus reset", 0, USB_CFG_DRIVE_ENABLE, 0 },
		{
			"keeping the transceiver enabled signals no bus reset",
			USB_CFG_PHY_ENABLE | USB_CFG_DRIVE_ENABLE,
			USB_CFG_PHY_ENABLE,
			0
		},
		{
			"clearing only the transceiver enable signals a bus reset",
			USB_CFG_PHY_ENABLE | USB_CFG_DRIVE_ENABLE,
			USB_CFG_DRIVE_ENABLE,
			USB_EVENT_INT_STATUS_RESET
		},
	};
	for (uint32_t index = 0; index < ARRAY_SIZE(TRANSITIONS); index++) {
		usb_start();
		USB_CFG = TRANSITIONS[index].from;
		stopwatch_usleep_wd(200);
		USB_EVENT_INT_STATUS = UINT8_MAX;
		USB_CFG = TRANSITIONS[index].to;
		stopwatch_usleep_wd(1000);
		test_eq_u32(TRANSITIONS[index].name, TRANSITIONS[index].event, USB_EVENT_INT_STATUS);
	}

	static const struct {
		const char *name;
		bool se0_ends_first;
		uint32_t event;
		uint32_t address;
	} DISABLED_CORE_SE0[] = {
		{ "SE0 ending while the core is disabled", true, 0, USB_DEVICE_ADDRESS_ENABLE | 5 },
		{ "SE0 still present when the core is enabled", false, USB_EVENT_INT_STATUS_RESET, USB_DEVICE_ADDRESS_ENABLE },
	};
	for (uint32_t index = 0; index < ARRAY_SIZE(DISABLED_CORE_SE0); index++) {
		usb_start();
		usb_enable_phy();
		USB_DEVICE_ADDRESS = USB_DEVICE_ADDRESS_ENABLE | 5;
		USB_EVENT_INT_STATUS = UINT8_MAX;
		USB_CFG = 0;
		USB_CONTROL = 0;
		stopwatch_usleep_wd(1000);
		if (DISABLED_CORE_SE0[index].se0_ends_first)
			usb_enable_phy();
		USB_CONTROL = USB_CONTROL_ENABLE;
		stopwatch_usleep_wd(1000);
		uint32_t event = USB_EVENT_INT_STATUS;
		uint32_t address = USB_DEVICE_ADDRESS;
		printf("# %s: event %02lX, device address %02lX\n", DISABLED_CORE_SE0[index].name, event, address);
		test_check(
			DISABLED_CORE_SE0[index].name,
			event == DISABLED_CORE_SE0[index].event && address == DISABLED_CORE_SE0[index].address
		);
	}
}

static void test_suspend(void) {
	usb_start();
	usb_enable_phy();
	test_check("idle bus raises the suspend event", usb_wait_event(USB_EVENT_INT_STATUS_SUSPEND, USB_EVENT_TIMEOUT_MS));
	test_eq_u32(
		"PHY reports suspend in the J state",
		USB_PHY_CONTROL_SUSPEND | USB_PHY_CONTROL_LINE_STATE_J,
		USB_PHY_CONTROL & (USB_PHY_CONTROL_SUSPEND | USB_PHY_CONTROL_LINE_STATE)
	);
	test_check("continued idle raises the idle event", usb_wait_event(USB_EVENT_INT_STATUS_IDLE, USB_EVENT_TIMEOUT_MS));
	test_eq_u32(
		"idle bus raises no other events",
		USB_EVENT_INT_STATUS_SUSPEND | USB_EVENT_INT_STATUS_IDLE,
		USB_EVENT_INT_STATUS
	);

	usb_start();
	USB_CFG = USB_CFG_DRIVE_ENABLE;
	stopwatch_usleep_wd(10000);
	test_eq_u32("disabled transceiver raises no suspend event", 0, USB_EVENT_INT_STATUS);

	usb_start();
	USB_CONTROL = 0;
	USB_CFG = USB_CFG_PHY_ENABLE;
	stopwatch_usleep_wd(10000);
	USB_CONTROL = USB_CONTROL_ENABLE;
	test_eq_u32("disabled core raises no suspend event", 0, USB_EVENT_INT_STATUS);
}

static bool usb_wait_line_state(uint32_t line_state, stopwatch_t start) {
	while ((USB_PHY_CONTROL & USB_PHY_CONTROL_LINE_STATE) != line_state) {
		if (stopwatch_elapsed_ms(start) >= USB_EVENT_TIMEOUT_MS)
			return false;
	}
	return true;
}

static void test_remote_wakeup(void) {
	usb_start();
	USB_PHY_CONTROL = USB_PHY_CONTROL_RESUME;
	test_eq_u32(
		"remote wakeup without transceiver finishes at once",
		USB_EVENT_INT_STATUS_WAKEUP_DONE,
		USB_EVENT_INT_STATUS
	);

	usb_start();
	USB_CFG = USB_CFG_PHY_ENABLE | USB_CFG_DRIVE_ENABLE;
	USB_PHY_CONTROL = USB_PHY_CONTROL_RESUME;
	stopwatch_t start = stopwatch_get();
	test_check("remote wakeup drives K after bus idle", usb_wait_line_state(USB_PHY_CONTROL_LINE_STATE_K, start));
	test_eq_u32(
		"wakeup started flag is set while K is driven",
		USB_PHY_CONTROL_WAKEUP_STARTED,
		USB_PHY_CONTROL & USB_PHY_CONTROL_WAKEUP_STARTED
	);
	bool k_ended = usb_wait_line_state(USB_PHY_CONTROL_LINE_STATE_J, start);
	USB_PHY_CONTROL = 0;
	test_check("remote wakeup K returns to J", k_ended);
	test_eq_u32(
		"wakeup started flag stays set after remote wakeup",
		USB_PHY_CONTROL_WAKEUP_STARTED | USB_PHY_CONTROL_LINE_STATE_J,
		USB_PHY_CONTROL & (USB_PHY_CONTROL_WAKEUP_STARTED | USB_PHY_CONTROL_LINE_STATE)
	);
	test_eq_u32(
		"remote wakeup on an idle bus does not finish",
		0,
		USB_EVENT_INT_STATUS & USB_EVENT_INT_STATUS_WAKEUP_DONE
	);

	usb_start();
	USB_CFG = USB_CFG_PHY_ENABLE;
	USB_PHY_CONTROL = USB_PHY_CONTROL_RESUME;
	start = stopwatch_get();
	bool k_driven = usb_wait_line_state(USB_PHY_CONTROL_LINE_STATE_K, start);
#ifdef PMB8875
	test_check("remote wakeup drives K without a drive enable bit", k_driven);
#else
	test_check("remote wakeup drives no K without the drive enable", !k_driven);
#endif
	test_eq_u32(
		"remote wakeup without the drive enable sets the wakeup started flag",
		USB_PHY_CONTROL_WAKEUP_STARTED,
		USB_PHY_CONTROL & USB_PHY_CONTROL_WAKEUP_STARTED
	);
}

static void test_core_disable(void) {
	usb_start();
	USB_CFG = USB_CFG_PHY_ENABLE;
	USB_DEVICE_ADDRESS = USB_DEVICE_ADDRESS_ENABLE | 5;
	USB_EP_ENABLE_LOW = 0x07;
	USB_EP_ENABLE_HIGH = 0x05;
	USB_EP_CONFIG(EP1_IN_SLOT) = 0x48;
	USB_EP_MAX_PACKET_LOW(0) = 0x20;
	USB_GLOBAL_EVENT_EP_DONE_INT_ENABLE = 0x07FFFFFF;
	USB_DMA_EP_READY_INT_ENABLE = 0x07FF03FF;
	ep1_configure_fifo();
	ep1_write(1);
	USB_EP_CONTROL(EP1_IN_SLOT) |= USB_EP_CONTROL_COMMIT;
	usb_wait_event(USB_EVENT_INT_STATUS_IDLE, USB_EVENT_TIMEOUT_MS);
	USB_CONTROL = 0;
	test_eq_u32("disabled core reads the device address as zero", 0, USB_DEVICE_ADDRESS);
	test_eq_u32("disabled core reads the endpoint enables as zero", 0, USB_EP_ENABLE_LOW);
	test_eq_u32("disabled core reads the FIFO size as zero", 0, USB_EP_FIFO_SIZE(1));
	test_eq_u32("disabled core reads the PHY control as zero", 0, USB_PHY_CONTROL);
	test_eq_u32("disabled core keeps the wrapper readable", USB_CFG_PHY_ENABLE, USB_CFG);
	USB_CONTROL = USB_CONTROL_ENABLE;
	test_eq_u32("core disable clears event and global status", 0, USB_GLOBAL_EVENT_INT_STATUS);
	test_eq_u32("core disable resets endpoint configuration", 0x28, USB_EP_CONFIG(EP1_IN_SLOT));
	test_eq_u32("core disable resets FIFO control", 0, USB_EP_CONTROL(EP1_IN_SLOT));
	test_eq_u32("core disable resets the maximum packet size", 0, USB_EP_MAX_PACKET_LOW(0));
	test_eq_u32("core disable resets interrupt enables", 0, USB_GLOBAL_EVENT_EP_DONE_INT_ENABLE);
	test_eq_u32("core disable resets FIFO ready and DMA enables", 0, USB_DMA_EP_READY_INT_ENABLE);
	test_eq_u32("core disable keeps the device address", USB_DEVICE_ADDRESS_ENABLE | 5, USB_DEVICE_ADDRESS);
	test_eq_u32("core disable keeps endpoints 0-7 enabled", 0x07, USB_EP_ENABLE_LOW);
	test_eq_u32("core disable keeps endpoints 8-10 enabled", 0x05, USB_EP_ENABLE_HIGH);
	test_eq_u32("core disable keeps the FIFO size", 2 << USB_EP_FIFO_SIZE_SIZE_SHIFT, USB_EP_FIFO_SIZE(1));
	test_eq_u32("core disable keeps FIFO data", EP1_FIFO_BYTES - 1, USB_EP_COUNT_LOW(EP1_IN_SLOT));
	test_eq_u32("core disable keeps the wrapper configuration", USB_CFG_PHY_ENABLE, USB_CFG);
}

static bool usb_is_bus_connected(void) {
	return (USB_PHY_CONTROL & USB_PHY_CONTROL_LINE_STATE) == USB_PHY_CONTROL_LINE_STATE_J;
}

static void test_bus_category(const char *name, void (*test)(void), bool is_bus_connected) {
	test_category(name);
	if (is_bus_connected) {
		test();
	} else {
		test_skip(name, "USB data lines are not connected");
	}
}

int main(void) {
	test_start("USB peripheral test");
	if (sio_is_usb()) {
		test_skip("USB peripheral test", "resets the USB controller that carries the console");
		return test_finish();
	}
	cgu_pll_set(7, 1);
	cgu_phases_enable(CGU_PHASE4);
	cgu_phase_set_divider(4, 4, 2);
	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_CLK48M_CLKSEL) | CGU_CON2_CLK48M_CLKSEL_PHASE4;
#ifdef PMB8876
	CGU_CON3 = (CGU_CON3 & ~CGU_CON3_CLK48M_CLKDIV) | CGU_CON3_CLK48M_CLKDIV_DIV1;
#endif
#ifdef BOARD_HAS_PMIC_PMB6812
	i2c_init();
	test_check(
		"PMIC enables the USB transceiver supply",
		pmb681x_reg_write(PMB6812_USBCTRL, PMB6812_USBCTRL_LUSBMD_ON | PMB6812_USBCTRL_PULLUP_PULLUP)
	);
#endif

	test_category("Reset values");
	test_reset_values();
	test_category("Wrapper reset");
	test_wrapper_reset();
	test_category("Interrupt enable reset");
	test_interrupt_enable_reset();
	test_category("Packed registers");
	test_packed_registers();
	test_category("FIFO size");
	test_fifo_size();
	test_category("FIFO levels");
	test_fifo_levels();
	test_category("OUT FIFO arming");
	test_out_fifo_arming();
	test_category("Endpoint interrupts");
	test_endpoint_interrupts();

	usb_start();
	usb_enable_phy();
	bool is_bus_connected = usb_is_bus_connected();
	test_bus_category("Packed status", test_packed_status, is_bus_connected);
	test_bus_category("Interrupt summary", test_interrupt_summary, is_bus_connected);
	test_bus_category("Bus reset", test_bus_reset, is_bus_connected);
	test_bus_category("Suspend", test_suspend, is_bus_connected);
	test_bus_category("Remote wakeup", test_remote_wakeup, is_bus_connected);
	test_bus_category("Core disable", test_core_disable, is_bus_connected);

	return test_finish();
}
