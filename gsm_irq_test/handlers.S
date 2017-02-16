

	.global __enter_reset
	.arm
	.section .text.init
__enter_reset:
	
	/* set last watch dog time */
	ldr	r0, =last_watchdog_time
	str r12, [r0]
	
	/* set supervisor stack */
	ldr sp, =stack_top
	add sp, #0x70000

	/* get program status register */
	mrs r0, cpsr
	/* go in irq mode */
	bic r1, r0, #0x1f
	orr r1, r1, #0x12
	msr cpsr, r1
	/* set irq stack */
	ldr sp, =irq_stack_top
	add sp, #0x70000
	/* enable irqs */
	bic r0, r0, #0x80
	/* go back in supervisor mode */
	msr cpsr, r0
	bl 	main
	
endless_loop:
	b	endless_loop
	
	.align 4
	.global _cpu_vectors
	.section .text
_cpu_vectors:
	ldr pc, reset_addr    /* Reset handler */
	ldr pc, undef_addr	  /* Undefined instruction handler */
	ldr pc, swi_addr	  /* Software interrupt handler */
	ldr pc, prefetch_addr /* Prefetch abort handler */
	ldr pc, abort_addr	  /* Data abort handler */
	ldr pc, reserved_addr /* Reserved */
	ldr pc, irq_addr	  /* IRQ (Interrupt request) handler */
	ldr pc, fiq_addr	  /* FIQ (Fast interrupt request) handler */
	reset_addr:     .word 0
	undef_addr:     .word 0
	swi_addr:       .word 0
	prefetch_addr:  .word 0
	abort_addr:     .word 0
	reserved_addr:  .word 0
	irq_addr:       .word 0
	fiq_addr:       .word 0

	.global last_watchdog_time
last_watchdog_time: 
	.int 0
