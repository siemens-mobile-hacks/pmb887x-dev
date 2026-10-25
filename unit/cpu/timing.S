@ Each workload runs its body r0 times; r1 points to a 32-byte aligned 128 KiB buffer whose first word holds
@ its own address. Bodies repeat the measured sequence 8 times before SUBS + BNE close the loop.
.syntax unified
.text

.macro arm_loop name
.arm
.align 5
.global \name
.type \name, %function
\name:
	push {r4-r11, lr}
	mov r4, #3
	mov r5, #5
1:
.endm

.macro arm_loop_end
	subs r0, r0, #1
	bne 1b
	pop {r4-r11, pc}
	.ltorg
.endm

.macro thumb_loop name
.thumb
.align 5
.global \name
.type \name, %function
.thumb_func
\name:
	push {r4-r7, lr}
	movs r4, #3
	movs r5, #5
1:
.endm

.macro thumb_loop_end
	subs r0, r0, #1
	bne 1b
	pop {r4-r7, pc}
	.ltorg
.endm

.macro arm_workload name, body:vararg
arm_loop \name
	.rept 8
	\body
	.endr
arm_loop_end
.endm

.macro thumb_workload name, body:vararg
thumb_loop \name
	.rept 8
	\body
	.endr
thumb_loop_end
.endm

arm_workload timing_arm_empty, nop
arm_workload timing_arm_alu, add r2, r2, #1
arm_workload timing_arm_alu_shift_imm, add r2, r2, r3, lsl #2
arm_workload timing_arm_alu_shift_reg, add r2, r2, r3, lsl r4
arm_workload timing_arm_cond_skipped, addeq r2, r2, #1
arm_workload timing_arm_mul, mul r2, r4, r5
arm_workload timing_arm_muls, muls r2, r4, r5
arm_workload timing_arm_mla, mla r2, r4, r5, r3
arm_workload timing_arm_umull, umull r2, r3, r4, r5
arm_workload timing_arm_smlal, smlal r2, r3, r4, r5
arm_workload timing_arm_smulbb, smulbb r2, r4, r5
arm_workload timing_arm_clz, clz r2, r4
arm_workload timing_arm_qadd, qadd r2, r4, r5

arm_loop timing_arm_mul_use
	.rept 8
	mul r2, r4, r5
	add r3, r3, r2
	.endr
arm_loop_end

arm_loop timing_arm_b
	.rept 8
	b 2f
2:
	.endr
arm_loop_end

arm_loop timing_arm_b_not_taken
	.rept 8
	beq 2f
2:
	.endr
arm_loop_end

arm_loop timing_arm_bl
	.rept 8
	bl 2f
2:
	.endr
arm_loop_end

arm_loop timing_arm_bx
	.rept 8
	adr r6, 2f
	bx r6
2:
	.endr
arm_loop_end

arm_loop timing_arm_mov_pc
	.rept 8
	add pc, pc, #0
	nop
	.endr
arm_loop_end

