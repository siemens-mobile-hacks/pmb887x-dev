.arm
.text
.align 2

.equ TPU_BASE, 0xF6400000
.equ TPU_RFCON2_OFFSET, 0x14
.equ TPU_RFSSCTB_OFFSET, 0x18
.equ TPU_RFSSC_SRC_OFFSET, 0xE0
.equ TPU_RFCON2_SSCEN, 0x8000
.equ MOD_SRC_SRR, 0x2000
.equ MOD_SRC_CLRR, 0x4000
.equ RF_SSC_POLL_ITERATIONS, 10000

.global rf_ssc_tcm_template_start
.global rf_ssc_tcm_template_end
.type rf_ssc_tcm_template_start, %function
rf_ssc_tcm_template_start:
	push {r4, r5, lr}
	ldr r12, .Ltpu_base
	ldr r4, .Lpoll_iterations
.Lwait_idle:
	ldr r5, [r12, #TPU_RFCON2_OFFSET]
	tst r5, #TPU_RFCON2_SSCEN
	beq .Lidle
	subs r4, r4, #1
	bne .Lwait_idle
	b .Lfailed

.Lidle:
	str r0, [r12, #TPU_RFCON2_OFFSET]
	str r1, [r12, #TPU_RFSSCTB_OFFSET]
	cmp r3, #0
	beq .Lwait_request

	ldr r0, .Lpoll_iterations
.Lpoll_first_request:
	ldr r1, [r12, #TPU_RFSSC_SRC_OFFSET]
	tst r1, #MOD_SRC_SRR
	bne .Lfirst_request_ready
	subs r0, r0, #1
	bne .Lpoll_first_request
	b .Lfailed

.Lfirst_request_ready:
	orr r1, r1, #MOD_SRC_CLRR
	str r1, [r12, #TPU_RFSSC_SRC_OFFSET]
	str r2, [r12, #TPU_RFSSCTB_OFFSET]

.Lwait_request:
	ldr r0, .Lpoll_iterations
.Lpoll_request:
	ldr r1, [r12, #TPU_RFSSC_SRC_OFFSET]
	tst r1, #MOD_SRC_SRR
	bne .Lrequest_ready
	subs r0, r0, #1
	bne .Lpoll_request
	b .Lfailed

.Lrequest_ready:
	orr r1, r1, #MOD_SRC_CLRR
	str r1, [r12, #TPU_RFSSC_SRC_OFFSET]
	ldr r0, .Lpoll_iterations
.Lwait_complete:
	ldr r1, [r12, #TPU_RFCON2_OFFSET]
	tst r1, #TPU_RFCON2_SSCEN
	beq .Lsucceeded
	subs r0, r0, #1
	bne .Lwait_complete

.Lfailed:
	mov r0, #0
	pop {r4, r5, pc}

.Lsucceeded:
	mov r0, #1
	pop {r4, r5, pc}

.align 2
.Ltpu_base:
	.word TPU_BASE
.Lpoll_iterations:
	.word RF_SSC_POLL_ITERATIONS
rf_ssc_tcm_template_end:
