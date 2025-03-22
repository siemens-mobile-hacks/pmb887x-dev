.arm

@ r0 = command
@ r1 = flash base
.global send_flash_cmd2
send_flash_cmd2:
	MOV	R3, #0xAA
	ADD	R2, R1,	#0xA00 @ 0xA0000A00
	STRH	R3, [R2,#0xAA] @ 0xA0000AAA

	MOV	R3, #0x55
	ADD	R2, R1,	#0x500 @ 0xA0000500
	STRH	R3, [R2,#0x54] @ 0xA0000554

	ADD	R2, R1,	#0xA00 @ 0xA0000A00
	STRH	R0, [R2,#0xAA] @ 0xA0000AAA

	BX	LR
