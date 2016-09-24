code32
start:
	mov r0, 0x48; 'H'
	bl uart_tx_byte
	
	mov r0, 0x65; 'e'
	bl uart_tx_byte
	
	mov r0, 0x6C; 'l'
	bl uart_tx_byte
	
	mov r0, 0x6C; 'l'
	bl uart_tx_byte
	
	mov r0, 0x6F; 'o'
	bl uart_tx_byte
	
	mov r0, 0x20; ' '
	bl uart_tx_byte
	
	mov r0, 0x77; 'w'
	bl uart_tx_byte
	
	mov r0, 0x6F; 'o'
	bl uart_tx_byte
	
	mov r0, 0x72; 'r'
	bl uart_tx_byte
	
	mov r0, 0x6C; 'l'
	bl uart_tx_byte
	
	mov r0, 0x64; 'd'
	bl uart_tx_byte
	
	mov r0, 0x21; '!'
	bl uart_tx_byte
	
	mov r0, 0x0A; '\n'
	bl uart_tx_byte
	
	b start

; Отправка по UART взята из бутлоадера Chaos'а
; -------------------------
; uart_tx_byte(r0=byte)
uart_tx_byte:
	mov r2, #0xf1000000 ; USART0_CLC
	ldr r1, [r2, #0x20] ; USART0_TXB
	bic r1, r1, #0xff
	orr r1, r0, r1
	str r1, [r2, #0x20] ; USART0_TXB
tx_w:
	ldr r1, [r2, #0x68] ; USART0_FCSTAT
	ands r1, r1, #0x02
	beq tx_w
	
	ldr r1, [r2, #0x70] ; USART0_ICR
	orr r1, r1, #2
	str r1, [r2, #0x70]
	
	bx lr
