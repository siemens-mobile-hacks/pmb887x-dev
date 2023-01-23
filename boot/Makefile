LD=arm-none-eabi-ld
AS=arm-none-eabi-as
OBJCOPY=arm-none-eabi-objcopy

all: clean pv_boot_x85 chaos_x85 linux_boot

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

linux_boot:
	$(AS) -c -march=armv5te -mcpu=arm926ej-s linux_boot.S -o linux_boot.o
	$(LD) linux_boot.o -o linux_boot.elf  -T "../lib/ld/intram.ld"
	$(OBJCOPY) -O binary linux_boot.elf linux_boot.bin
	perl bin2hex.pl linux_boot.bin > linux_boot.hex

clean:
	rm *.elf *.bin *.hex *.o 2>/dev/null || true
