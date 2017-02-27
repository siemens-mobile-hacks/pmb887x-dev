#include <printf.h>
#include "main.h"
 


/* handlers */
__attribute__ ((weak))
void __IRQ reset_addr() {
	//pmb8876_serial_print("\n***** reset_addr! *****\n");
}

__attribute__ ((weak))
void __IRQ undef_addr() {
	//pmb8876_serial_print("\n***** undef_addr! *****\n");
}

__attribute__ ((weak))
void __IRQ swi_addr() {
	
}

__attribute__ ((weak))
void __IRQ prefetch_addr() {
	//pmb8876_serial_print("\n***** prefetch_addr! *****\n");
	//while (1);
}

__attribute__ ((weak))
void __IRQ abort_addr() {
	int lr;
	asm("mov %0, lr" : "=r"(lr));
	
	enable_irq(0);
	printf("\n***** abort_addr! %X *****\n", lr);
	enable_irq(1);
	//while (1);
}

__attribute__ ((weak))
void __IRQ reserved_addr() {
	
}

__attribute__ ((weak))
void __IRQ c_irq_handler() {
	
	int irqn = REG(IRQ_CURRENT_NUM);
	
	printf("IRQ FIRED: %X\n", irqn);
	
	if(irqn == 0x77) {
		GSM_CON_SET( GSM_CON() | PMB8876_GSM_TPU_CON_RESET );
	}
	
	REG(IRQ_ACK) = 1;
}

__attribute__ ((weak))
void __IRQ fiq_test() {
	enable_irq(0);
	pmb8876_serial_print("fiq_test!\n");
	while (1);
}



