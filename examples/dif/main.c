#include <pmb887x.h>
#include <printf.h>
#include <stdint.h>

#ifdef PMB8876
void test_dif() {
	GPIO_PIN(GPIO_DIF_VD) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_LOW;
	GPIO_PIN(GPIO_DIF_RESET1) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;

	GPIO_PIN(GPIO_DIF_CD) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_CS1) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;

	GPIO_PIN(GPIO_DIF_RD) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_WR) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;

	GPIO_PIN(GPIO_DIF_D0) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D1) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D2) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D3) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D4) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D5) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D6) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;
	GPIO_PIN(GPIO_DIF_D7) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_IS_ALT0;

	stopwatch_init();
//	cpu_enable_irq(1);

//	VIC_CON(VIC_DIF_RX_SINGLE_IRQ) = 1;
//	VIC_CON(VIC_DIF_RX_BURST_IRQ) = 1;
//	VIC_CON(VIC_DIF_TX_IRQ) = 1;
//	VIC_CON(VIC_DIF_ERR_IRQ) = 1; // error

	DIF_CLC = 0xFF << MOD_CLC_RMC_SHIFT;

	DIF_RUNCTRL = 0;
	DIF_PERREG = DIF_PERREG_DIFPERMODE_PARALLEL;
	DIF_TXFIFO_CFG =
	DIF_TXFIFO_CFG_TXFA_1 | DIF_TXFIFO_CFG_TXBS_4_WORD | DIF_TXFIFO_CFG_TXFC |
	DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFC;
	// DIF_TXFIFO_CFG = 0;
	DIF_FDIV = 0xFFFF;
	DIF_BR = 0;
	DIF_IMSC = 0xFFFFFFFF;
	DIF_CSREG |= DIF_CSREG_CS1;
	DIF_RUNCTRL = 1;

	DIF_IMSC = 0xFFFFFFFF;

	DIF_ERRIRQSM = 0;
	for (int i = 0; i < 100; i++)
		DIF_TXD = 0;
	DIF_ERRIRQSM = 0xFFFFFFFF;

	//            10000001000000010111
	// 1000000000000000000000000000000 - usb
	//   10000000000000000000000000000 - usart1
	//  100000000000000000000000000000 - bypass

	SCU_BOOT_CFG = 0xFFFFFFFF;
	printf("SCU_BOOT_CFG=%08X\n", SCU_BOOT_CFG);

	while (1);

	DIF_TPS_CTRL = 16;
