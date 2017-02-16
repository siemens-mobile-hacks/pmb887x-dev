INCLUDE=-I ../common
C_SOURCES=../common/pmb8876.c ../common/printf.c main.c
OBJECTS=handlers.o pmb8876.o main.o printf.o __aeabi_uidivmod.o
BIN_NAME=uart_irq

all: clean
# compile
	arm-none-eabi-gcc -c -mcpu=arm926ej-s $(INCLUDE) -O2 $(C_SOURCES)
	
# init
	arm-none-eabi-as -c -mcpu=arm926ej-s handlers.S -o handlers.o
	arm-none-eabi-as -c -mcpu=arm926ej-s div/__aeabi_uidivmod.S -o __aeabi_uidivmod.o
	
# link
	arm-none-eabi-ld -e __enter_reset -nostdlib --gc-sections $(OBJECTS) -o "$(BIN_NAME).elf" -T linker.ld
	
# extract binary
	arm-none-eabi-objcopy -O binary "$(BIN_NAME).elf" "$(BIN_NAME).bin"
	ls -lah "$(BIN_NAME).bin"
clean:
	rm *.elf *.bin *.o 2>/dev/null || true
