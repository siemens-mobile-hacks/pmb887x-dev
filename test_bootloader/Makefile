all:
	arm-none-eabi-gcc -c -fomit-frame-pointer -mcpu=arm926ej-s -nostdlib -nostdinc -O0 main.c -o main.o
#	arm-none-eabi-objdump -d main.o
	arm-none-eabi-ld main.o -o bootloader.elf -T test.ld
	arm-none-eabi-objcopy -j .xuj -O binary bootloader.elf bootloader.bin
#	hexdump -C bootloader.bin
test.bin:
	fasmarm test.asm
clean:
	rm bootloader.elf bootloader.bin main.o test.bin || true