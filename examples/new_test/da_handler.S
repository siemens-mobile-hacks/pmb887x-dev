.global data_abort_handler
.align 4
data_abort_handler:
	# save pc
	ldr sp, =data_abort_context
	str lr, [sp, #15 * 4]
	
	# save r0 .. r12
	ldr sp, =data_abort_context
	stmia sp, {r0-r12}
	
	# call handler
	ldr sp, =data_abort_stack
	add sp, sp, #0x4000
	bl data_abort_handler2
	
	# restore r0 .. r12
	ldr sp, =data_abort_context
	ldmia sp, {r0-r12}
	
	# restore pc
	ldr sp, =data_abort_context
	ldr lr, [sp, #15 * 4]
	
	sub lr, lr, #4
	movs pc, lr
