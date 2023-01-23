_start:
	MOV R0,#0
	
	ADR	R8, machine_id
	LDR	R1, [R8]
	
	ADR	R8, dtb_addr
	LDR	R2, [R8]
	
	ADR	R8, _start
	ADD PC, R8, #0x8000

machine_id:
	.long 0xAAAAAAAA
dtb_addr:
	.long 0xBBBBBBBB
