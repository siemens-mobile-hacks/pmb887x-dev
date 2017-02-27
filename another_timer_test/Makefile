INCLUDE=-I ../common
SRCS=../common/pmb8876.c ../common/printf.c ../common/i2c.c main.c interrupt.c bogomips.c
SRCAS=handlers.S div/__aeabi_uidivmod.S
HEADERS=$(wildcard *.h)
OBJS=${SRCS:c=o} ${SRCAS:S=o}
EXECUTABLE=xuj.elf
BINARY=${EXECUTABLE:elf=bin} 

%.o : %.c $(HEADERS)
	arm-none-eabi-gcc -c -mcpu=arm926ej-s $(INCLUDE) -O2 -o $@ $<

%.o : %.S $(HEADERS)
	arm-none-eabi-as -c -mcpu=arm926ej-s -o $@ $<


${EXECUTABLE}: ${OBJS}
	arm-none-eabi-ld -e __enter_reset -nostdlib --gc-sections ${OBJS} -o $@ -T linker.ld
	arm-none-eabi-objcopy -O binary $(EXECUTABLE) $(BINARY)
	
clean:
	rm -f $(OBJS) $(EXECUTABLE) $(BINARY)
