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
