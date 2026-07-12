TARGET := qemu_wait_serial
AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy

.PHONY: all clean

all: $(TARGET).hex

$(TARGET).o: main.S
	$(AS) -march=armv5te -mcpu=arm926ej-s $< -o $@

$(TARGET).elf: $(TARGET).o
	$(LD) -Ttext=0 -e _start --gc-sections $< -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).hex: $(TARGET).bin
	perl ../bin2hex.pl $< > $@

clean:
	rm -f $(TARGET).o $(TARGET).elf $(TARGET).bin $(TARGET).hex
