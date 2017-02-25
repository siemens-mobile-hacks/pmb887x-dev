INCLUDE=-I ../common
C_SOURCES=../common/pmb8876.c main.c ../common/i2c.c
OBJECTS=init.o pmb8876.o main.o i2c.o
BIN_NAME=i2c_test

all: clean
# compile
	arm-none-eabi-gcc -e _start -fvisibility=hidden -fdata-sections -ffunction-sections -c -fomit-frame-pointer -mcpu=arm926ej-s -nostdlib -nostdinc $(INCLUDE) -O0 $(C_SOURCES)
	
# init
	arm-none-eabi-as -c -mcpu=arm926ej-s ../common/init.S -o init.o
	
# link
	arm-none-eabi-ld --gc-sections $(OBJECTS) -o "$(BIN_NAME).elf" -T "../common/linker.ld"
	
# extract binary
	arm-none-eabi-objcopy -j .xuj -O binary "$(BIN_NAME).elf" "$(BIN_NAME).bin"
	ls -lah "$(BIN_NAME).bin"
clean:
	rm *.elf *.bin *.o 2>/dev/null || true
