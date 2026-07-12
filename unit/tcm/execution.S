.arm
.text
.align 2

.global tcm_execute_with_resume
.type tcm_execute_with_resume, %function
tcm_execute_with_resume:
	push {r4, lr}
	adr r4, .Lresume
	bx r0
.Lresume:
	pop {r4, pc}
