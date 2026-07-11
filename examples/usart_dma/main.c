#include "stopwatch.h"
#include <pmb887x.h>
#include <printf.h>
#include <stdint.h>

void dmac_requests(const char *name, uint32_t requests) {
	if (!requests)
		return;
	printf("%s:", name);
	for (int i = 0; i < 16; i++) {
		if ((requests & (1 << i))) {
			printf(" +%d", i);
		}
	}
	printf("\r\n");
}

void dump_usart_ris() {
	uint32_t ris = USART_RIS(USART0);
	printf("USART_RIS: %08X\r\n", ris);
	printf(" RX: %d\r\n", ris & USART_RIS_RX ? 1 : 0);
	printf(" TX: %d\r\n", ris & USART_RIS_TX ? 1 : 0);
	printf(" TB: %d\r\n", ris & USART_RIS_TB ? 1 : 0);
	printf(" ERR: %d\r\n", ris & USART_RIS_ERR ? 1 : 0);
}

static int irq() {
	int irq = VIC_IRQ_CURRENT;
	if (irq != 0) {
		VIC_IRQ_ACK = 1;
	}
	return irq;
}

void test_usart() {
	stopwatch_init();

	USART_TXFCON(USART0) = USART_TXFCON_TXFEN | (1 << USART_TXFCON_TXFITL_SHIFT);
	USART_RXFCON(USART0) = USART_RXFCON_RXFEN | (1 << USART_RXFCON_RXFITL_SHIFT); // 1xWORD = 2

	USART_IMSC(USART0) = 0xFFFFFFFF;
	USART_ICR(USART0) = 0xFFFFFFFF;

	while (true) {

		dump_usart_ris();

	}

	USART_DMAE(USART0) = USART_DMAE_RX;

	DMAC_CONFIG = 0;

	static uint8_t dst[64] = { 0 };

	for (int i = 0; i < 64; i++)
		dst[i] = 0xFF;

	DMAC_CH_SRC_ADDR(0) = (uint32_t) &USART_RXB(USART0);
	DMAC_CH_DST_ADDR(0) = (uint32_t) &dst;
	DMAC_CH_CONTROL(0) = (2 << DMAC_CH_CONTROL_TRANSFER_SIZE_SHIFT) | // 2xWORD = 4
		DMAC_CH_CONTROL_SB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_BYTE |
		DMAC_CH_CONTROL_D_WIDTH_BYTE |
		DMAC_CH_CONTROL_S_AHB1 |
		DMAC_CH_CONTROL_D_AHB1 |
		DMAC_CH_CONTROL_DI |
		DMAC_CH_CONTROL_I;
	DMAC_CH_CONFIG(0) =
		(1 << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM |
		DMAC_CH_CONFIG_INT_MASK_ERR |
		DMAC_CH_CONFIG_INT_MASK_TC;

	// WRITE[4] F30001AC: 88000064 (DMAC_CH_CONTROL5): TRANSFER_SIZE(0x64) | SB_SIZE(SZ_1) | DB_SIZE(SZ_1) | S_WIDTH(BYTE) | D_WIDTH(BYTE) | S(AHB1) | D(AHB1) | DI | PROTECTION(0x00) | I (PC: A05D87F4, LR: 00000004)
	// WRITE[4] F300018C: 84000000 (DMAC_CH_CONTROL4): TRANSFER_SIZE(0x00) | SB_SIZE(SZ_1) | DB_SIZE(SZ_1) | S_WIDTH(BYTE) | D_WIDTH(BYTE) | S(AHB1) | D(AHB1) | SI | PROTECTION(0x00) | I (PC: A05D8394, LR: 00000004)

	printf("SRC: %08X\r\n", DMAC_CH_SRC_ADDR(0));
	printf("DST: %08X\r\n", DMAC_CH_DST_ADDR(0));
	printf("TS: %08X\r\n", (DMAC_CH_CONTROL(0) & DMAC_CH_CONTROL_TRANSFER_SIZE) >> DMAC_CH_CONTROL_TRANSFER_SIZE_SHIFT);

	DMAC_CONFIG = DMAC_CONFIG_ENABLE;

	while (!(USART_RIS(USART0) & USART_RIS_RX)) { }

	dump_usart_ris();

	dmac_requests("DMAC_SOFT_BREQ", DMAC_SOFT_BREQ);
	dmac_requests("DMAC_SOFT_SREQ", DMAC_SOFT_SREQ);
	dmac_requests("DMAC_SOFT_LBREQ", DMAC_SOFT_LBREQ);
	dmac_requests("DMAC_SOFT_LSREQ", DMAC_SOFT_LSREQ);

	DMAC_CH_CONFIG(0) |= DMAC_CH_CONFIG_ENABLE;
	// BREQ == FFL

	printf(
		"USART_FSTAT: RX=%d, TX=%d\r\n",
		(USART_FSTAT(USART0) & USART_FSTAT_RXFFL) >> USART_FSTAT_RXFFL_SHIFT,
		(USART_FSTAT(USART0) & USART_FSTAT_TXFFL) >> USART_FSTAT_TXFFL_SHIFT
	);

	while (!(DMAC_RAW_TC_STATUS & (1 << 0))) {
		if (DMAC_ERR_STATUS) {
			printf("ERR=%08X\r\n", DMAC_ERR_STATUS);
			break;
		}
	}
	printf("transfer done\r\n");

	dump_usart_ris();

	dmac_requests("DMAC_SOFT_BREQ", DMAC_SOFT_BREQ);
	dmac_requests("DMAC_SOFT_SREQ", DMAC_SOFT_SREQ);
	dmac_requests("DMAC_SOFT_LBREQ", DMAC_SOFT_LBREQ);
	dmac_requests("DMAC_SOFT_LSREQ", DMAC_SOFT_LSREQ);

	DMAC_TC_CLEAR = 0xFFFFFFFF;
	DMAC_ERR_CLEAR = 0xFFFFFFFF;

	while (!(DMAC_RAW_TC_STATUS & (1 << 0))) {
		if (DMAC_ERR_STATUS) {
			printf("ERR=%08X\r\n", DMAC_ERR_STATUS);
			break;
		}
	}
	printf("transfer done\r\n");

	printf("SRC: %08X\r\n", DMAC_CH_SRC_ADDR(0));
	printf("DST: %08X\r\n", DMAC_CH_DST_ADDR(0));
	printf("ENABLED: %08X\r\n", DMAC_CH_CONFIG(0) & DMAC_CH_CONFIG_ENABLE);

	dump_usart_ris();

	printf("OUT:");
	for (int i = 0; i < 32; i++)
		printf(" %02X", dst[i]);
	printf("\r\n");

	/*
	uint32_t mis = DIF_MIS;
	uint32_t con = DIF_CON;
	printf("DIF_MIS: %08X\r\n", mis);
	printf(" RX: %d\r\n", mis & DIF_MIS_RX ? 1 : 0);
	printf(" TX: %d\r\n", mis & DIF_MIS_TX ? 1 : 0);
	printf(" TB: %d\r\n", mis & DIF_MIS_TB ? 1 : 0);
	printf(" ERR: %d\r\n", mis & DIF_MIS_ERR ? 1 : 0);
	printf(" IRQ: %d\r\n", VIC_IRQ_CURRENT);
	printf("DIF_CON: %08X\r\n", con);
	printf(" BSY: %d\r\n", con & DIF_CON_BSY ? 1 : 0);
	printf(" TE: %d\r\n", con & DIF_CON_PE ? 1 : 0);
	printf(" RE: %d\r\n", con & DIF_CON_RE ? 1 : 0);
	printf(" PE: %d\r\n", con & DIF_CON_PE ? 1 : 0);
	printf(" BE: %d\r\n", con & DIF_CON_BE ? 1 : 0);
	printf(" BC: %d\r\n", ((con & DIF_CON_BC) >> DIF_CON_BC_SHIFT) + 1);
	printf(" FFL: RX=%d, TX=%d\r\n", (DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT, (DIF_FSTAT & DIF_FSTAT_TXFFL) >> DIF_FSTAT_TXFFL_SHIFT);
	printf("\r\n");

	dmac_requests("DMAC_SOFT_BREQ", DMAC_SOFT_BREQ);
	dmac_requests("DMAC_SOFT_SREQ", DMAC_SOFT_SREQ);
	dmac_requests("DMAC_SOFT_LBREQ", DMAC_SOFT_LBREQ);
	dmac_requests("DMAC_SOFT_LSREQ", DMAC_SOFT_LSREQ);

	while (1);

	uint32_t last_mis = 0;
	uint32_t last_con = 0;

	VIC_CON(VIC_DIF_TX_IRQ) = 1;
	VIC_CON(VIC_DIF_RX_IRQ) = 1;
	VIC_CON(VIC_DIF_ERR_IRQ) = 1;
	VIC_CON(VIC_DIF_TMO_IRQ) = 1;

	DIF_TB = 0x0102;
	stopwatch_msleep(100);

	printf("DIF_IMSC=%08X\r\n", DIF_IMSC);

	while (true) {
		uint32_t mis = DIF_MIS;
		uint32_t con = DIF_CON;

		if (last_mis != mis) {
			last_mis = mis;
		}

		if (last_con != con) {
			last_con = con;
		}

		int curr_irq = irq();
		if (curr_irq != 0) {
			if (curr_irq == VIC_DIF_TX_IRQ) {
				DIF_ICR |= DIF_ICR_TX;
			}
			if (curr_irq == VIC_DIF_RX_IRQ) {
				DIF_RB;
				DIF_ICR |= DIF_ICR_RX;
			}
			if (curr_irq == VIC_DIF_ERR_IRQ)
				DIF_ICR |= DIF_ICR_ERR;
			printf("curr_irq=%d\r\n", curr_irq);
			printf("DIF_MIS: %08X\r\n", mis);
			printf(" RX: %d\r\n", mis & DIF_MIS_RX ? 1 : 0);
			printf(" TX: %d\r\n", mis & DIF_MIS_TX ? 1 : 0);
			printf(" TB: %d\r\n", mis & DIF_MIS_TB ? 1 : 0);
			printf(" ERR: %d\r\n", mis & DIF_MIS_ERR ? 1 : 0);
			printf(" IRQ: %d\r\n", VIC_IRQ_CURRENT);
			printf("DIF_CON: %08X\r\n", con);
			printf(" BSY: %d\r\n", con & DIF_CON_BSY ? 1 : 0);
			printf(" TE: %d\r\n", con & DIF_CON_PE ? 1 : 0);
			printf(" RE: %d\r\n", con & DIF_CON_RE ? 1 : 0);
			printf(" PE: %d\r\n", con & DIF_CON_PE ? 1 : 0);
			printf(" BE: %d\r\n", con & DIF_CON_BE ? 1 : 0);
			printf(" BC: %d\r\n", ((con & DIF_CON_BC) >> DIF_CON_BC_SHIFT) + 1);
			printf(" FFL: RX=%d, TX=%d\r\n", (DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT, (DIF_FSTAT & DIF_FSTAT_TXFFL) >> DIF_FSTAT_TXFFL_SHIFT);
			printf("\r\n");
		}

	}
	*/
}

int main(void) {
	wdt_init();
	wdt_set_max_execution_time(3000);
	stopwatch_init();

	USART_TXB(USART0) = 'A';
	stopwatch_msleep(100);
	USART_TXB(USART0) = 'B';
	stopwatch_msleep(100);
	USART_TXB(USART0) = 'C';
	stopwatch_msleep(100);

	printf("usart_send_common?\r\n");
	printf("done?\r\n");

//	test_usart();
	return 0;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\r\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\r\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\r\n");
	while (true);
}

__IRQ void irq_handler(void) {
	int irqn = VIC_IRQ_CURRENT;
	
	printf("irqn=%d\r\n", irqn);
	
	DIF_ICR = 0xFFFFFFFF;
#ifdef PMB8876
	DIF_ERRIRQSC = 0xFFFFFFFF;
#endif
	VIC_IRQ_ACK = 1;
}
