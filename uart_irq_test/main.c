#include <printf.h>
#include "main.h"


#if 0
#define USART0_BASE		0xF1000000
#define USART1_BASE		0xF1800000

#define USART0_CLC		USART0_BASE		/* Clock Control Register */
#define USART0_ID		USART0_BASE + 0x08	/* Module Identification Register */
#define USART0_CON		USART0_BASE + 0x10	/* Control Register */
#define USART0_BG		USART0_BASE + 0x14	/* Baudrate Timer Reload Register */
#define USART0_FDV		USART0_BASE + 0x18	/* Fractional Divider Register */
#define USART0_TXB		USART0_BASE + 0x20	/* Transmit Buffer */
#define USART0_RXB		USART0_BASE + 0x24	/* Receive Buffer */
#define USART0_ABCON		USART0_BASE + 0x30	/* Autobaud control register */
#define USART0_ABSTAT		USART0_BASE + 0x34	/* Autobaud status register */
#define USART0_RXFCON		USART0_BASE + 0x40	/* Receive FIFO control register */
#define USART0_TXFCON		USART0_BASE + 0x44	/* Transmit FIFO control register */
#define USART0_FSTAT		USART0_BASE + 0x48	/* FIFO status register */
#define USART0_WHBCON		USART0_BASE + 0x50	/* Write hardware modified control register */
#define USART0_WHBABCON		USART0_BASE + 0x54	/* Write hardware modified autobaud control register */
#define USART0_WHBABSTAT	USART0_BASE + 0x58	/* Write hardware modified autobaud status register */
#define USART0_FCCON		USART0_BASE + 0x5C	/* Flowcontrol control register */
#define USART0_FCSTAT		USART0_BASE + 0x60	/* Flowcontrol status register */
#define USART0_IMSC		USART0_BASE + 0x64	/* Interrupt mask control register */
#define USART0_RIS		USART0_BASE + 0x68	/* Raw interrupt status register */
#define USART0_MIS		USART0_BASE + 0x6C	/* Masked interrupt status register */
#define	USART0_ICR		USART0_BASE + 0x70	/* Interrupt clear register */
#define USART0_ISR		USART0_BASE + 0x74	/* Interrupt set register */
#define USART0_TMO		USART0_BASE + 0x7C	/* Timeout detection control register */
#endif
#define USART0_FCCON		0xF1000000 + 0x5C	/* Flowcontrol control register */
#define USART0_IMSC		0xF1000000 + 0x64
#define USART0_ISR		0xF1000000 + 0x74

#define CLC_SMC_CLK_DIV(x)	((x << 16) & 0xFF0000)
#define CLC_RMC_CLK_DIV(x)	((x <<  8) & 0x00FF00)
#define CLC_FSOE		(1 << 5)	/* Fast shut off enable (1: enable; 0: disable) */
#define CLC_SBWE		(1 << 4)	/* Suspend bit write enable (1: enable; 0: disable) */
#define CLC_EDIS		(1 << 3)	/* External request disable (1: disable; 0: enable) */
#define CLC_SPEN		(1 << 2)	/* Suspend bit enable (1: enable; 0: disable) */
#define CLC_DISS		(1 << 1)	/* Disable status bit (1: disable; 0: enable) */
#define CLC_DISR		(1 << 0)	/* Disable request bit (1: enable; 0: disable) */

#define	CON_R			(1 << 15)	/* Baud rate generator run control (0: disable; 1: enable) */
#define CON_LB			(1 << 14)	/* Loopback mode (0: disable; 1: enable) */
#define CON_BRS			(1 << 13)	/* Baudrate selection (0: Pre-scaler /2; 1: Pre-scaler / 3) */
#define CON_ODD			(1 << 12)	/* Parity selection (0: even; 1: odd)  */
#define	CON_FDE			(1 << 11)	/* Fraction divider enable (0: disable; 1: enable) */
#define CON_OE			(1 << 10)	/* Overrun error flag */
#define CON_FE			(1 <<  9)	/* Framing error flag */
#define CON_PE			(1 <<  8)	/* Parity error flag */
#define CON_OEN			(1 <<  7)	/* Overrun check enable (0: ignore; 1: check) */
#define CON_FEN			(1 <<  6)	/* Framing error check (0: ignore; 1: check) */
#define CON_PEN			(1 <<  5)	/* Parity check enable (0: ignore; 1: check) */
#define CON_REN			(1 <<  4)	/* Receiver bit enable (0: disable; 1: enable) */
#define CON_STP			(1 <<  3)	/* Number of stop bits (0: 1 stop bit; 1: two stop bits) */
#define CON_MODE_MASK		(7)		/* Mask for mode control */

