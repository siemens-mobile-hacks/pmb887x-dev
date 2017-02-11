LD=arm-none-eabi-ld
AS=arm-none-eabi-as
OBJCOPY=arm-none-eabi-objcopy

all: clean
	$(AS) -c -mcpu=arm926ej-s chaos_x85.S -o chaos_x85.o
	$(LD) chaos_x85.o -o chaos_x85.elf
	$(OBJCOPY) -j .text -O binary chaos_x85.elf chaos_x85.bin -v
	perl bin2hex.pl chaos_x85.bin > chaos_x85.hex
	ls -lah chaos_x85.bin

clean:
	rm *.elf *.bin *.o 2>/dev/null || true
