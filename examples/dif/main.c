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

	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;

	DIF_RUNCTRL = 0;
	DIF_CON1 = DIF_CON1 & 0xfffffffd;
	DIF_CON2 = DIF_CON2 | 0x10;
	DIF_CON2 = DIF_CON2 & 0xffffff9f;
	DIF_RUNCTRL = 1;

	DIF_RUNCTRL = 0;
	DIF_PROG(0) = 0x14830820;
	DIF_PROG(1) = 0x2d4920e6;
	DIF_PROG(2) = 0x460f39ac;
	DIF_PROG(3) = 0x5ed55272;
	DIF_PROG(4) = 0x779b6b38;
	DIF_PROG(5) = 0x3fe;
	DIF_RUNCTRL = 1;

	while (true) {
		DIF_RUNCTRL = 0;
		DIF_CON2 = DIF_CON2 & 0xfffffffe;
		DIF_RUNCTRL = 1;
		DIF_FIFO = 0;
		while (DIF_STAT & 1);

		printf(".\n");
	}

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

void test_dif() {
	stopwatch_init();

	DIF_CLC = 0x100;
	DIF_BR = 0;
	DIF_CON = DIF_CON_MS_MASTER |
		DIF_CON_HB_MSB | DIF_CON_PH_1 | DIF_CON_PO_1 |
		//DIF_CON_REN |
		//DIF_CON_TEN | DIF_CON_REN | DIF_CON_PEN | DIF_CON_BEN |
		DIF_CON_LB |
		DIF_CON_BM_16 |
		DIF_CON_EN;
	DIF_IMSC = 0xFFFFFFFF;

	//DIF_TXFCON = DIF_TXFCON_TXFEN | (1 << DIF_TXFCON_TXFITL_SHIFT);
	//DIF_RXFCON = DIF_RXFCON_RXFEN | (1 << DIF_RXFCON_RXFITL_SHIFT);

	uint32_t last_mis = 0;
	uint32_t last_con = 0;

	VIC_CON(VIC_DIF_TX_IRQ) = 1;
	VIC_CON(VIC_DIF_RX_IRQ) = 1;
	VIC_CON(VIC_DIF_ERR_IRQ) = 1;
	VIC_CON(VIC_DIF_UNK_IRQ) = 1;

	DIF_TB = 0x0102;
	stopwatch_msleep(100);

	DIF_TB = 0x0203;
	stopwatch_msleep(100);

	DIF_TB = 0x0405;
	stopwatch_msleep(100);

	DIF_TB = 0x0608;
	stopwatch_msleep(100);

	DIF_TB = 0x0910;
	stopwatch_msleep(100);


	stopwatch_msleep(100);

	printf("RB: %04X\n", DIF_RB);
	printf("RB: %04X\n", DIF_RB);
	printf("RB: %04X\n", DIF_RB);
	printf("RB: %04X\n", DIF_RB);
	printf("RB: %04X\n", DIF_RB);
	printf("RB: %04X\n", DIF_RB);

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
			if (curr_irq == VIC_DIF_TX_IRQ)
				DIF_ICR |= DIF_ICR_TX;
			if (curr_irq == VIC_DIF_RX_IRQ)
				DIF_ICR |= DIF_ICR_RX;
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
			printf(" IRQ: %d\n", VIC_IRQ_CURRENT);
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
	
	VIC_IRQ_ACK = 1;
}
