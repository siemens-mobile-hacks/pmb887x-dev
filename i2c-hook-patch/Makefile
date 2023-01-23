PROJECT := app

PREFIX	?= arm-none-eabi-
GDB	= $(PREFIX)gdb
CC	= $(PREFIX)gcc
CXX	= $(PREFIX)g++
LD	= $(PREFIX)gcc
OBJCOPY	= $(PREFIX)objcopy
OBJDUMP	= $(PREFIX)objdump

CFILES := main.c printf.c

BUILD_DIR := bin

CFLAGS := -march=armv5te -mtune=arm926ej-s -msoft-float -mfloat-abi=soft -ffreestanding
CFLAGS += -Wall -Wundef -fno-common -ffunction-sections -fdata-sections  -Wextra -Wshadow
CFLAGS += -Wno-unused-variable -Wimplicit-function-declaration -fPIC
CFLAGS += -Wredundant-decls -Wstrict-prototypes -Wmissing-prototypes -fvisibility=hidden -O0
CFLAGS += -I../lib -DPMB8876

LDFLAGS := -Tlinker.ld -shared -nostartfiles -Wl,--gc-sections -fPIC
LDLIBS := -Wl,--start-group -nostdlib -lgcc -lnosys -Wl,--end-group

OBJS := $(CFILES:%.c=$(BUILD_DIR)/%.o)
OBJS += $(AFILES:%.S=$(BUILD_DIR)/%.o)

all: $(PROJECT).elf $(PROJECT).bin

$(BUILD_DIR)/%.o: %.c
	@printf "  CC\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: %.cpp
	@printf "  CXX\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: %.S
	@printf "  AS\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(ASFLAGS) $(CPPFLAGS) -o $@ -c $<

$(PROJECT).elf: $(OBJS) $(LDSCRIPT) $(LIBDEPS)
	@printf "  LD\t$@\n"
	$(Q)$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

%.bin: %.elf
	@printf "  OBJCOPY\t$@\n"
	$(Q)$(OBJCOPY) -O binary  $< $@

clean:
	@printf "$(OBJS)"
	rm -rf $(BUILD_DIR) $(PROJECT).elf $(PROJECT).bin
