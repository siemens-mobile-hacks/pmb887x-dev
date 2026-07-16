.arm
.section .pll_loader, "ax"
.align 2

.global pll_loader
.type pll_loader, %function
pll_loader:
	ldr r0, =_intram_loadaddr
	ldr r1, =_intram_start
	ldr r2, =_intram_loadend
.Lcopy:
	cmp r0, r2
	ldrlo r3, [r0], #4
	strlo r3, [r1], #4
	blo .Lcopy
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4
	mcr p15, 0, r0, c7, c5, 0
	ldr pc, =_start

	.ltorg
