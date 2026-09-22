#ifdef BOOT_TCM
.global tcm_relocate
.arm
.section .boot.relocate, "ax"

tcm_relocate:
	ldr	r0, =0x01000011
	mcr	p15, 0, r0, c9, c1, 1
	ldr	r0, =_image_load_start
	ldr	r1, =_image_start
	ldr	r2, =_image_end

1:
	cmp	r1, r2
	ldrcc	r3, [r0], #4
	strcc	r3, [r1], #4
	bcc	1b

2:
	mrc	p15, 0, r15, c7, c10, 3
	nop
	nop
	nop
	bne	2b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4
	mcr	p15, 0, r0, c7, c5, 0
	ldr	pc, =_start
#endif
