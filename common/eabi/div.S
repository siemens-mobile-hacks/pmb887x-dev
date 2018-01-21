.arm
.section .text.__aeabi_uidivmod
.global	__aeabi_uidivmod

__aeabi_uidivmod:
	stmfd	sp!, {r0, r1, ip, lr}
	bl	__aeabi_uidiv
	ldmfd	sp!, {r1, r2, ip, lr}
	mul	r3, r0, r2
	sub	r1, r1, r3
	mov	pc, lr

.arm
.section .text.__aeabi_uldivmod
.global	__aeabi_uldivmod

__aeabi_uldivmod:
	push	{r11, lr}
	sub	sp, sp, #16
	add	r12, sp, #8
	str	r12, [sp]
	bl	__udivmoddi4
	ldr	r2, [sp, #8]
	ldr	r3, [sp, #12]
	add	sp, sp, #16
	pop	{r11, pc}
