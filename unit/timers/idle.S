/* ARM926 idle from the documented register semantics: IRQs are off on entry,
   the write buffer is drained, FIQ is masked, the control register's I bit is
   cleared, then the core waits and both registers are restored. */
.arm
.section .text.idle_wait_for_interrupt, "ax"
.align 2

.global idle_wait_for_interrupt
.type idle_wait_for_interrupt, %function
idle_wait_for_interrupt:
	mov	r0, #0
	mrc	p15, 0, ip, c1, c0, 0
	mcr	p15, 0, r0, c7, c10, 4
	mrs	r3, cpsr
	orr	r2, r3, #64
	msr	cpsr_c, r2
	bic	r2, ip, #4096
	mcr	p15, 0, r2, c1, c0, 0
	mcr	p15, 0, r0, c7, c0, 4
	mcr	p15, 0, ip, c1, c0, 0
	msr	cpsr_c, r3
	bx	lr
.size idle_wait_for_interrupt, . - idle_wait_for_interrupt
