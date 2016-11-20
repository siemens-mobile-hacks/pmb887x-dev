.section .xuj0
.align 4
.extern _start

  MRS	   R2,  CPSR

  MRS      R1, CPSR
  ORR      R1, R1,#0xC0
  MSR      CPSR_c, R1

	  MOV      R0, #0xFFFFFFFF
	  MCR      p15, 0, R0, c3, c0 
	  
	  NOP
  NOP
  NOP
  NOP
	  NOP
  NOP
  NOP
  NOP
	  NOP
  NOP
  NOP
  NOP
	  NOP
  NOP
  NOP
  NOP
	  
  MSR	   CPSR_c,R2
  
ldr SP, stack_addr
b _start

stack_addr: .word 0x81000

.section .xuj2
.align 4

.global _cpu_vectors
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
