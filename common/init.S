.section .xuj0
.align 4
.extern _start

@ all access?
@ MRS		R2, CPSR
@ MRS		R1, CPSR
@ ORR		R1, R1,#0xC0
@ MSR		CPSR_c, R1
@ MOV		R0, #0xFFFFFFFF
@ MCR		p15, 0, R0, c3, c0 
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ NOP
@ MSR	   CPSR_c,R2

@ Init SP
@ MSR     CPSR_c, #0xDB
@ LDR     SP, sp_addr_undef_mode

@ MSR     CPSR_c, #0xD7
@ LDR     SP, sp_addr_abt_mode

@ MSR     CPSR_c, #0xD1
@ LDR     SP, sp_addr_fiq_mode

MSR     CPSR_c, #0xD2
LDR     SP, sp_addr_irq_mode

@ MSR     CPSR_c, #0xD3
@ LDR     SP, sp_addr_svc_mode

MSR     CPSR_c, #0xDF
LDR     SP, sp_addr_sys_mode

b _start

stack_addr: .word 0x81000

sp_addr_sys_mode: .word 0x81400
sp_addr_irq_mode: .word 0x81800

@ SIEMENS
@ sp_addr_sys_mode: .word 0x82A4C ; 0x80
@ sp_addr_und_mode: .word 0x82ACC ; 0x80
@ sp_addr_abt_mode: .word 0x82B4C ; 0x100
@ sp_addr_fiq_mode: .word 0x82C4C ; 0x400
@ sp_addr_irq_mode: .word 0x8304C ; 0x200
@ sp_addr_svc_mode: .word 0x8324C ; ???

.section .xuj2
.align 4

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

@ IRQ regs
IRQ_NUM: .word 0xF280001C
IRQ_ACK: .word 0xF2800014
