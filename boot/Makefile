LD=arm-none-eabi-ld
AS=arm-none-eabi-as
OBJCOPY=arm-none-eabi-objcopy

all: clean pv_boot_x85

chaos_x85:
	$(AS) -c -march=armv5te -mcpu=arm926ej-s chaos_x85.S -o chaos_x85.o
	$(LD) chaos_x85.o -o chaos_x85.elf  -T "../lib/ld/intram.ld"
	$(OBJCOPY) -O binary chaos_x85.elf chaos_x85.bin
	perl bin2hex.pl chaos_x85.bin > chaos_x85.hex

pv_boot_x85:
	$(AS) -c -march=armv5te -mcpu=arm926ej-s pv_boot_x85.S -o pv_boot_x85.o
	$(LD) pv_boot_x85.o -o pv_boot_x85.elf  -T "../lib/ld/intram.ld"
	$(OBJCOPY) -O binary pv_boot_x85.elf pv_boot_x85.bin
	perl bin2hex.pl pv_boot_x85.bin > pv_boot_x85.hex

clean:
	rm *.elf *.bin *.hex *.o 2>/dev/null || true
