.global _start
.type _start, function
.arm
.section .text

_start:
	b main
.int 0
.int 0
.int 0
.int 0
.int 0
.int 0
.int 0
.int 0

.global clean_all_caches
.type clean_all_caches, function
clean_all_caches:
	MRC p15, 0, r15, c7, c10, 3 @ clean entire dcache using test and clean
	NOP
	NOP
	NOP
	BNE	clean_all_caches
	MOV	R0, #0
	MCR	p15, 0, r0, c7, c10, 4 @ drain write buffer
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP
	MCR	p15, 0, r0, c7, c5, 0 @ invalidate icache
	NOP
	NOP
	BX LR
