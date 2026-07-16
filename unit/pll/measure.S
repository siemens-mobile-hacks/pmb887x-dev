.arm
.text
.align 2

.global pll_execute_itcm
.type pll_execute_itcm, %function
pll_execute_itcm:
	push {r4, lr}
	mov r2, r0
	mov r0, r1
	adr r4, .Lresume
	bx r2
.Lresume:
	pop {r4, pc}

.global pll_execute_itcm_source
.type pll_execute_itcm_source, %function
pll_execute_itcm_source:
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

.global pll_execute_itcm_cpu_div
.type pll_execute_itcm_cpu_div, %function
pll_execute_itcm_cpu_div:
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

.align 2
.global pll_cpu_div_itcm_template_start
.global pll_cpu_div_itcm_template_end
pll_cpu_div_itcm_template_start:
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
pll_cpu_div_itcm_template_end:
