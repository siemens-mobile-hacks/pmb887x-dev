	.syntax unified
	.cpu arm926ej-s
	.arm

	.equ USART_CLC, 0x00
	.equ USART_CON, 0x10
	.equ USART_BG, 0x14
	.equ USART_FDV, 0x18
	.equ USART_WHBCON, 0x50
	.equ USART_IMSC, 0x64
	.equ USART_MIS, 0x6C

	.section .text.start, "ax", %progbits
	.global _start
	.type _start, %function
_start:
	mov r0, #0xF1000000
	mov r1, #0x00000108
	str r1, [r0, #USART_CLC]
	mov r1, #0x000001D8
	str r1, [r0, #USART_FDV]
	mov r1, #0x0000000C
	str r1, [r0, #USART_BG]
	mov r1, #0x00008800
	orr r1, r1, #0x00000011
	str r1, [r0, #USART_CON]
	mov r1, #0x00000020
	str r1, [r0, #USART_WHBCON]
	mov r2, #0x00000004
	str r2, [r0, #USART_IMSC]

1:
	ldr r1, [r0, #USART_MIS]
	tst r1, r2
	beq 1b

	mov r0, #0
	bx r0

	.size _start, . - _start
