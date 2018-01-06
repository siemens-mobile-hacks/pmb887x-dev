
	.global __delay
	.arm
__delay:
	subs	r0, r0, #1
	bhi	__delay
	bx	lr
