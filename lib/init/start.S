.global _start
.arm
.section .startup, "ax"

.equ PSR_MODE_FIQ, 0x11
.equ PSR_MODE_IRQ, 0x12
.equ PSR_MODE_SVC, 0x13
.equ PSR_MODE_ABT, 0x17
.equ PSR_MODE_UND, 0x1B
.equ PSR_MODE_SYS, 0x1F
.equ PSR_MASKED, 0xC0

_start:
	msr	cpsr_c, #(PSR_MODE_FIQ | PSR_MASKED)
	ldr	sp, =_stack_fiq
	msr	cpsr_c, #(PSR_MODE_IRQ | PSR_MASKED)
	ldr	sp, =_stack_irq
	msr	cpsr_c, #(PSR_MODE_ABT | PSR_MASKED)
	ldr	sp, =_stack_abt
	msr	cpsr_c, #(PSR_MODE_UND | PSR_MASKED)
	ldr	sp, =_stack_und
	msr	cpsr_c, #(PSR_MODE_SVC | PSR_MASKED)
	ldr	sp, =_stack_svc
	msr	cpsr_c, #(PSR_MODE_SYS | PSR_MASKED)
	ldr	sp, =_stack_sys

	@ Clean D-cache and drain the write buffer while the cache is still on
1:	mrc	p15, 0, r15, c7, c10, 3
	bne	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4

	@ Low vectors, I-cache, D-cache and MMU off
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 13)
	bic	r0, r0, #(1 << 12)
	bic	r0, r0, #(1 << 2)
	bic	r0, r0, #(1 << 0)
	mcr	p15, 0, r0, c1, c0, 0
	nop
	nop
	nop

	@ Invalidate I/D caches and TLBs
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0
	mcr	p15, 0, r0, c8, c7, 0

	@ I-cache on; without the MMU the DCache bit has no effect (ARM926EJ-S TRM, Table 4-3)
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #(1 << 12)
	mcr	p15, 0, r0, c1, c0, 0

#if defined(BOOT_INTRAM) || defined(BOOT_EXTRAM) || defined(BOOT_FLASH)
	@ Enable 8 KiB ITCM at 0x01000000
	ldr	r0, =0x01000011
	mcr	p15, 0, r0, c9, c1, 1
#endif

	b	reset_handler

.global _vectors_table_start
.global _vectors_table_end

_vectors_table_start:
	ldr	pc, reset_addr		@ Reset handler
	ldr	pc, undef_addr		@ Undefined instruction handler
	ldr	pc, swi_addr		@ Software interrupt handler
	ldr	pc, prefetch_addr	@ Prefetch abort handler
	ldr	pc, abort_addr		@ Data abort handler
	ldr	pc, reserved_addr	@ Reserved
	ldr	pc, irq_addr		@ IRQ handler
	ldr	pc, fiq_addr		@ FIQ handler
reset_addr:
	.word	reset_handler
undef_addr:
	.word	undef_handler
swi_addr:
	.word	swi_handler
prefetch_addr:
	.word	prefetch_abort_handler
abort_addr:
	.word	data_abort_handler
reserved_addr:
	.word	reserved_handler
irq_addr:
	.word	irq_handler
fiq_addr:
	.word	fiq_handler
_vectors_table_end:
