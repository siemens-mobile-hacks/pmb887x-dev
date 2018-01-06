.global __enter_reset
.arm
.section .text.init

__enter_reset:
	ldr sp, =sp_addr_sys_mode
	mrs r0, cpsr
	bic r1, r0, #0x1f
	orr r1, r1, #0x12
	msr cpsr, r1
	ldr sp, =sp_addr_irq_mode
	bic r1, r0, #0x1f
	orr r1, r1, #0x17
	msr cpsr, r1
	ldr sp, =sp_addr_irq_mode
	msr cpsr, r0
	bl __init_vectors
	bl main
	
endless_loop:
	b	endless_loop

.global _cpu_vectors
_cpu_vectors:
	ldr pc, reset_addr    @ Reset handler
	ldr pc, undef_addr	  @ Undefined instruction handler
	ldr pc, swi_addr	  @ Software interrupt handler
	ldr pc, prefetch_addr @ Prefetch abort handler
	ldr pc, abort_addr	  @ Data abort handler
	ldr pc, reserved_addr @ Reserved
	ldr pc, irq_addr	  @ IRQ (Interrupt request) handler
	ldr pc, fiq_addr	  @ FIQ (Fast interrupt request) handler
	reset_addr:     .word 0
	undef_addr:     .word 0
	swi_addr:       .word 0
	prefetch_addr:  .word 0
	abort_addr:     .word 0
	reserved_addr:  .word 0
	irq_addr:       .word 0
	fiq_addr:       .word 0

sp_addr_sys_mode: .word 0x97800
sp_addr_irq_mode: .word 0x82000
