.section .text
.align 4
.global test_from_asm
test_from_asm:

ldr r0, STM_0

ldr r1, [r0]
ldr r2, [r0]

sub r0, r2, r1

bx lr

STM_0: .word 0xF4B00020
