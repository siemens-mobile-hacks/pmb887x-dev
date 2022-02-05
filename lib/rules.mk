BUILD_DIR ?= bin
OPT ?= -O2
CSTD ?= -std=c11
CXXSTD ?= -std=c++17
BOOT ?= intram
BOARD ?= EL71

############################################################################

INCLUDES += $(patsubst %,-I%, . $(LIB_DIR))

LIB_AFILES += $(LIB_DIR)/init/start.S
LIB_CFILES += $(LIB_DIR)/init/reset_handler.c
LIB_CFILES += $(LIB_DIR)/usart.c
LIB_CFILES += $(LIB_DIR)/i2c.c
LIB_CFILES += $(LIB_DIR)/printf.c
LIB_CFILES += $(LIB_DIR)/wdt.c
LIB_CFILES += $(LIB_DIR)/stopwatch.c

ifeq ($(BOOT),intram)
	ARCH_FLAGS += -DBOOT_INTRAM
	LDSCRIPT = $(LIB_DIR)/ld/intram.ld
endif

ifeq ($(BOOT),extram)
	ARCH_FLAGS += -DBOOT_EXTRAM
	LDSCRIPT = $(LIB_DIR)/ld/extram.ld
endif

ARCH_FLAGS += -mcpu=arm926ej-s -DBOARD_$(BOARD)

############################################################################

# Be silent per default, but 'make V=1' will show all compiler calls.
# If you're insane, V=99 will print out all sorts of things.
V?=0
ifeq ($(V),0)
Q	:= @
NULL	:= 2>/dev/null
endif

# Tool paths.
PREFIX	?= arm-none-eabi-
GDB	= $(PREFIX)gdb
CC	= $(PREFIX)gcc
CXX	= $(PREFIX)g++
LD	= $(PREFIX)gcc
OBJCOPY	= $(PREFIX)objcopy
OBJDUMP	= $(PREFIX)objdump

# Inclusion of library header files
INCLUDES += $(patsubst %,-I%, . $(LIB_DIR) )

OBJS += $(LIB_CFILES:$(LIB_DIR)/%.c=$(BUILD_DIR)/lib/%.o)
OBJS += $(LIB_CXXFILES:$(LIB_DIR)/%.cpp=$(BUILD_DIR)/lib/%.o)
OBJS += $(LIB_AFILES:$(LIB_DIR)/%.S=$(BUILD_DIR)/lib/%.o)

OBJS += $(CFILES:%.c=$(BUILD_DIR)/%.o)
OBJS += $(CXXFILES:%.cpp=$(BUILD_DIR)/%.o)
OBJS += $(AFILES:%.S=$(BUILD_DIR)/%.o)

GENERATED_BINS = $(PROJECT).elf $(PROJECT).bin $(PROJECT).old.elf $(PROJECT).map $(PROJECT).list $(PROJECT).lss

TGT_CPPFLAGS += -MD -flto
TGT_CPPFLAGS += -Wall -Wundef $(INCLUDES)
TGT_CPPFLAGS += $(INCLUDES) $(OPENCM3_DEFS)

TGT_CFLAGS += $(OPT) $(CSTD) -ggdb3
TGT_CFLAGS += $(ARCH_FLAGS)
TGT_CFLAGS += -fno-common
TGT_CFLAGS += -ffunction-sections -fdata-sections
TGT_CFLAGS += -Wextra -Wshadow -Wno-unused-variable -Wimplicit-function-declaration
TGT_CFLAGS += -Wredundant-decls -Wstrict-prototypes -Wmissing-prototypes

TGT_CXXFLAGS += $(OPT) $(CXXSTD) -ggdb3 -fno-exceptions -fno-use-cxa-atexit -fno-rtti
TGT_CXXFLAGS += $(ARCH_FLAGS)
TGT_CXXFLAGS += -fno-common
TGT_CXXFLAGS += -ffunction-sections -fdata-sections
TGT_CXXFLAGS += -Wextra -Wshadow -Wredundant-decls -Weffc++

TGT_ASFLAGS += $(OPT) $(ARCH_FLAGS) -ggdb3

TGT_LDFLAGS += -T$(LDSCRIPT) -L$(OPENCM3_DIR)/lib -nostartfiles
TGT_LDFLAGS += $(ARCH_FLAGS)
TGT_LDFLAGS += -specs=nano.specs
TGT_LDFLAGS += -u _printf_float
TGT_LDFLAGS += -Wl,--gc-sections
# OPTIONAL
#TGT_LDFLAGS += -Wl,-Map=$(PROJECT).map
ifeq ($(V),99)
TGT_LDFLAGS += -Wl,--print-gc-sections
endif

#LDLIBS += -Wl,--start-group -lc_nano -lgcc -lnosys -Wl,--end-group
LDLIBS += -Wl,--start-group -nostdlib -lgcc -lnosys -Wl,--end-group

# Burn in legacy hell fortran modula pascal yacc idontevenwat
.SUFFIXES:
.SUFFIXES: .c .S .h .o .cxx .cpp .elf .bin .list .lss

# Bad make, never *ever* try to get a file out of source control by yourself.
%: %,v
%: RCS/%,v
%: RCS/%
%: s.%
%: SCCS/s.%

all: $(PROJECT).elf $(PROJECT).bin

# Need a special rule to have a bin dir
$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.c
	@printf "  CC\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(TGT_CFLAGS) $(CFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.cpp
	@printf "  CXX\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(TGT_CXXFLAGS) $(CXXFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.S
	@printf "  AS\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(TGT_ASFLAGS) $(ASFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: %.c
	@printf "  CC\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(TGT_CFLAGS) $(CFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: %.cpp
	@printf "  CXX\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CXX) $(TGT_CXXFLAGS) $(CXXFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: %.S
	@printf "  AS\t$<\n"
	@mkdir -p $(dir $@)
	$(Q)$(CC) $(TGT_ASFLAGS) $(ASFLAGS) $(TGT_CPPFLAGS) $(CPPFLAGS) -o $@ -c $<

$(PROJECT).elf: $(OBJS) $(LDSCRIPT) $(LIBDEPS)
	@printf "  LD\t$@\n"
	$(Q)$(LD) $(TGT_LDFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

%.bin: %.elf
	@printf "  OBJCOPY\t$@\n"
	$(Q)$(OBJCOPY) -O binary  $< $@

%.lss: %.elf
	$(OBJDUMP) -h -S $< > $@

%.list: %.elf
	$(OBJDUMP) -S $< > $@

clean:
	@printf "$(OBJS)"
	rm -rf $(BUILD_DIR) $(GENERATED_BINS)

.PHONY: all clean
-include $(OBJS:.o=.d)

