#include <pmb887x.h>

int main(void) {
	wdt_init();
	cpu_enable_irq(false);
	
	uint8_t src[8 * 4] = {
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
	};
	uint8_t dst[8 * 4] = {
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	
	int ch = 1;
	
	DMAC_CONFIG = 0;
	
	SCU_DMAE = 1 << ch;
	
	DMAC_CH_SRC_ADDR(ch) = (uint32_t) &src;
	DMAC_CH_DST_ADDR(ch) = (uint32_t) &dst;
	DMAC_CH_CONTROL(ch) =
		(4 << DMAC_CH_CONTROL_TRANSFER_SIZE_SHIFT) |
		DMAC_CH_CONTROL_SB_SIZE_SZ_256 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_256 |
		DMAC_CH_CONTROL_S_WIDTH_WORD |
		DMAC_CH_CONTROL_D_WIDTH_DWORD |
		DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 |
		DMAC_CH_CONTROL_SI |
		DMAC_CH_CONTROL_DI |
		DMAC_CH_CONTROL_I;
	DMAC_CH_CONFIG(ch) =
		(8 << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM |
		DMAC_CH_CONFIG_INT_MASK_ERR |
		DMAC_CH_CONFIG_INT_MASK_TC;
	
	// DMAC_CH_LLI(ch) = 1 << DMAC_CH_LLI_ITEM_SHIFT;
	
	printf("SRC: %08X\n", DMAC_CH_SRC_ADDR(ch));
	printf("DST: %08X\n", DMAC_CH_DST_ADDR(ch));
	printf("TS: %08X\n", (DMAC_CH_CONTROL(ch) & DMAC_CH_CONTROL_TRANSFER_SIZE) >> DMAC_CH_CONTROL_TRANSFER_SIZE_SHIFT);
	
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	
	DMAC_CH_CONFIG(ch) |= DMAC_CH_CONFIG_ENABLE;
	
	VIC_CON(VIC_DMAC_ERR_IRQ) = 1;
	VIC_CON(VIC_DMAC_CH0_IRQ) = 1;
	
	while (!(DMAC_RAW_TC_STATUS & (1 << ch))) {
		if (DMAC_ERR_STATUS) {
			printf("ERR=%08X\n", DMAC_ERR_STATUS);
			// DMAC_ERR_CLEAR = DMAC_ERR_STATUS;
			printf("irqn: %d\n", VIC_IRQ_CURRENT);
			break;
		}
	}
	
	printf("irqn: %d\n", VIC_IRQ_CURRENT);
	
	DMAC_TC_CLEAR = (1 << ch);
	
	stopwatch_msleep(10);
	
	printf("SRC: %08X\n", DMAC_CH_SRC_ADDR(ch));
	printf("DST: %08X\n", DMAC_CH_DST_ADDR(ch));
	printf("TS: %08X\n", (DMAC_CH_CONTROL(ch) & DMAC_CH_CONTROL_TRANSFER_SIZE) >> DMAC_CH_CONTROL_TRANSFER_SIZE_SHIFT);
	
	printf("src: ");
	for (size_t i = 0; i < ARRAY_SIZE(src); i++)
		printf("%02X ", src[i]);
	printf("\n");
	
	printf("dst: ");
	for (size_t i = 0; i < ARRAY_SIZE(dst); i++)
		printf("%02X ", dst[i]);
	printf("\n");
	
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