//	printf("DIF_TPS_CTRL=%d\n", DIF_TPS_CTRL);

	uint32_t last_ris = 0;
	uint32_t last_stat = 0;
	while (true) {
		uint32_t ris = DIF_RIS;
		uint32_t stat = DIF_STAT;

		if ((ris & DIF_RIS_TXBREQ)) {
			DIF_TXD = 0;
			DIF_ICR |= DIF_ICR_TXBREQ;
			printf("  [TXBREQ] DIF_TPS_CTRL=%d\n", DIF_TPS_CTRL);
		}

		if ((ris & DIF_RIS_TXLBREQ)) {
			DIF_TXD = 0;
			uint32_t st = DIF_STAT;
			DIF_ICR = DIF_ICR_TXLBREQ;
			printf("  [TXLBREQ] DIF_TPS_CTRL=%d // %08X\n", DIF_TPS_CTRL, st);
		}

		if ((ris & DIF_RIS_TXSREQ)) {
			DIF_TXD = 0;
			DIF_ICR |= DIF_ICR_TXSREQ;
			printf("  [TXSREQ] DIF_TPS_CTRL=%d\n", DIF_TPS_CTRL);
		}

		if ((ris & DIF_RIS_TXLSREQ)) {
			DIF_TXD = 0;
			DIF_ICR |= DIF_ICR_TXLSREQ;
			printf("  [TXLSREQ] DIF_TPS_CTRL=%d\n", DIF_TPS_CTRL);
		}

		if ((ris & DIF_RIS_RXSREQ)) {
			printf("  DIF_RPS_STAT=%08X\n", DIF_RPS_STAT);
			printf("  DIF_RXD=%08X\n", DIF_RXD);
			DIF_ICR |= DIF_ICR_RXSREQ;
			printf("  DIF_STARTLCDRD=%08X\n", DIF_STARTLCDRD);
		}

		if ((ris & DIF_RIS_ERR)) {
			DIF_ICR |= DIF_ICR_ERR;
			printf("  DIF_ERRIRQSS=%08X\n", DIF_ERRIRQSS);
			DIF_ERRIRQSC = 0xFFFFFFFF;
		}

		if (last_stat != stat) {
			last_stat = stat;
			printf("  DIF_STAT=%08X\n", last_stat);
		}

		if (last_ris != ris) {
			last_ris = ris;

			printf("EV [%08X]:", last_ris);
			if (last_ris) {
				if ((last_ris & DIF_RIS_RXLSREQ))
					printf(" +DIF_RIS_RXLSREQ");
				if ((last_ris & DIF_RIS_RXSREQ))
					printf(" +DIF_RIS_RXSREQ");
				if ((last_ris & DIF_RIS_RXLBREQ))
					printf(" +DIF_RIS_RXLBREQ");
				if ((last_ris & DIF_RIS_RXBREQ))
					printf(" +DIF_RIS_RXBREQ");
				if ((last_ris & DIF_RIS_TXLSREQ))
					printf(" +DIF_RIS_TXLSREQ");
				if ((last_ris & DIF_RIS_TXSREQ))
					printf(" +DIF_RIS_TXSREQ");
				if ((last_ris & DIF_RIS_TXLBREQ))
					printf(" +DIF_RIS_TXLBREQ");
				if ((last_ris & DIF_RIS_TXBREQ))
					printf(" +DIF_RIS_TXBREQ");
				if ((last_ris & DIF_RIS_ERR))
					printf(" +DIF_RIS_ERR");
			}
			printf("\n");
		}
	}

	/*
	DIF_IMSC = 0xFFFFFFFF;

	for (int i = 0; i < 9; i++) {
		printf("bit %d\n", i);
		DIF_ISR = 1 << i;
		stopwatch_msleep_wd(10);
	}
	*/

	// I2C_SINGLE_REQ_IRQ
	// DIF_ISR_RXLSREQ - 134
	// DIF_ISR_RXSREQ - 134
	// DIF_ISR_RXLBREQ - 134

	// I2C_BURST_REQ_IRQ
	// DIF_ISR_RXBREQ - 135

	// TX
	// DIF_ISR_TXLSREQ - 136
	// DIF_ISR_TXSREQ - 136
	// DIF_ISR_TXLBREQ - 136
	// DIF_ISR_TXBREQ - 136

	// ERR
	// DIF_ISR_ERR - 137

	printf("DIF_IMSC=%08X\n", DIF_IMSC);
	printf("DIF_RIS=%08X\n", DIF_RIS);
	printf("DIF_MIS=%08X\n", DIF_MIS);

	/*
	DIF_RUNCTRL = 0;
	DIF_CON = DIF_CON_HB_MSB;
	DIF_CSREG = DIF_CSREG_CS1;
	DIF_BMREG0 = 0x14830820;
	DIF_BMREG1 = 0x2d4920e6;
	DIF_BMREG2 = 0x460f39ac;
	DIF_BMREG3 = 0x5ed55272;
	DIF_BMREG4 = 0x779b6b38;
	DIF_BMREG5 = 0x3fe;
	DIF_RUNCTRL = 1;

	while (true) {
		DIF_TXD = 0;
		while (DIF_STAT & DIF_STAT_BSY);

		printf(".\n");
	}
	*/
}
#endif

