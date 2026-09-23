.arm
.text
.align 2

.global cgu_execute_itcm_source
.type cgu_execute_itcm_source, %function
cgu_execute_itcm_source:
	push {r4, lr}
	mov r12, r0
	mov r0, r1
	mov r1, r2
	mov r2, r3
	ldr r3, =0xF45000A8
	adr r4, .Lsource_resume
	bx r12
.Lsource_resume:
	pop {r4, pc}

	.ltorg

.section .tcm, "ax"
.align 2

.global cgu_itcm_source_loop
.type cgu_itcm_source_loop, %function
cgu_itcm_source_loop:
	str r1, [r3]
	ldr r12, [r3]
.Lsource_measure_loop:
	.rept 512
	nop
	.endr
	subs r0, r0, #1
	bne .Lsource_measure_loop
	str r2, [r3]
	ldr r12, [r3]
	bx r4