arm_workload timing_arm_ldr, ldr r2, [r1, #4]

arm_loop timing_arm_ldr_use
	.rept 8
	ldr r2, [r1, #4]
	add r3, r3, r2
	.endr
arm_loop_end

arm_loop timing_arm_ldr_use_second
	.rept 8
	ldr r2, [r1, #4]
	add r3, r3, #1
	add r3, r3, r2
	.endr
arm_loop_end

arm_loop timing_arm_ldr_use_address
	.rept 8
	ldr r2, [r1]
	ldr r3, [r2, #4]
	.endr
arm_loop_end

arm_loop timing_arm_ldrb_use
	.rept 8
	ldrb r2, [r1, #4]
	add r3, r3, r2
	.endr
arm_loop_end

arm_loop timing_arm_ldrh_use
	.rept 8
	ldrh r2, [r1, #4]
	add r3, r3, r2
	.endr
arm_loop_end

arm_loop timing_arm_ldr_pc
	.rept 8
	ldr pc, [pc, #-4]
	.word 2f
2:
	.endr
arm_loop_end

arm_workload timing_arm_str, str r2, [r1, #4]
arm_workload timing_arm_strb, strb r2, [r1, #4]
arm_workload timing_arm_ldm1, ldmia r1, {r2}
arm_workload timing_arm_ldm2, ldmia r1, {r2-r3}
arm_workload timing_arm_ldm4, ldmia r1, {r2-r5}
arm_workload timing_arm_ldm8, ldmia r1, {r2-r9}
arm_workload timing_arm_stm1, stmia r1, {r2}
arm_workload timing_arm_stm2, stmia r1, {r2-r3}
arm_workload timing_arm_stm4, stmia r1, {r2-r5}
arm_workload timing_arm_stm8, stmia r1, {r2-r9}

arm_loop timing_arm_ldm_pc
	.rept 8
	adr r6, 3f
	ldmia r6, {r2, pc}
3:
	.word 0
	.word 2f
2:
	.endr
arm_loop_end

arm_workload timing_arm_mrs, mrs r2, cpsr
arm_workload timing_arm_msr_flags, msr cpsr_f, r0

arm_loop timing_arm_msr_control
	mrs r6, cpsr
	.rept 8
	msr cpsr_c, r6
	.endr
arm_loop_end

thumb_workload timing_thumb_alu, adds r2, r2, #1

thumb_loop timing_thumb_b
	.rept 8
	b 2f
2:
	.endr
thumb_loop_end

thumb_loop timing_thumb_b_not_taken
	.rept 8
	beq 2f
2:
	.endr
thumb_loop_end

thumb_loop timing_thumb_bl
	.rept 8
	bl 2f
2:
	.endr
thumb_loop_end

thumb_loop timing_thumb_ldr_use
	.rept 8
	ldr r2, [r1, #4]
	adds r3, r3, r2
	.endr
thumb_loop_end

thumb_loop timing_thumb_ldrh_use
	.rept 8
	ldrh r2, [r1, #4]
	adds r3, r3, r2
	.endr
thumb_loop_end

thumb_workload timing_thumb_ldr_literal, ldr r2, =0x12345678

thumb_loop timing_thumb_push_pop
	.rept 8
	push {r2, r3}
	pop {r2, r3}
	.endr
thumb_loop_end

arm_loop timing_arm_icache_line_invalidate
	.rept 8
	mcr p15, 0, r1, c7, c5, 1
	.endr
arm_loop_end

arm_loop timing_arm_dcache_line_invalidate
	add r6, r1, #65536
	.rept 8
	mcr p15, 0, r6, c7, c6, 1
	.endr
arm_loop_end

arm_loop timing_arm_drain_write_buffer
	.rept 8
	mcr p15, 0, r1, c7, c10, 4
	.endr
arm_loop_end

thumb_loop timing_thumb_call
	.rept 8
	bl 3f
	.endr
	b 4f
3:
	push {lr}
	pop {pc}
4:
thumb_loop_end

arm_loop timing_arm_ldr_str_data
	.rept 8
	ldr r2, [r1, #4]
	str r2, [r1, #8]
	.endr
arm_loop_end

arm_loop timing_arm_ldr_shift_reg
	.rept 8
	ldr r2, [r1, #4]
	add r3, r3, r4, lsl r2
	.endr
arm_loop_end

arm_loop timing_arm_ldrh_use_second
	.rept 8
	ldrh r2, [r1, #4]
	add r3, r3, #1
	add r3, r3, r2
	.endr
arm_loop_end

arm_loop timing_arm_ldr_ldm_base
	.rept 8
	ldr r2, [r1]
	ldmia r2, {r3}
	.endr
arm_loop_end

arm_loop timing_arm_ldm_use
	.rept 8
	ldmia r1, {r2, r3}
	add r6, r6, r3
	.endr
arm_loop_end

arm_loop timing_arm_ldm_use_first
	.rept 8
	ldmia r1, {r2, r3}
	add r6, r6, r2
	.endr
arm_loop_end

arm_loop timing_arm_mul_mla_acc
	.rept 8
	mul r2, r4, r5
	mla r3, r4, r5, r2
	.endr
arm_loop_end

arm_loop timing_arm_mul_use_second
	.rept 8
	mul r2, r4, r5
	add r6, r6, #1
	add r3, r3, r2
	.endr
arm_loop_end

arm_loop timing_arm_ldr_writeback_use
	.rept 8
	ldr r2, [r1], #0
	add r3, r3, r1
	.endr
arm_loop_end

arm_loop timing_arm_bne_taken
	.rept 8
	bne 2f
2:
	.endr
arm_loop_end

thumb_loop timing_thumb_pop_pc_call
	.rept 8
	bl 3f
	.endr
	b 4f
3:
	push {r2, lr}
	pop {r2, pc}
4:
thumb_loop_end

@ 64 KiB copy through D-cache misses, 32 bytes per inner iteration.
arm_loop timing_arm_stream_copy
	mov r10, r1
	add r11, r1, #65536
	mov r12, #2048
2:
	ldmia r10!, {r2-r9}
	stmia r11!, {r2-r9}
	subs r12, r12, #1
	bne 2b
arm_loop_end

@ The firmware's 16-bit color-key blit inner loop (FUN_a0a19a96) over 64 KiB.
thumb_loop timing_thumb_blit
	adds r2, r1, #0
	movs r4, #1
	lsls r4, r4, #15
	adds r3, r1, r4
	adds r3, r3, r4
	ldr r5, =0xE000
2:
	ldrh r6, [r2]
	cmp r6, r5
	beq 3f
	strh r6, [r3]
3:
	adds r2, r2, #2
	adds r3, r3, #2
	subs r4, r4, #1
	bne 2b
thumb_loop_end