#ifdef PMB8875
static int irq() {
	int irq = VIC_IRQ_CURRENT;
	if (irq != 0) {
		VIC_IRQ_ACK = 1;
	}
	return irq;
}

void dmac_requests(const char *name, uint32_t requests) {
	if (!requests)
		return;
	printf("%s:", name);
	for (int i = 0; i < 16; i++) {
		if ((requests & (1 << i))) {
			printf(" +%d", i);
		}
	}
	printf("\n");
}

void test_dif() {
	stopwatch_init();

	DIF_CLC = 0xFF << MOD_CLC_RMC_SHIFT;
	DIF_BR = 0;
	DIF_TXFCON = DIF_TXFCON_TXFEN | (1 << DIF_TXFCON_TXFITL_SHIFT);
	DIF_RXFCON = DIF_RXFCON_RXFEN | (1 << DIF_RXFCON_RXFITL_SHIFT); // 1xWORD = 2
	DIF_CON = DIF_CON_MS_MASTER |
		DIF_CON_HB_MSB | DIF_CON_PH_1 | DIF_CON_PO_1 |
		//DIF_CON_REN |
		DIF_CON_TEN | DIF_CON_REN | DIF_CON_PEN | DIF_CON_BEN |
		DIF_CON_LB |
		DIF_CON_BM_16 |
		DIF_CON_EN;
	DIF_IMSC = 0xFFFFFFFF;
	DIF_DMACON = DIF_DMACON_RX | DIF_DMACON_TX;

	DMAC_CONFIG = 0;

	// SCU_DMAE = 1 << 0;

	static uint8_t dst[64] = { 0 };

	for (int i = 0; i < 64; i++)
		dst[i] = 0xFF;

	DMAC_CH_SRC_ADDR(0) = (uint32_t) &DIF_RB;
	DMAC_CH_DST_ADDR(0) = (uint32_t) &dst;
	DMAC_CH_CONTROL(0) = (1 << DMAC_CH_CONTROL_TRANSFER_SIZE_SHIFT) | // 2xWORD = 4
		DMAC_CH_CONTROL_SB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_WORD |
		DMAC_CH_CONTROL_D_WIDTH_WORD |
		DMAC_CH_CONTROL_S_AHB1 |
		DMAC_CH_CONTROL_D_AHB1 |
		DMAC_CH_CONTROL_DI |
		DMAC_CH_CONTROL_I;
	DMAC_CH_CONFIG(0) =
		(5 << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM |
		DMAC_CH_CONFIG_INT_MASK_ERR |
		DMAC_CH_CONFIG_INT_MASK_TC;

	printf("SRC: %08X\n", DMAC_CH_SRC_ADDR(0));
	printf("DST: %08X\n", DMAC_CH_DST_ADDR(0));
	printf("TS: %08X\n", (DMAC_CH_CONTROL(0) & DMAC_CH_CONTROL_TRANSFER_SIZE) >> DMAC_CH_CONTROL_TRANSFER_SIZE_SHIFT);

	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_CH_CONFIG(0) |= DMAC_CH_CONFIG_ENABLE;

	DIF_TB = 0xAABB;
	while ((DIF_CON & DIF_CON_BSY));

	dmac_requests("DMAC_SOFT_BREQ", DMAC_SOFT_BREQ);
	dmac_requests("DMAC_SOFT_SREQ", DMAC_SOFT_SREQ);
	dmac_requests("DMAC_SOFT_LBREQ", DMAC_SOFT_LBREQ);
	dmac_requests("DMAC_SOFT_LSREQ", DMAC_SOFT_LSREQ);

	// BREQ == FFL

	printf("DIF_FSTAT: RX=%d, TX=%d\n", DIF_FSTAT & DIF_FSTAT_RXFFL, DIF_FSTAT & DIF_FSTAT_TXFFL);

	while (!(DMAC_RAW_TC_STATUS & (1 << 0))) {
		if (DMAC_ERR_STATUS) {
			printf("ERR=%08X\n", DMAC_ERR_STATUS);
			break;
		}
	}

	printf("transfer done\n");
	printf("SRC: %08X\n", DMAC_CH_SRC_ADDR(0));
	printf("DST: %08X\n", DMAC_CH_DST_ADDR(0));
	printf("ENABLED: %08X\n", DMAC_CH_CONFIG(0) & DMAC_CH_CONFIG_ENABLE);

	printf("OUT:");
	for (int i = 0; i < 32; i++)
		printf(" %02X", dst[i]);
	printf("\n");

	uint32_t mis = DIF_MIS;
	uint32_t con = DIF_CON;
	printf("DIF_MIS: %08X\n", mis);
	printf(" RX: %d\n", mis & DIF_MIS_RX ? 1 : 0);
	printf(" TX: %d\n", mis & DIF_MIS_TX ? 1 : 0);
	printf(" TB: %d\n", mis & DIF_MIS_TB ? 1 : 0);
	printf(" ERR: %d\n", mis & DIF_MIS_ERR ? 1 : 0);
	printf(" IRQ: %d\n", VIC_IRQ_CURRENT);
	printf("DIF_CON: %08X\n", con);
	printf(" BSY: %d\n", con & DIF_CON_BSY ? 1 : 0);
	printf(" TE: %d\n", con & DIF_CON_PE ? 1 : 0);
	printf(" RE: %d\n", con & DIF_CON_RE ? 1 : 0);
	printf(" PE: %d\n", con & DIF_CON_PE ? 1 : 0);
	printf(" BE: %d\n", con & DIF_CON_BE ? 1 : 0);
	printf(" BC: %d\n", ((con & DIF_CON_BC) >> DIF_CON_BC_SHIFT) + 1);
	printf(" FFL: RX=%d, TX=%d\n", (DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT, (DIF_FSTAT & DIF_FSTAT_TXFFL) >> DIF_FSTAT_TXFFL_SHIFT);
	printf("\n");

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

	printf("DIF_IMSC=%08X\n", DIF_IMSC);

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
			printf("curr_irq=%d\n", curr_irq);
			printf("DIF_MIS: %08X\n", mis);
			printf(" RX: %d\n", mis & DIF_MIS_RX ? 1 : 0);
			printf(" TX: %d\n", mis & DIF_MIS_TX ? 1 : 0);
			printf(" TB: %d\n", mis & DIF_MIS_TB ? 1 : 0);
			printf(" ERR: %d\n", mis & DIF_MIS_ERR ? 1 : 0);
			printf(" IRQ: %d\n", VIC_IRQ_CURRENT);
			printf("DIF_CON: %08X\n", con);
			printf(" BSY: %d\n", con & DIF_CON_BSY ? 1 : 0);
			printf(" TE: %d\n", con & DIF_CON_PE ? 1 : 0);
			printf(" RE: %d\n", con & DIF_CON_RE ? 1 : 0);
			printf(" PE: %d\n", con & DIF_CON_PE ? 1 : 0);
			printf(" BE: %d\n", con & DIF_CON_BE ? 1 : 0);
			printf(" BC: %d\n", ((con & DIF_CON_BC) >> DIF_CON_BC_SHIFT) + 1);
			printf(" FFL: RX=%d, TX=%d\n", (DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT, (DIF_FSTAT & DIF_FSTAT_TXFFL) >> DIF_FSTAT_TXFFL_SHIFT);
			printf("\n");
		}

	}
}
#endif

int main(void) {
	wdt_init();
	wdt_set_max_execution_time(1000);
	test_dif();
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

__IRQ void irq_handler(void) {
	int irqn = VIC_IRQ_CURRENT;
	
	printf("irqn=%d\n", irqn);
	
	DIF_ICR = 0xFFFFFFFF;
#ifdef PMB8876
	DIF_ERRIRQSC = 0xFFFFFFFF;
#endif
	VIC_IRQ_ACK = 1;
}
