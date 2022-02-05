.global _start
.arm
.section .text.startup

_start:
	#ifdef BOOT_EXTRAM
	/* chaos bootloader specific */
	ldr	r0, =_last_wdt_serve_from_boot
	str r11, [r0]
	#endif
	
	mrs r0, cpsr
	
	/* stack for sys mode */
	ldr sp, =_stack_sys
	
	/* stack for irq mode */
	bic r1, r0, #0x1f
	orr r1, r1, #0x12
	msr cpsr, r1
	ldr sp, =_stack_irq
	
	/* back to sys mode */
	msr cpsr, r0
	
	bl reset_handler

.global _vectors_table_start
.global _vectors_table_handlers
.global _vectors_table_end

_vectors_table_start:
	ldr pc, reset_addr    @ Reset handler
	ldr pc, undef_addr	  @ Undefined instruction handler
	ldr pc, swi_addr	  @ Software interrupt handler
	ldr pc, prefetch_addr @ Prefetch abort handler
	ldr pc, abort_addr	  @ Data abort handler
	ldr pc, reserved_addr @ Reserved
	ldr pc, irq_addr	  @ IRQ (Interrupt request) handler
	ldr pc, fiq_addr	  @ FIQ (Fast interrupt request) handler
_vectors_table_handlers:
	reset_addr:     .word 0
	undef_addr:     .word 0
	swi_addr:       .word 0
	prefetch_addr:  .word 0
	abort_addr:     .word 0
	reserved_addr:  .word 0
	irq_addr:       .word 0
	fiq_addr:       .word 0
_vectors_table_end:

#ifdef BOOT_EXTRAM
.global _last_wdt_serve_from_boot
_last_wdt_serve_from_boot:
.int 0
#endif