#define WHBCON_SETOE		(1 << 13)	/* Set overrun error flag */
#define WHBCON_SETFE		(1 << 12)	/* Set framing error flag */
#define WHBCON_SETPE		(1 << 11)	/* Set parity error flag */
#define WHBCON_CLROE		(1 << 10)	/* Clear overrun error flag */
#define WHBCON_CLRFE		(1 <<  9)	/* Clear framing error flag */
#define WHBCON_CLRPE		(1 <<  8)	/* Clear parity error flag */
#define WHBCON_SETREN		(1 <<  5)	/* Set receiver enable bit */
#define WHBCON_CLRREN		(1 <<  4)	/* Clear receiver enable bit */

#define RX_DMA_ENABLE		(1 <<  1)	/* Receive DMA enable (0: disable, 1: enable) */
#define TX_DMA_ENABLE		(1 <<  0)	/* Transmit DMA enable (0: disable, 1: enable) */

#define ISR_TMO			(1 <<  7)	/* RX timeout interrupt mask */
#define ISR_CTS			(1 <<  6)	/* CTS interrupt mask */
#define ISR_ABDET		(1 <<  5)	/* Autobaud detected interrupt mask */
#define ISR_ABSTART		(1 <<  4)	/* Autobaud start interrupt mask */
#define ISR_ERR			(1 <<  3)	/* Error interrupt mask */
#define ISR_RX			(1 <<  2)	/* Receive interrupt mask */
#define ISR_TB			(1 <<  1)	/* Transmit buffer interrupt mask */
#define ISR_TX			(1 <<  0)	/* Transmit interrupt mask */

#define ICR_TMO			(1 <<  7)	/* RX timeout interrupt mask */
#define ICR_CTS			(1 <<  6)	/* CTS interrupt mask */
#define ICR_ABDET		(1 <<  5)	/* Autobaud detected interrupt mask */
#define ICR_ABSTART		(1 <<  4)	/* Autobaud start interrupt mask */
#define ICR_ERR			(1 <<  3)	/* Error interrupt mask */
#define ICR_RX			(1 <<  2)	/* Receive interrupt mask */
#define ICR_TB			(1 <<  1)	/* Transmit buffer interrupt mask */
#define ICR_TX			(1 <<  0)	/* Transmit interrupt mask */

#define FCSTAT_RTS		(1 <<  1)	/* RTS Status (0: inactive; 1: active) */
#define FCSTAT_CTS		(1 <<  0)	/* CTS Status (0: inactive; 1: active) */
#define FCCON_RTS_TRIGGER(x)	((x << 8) & 0x3F00) /* RTS receive FIFO trigger level */
#define FCCON_RTS		(1 <<  4)	/* RTS control bit */
#define FCCON_CTSEN		(1 <<  1)	/* CTS enable (0: disable; 1: enable) */
#define FCCON_RTSEN		(1 <<  0)	/* RTS enbled (0: disable; 1: enable) */

#define ABCON_RXINV		(1 << 11)	/* Receive invert enable (0: disable; 1: enable) */
#define ABCON_TXINV		(1 << 10)	/* Transmit invert enable (0: disable; 1: enable) */
#define ABCON_ABEM_ECHO_DET	(1 <<  8)	/* Autobaud echo mode enabled during detection */
#define ABCON_ABEM_ECHO_ALWAYS	(1 <<  9)	/* Autobaud echo mode always enabled */
#define ABCON_FCDETEN		(1 <<  4)	/* Fir char of two byte frame detect */
#define ABCON_ABDETEN		(1 <<  3)	/* Autobaud detection interrupt enable (0: dis; 1: en) */
#define ABCON_ABSTEN		(1 <<  2)	/* Start of autobaud detect interrupt (0: dis; 1: en) */
#define ABCON_AUREN		(1 <<  1)	/* Auto control of CON.REN (too complex for here) */
#define ABCON_ABEN		(1 <<  0)	/* Autobaud detection enable */

#define ABSTAT_DETWAIT		(1 <<  4)	/* Autobaud detect is waiting */
#define ABSTAT_SCCDET		(1 <<  3)	/* Second character with capital letter detected */
#define ABSTAT_SCSDET		(1 <<  2)	/* Second character with small letter detected */
#define ABSTAT_FCCDET		(1 <<  1)	/* First character with capital letter detected */
#define ABSTAT_FCSDET		(1 <<  0)	/* First character with small letter detected */

