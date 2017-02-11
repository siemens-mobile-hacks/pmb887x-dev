.section .text
.arm

B	start

@ ---------------------------------------------------------------------------
	.long 0, 0, 0, 0, 0, 0, 0, 0, 0
	.ascii "SIEMENS_BOOTCODE"
	.long 3, 0
	.long 0, 0, 0, 0 @ bootkey
	.long 0, 0, 0, 0
@ ---------------------------------------------------------------------------

start:
	MRS	R0, CPSR
	ORR	R0, R0,	#0xC0
	MSR	CPSR_c,	R0
	
	MOV	R0, #0
	BL	set_einit
	
	LDR	R1, =0xF4400000
	LDR	R0, [R1,#0x7C]
	BIC	R0, R0,	#1
	STR	R0, [R1,#0x7C]
	MOV	R0, #8
	STR	R0, [R1,#0x28]
	
	MOV	R0, #1
	BL	set_einit
	
	MOV	R10, #0x3E00
	BL	sub_9AC
	
	MOV	R0, #0xA5
	BL	tx_byte

loc_A4:
	BL	rx_byte
	CMP	R0, #0x41 @ 'A'
	BEQ	loc_BC
	CMP	R0, #0x2E @ '.'
	BLEQ	serve_watchdog
	B	loc_A4

loc_BC:
	BL	sub_6F4
	BL	sub_8E4

cmd_ping:
	MOV	R0, #0x52 @ 'R'
	BL	tx_byte

loop:
	BL	rx_byte
	CMP	R0, #0x41 @ 'A'
	BEQ	cmd_ping
	CMP	R0, #0x48 @ 'H'
	BEQ	cmd_speed
	CMP	R0, #0x49 @ 'I'
	BEQ	cmd_info
	CMP	R0, #0x51 @ 'Q'
	BEQ	cmd_quit
	CMP	R0, #0x54 @ 'T'
	BEQ	cmd_test
	CMP	R0, #0x52 @ 'R'
	BEQ	cmd_read
	CMP	R0, #0x46 @ 'F'
	BEQ	cmd_flash
	CMP	R0, #0x43 @ 'C'
	BEQ	read_cfi
	CMP	R0, #0x2E @ '.'
	BLEQ	serve_watchdog
	B	loop

cmd_test:
	BL	sub_5B4

loc_120:
	LDMIA	R6!, {R0,R1}
	AND	R0, R0,	R1
	CMN	R0, #1
	MOVNE	R0, #0
	BNE	loc_13C
	SUBS	R7, R7,	#8
	BNE	loc_120

loc_13C:
	BL	tx_byte
	B	loop

read_cfi:
	BL	sub_5B4
	BL	sub_920
	SUBS	R6, R10, #0x10
	STR	R0, [R6]
	MOV	R7, #4
	B	loc_1E0

cmd_read:
	BL	sub_5B4
	MOV	R8, #0

loc_164:
	LDRB	R0, [R6],#1
	EOR	R8, R8,	R0
	BL	tx_byte
	SUBS	R7, R7,	#1
	BNE	loc_164
	MOV	R0, #0x4F
	BL	tx_byte
	MOV	R0, #0x4B
	BL	tx_byte
	MOV	R0, R8
	BL	tx_byte
	MOV	R0, #0
	BL	tx_byte
	B	loop

cmd_speed:
	BL	rx_byte
	MOV	R6, R0
	MOV	R0, #0x68
	BL	tx_byte
	MOV	R0, #0xA00

loc_1B0:
	SUBS	R0, R0,	#1
	BNE	loc_1B0
	MOV	R0, R6
	BL	set_uart_speed

loc_1C0:
	BL	rx_byte
	CMP	R0, #0x41
	BNE	loc_1C0
	MOV	R0, #0x48
	BL	tx_byte
	B	loop

cmd_info:
	MOV	R6, R10
	MOV	R7, #0x80

loc_1E0:
	LDRB	R0, [R6],#1
	BL	tx_byte
	SUBS	R7, R7,	#1
	BNE	loc_1E0
	B	loop

cmd_flash:
	BL	sub_5B4
	MOV	R4, #0xA8000000
	MOV	R5, R7
	MOV	R8, #0

loc_204:
	BL	serve_watchdog
	BL	rx_byte
	EOR	R8, R8,	R0
	STRB	R0, [R4],#1
	SUBS	R5, R5,	#1
	BNE	loc_204
	BL	rx_byte
	CMP	R0, R8
	BEQ	loc_23C
	MOV	R0, #0xBB
	BL	tx_byte
	MOV	R0, #0xBB
	BL	tx_byte
	B	loop

loc_23C:
	MOV	R0, #0x40000
	SUB	R0, R0, #1
	
	MOV	R1, R6
	ADD	R2, R6,	R7
	SUB	R2, R2,	#1
	BIC	R1, R1,	R0
	BIC	R2, R2,	R0
	CMP	R1, R2
	BEQ	loc_274
	MOV	R0, #0xFF
	BL	tx_byte
	MOV	R0, #0xFF
	BL	tx_byte
	B	loop

loc_274:
	MOV	R8, #0xA8000000
	LDRH	R0, [R10,#0x50]
	CMP	R0, #0x89
	CMPNE	R0, #0x20
	BEQ	loc_2B8
	CMP	R0, #1
	CMPNE	R0, #4
	BEQ	loc_3D0
	MOV	R0, #0xCC
	BL	tx_byte
	MOV	R0, #0xCC
	BL	tx_byte
	LDRH	R0, [R10,#0x50]
	BL	tx_byte
	LDRH	R0, [R10,#0x52]
	BL	tx_byte
	B	loop

loc_2B8:
	MOV	R0, #1
	BL	tx_byte
	MOV	R0, #1
	BL	tx_byte
	MOV	R0, #0x60
	STRH	R0, [R6]
	MOV	R0, #0xD0
	STRH	R0, [R6]
	NOP
	MOV	R0, #0x20
	STRH	R0, [R6]
	MOV	R0, #0xD0
	STRH	R0, [R6]

loc_2EC:
	BL	serve_watchdog
	MOV	R0, #0x70
	STRH	R0, [R6]
	LDRH	R0, [R6]
	TST	R0, #0x80
	BEQ	loc_2EC
	TST	R0, #0x3A
	BNE	loc_3B0
	MOV	R0, #0xFF
	STRH	R0, [R6]
	MOV	R0, #2
	BL	tx_byte
	MOV	R0, #2
	BL	tx_byte
	LDRB	R0, [R10,#0x55]
	MOV	R3, #1
	MOV	R3, R3,LSL R0
	MOV	R3, R3,LSR#1
	MOV	R4, R6
	MOV	R5, R7,LSR#1

loc_33C:
	BL	serve_watchdog
	LDRB	R0, [R10,#0x56]
	CMP	R0, #2
	MOVNE	R0, #0xE8
	MOVEQ	R0, #0xE9
	STRH	R0, [R4]
	SUB	R0, R3,	#1
	STRH	R0, [R4]
	MOV	R1, R3

loc_360:
	LDRH	R0, [R8],#2
	STRH	R0, [R4],#2
	SUBS	R1, R1,	#1
	BNE	loc_360
	MOV	R0, #0xD0
	STRH	R0, [R6]

loc_378:
	LDRH	R0, [R6]
	TST	R0, #0x80
	BEQ	loc_378
	TST	R0, #0x3A
	BNE	loc_3B0
	SUBS	R5, R5,	R3
	BHI	loc_33C
	MOV	R0, #0xFF
	STRH	R0, [R6]
	MOV	R0, #3
	BL	tx_byte
	MOV	R0, #3
	BL	tx_byte
	B	loc_4B0

loc_3B0:
	MOV	R0, #0x50
	STRH	R0, [R6]
	MOV	R0, #0xFF
	STRH	R0, [R6]
	BL	tx_byte
	MOV	R0, #0xFF
	BL	tx_byte
	B	loop

loc_3D0:
	MOV	R0, #1
	BL	tx_byte
	MOV	R0, #1
	BL	tx_byte
	MOV	R1, R6
	MOV	R0, #0xE0
	BL	sub_974
	MOV	R0, #0xA0
	STRH	R0, [R1]
	MOV	R0, #1
	STRH	R0, [R1]
	MOV	R0, #0x90
	STRH	R0, [R1]
	MOV	R0, #0
	STRH	R0, [R1]
	MOV	R1, R6
	MOV	R0, #0x80
	BL	sub_974
	MOV	R0, #0x30
	BL	sub_974

loc_420:
	BL	serve_watchdog
	LDR	R0, [R6]
	CMN	R0, #1
	BNE	loc_420
	MOV	R0, #2
	BL	tx_byte
	MOV	R0, #2
	BL	tx_byte
	MOV	R1, R6
	MOV	R0, #0x20 @ ' '
	BL	sub_974
	MOV	R4, R6
	MOV	R5, R7,LSR#1

loc_454:
	BL	serve_watchdog
	MOV	R0, #0xA0
	STRH	R0, [R6]
	LDRH	R0, [R8]
	STRH	R0, [R4]

loc_468:
	LDRH	R1, [R4]
	CMP	R1, R0
	BNE	loc_468
	ADD	R8, R8,	#2
	ADD	R4, R4,	#2
	SUBS	R5, R5,	#1
	BNE	loc_454
	MOV	R0, #0x90
	STRH	R0, [R6]
	MOV	R0, #0
	STRH	R0, [R6]
	MOV	R0, #0xF0
	STRH	R0, [R6]
	MOV	R0, #3
	BL	tx_byte
	MOV	R0, #3
	BL	tx_byte
	B	loc_4B0

loc_4B0:
	MOV	R8, #0
	MOV	R1, R6
	MOV	R2, R7,LSR#1

loc_4BC:
	LDRH	R0, [R1],#2
	ADD	R8, R8,	R0
	SUBS	R2, R2,	#1
	BNE	loc_4BC
	MOV	R0, R8
	BL	tx_byte
	MOV	R0, R8,LSR#8
	BL	tx_byte
	MOV	R0, #0x4F @ 'O'
	BL	tx_byte
	MOV	R0, #0x4B @ 'K'
	BL	tx_byte
	B	loop

set_uart_speed:
	ADR	R1, speeds_list
	LDR	R1, [R1,R0,LSL#2]
	MOV	R2, #0xF1000000
	MOV	R0, R1,LSR#16
	STR	R0, [R2,#0x14]
	MOV	R0, R1,LSL#16
	MOV	R0, R0,LSR#16
	STR	R0, [R2,#0x18]
	BX	LR

@ ---------------------------------------------------------------------------
speeds_list:
	.long 0x001901d8 @ 57600/0x1d8=122.033898*0x1a=3172.881348
	.long 0x000c01d8 @ 115200/0x1d8=244.067797*13=3172.881361
	.long 0x000501b4 @ 230400/0x1b4=528.440367*0x6=3170.642202
	.long 0x00000092 @ 460800/0x92=3156.164384
	.long 0x000000c3 @ 614400/0xc3=3150.769231
	.long 0x00000127 @ 921600/0x127=3124.067797
	.long 0x0000018a @ 1228800/0x18a=3118.781726
	.long 0x00000000 @ 1600000/0x200=3125
	.long 0x000001d0 @ 1500000/0x1d0=3232.758621
@ ---------------------------------------------------------------------------

tx_byte:
	MOV	R2, #0xF1000000
	LDR	R1, [R2,#0x20]
	BIC	R1, R1,	#0xFF
	ORR	R1, R0,	R1
	STR	R1, [R2,#0x20]

loc_548:
	LDR	R1, [R2,#0x68]
	ANDS	R1, R1,	#2
	BEQ	loc_548
	LDR	R1, [R2,#0x70]
	ORR	R1, R1,	#2
	STR	R1, [R2,#0x70]
	B	serve_watchdog

rx_byte:
	MOV	R1, #0xF1000000
	LDR	R0, [R1,#0x68]
	ANDS	R0, R0,	#4
	BEQ	rx_byte
	LDR	R0, [R1,#0x70]
	ORR	R0, R0,	#4
	STR	R0, [R1,#0x70]
	LDR	R0, [R1,#0x24]
	AND	R0, R0,	#0xFF
	BX	LR

sub_58C:
	MOV	R3, LR
	BL	rx_byte
	MOV	R4, R0,LSL#24
	BL	rx_byte
	ORR	R4, R4,	R0,LSL#16
	BL	rx_byte
	ORR	R4, R4,	R0,LSL#8
	BL	rx_byte
	ORR	R0, R4,	R0
	BX	R3

sub_5B4:
	MOV	R5, LR
	BL	sub_58C
	MOV	R6, R0
	CMP	R6, #0x10000000
	ADDCS	R6, R0,	R9
	BL	sub_58C
	MOV	R7, R0
	BX	R5

set_einit:
	LDR	R3, =0xF4400000
	LDR	R1, [R3,#0x24]
	BIC	R1, R1,	#0xE
	ORR	R1, R1,	#0xF0
	LDR	R2, [R3,#0x28]
	AND	R2, R2,	#0xC
	ORR	R1, R1,	R2
	STR	R1, [R3,#0x24]
	BIC	R1, R1,	#0xD
	ORR	R1, R1,	#2
	ORR	R0, R0,	R1
	STR	R0, [R3,#0x24]
	BX	LR

sub_608:
	ADD	R1, R5,	#0x31
	LDR	R0, =0x522600
	STR	R1, [R6]
	STR	R0, [R6,#0x40]
	MOV	R0, #0x98
	ADD	R1, R5,	#0xA00
	STRH	R0, [R1,#0xAA]
	NOP
	LDRH	R0, [R5,#0x20]
	CMP	R0, #0x51
	BNE	loc_6E4
	LDRH	R0, [R5,#0x22]
	CMP	R0, #0x52
	BNE	loc_6E4
	LDRB	R0, [R10,#0x54]
	ADD	R0, R0,	#1
	STRB	R0, [R10,#0x54]
	LDRH	R0, [R5,#0x54]
	STRB	R0, [R10,#0x55]
	LDRH	R0, [R5,#0x28]
	STRB	R0, [R10,#0x56]
	LDRB	R2, [R10,#0x57]
	ADD	R2, R10, R2,LSL#2
	ADD	R2, R2,	#0x58
	ADD	R3, R5,	#0x5A

loc_66C:
	LDRH	R0, [R3],#2
	STRB	R0, [R2],#1
	AND	R0, R3,	#0xFF
	CMP	R0, #0x7A @ 'z'
	BNE	loc_66C
	LDRH	R0, [R5,#0x58]
	LDRB	R2, [R10,#0x57]
	ADD	R2, R2,	R0
	STRB	R2, [R10,#0x57]
	LDRH	R0, [R5,#0x4E]
	CMP	R0, #0x18
	BCC	loc_6E4
	ADDEQ	R1, R5,	#0x1000000
	ADDEQ	R2, R5,	#0x31
	CMP	R0, #0x19
	ADDEQ	R1, R5,	#0x2000000
	ADDEQ	R2, R5,	#0x21
	CMP	R0, #0x1A
	ADDEQ	R1, R5,	#0x4000000
	ADDEQ	R2, R5,	#0x11
	CMP	R0, #0x1B
	BHI	loc_6E4
	ADDEQ	R1, R5,	#0x8000000
	ADDEQ	R2, R5,	#1
	MOV	R0, #0xFF
	STRH	R0, [R5]
	STR	R2, [R6]
	MOV	R5, R1
	CMP	R0, R0
	BX	LR

loc_6E4:
	MOV	R0, #0
	STR	R0, [R6]
	CMP	R0, #1
	BX	LR

sub_6F4:
	MOV	R7, LR
	MOV	R9, #0x10000000
	MOV	R1, #0xF0000000
	
	MOV	R0, #0xA8000000  
	ADD	R0, R0, #0x41
	
	STR	R0, [R1,#0x88]
	LDR	R0, =0x30720200
	STR	R0, [R1,#0xC8]
	MOV	R0, #6
	STR	R0, [R1,#0x40]
	LDR	R0, =0x891C70
	STR	R0, [R1,#0x50]
	MOV	R0, #0x23
	STR	R0, [R1,#0x60]

loc_72C:
	MOV	R1, #0xF0000000
	LDR	R0, =0xA0000011
	STR	R0, [R1,#0x80]
	LDR	R0, =0x522600
	STR	R0, [R1,#0xC0]
	LDR	R2, =0x534C
	LDR	R0, =0xA0000200
	LDRH	R1, [R0,#2]
	CMP	R1, R2
	BNE	loc_774
	LDRH	R8, [R0]
	LDR	R4, =0xA000065C
	LDR	R5, =0xA0000238
	LDR	R6, =0xA0000210
	ANDS	R0, R8,	#2
	ADDNE	R4, R4,	#4
	ADDNE	R5, R5,	#4
	B	loc_794

loc_774:
	LDR	R0, =0xA0001200
	LDRH	R1, [R0,#2]
	CMP	R1, R2
	BNE	loc_7BC
	LDRH	R8, [R0]
	LDR	R4, =0xA003E410
	LDR	R5, =0xA003E400
	LDR	R6, =0xA003E000

loc_794:
	MOV	R1, R10
	MOV	R0, R6
	MOV	R2, #0x20
	BL	sub_960
	MOV	R0, R4
	MOV	R2, #0x10
	BL	sub_960
	MOV	R0, R5
	MOV	R2, #0x10
	BL	sub_960

loc_7BC:
	ADD	R1, R10, #0x40
	MOV	R2, #0x80
	MOV	R0, #0

loc_7C8:
	STR	R0, [R1],#4
	SUBS	R2, R2,	#4
	BNE	loc_7C8
	MOV	R1, #0xA0000000
	STR	R1, [R10,#0x40]
	STRH	R8, [R10,#0x48]
	MOV	R0, #0x90
	BL	sub_974
	LDRH	R0, [R1]
	STRH	R0, [R10,#0x50]
	BL	sub_998
	LDRH	R0, [R1]
	LDRH	R2, [R10,#0x50]
	SUBS	R0, R0,	R2
	streqh	R0, [R10,#0x50]
	streqh	R0, [R10,#0x52]
	BEQ	loc_850
	MOV	R0, #0x90
	BL	sub_974
	LDRH	R0, [R1,#2]
	STRH	R0, [R10,#0x52]
	BL	sub_998
	ADD	R5, R9,	#0xA0000000
	LDR	R6, =0xF0000080
	BL	sub_608
	BNE	loc_850
	CMP	R5, #0xB2000000
	MOVEQ	R9, #0x12000000
	BEQ	loc_72C
	LDR	R6, =0xF0000090
	BL	sub_608
	BNE	loc_850
	LDR	R6, =0xF0000098
	BL	sub_608

loc_850:
	ADD	R1, R9,	#0xA0000000
	ADD	R5, R10, #0x80
	LDRH	R0, [R10,#0x50]
	CMP	R0, #0x89
	CMPNE	R0, #0x20
	ADDEQ	R1, R1,	#0x100
	ADDEQ	R1, R1,	#2
	MOVEQ	R0, #0x90
	MOVEQ	R4, #0x10
	BEQ	loc_88C
	CMP	R0, #1
	CMPNE	R0, #4
	BNE	loc_8E0
	MOV	R0, #0x88
	MOV	R4, #0x100

loc_88C:
	BL	sub_974
	MOV	R6, R4

loc_894:
	LDRH	R0, [R1],#2
	STRH	R0, [R5],#2
	SUBS	R6, R6,	#2
	BNE	loc_894
	ADD	R1, R9,	#0xA0000000
	MOV	R0, #0x90
	BL	sub_974
	MOV	R0, #0
	STRH	R0, [R1]
	BL	sub_998
	LDR	R0, =0x3F7E532
	ADD	R2, R10, #0x80

loc_8C4:
	LDR	R1, [R2],#4
	CMN	R1, #1
	BEQ	loc_8DC
	EOR	R0, R0,	R1
	SUBS	R4, R4,	#4
	BNE	loc_8C4

loc_8DC:
	STR	R0, [R10,#0x44]

loc_8E0:
	BX	R7

sub_8E4:
	MOV	R6, #0x3800
	LDR	R5, =0xEDB88320
	MOV	R4, #0

loc_8F0:
	MOV	R2, R4
	MOV	R3, #8

loc_8F8:
	TST	R2, #1
	EORNE	R2, R5,	R2,LSR#1
	MOVEQ	R2, R2,LSR#1
	SUBS	R3, R3,	#1
	BNE	loc_8F8
	STR	R2, [R6],#4
	ADD	R4, R4,	#1
	CMP	R4, #0x100
	BCC	loc_8F0
	BX	LR

sub_920:
	MOV	R0, #0xFFFFFFFF
	MOV	R5, #0x3800
	ADD	R2, R6,	R7

loc_92C:
	LDR	R1, [R6],#4
	MOV	R4, #4

loc_934:
	EOR	R3, R1,	R0
	MOV	R1, R1,LSR#8
	AND	R3, R3,	#0xFF
	LDR	R3, [R5,R3,LSL#2]
	EOR	R0, R3,	R0,LSR#8
	SUBS	R4, R4,	#1
	BNE	loc_934
	CMP	R6, R2
	BNE	loc_92C
	SUB	R6, R6,	R7
	BX	LR

sub_960:
	LDRB	R3, [R0],#1
	STRB	R3, [R1],#1
	SUBS	R2, R2,	#1
	BNE	sub_960
	BX	LR

sub_974:
	MOV	R3, #0xAA
	ADD	R2, R1,	#0xA00
	STRH	R3, [R2,#0xAA]
	MOV	R3, #0x55
	ADD	R2, R1,	#0x500
	STRH	R3, [R2,#0x54]
	ADD	R2, R1,	#0xA00
	STRH	R0, [R2,#0xAA]
	BX	LR

sub_998:
	MOV	R0, #0xF0
	STRH	R0, [R1]
	MOV	R0, #0xFF
	STRH	R0, [R1]
	BX	LR

sub_9AC:
	LDR	R0, =0xF4300000
	MOV	R2, #0x100
	STR	R2, [R1]
	LDR	R0, =0xF4400000
	LDR	R0, [R0,#0x60]
	MOV	R0, R0,LSR#8
	AND	R0, R0,	#0xFF
	LDR	R1, =0xF4300118
	SUB	R2, R1,	#0xCC
	CMP	R0, #0x14
	BNE	loc_9E0
	ADD	R1, R1,	#0x60
	ADD	R2, R2,	#4

loc_9E0:
	ADD	R3, R2,	#4
	SUB	R4, R3,	#0xC
	MOV	R12, R1
	MOV	R0, #1
	STR	R0, [R2]
	MOV	R0, #0x10
	STR	R0, [R3]
	MOV	R0, #0x500
	STR	R0, [R1]
	
	MOV     R0, #0x4000
	ORR     R0, R0, #0x510              
	
	STR	R0, [R4]
	LDR	R1, =0xF4B00000
	LDR	R11, [R1,#0x20]
	B	loc_A3C

serve_watchdog:
	LDR	R2, =0xF4B00000
	LDR	R0, [R2,#0x20]
	SUB	R1, R0,	R11
	CMP	R1, #0x200
	BCC	locret_A5C
	MOV	R11, R0
	B	loc_A3C

cmd_quit:
	BL	loc_A3C

loc_A3C:
	LDR	R0, [R12]
	MOV	R0, R0,LSL#22
	LDR	R2, [R12]
	MVN	R0, R0,LSR#31
	BIC	R2, R2,	#0x200
	AND	R0, R0,	#1
	ORR	R0, R2,	R0,LSL#9
	STR	R0, [R12]

locret_A5C:
	BX	LR

.end
