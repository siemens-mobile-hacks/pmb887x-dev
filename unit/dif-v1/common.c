#include <pmb887x.h>

#include "dif-v1.h"

void dif_v1_configure(uint32_t width, uint32_t rxfcon, uint32_t txfcon, uint32_t format) {
#ifdef PMB8875
	DIF_CON = 0;
	DIF_BR = 3;
	DIF_IMSC = 0;
	DIF_ICR = DIF_ICR_TX | DIF_ICR_RX | DIF_ICR_ERR;
	DIF_RXFCON = 0;
	DIF_TXFCON = 0;
	DIF_PBCCON = 0;
	DIF_BMREG0 = 0x14830820;
	DIF_BMREG1 = 0x2D4920E6;
	DIF_BMREG2 = 0x460F39AC;
	DIF_BMREG3 = 0x5ED55272;
	DIF_BMREG4 = 0x779B6B38;
	DIF_BMREG5 = 0x000003FE;
	DIF_BCREG = 0;
	DIF_BCSEL0 = 0;
	DIF_BCSEL1 = 0;
	DIF_RXFCON = rxfcon;
	DIF_TXFCON = txfcon;
	DIF_CON = format | DIF_CON_LB | DIF_CON_MS_MASTER | ((width - 1) << DIF_CON_BM_SHIFT) | DIF_CON_EN;
#else
	(void) width;
	(void) rxfcon;
	(void) txfcon;
	(void) format;
#endif
}
