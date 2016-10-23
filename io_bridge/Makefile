all:
	arm-none-eabi-gcc -c -fomit-frame-pointer -mcpu=arm926ej-s -nostdlib -nostdinc -O0 main.c -o main.o
	arm-none-eabi-as -c -mcpu=arm926ej-s test.S -o test.o
	arm-none-eabi-ld main.o test.o -o io.elf -T test.ld
	arm-none-eabi-objcopy -j .xuj -O binary io.elf io.bin
clean:
	rm io.elf io.bin main.o || true