@ Business application is forbidden.
@ Punishment - unavoidable crack and propagation on everything inet.
@ Model: SIEMENS SL75
@ FlashID(1) - 0089 (Intel), 880D,
@ FlashID(2) - 0089 (Intel), 8819,
@ FlashID(3) - 0089 (Intel), 881C,
@ FlashID(4) - 0020 (ST), 8819.

.section .text
.arm

start:
		adr SP, start
		@ disable interrupts
		mrs r0, CPSR
		orr r0, r0, #0xc0
		msr CPSR_c, r0
		@ enable write to einit-protected registers
		mov r0, #0
		bl set_einit
		@ disable first watchdog
		ldr r1, =0xF4400000
		ldr r0, [r1,#0x7C]  @ SCU_ROMAMCR
		bic r0, r0, #1
		b  next
		.ascii "SIEMENS_BOOTCODE"
		.long 0x00000003	@ boot mode + boot mode function (0x0003 = flasher mode)
		.long 115200		@ speed
		.long 0, 0, 0, 0	@ bkey
next:
		str r0, [r1,#0x7C]  @ SCU_ROMAMCR
		mov r0, #8
		str r0, [r1,#0x28]  @ SCU_ROMAMCR
		@ disable write to einit-protected registers
		mov r0, #1
		bl set_einit
		bl init_watchdog
		@ set default speed (115200)
@		mov r0, #1
@		bl set_speed
		@ answer: OK
		mov r0, #0xa5
		bl tx_byte
startwait:
		bl rx_byte
		cmp r0, #0x55
		beq stopwait
		cmp r0, #0x2e   @ '.' keepalive
		bleq serve_watchdog
		b  startwait
stopwait:
@		bl serve_watchdog
		bl read_info
main_aa:
		@ answer: OK
		mov r0, #0xAA
main_txb:
		bl tx_byte
main_loop:
@		ldr sp,=0x17FF0
		bl rx_byte
@		mov r10,r0
		cmp r0, #0x41   @ 'A' ping
		moveq r0, #0x52  @ 'R'
		beq main_txb
		cmp r0, #0x48   @ 'H' set baudrate
		beq cmd_speed
		cmp r0, #0x49   @ 'I' get info
		beq cmd_info
		cmp r0, #0x51   @ 'Q' quit
		beq cmd_quit
		cmp r0, #0x54   @ 'T' test for FFs
		beq cmd_test
		cmp r0, #0x52   @ 'R' read block
		beq cmd_read
		cmp r0, #0x46   @ 'F' write block
		beq cmd_flash
		cmp r0, #0x45   @ 'E' erase block
		beq cmd_erase
		cmp r0, #0x50   @ 'P' Patch
		beq cmd_patch
		cmp r0, #0x57   @ 'W' Write RAM
		beq cmd_write
		cmp r0, #0x47   @ 'G' GoTo
		beq cmd_goto
		cmp r0, #0x4F   @ 'O' Read OTP
		beq cmd_otp
		cmp r0, #0x43   @ 'C' Read CFI
		beq cmd_cfi
		cmp r0, #0x58   @ 'X' RAM Test
		beq cmd_ramtst
		adr r7, start
		cmp r0, #0x56   @ 'V' ADDR
		beq cmd_addr
		cmp r0, #0x2e   @ '.' keepalive
		bleq serve_watchdog
		b main_loop

@ ============= W
cmd_write:
		bl  rx_r6_r7 @r6=addr,r7=size
cmd_wrram:
		mov r4,r6
		mov r5,r7
		bl  rx_memblk		@ r4 addr @ r5 size
		b   send_ok
@ ============= G
cmd_goto:
		bl rx_r6_r7
		cmp r6,r7
		bleq cjmpr6
		b  main_aa
cjmpr6:
		mov  r0,r6
		bl   tx_byte
		mov  r0,r6,LSR#8
		bl   tx_byte
		mov  r0,r6,LSR#16
		bl   tx_byte
		mov  r0,r6,LSR#24
		bl   tx_byte
		mov  pc,r6
@ ============= Q
cmd_quit:
		@ fast power-off
		bl switch_watchdog
		bl switch_watchdog
q_l:
		b q_l
@ ============= T
cmd_test:
		bl rx_r6_r7
ct_l:
		ldr r0, [r6],#4
		ldr r1, [r6],#4
		and r0, r0, r1
		cmp r0, #0xffffffff
		movne r0, #0
		bne ct_e
		subs r7, r7, #8
		bne ct_l
ct_e:
		b main_txb
@ ============= O C
_cmd_oc:
		mov r4,lr
		bl  rx_r6_r7 @r6=addr,r7=size
		bl  tst_bouds @ only flash area can be write
		cmp r7,#0x400
		bcs err_bouds
		mov r1,r6
		adrl r5, start+0x1800
		mov r8,r7,LSR#1
		cmp r10, #0x89
		cmpne r10, #0x20
		bx  r4
@ ============= O
cmd_otp:
		bl  _cmd_oc
		blne rd_otp_amd @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		cmp r10, #0x89
		cmpne r10, #0x20
		bleq rd_otp_intel @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		b tx_mem_buf
@ ============= C
cmd_cfi:
		bl  _cmd_oc
		blne rd_cfi_amd @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		cmp r10, #0x89
		cmpne r10, #0x20
		bleq rd_cfi_intel @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
tx_mem_buf:
		adrl r6, start+0x1800
		b tx_mem_bkl
@ ============= R
cmd_read:
		bl rx_r6_r7 @ r6=addr,r7=size
tx_mem_bkl:
		mov r8, #0
r_loop:
		ldrb r0, [r6],#1
		eor r8, r8, r0
		bl tx_byte
		subs r7, r7, #1
		bne r_loop
		@ answer: OK
		mov r0, #0x4f   @ 'O'
		bl tx_byte
		mov r0, #0x4b   @ 'K'
		bl tx_byte
		@ checksum
		mov r0, r8
		bl tx_byte
		mov r0, #0
		b main_txb
@ ============= H
cmd_speed:
		@ baudrate code (0..7)
		bl rx_byte
		mov r6, r0
		@ answer 'h'
		mov r0, #0x68   @ 'h'
		bl tx_byte
		@ delay to send last byte
		mov r0, #0xa00
cs_w:
		subs r0, r0, #1
		bne cs_w
		@
		mov r0, r6
		bl set_speed
cs_a:
		@ waiting for 'A',
		@ not serving watchdog
		bl rx_byte
		cmp r0, #0x41   @ 'A'
		bne cs_a
		@ answer: OK
		mov r0, #0x48   @ 'H'
		b main_txb
@ ============= I
cmd_info:
		@ flash info block
		adrl r6, start+0x1E00
		mov r7, #128
i_loop:
		ldrb r0, [r6],#1
		bl tx_byte
		subs r7, r7, #1
		bne i_loop
		b main_loop
@ ============= P
cmd_patch:
		bl rx_r6_r7 @r6=addr,r7=size
		@ only flash area can be write
		bl tst_bouds
		@ copy seg flash to ram
		mov r4, #0xA8000000
		mov r1, r7 @size
		mov r5, r6 @addr
cf_lp1:
		ldr r0, [r5],#4
		str r0, [r4],#4
		subs r1, r1, #4
		bne cf_lp1
		mov r0,#0x55
		bl  tx_byte
		@ patch addr
		bl  rx_word
		mov r8, r0   @offset
		bl  rx_word
		mov r5, r0   @size
		add r4, r8, #0xA8000000
		b   rx_clr_wr @r4=addr, r5=size rx @r6=sector addr erase @ r6=addr,r7=size write
@ ============= E
cmd_erase:
		bl rx_word
		mov r6, r0 @addr
		bl tst_bouds @ only flash area can be write
		bl erase_blk1
		bl erase_blk2
		b  main_loop
@ -------------
@ -------------
@ R6 - addr, R7 - size
erase_blk1:
		mov r9,lr
		cmp r10, #0x89
		cmpne r10, #0x20
		bne erase_amd1
@ -------------------------
erase_intel1:
@ flash_intel(r6,r7,r8) (jump)
@ r6 = flash address
@ r7 = length
@ r8 = data address
		@ unlock
		mov r0, #0x60   @Block to lock/unlock/lock-down
		strh r0, [r6]
		mov r0, #0xd0   @Block Unlock
		strh r0, [r6]
		nop

		@ erase
		mov r0, #0x20   @BlockEraseSetup
		strh r0, [r6]
		mov r0, #0xd0   @BlockEraseConfirm
		strh r0, [r6]
		bx  r9

@ -------------
@ -------------
@ R6 - addr, R7 - size
erase_blk2:
		mov r9,lr
		mov r0, #0x01  @OK_ACK
		bl tx_byte
		mov r0, #0x01
		bl tx_byte
		cmp r10, #0x89
		cmpne r10, #0x20
		bne erase_amd2
@ erase_intel2
i_el:
		bl serve_watchdog
		mov r0, #0x70   @StatusRead
		strh r0, [r6]
		ldrh r0, [r6]
		tst r0, #0x80
		beq i_el
		tst r0, #0x3a
		bne i_err
		mov r0,#0xFF    @ArrayMode
		strh r0, [r6]
		b   erase_end
@ -------------------------
erase_amd1:
@ flash_amd(r6,r7,r8) (jump)
@ r6 = flash address
@ r7 = length
@ r8 = data address
@		mov r0, #0x01
@		bl tx_byte
@		mov r0, #0x01
@		bl tx_byte
@ -- Amd Erase Sector
		mov r1, r6
@		mov r0, #0x20     @ Unlock Bypass Entry
@		bl send_flash_cmd @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3

		mov r0, #0x80		  @SectorEraseCmd1
		bl send_flash_cmd   @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3
		mov r0, #0x30		  @SectorEraseCmd2
		bl send_flash_cmd   @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3
		bx  r9

erase_amd2:
@		bl serve_watchdog
		mov  r1,#0x10000
		sub  r1,r1,#1
		mov  r2,#0x28000    @ #0x40000 =1.5sec!
a_eldq3:
		subs r2,r2,#1
		beq  a_err
		ldrh r0, [r6]
		cmp  r0,r1
		bne  a_eldq3
		ldrh r0, [r6]
		cmp  r0,r1
		bne  a_eldq3
		ldrh r0, [r6]
		cmp  r0,r1
		bne  a_eldq3
		bl   a_ubres

erase_end:
		mov r0, #0x02       @ERASE_ACK
		bl tx_byte
		mov r0, #0x02
		bl tx_byte
		bx  r9
@--- Amd Unlock Bypass Reset
a_ubres:
		mov r0, #0x90
		strh r0, [r6]
		mov r0, #0x00
		strh r0, [r6]
@--- Amd Reset
a_res:
		mov r0, #0xf0
		strh r0, [r6]
		bx  lr
@--- Amd Err
a_err:
		bl  a_ubres
		mov r0, #0x45
		bl tx_byte
		mov r0, #0x45
		b main_txb
@--- only flash area can be write
tst_bouds:
		cmp r6, #0xA0000000
		bcc err_bouds
		cmp r6, #0xA8000000
		bcs err_bouds
		bx  lr
@--- bounds error
err_bouds:
		mov r0, #0xff
		bl tx_byte
		mov r0, #0xff
		b main_txb
@ ============= F
cmd_flash:
		bl rx_r6_r7 @r6=addr,r7=size
		bl tst_bouds @ only flash area can be write
		@ read data to RAM
		mov r4, #0xA8000000 @addr
		mov r5, r7  @size
rx_clr_wr:
		bl erase_blk1
		bl  rx_memblk
		@ check for bounds
@		mov r0, #0x20000
@		sub r0, r0, #1   @r0=0x1ffff
@		mov r1, r6       @addr
@		add r2, r6, r7   @size
@		sub r2, r2, #1
@		bic r1, r1, r0
@		bic r2, r2, r0
@		cmp r1, r2
@		bne err_bouds
cf_xx1:
		bl erase_blk2
@ --------------------------
@ --------------------------
@ r6 addr
@ r7 size
@ r8 buf
fwrite:
		mov r8, #0xA8000000     @ R8 - src addrress
		cmp r10, #0x89   @FlashID
		cmpne r10, #0x20
		bne fwritea
@ ---------------------------
fwritei:
		@ write
		adrl r4, start+0x1E00
		ldrb r0, [r4, #0x55]    @ Word Count ?
		mov r3, #1
		mov r3, r3, LSL r0
		mov r3, r3, LSR#1       @ R3 - write buffer size (words)

		mov r4, r6		      @ R4 - dest. address
		mov r5, r7, LSR#1       @ R5 - length (words)

i_xwl:
		bl serve_watchdog

		mov r0, #0xe8		   @ Buffer Prog.Setup
		strh r0, [r4]
		ldrh r0, [r4]		   @ Check SR[7]: 1 = Write Buffer available@ 0 = No Write Buffer available
		tst r0, #0x80		   @ Device Supports Buffer Writes? (SR[7] = Valid)
		beq i_err

		@ send word count
		sub r0, r3, #1		  @
		strh r0, [r4]		   @ Write Word Count, Word Address

		@ sending data
		mov r1, r3
i_sl:
		ldrh r0, [r8],#2
		strh r0, [r4],#2
		subs r1, r1, #1
		bne i_sl

		@ confirm
		mov r0, #0xd0
		strh r0, [r6]

i_wl:
		ldrh r0, [r6]
		tst r0, #0x80
		beq i_wl
		tst r0, #0x3a
		bne i_err

		subs r5, r5, r3
		bhi i_xwl

		mov r0, #0xff
		strh r0, [r6]
		b fwriteok
i_err:
		@ error
		mov r0, #0x50
		strh r0, [r6]
		mov r0, #0xff
		strh r0, [r6]
		b  err_bouds
@ -------------------------
fwritea:
		@ unlock bypass
		mov r1, r6
		mov r0, #0x20     @ Unlock Bypass Entry
		bl send_flash_cmd @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3

		mov r4, r6		      @ R4 - dest. address
		mov r5, r7, LSR#1       @ R5 - length (words)

		bl serve_watchdog
		mov  r2,#0x20000    @ #0x40000 =1.5���!
a_xwl:
		mov r0, #0xA0    @ Unlock Bypass Program
		strh r0, [r4]

		ldrh r0, [r8],#2
		strh r0, [r4]
a_wl:
		subs r2,r2,#1
		beq a_err
		ldrh r1, [r4]
		cmp r1,r0
		bne a_wl
		ldrh r1, [r4]
		cmp r1,r0
		bne a_wl
		ldrh r1, [r4]
		cmp r1,r0
		bne a_wl

		add r2, r2, #1
		add r4, r4, #2
		subs r5, r5, #1
		bne a_xwl
		bl  a_ubres
fwriteok:
		mov r0, #0x03
		bl tx_byte
		mov r0, #0x03
		bl tx_byte
@		b flash_checksum
@ -------------------------

flash_checksum:

@ flash_checksum(r6,r7) (jump)
@ r6 = address
@ r7 = length

		mov r8, #0
		mov r1, r6
		mov r2, r7, LSR#1
cks_l:
		ldrh r0, [r1],#2
		add r8, r8, r0
		subs r2, r2, #1
		bne cks_l

		mov r0, r8
		bl tx_byte
		mov r0, r8, LSR#8
		bl tx_byte

send_ok:
		mov r0, #0x4f   @ 'O'
		bl tx_byte
		mov r0, #0x4b   @ 'K'
		b main_txb
@ =====================
set_speed:

@ void set_speed(r0=n)
@ 0=57600,  1=115200, 2=230400, 3=460800,
@ 4=614400, 5=921600, 6=1228800, 7=1600000
@ uses r0, r1, r2

		adr r1, iSpeeds
		ldr r1, [r1, r0,LSL#2]

		mov r2, #0xf1000000  @USART0_CLC
		mov r0, r1, LSR#16
		str r0, [r2, #0x14]  @USART0_BG
		mov r0, r1, LSL#16
		mov r0, r0, LSR#16
		str r0, [r2, #0x18]  @USART0_FDV

		bx lr

iSpeeds:
		.long 0x001901d8 @57600/0x1d8=122.033898*0x1a=3172.881348
		.long 0x000c01d8 @115200/0x1d8=244.067797*13=3172.881361
		.long 0x000501b4 @230400/0x1b4=528.440367*0x6=3170.642202
		.long 0x00000092 @460800/0x92=3156.164384
		.long 0x000000c3 @614400/0xc3=3150.769231
		.long 0x00000127 @921600/0x127=3124.067797
		.long 0x0000018a @1228800/0x18a=3118.781726
		.long 0x00000000 @1600000/0x200=3125
		.long 0x000001d0 @1500000/0x1d0=3232.758621

@ --------------------------
@ r4 addr
@ r5 size
rx_memblk:
		mov r9,lr
		mov r8, #0
cf_r:
		bl serve_watchdog
		bl rx_byte
		eor r8, r8, r0
		strb r0, [r4],#1
		subs r5, r5, #1
		bne cf_r

		bl rx_byte
		cmp r0, r8 @chk
		bne cf_chkee
		bx  r9
cf_chkee:
		@ checksum error
		mov r0, #0xbb
		bl tx_byte
		mov r0, #0xbb
		b main_txb
@ --------------------------
rx_r6_r7:
		mov r9,lr
		@ address
		bl rx_word
		mov r6, r0
		@ length
		bl rx_word
		mov r7, r0
		bx  r9
@ ---------------------------

tx_byte:

@ void tx_byte(r0=byte)
@ uses r0, r1, r2

		mov r2, #0xf1000000 @USART0_CLC
		ldr r1, [r2, #0x20] @USART0_TXB
		bic r1, r1, #0xff
		orr r1, r0, r1
		str r1, [r2, #0x20] @USART0_TXB
tx_w:
		ldr r1, [r2, #0x68] @USART0_FCSTAT
		ands r1, r1, #0x02
		beq tx_w

		ldr r1, [r2, #0x70] @USART0_ICR
		orr r1, r1, #2
		str r1, [r2, #0x70]

		b serve_watchdog


@ --------------------------

rx_byte:

@ byte rx_byte()
@ uses r0, r1, r2

rx_loop:
		mov r1, #0xf1000000
		ldr r0, [r1, #0x68] @USART0_FCSTAT
		ands r0, r0, #0x04
		beq rx_loop

rx_c:
		ldr r0, [r1, #0x70] @USART0_ICR
		orr r0, r0, #4
		str r0, [r1, #0x70]

		ldr r0, [r1, #0x24] @USART0_RXB
		and r0, r0, #0xff
		bx lr

@ ------------------------

rx_word:

@ word rx_word()
@ uses r0, r1, r2, r3, r4, r5

		mov r5, lr

		bl rx_byte
		mov r4, r0, LSL#24
		bl rx_byte
		orr r4, r4, r0,LSL#16
		bl rx_byte
		orr r4, r4, r0,LSL#8
		bl rx_byte
		orr r0, r4, r0

		bx r5

@ ----------------------------

set_einit:

@ void set_einit(r0=on/off)
@ uses r0, r1, r2, r3

		ldr r3, =0xf4400000
		ldr r1, [r3, #0x24] @SCU_WDTCON0
		bic r1, r1, #0x0e
		orr r1, r1, #0xf0
		ldr r2, [r3, #0x28] @SCU_WDTCON1
		and r2, r2, #0x0c
		orr r1, r1, r2
		str r1, [r3, #0x24] @SCU_WDTCON0

		bic r1, r1, #0x0d
		orr r1, r1, #2
		orr r0, r0, r1
		str r0, [r3, #0x24] @SCU_WDTCON0

		bx lr


read_info:

@ void read_info()
;
		mov r7, lr

		bl init_F0_common

		adrl  r4, start+0x1E00
		mov r1, r4
		mov r2, #0x80
		mov r0, #0
ri_clr:
		str r0, [r1], #4
		subs r2, r2, #4
		bne ri_clr

		@ phone info
		mov r1, r4
@		adrl  r1, start+0x1E00
		ldr  r0, =0xA003E000
		mov  r2, #32
		bl memcpy
		@ IMEI
		ldr r0, =0xA003E410
		mov r2, #16
		bl memcpy
		@ HASH
		ldr r0, =0xA003E400
		mov r2, #16
		bl memcpy
		@ flash base
 @       adrl  r4, start+0x1E00
		mov r0, #0xa0000000
		str r0, [r4, #0x40]
		@ flash id
		mov r1, #0xa0000000

		mov r0, #0x90
		bl send_flash_cmd @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3
		ldrh r10, [r1, #0x0]
		strh r10, [r4, #0x50] @R10 = flash id
		bl send_flash_reset @r1 = flash base @ uses r0, r1

		ldrh r0, [r1, #0x0]
		ldrh r2, [r4, #0x50]
		subs r0, r0, r2
		streqh r0, [r4, #0x50]
		streqh r0, [r4, #0x52]
		beq ri_cf

		mov r0, #0x90
		bl send_flash_cmd @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3
		ldrh r0, [r1, #0x02]
		strh r0, [r4, #0x52]  @flash id2
		bl send_flash_reset @r1 = flash base @ uses r0, r1
		@ OTP
@		ldrh    r0, [r4,#0x50]
		cmp r10, #0x01
		beq Amd_OTP
		cmp r10, #0x04
		beq Amd_OTP
		cmp r10, #0x89
		cmpne r10, #0x20
		bne ri_cf
@ -------------------------
Intel_OTP:
		add r5,r4,#0x44   @ldr r5, =0x3e44
		add r6,r1,#0x100  @ldr r6, =0xa0000102  @Reg 81h
		add r6,r6,#2
		mov r8,#6 @12
		bl rd_otp_intel  @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		add r5,r4,#0x74      @ldr r5, =0x3e70
		mov r8,#2 @4
		bl rd_otp_intel  @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		add r6,r6,#2 @ldr r6, =0xa000010C  @Reg 86h
		@ldr r5, =0x3E78
		mov r8,#4 @8
		bl rd_otp_intel  @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		b ri_cf
		@ -------------------------
Amd_OTP:
@		bl  set_config_amd
@ -------------------------
@ Intel 0089:880D
@ 1011111111001111b=0xBFCF
@		      *** 111 =Continuous-word burst
@		     *--- 1 = No Wrap@ Burst accesses do not wrap within burst length
@		   **---- Reserved bits should be cleared (0)
@		  *------ 1 = Rising edge
@		 *------- 1 = Linear (Burst Sequence)
@		*-------- 1 = WAIT deasserted one data cycle before valid data (default)
@       *--------- 1 = Data held for a 2-clock data cycle
@      *---------- 1 = WAIT signal is active high
@   ***----------- Latency Count 7
@  *-------------- Reserved bits should be cleared (0)
@ *--------------- 1 = Asynchronous page-mode read
@ -------------------------
@ 11100101000000000000b=0xE5000
@      101= Data is valid on the 7th active CLK edge after AVD# transition to VIH
@    00 = Continuous
@   1 = Burst starts and data is output on the rising edge of CLK
@  1 = RDY active with data
@ 1 = Asynchronous Mode
@set_config_amd
@		mov r3, #0xaa
@		add r2, r1, #0xA00      @0xA0000A00
@		strh r3, [r2, #0xaa]    @0xA0000AAA  AAAh>>1=0x555
;
@		mov r3, #0x55
@		add r2, r1, #0x500      @0xA0000500
@		strh r3, [r2, #0x54]    @0xA0000554  554h>>1=0x2AA
;
@		add r2, r1, #0x8A00      @0xA0000A00
@		add r2, r2, #0x72000     @0x72800|0xA00=0x72AAA>>1=0x39555
@		mov r0, #0xC0
@		strh r0, [r2, #0xaa]    @0xA0000AAA  AAAh>>1=0x555

		add r5,r4,#0x44  @ldr r5, =0x3e46
		mov r6,r1		@ldr r6, =0xa0000000
		mov r8,#6 @12
		bl rd_otp_amd   @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		add r5,r4,#0x74 @ldr r5, =0x3e74
		mov r8,#2 @4
		bl rd_otp_amd   @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		add r6,r1,#0x80   @ldr r6, =0xa0000080
		add r5,r4,#0x78   @ldr r5, =0x3e78
		mov r8,#4 @8
		bl rd_otp_amd   @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		add r5,r4,#0x78   @ldr r5, =0x3e78
		ldr r0, [r4]
		adds r0, r0, #0x01
		bne ri_cf
		add r6,r1,#0x100   @ldr r6, =0xa0000100
		mov r8,#4 @8
		bl rd_otp_amd   @ r1 - base@ r6 - OTP addr@ r5 - buf@ r8 - size
		@ -------------------------
ri_cf:
		@ cfi information
		mov r0, #0x98
		strh r0, [r1, #0xaa]
		nop

		add r2, r4, #0x54
		ldrh r0, [r1, #0x4e]  @(19h)"n" such that device size = 2^n in number of bytes
		strb r0, [r2],#1
						      @0x2A..0x3C
		add r3, r1, #0x54     @"n" such that maximum number of bytes in write buffer = 2^n
cfi_l:
		ldrh r0, [r3],#2      @
		strb r0, [r2],#1
		and r0, r3, #0xff
		cmp r0, #0x7a		 @0x3D
		bne cfi_l

		bl send_flash_reset @r1 = flash base @ uses r0, r1

		ldrb r0, [r4, #0x54]
		cmp r0, #0x1A  @ 512Mbit(64Mb)?
		beq cfi_32
		bl  init_F0_part2  @r0=19? 256Mbit(32Mb)?
		@ flash part 2
@		mov r1, #0xA1000000/0xA2000000
		and r1, r2, #0xFF000000 @ 0xA1000000/0xA2000000

		@ cfi information
		mov r0, #0x98
		strh r0, [r1, #0xaa]
		nop

		ldrh r0, [r1, #0x20]  @"QRY"?
		cmp r0, #0x51		 @'Q'?
		bne cfi_32

		ldrb r0, [r4, #0x54]
		add r0, r0, #1
		strb r0, [r4, #0x54]

		ldrb r2, [r4, #0x57] @0x2B N regions of flash 1
		add r2, r4, r2,LSL#2 @
		add r2, r2, #0x58

		add r3, r1, #0x5a    @ region info

cfi_l2:
		ldrh r0, [r3],#2
		strb r0, [r2],#1
		and r0, r3, #0xff
		cmp r0, #0x7a
		bne cfi_l2

		ldrh r0, [r1, #0x58] @ N regions
		ldrb r2, [r4, #0x57]
		add r2, r2, r0
		strb r2, [r4, #0x57]

		bl send_flash_reset @r1 = flash base @ uses r0, r1

cfi_32:
		bx r7
@ ------------------------- RD OTP Intel
@ r1 - flash base
@ r6 - OTP addr
@ r5 - buf
@ r8 - size
@ r4,r7,r10... - not use
rd_cfi_intel:
		mov r0, #0x98
		b rd_xxx_intel
rd_otp_intel:
		mov r0, #0x90
rd_xxx_intel:
		mov r9,lr
		bl send_flash_cmd @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3
rdoi_loop:
		ldrh r0, [r6], #2
		strh r0, [r5], #2
		subs r8, r8, #1
		bne rdoi_loop
		bl send_flash_reset @r1 = flash base @ uses r0, r1
		bx r9
@ ------------------------- RD OTP Amd
@ r1 - base
@ r6 - OTP addr
@ r5 - buf
@ r8 - size
@ r4,r7,r10... - not use
rd_cfi_amd:
		mov r0, #0x98    @AMD CFI
		b   rd_xxx_amd
rd_otp_amd:
		mov r0, #0x88    @AMD OTP Open
rd_xxx_amd:
		mov r9,lr
		bl send_flash_cmd @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3
rdotpa_loop:
		ldrh r0, [r6], #2
		strh r0, [r5], #2
		subs r8, r8, #1
		bne rdotpa_loop
		mov r0, #0x90
		bl send_flash_cmd @ r0 = command @ r1 = flash base @ uses r0, r1, r2, r3
@		mov r0, #0x00
		strh r1, [r1]    @0xA0000000
@		bl send_flash_reset @r1 = flash base @ uses r0, r1
		bx  r9
@ -------------------------
memcpy:
@ uses r0, r1, r2, r3
@ r0=src
@ r1=dst
@ r2=len
sc_loop:
		ldrb r3, [r0], #1
		strb r3, [r1], #1
		subs r2, r2, #1
		bne sc_loop
		bx lr
@ --------------------------
send_flash_cmd:
@ r0 = command
@ r1 = flash base
@ uses r0, r1, r2, r3

		mov r3, #0xaa
		add r2, r1, #0xa00      @0xA0000A00
		strh r3, [r2, #0xaa]    @0xA0000AAA

		mov r3, #0x55
		add r2, r1, #0x500      @0xA0000500
		strh r3, [r2, #0x54]    @0xA0000554

		add r2, r1, #0xa00      @0xA0000A00
		strh r0, [r2, #0xaa]    @0xA0000AAA

		bx lr

@ -------------------------

send_flash_reset:

@ r1 = flash base
@ uses r0, r1
		mov  r0, #0xF0
		cmp  r10,#0x0089
		cmpne r10, #0x0020
		moveq r0, #0xFF
		strh r0, [r1]
		bx lr

@ -------------------------

init_watchdog:

 @ void init_watchdog()
 @ uses r0, r1, r2, r3, r4

@				LDR     R0, =0xF4300000  @PCL_CLC
@				MOV     R2, #0x100
@				STR     R2, [R1]

				LDR     R0, =0xF4400000
				LDR     R0, [R0,#0x60]   @SCU_CHIPID
				MOV     R0, R0,LSR#8
				AND     R0, R0, #0xff

				ldr     r1, =0xf4300118 @PCL_62
				sub     r2, r1, #0xcc

				cmp     r0, #0x14
				bne     iwd_1a

				add     r1, r1, #0x60
				add     r2, r2, #0x04

iwd_1a:
				add     r3, r2, #0x4
				sub     r4, r3, #0x0c
				mov     r12, r1

				MOV     R0, #1
				STR     R0, [R2]
				MOV     R0, #0x10
				STR     R0, [R3]
				MOV     R0, #0x500
				STR     R0, [R1]
				MOV     R0, #0x4000
				ORR     R0, R0, #0x510
				STR     R0, [R4]

				LDR     r1,= 0xF4B00020  @STM_4
				LDR     r11, [R1]

				b switch_watchdog


@ -------------------------

serve_watchdog:

 @ void serve_watchdog()
 @ uses r0, r1, r2

		@ read timer
		ldr r2, =0xf4b00020  @STM_4
		ldr r0, [r2]
		sub r1, r0, r11
		cmp r1, #0x200
		bcc swd_exit
		@ save timer value
		mov r11, r0
@		b switch_watchdog

@ ------------------------

switch_watchdog:

@ void switch_watchdog()
@ uses r0, r1, r2

		LDR     R0, [R12]
		MOV     R0, R0,LSL#22
		LDR     R2, [R12]
		MVN     R0, R0,LSR#31
		BIC     R2, R2, #0x200
		AND     R0, R0, #1
		ORR     R0, R2, R0,LSL#9
		STR     R0, [R12]

swd_exit:

		BX LR
@ -------------------------
init_F0_common:

@ void init_F0_common()
@ uses r0, r1

		mov r1, #0xF0000000
		ldr r0, =0xA8000041
		str r0, [r1,#0x88]  @ADDRSEL1
		ldr r0, =0x30720200
		str r0, [r1,#0xC8]  @BUSCON1
		mov r0, #6
		str r0, [r1,#0x40]  @SDRMREF0
		ldr r0, =0x891C70
		str r0, [r1,#0x50]  @SDRMCON0
		mov r0, #0x23
		str r0, [r1,#0x60]  @SDRMOD0

		ldr r0, =0xA0000011
		str r0, [r1,#0x80]  @ADDRSEL0
		str r0, [r1,#0xA0]  @ADDRSEL4
		ldr r0, =0x00522600
		str r0, [r1,#0xC0]  @BUSCON0
		str r0, [r1,#0xE0]  @BUSCON4

		bx lr

@ -------------------------
init_F0_part2:
@ uses r0, r1
		mov r1, #0xF0000000
		mov r2, #0xA0000000
		add r2,r2,#0x21      @ r2=0xA0000021
		cmp r0, #0x19		@ 256Mbit(32Mb)?
		addne r2,r2,#0x10    @ 0xA0000021/0xA0000031 - 32/16 Mb
		@ alternate amd configuration
		str r2, [r1,#0x80]   @ ADDRSEL0
		str r2, [r1,#0xA0]   @ ADDRSEL4
		addeq r2,r2,#0x01000000 @ 0xA1000021/0xA0000031
		add r2,r2,#0x01000000 @ 0xA2000021/0xA1000031 - 32/16 Mb
		str r2, [r1,#0x90]   @ ADDRSEL2
		str r2, [r1,#0xB0]   @ ADDRSEL6
		ldr r0, =0x00522600
		str r0, [r1,#0xD0]   @ BUSCON2
		str r0, [r1,#0xF0]   @ BUSCON6
		bx lr
@ ----------------
load_bits:
		mov r7, #0xA8000000
ld_bits_x:
		adrl r0, start+0x1800
		ldr r1,[r0],#4  @ 00111
		ldr r2,[r0],#4  @ 10011
		ldr r3,[r0],#4  @ 11001
		ldr r4,[r0],#4  @ 11100
		ldr r5,[r0],#4  @ 01110
		mov r6,#0x00010000   @0x00800000/20/0x00010000=6.4
		bx lr
@ ============= X
cmd_ramtst:
		bl load_bits
for_wr_test:
		str r1, [r7],#4
		str r2, [r7],#4
		tst r7,#0x00800000
		bne read_test
		str r3, [r7],#4
		str r4, [r7],#4
		str r5, [r7],#4
		b   for_wr_test
read_test:
		bl serve_watchdog
		mov  r0,#0x55
		bl tx_byte
		bl load_bits
for_rd_test:
		ldr r0, [r7],#4
		cmp r0,r1
		bne ram_err1
		ldr r0, [r7],#4
		cmp r0,r2
		bne ram_err1
		tst r7,#0x00800000
		bne main_aa		     @ end test, send 0xAA
		subs r6, r6, #1
		bne x_rd_test
		bl serve_watchdog
		mov r0,#0x56
		bl tx_byte
		bl ld_bits_x
x_rd_test:
		ldr r0, [r7],#4
		cmp r0,r3
		bne ram_err1
		ldr r0, [r7],#4
		cmp r0,r4
		bne ram_err1
		ldr r0, [r7],#4
		cmp r0,r5
		beq  for_rd_test
ram_err1:
		mov  r0,#0x45     @ "E"
		bl   tx_byte
cmd_addr:
		mov  r0,r7
		bl   tx_byte
		mov  r0,r7,LSR#8
		bl   tx_byte
		mov  r0,r7,LSR#16
		bl   tx_byte
		mov  r0,r7,LSR#24
		b    main_txb
.end
