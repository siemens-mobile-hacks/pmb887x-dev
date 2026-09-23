.arm
.text
.align 2

.global cgu_execute_itcm
.type cgu_execute_itcm, %function
cgu_execute_itcm:
	push {r4, lr}
	mov r2, r0
	mov r0, r1
	adr r4, .Lresume
	bx r2
.Lresume:
	pop {r4, pc}

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

.global cgu_execute_itcm_cpu_div
.type cgu_execute_itcm_cpu_div, %function
cgu_execute_itcm_cpu_div:
	push {r4-r6, lr}
	mov r5, r0
	mov r0, r1
	mov r1, r2
	mov r2, r3
	ldr r3, =0xF45000AC
	ldr r12, =0xF4400044
	adr r4, .Lcpu_div_resume
	bx r5
.Lcpu_div_resume:
	pop {r4-r6, pc}
	.ltorg

.section .tcm, "ax"
.align 2

.global cgu_itcm_loop
.type cgu_itcm_loop, %function
cgu_itcm_loop:
.Litcm_measure_loop:
	.rept 512
	nop
	.endr
	subs r0, r0, #1
	bne .Litcm_measure_loop
	bx r4

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

.global cgu_itcm_cpu_div_loop
.type cgu_itcm_cpu_div_loop, %function
cgu_itcm_cpu_div_loop:
	ldr r6, [r12]
	bic r5, r6, #1
	str r5, [r12]
.Lebu_disable_wait:
	ldr r5, [r12]
	tst r5, #0x10
	beq .Lebu_disable_wait
	str r1, [r3]
	ldr r5, [r3]
.Lcpu_measure_loop:
	.rept 512
	nop
	.endr
	subs r0, r0, #1
	bne .Lcpu_measure_loop
	str r2, [r3]
	ldr r5, [r3]
	str r6, [r12]
.Lebu_restore_wait:
	ldr r5, [r12]
	tst r5, #0x10
	beq .Lebu_restore_wait
	bx r4