#define RXFCON_RXFITL(x)	((x & 8) <<  8)	/* Receive FIFO interrupt trigger level */
#define RXFCON_RXTMEN		(1 <<  2)	/* Receive FIFO transparent mode enable */
#define RXFCON_RXFFLU		(1 <<  1)	/* Receive FIFO flush */
#define RXFCON_RXFEN		(1 <<  0)	/* Receive FIFO enable */

#define TXFCON_TXFITL(x)	((x & 8) <<  8)	/* Transmit FIFO interrupt trigger level */
#define TXFCON_TXTMEN		(1 <<  2)	/* Transmit FIFO transparent mode enable */
#define TXFCON_TXFFLU		(1 <<  1)	/* Transmit FIFO flush */
#define TXFCON_TXFEN		(1 <<  0)	/* Transmit FIFO enable */

#define FSTAT_TXFFL		(0xF <<  8)	/* Transmit FIFO filling level mask */
#define FSTAT_RXFFL		(0xF)		/* Receive FIFO filling level mask */


/* stack */
char stack_top[512 * 1024];
char irq_stack_top[512 * 1024];



/* handlers */
void __IRQ reset_addr() {
	pmb8876_serial_print("\n***** reset_addr! *****\n");
}

void __IRQ undef_addr() {
	pmb8876_serial_print("\n***** undef_addr! *****\n");
}

void __IRQ swi_addr() {
	
}

void __IRQ prefetch_addr() {
	pmb8876_serial_print("\n***** prefetch_addr! *****\n");
	while (1);

	
}
void __IRQ abort_addr() {
	pmb8876_serial_print("\n***** abort_addr! *****\n");
	while (1);
}

void __IRQ reserved_addr() {
	
}

void __IRQ c_irq_handler() {
	
	int irqn = REG(IRQ_CURRENT_NUM);	
	printf("IRQ FIRED: %X\n", irqn);

	// RX
	if(irqn == 0x6) {
		REG(USART0_ICR) = ICR_ERR | ICR_RX;
		
		int last_chr = 0;
		while(1)
		{
			unsigned int fstat = REG(USART0_FSTAT);
			//printf("fstat: %d\n", fstat);
			
			if( (REG(USART0_FSTAT) & FSTAT_RXFFL) ) {
				int ch = REG(USART0_RXB) & 0xff;
				//if( last_chr != ch ) {
					printf(" -> recv: %X(%c), fstat %X(cur %X)\n", ch, ch == 0xd || ch == 0xa? ' ' : ch, fstat, REG(USART0_FSTAT));
					last_chr = ch;
				//}
			} else {
				break;
			}
		}
		
		if( REG(USART0_FSTAT) )
			REG(USART0_ISR) = ISR_RX;
	}
	
	REG(IRQ_ACK) = 1;
}

void __IRQ fiq_test() {
	pmb8876_serial_print("fiq_test!\n");
	while (1);
}


void main() {
	init_watchdog();
	
	int i;
	void **vectors = (void **) 0;
	for (i = 0; i < 8; ++i)
		vectors[i] = (&_cpu_vectors)[i];
	vectors[8] = reset_addr;
	vectors[9] = undef_addr;
	vectors[10] = swi_addr;
	vectors[11] = prefetch_addr;
	vectors[12] = abort_addr;
	vectors[13] = reserved_addr;
	vectors[14] = c_irq_handler; // asm_irq_handler;
	vectors[15] = fiq_test;
	
	unsigned int addr;
	for (addr = 0xf2800030; addr <= 0xf28002a8; ++addr) {
		REG(addr) = 0;
	}
	
	enable_irq(1);
	
	/* charge enable */
	REG(0xF43000D0) = 0x700;
	
	/* set async mode */
	int con = REG(USART0_CON);
	REG( USART0_CON ) = ((con & ~CON_MODE_MASK) | 1);
	
	REG(USART0_RXFCON) = RXFCON_RXFEN | RXFCON_RXFITL(1);
	REG(USART0_TXFCON) = TXFCON_TXFEN | TXFCON_TXFITL(1);
	
	REG(USART0_IMSC) = ISR_RX;
	
	
	//PMB8876_IRQ(4) = 0x8; // TX
	//PMB8876_IRQ(5) = 0x8;
	PMB8876_IRQ(6) = 1; // RX
	//PMB8876_IRQ(7) = 0x8;
	
	printf("Xuj!\n");
	
	while(1) {
		serve_watchdog();
	}
}

