####################################################
# kahuna -- simple USBasp compatible isp programmer
# 'make' configuration file for avr-gcc
# by fd0@koeln.ccc.de, for U23 2008
####################################################

# hardware platform
HARDWARE = kahuna

# usb serial number
USB_SERIAL = "8eea75b0decb11e5"

# controller
MCU = atmega8

# frequency
F_CPU = 16000000UL

# main application name (without .hex)
# eg 'test' when the main function is defined in 'test.c'
TARGET = rumpusbasp

# c sourcecode files
# eg. 'test.c foo.c foobar/baz.c'
SRC = $(wildcard *.c) usbdrv/usbdrv.c

# asm sourcecode files
# eg. 'interrupts.S foobar/another.S'
ASRC = usbdrv/usbdrvasm.S

# headers which should be considered when recompiling
# eg. 'global.h foobar/important.h'
HEADERS =

# include directories (used for both, c and asm)
# eg '. usbdrv/'
INCLUDES = . usbdrv/


# use more debug-flags when compiling
DEBUG = 0


# avrdude programmer protocol
PROG = usbasp
# avrdude programmer device
DEV = usb
# further flags for avrdude
AVRDUDE_FLAGS =

# magic for usb serial
USB_SERIAL_TEXT=$(shell echo $(USB_SERIAL) | sed "s/./'\\0',/g")
USB_SERIAL_LEN=$(shell echo -n $(USB_SERIAL) | wc -c)

####################################################
# 'make' configuration
####################################################
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AS = avr-as
SIZE = avr-size
CP = cp
RM = rm -f
RMDIR = rm -rf
MKDIR = mkdir
AVRDUDE = avrdude

# flags for automatic dependency handling
DEPFLAGS = -MD -MP -MF .dep/$(@F).d

# flags for the compiler (for .c files)
CFLAGS += -g -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=gnu99 -fshort-enums $(DEPFLAGS)
CFLAGS += $(addprefix -I,$(INCLUDES))
# flags for the compiler (for .S files)
ASFLAGS += -g -mmcu=$(MCU) -DF_CPU=$(F_CPU) -x assembler-with-cpp $(DEPFLAGS)
ASFLAGS += $(addprefix -I,$(INCLUDES))
# flags for the linker
LDFLAGS += -mmcu=$(MCU)

# fill in object files
OBJECTS += $(SRC:.c=.o)
OBJECTS += $(ASRC:.S=.o)

# include config file
-include $(CURDIR)/config.mk

# include more debug flags, if $(DEBUG) is 1
ifeq ($(DEBUG),1)
	CFLAGS += -Wall -W -Wchar-subscripts -Wmissing-prototypes
	CFLAGS += -Wmissing-declarations -Wredundant-decls
	CFLAGS += -Wstrict-prototypes -Wshadow -Wbad-function-cast
	CFLAGS += -Winline -Wpointer-arith -Wsign-compare
	CFLAGS += -Wunreachable-code -Wdisabled-optimization
	CFLAGS += -Wcast-align -Wwrite-strings -Wnested-externs -Wundef
	CFLAGS += -Wa,-adhlns=$(basename $@).lst
	CFLAGS += -DDEBUG
endif

# set hardware platform and usb serial number
CFLAGS += "-DHARDWARE_$(HARDWARE)" -DUSB_SERIAL="$(USB_SERIAL_TEXT)" -DUSB_SERIAL_LEN="$(USB_SERIAL_LEN)"
ASFLAGS += "-DHARDWARE_$(HARDWARE)" -DUSB_SERIAL="$(USB_SERIAL_TEXT)" -DUSB_SERIAL_LEN="$(USB_SERIAL_LEN)"

####################################################
# avrdude configuration
####################################################
ifeq ($(MCU),atmega8)
	AVRDUDE_MCU=m8
endif
ifeq ($(MCU),atmega48)
	AVRDUDE_MCU=m48
endif
ifeq ($(MCU),atmega88)
	AVRDUDE_MCU=m88
endif
ifeq ($(MCU),atmega168)
	AVRDUDE_MCU=m168
endif

AVRDUDE_FLAGS += -p $(AVRDUDE_MCU)

####################################################
# make targets
####################################################

.PHONY: all clean distclean avrdude-terminal

# main rule
all: $(TARGET).hex

$(TARGET).elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# all objects (.o files)
$(OBJECTS): $(HEADERS)

# remove all compiled files
clean:
	$(RM) $(foreach ext,elf hex eep.hex map,$(TARGET).$(ext)) \
		$(foreach file,$(patsubst %.o,%,$(OBJECTS)),$(foreach ext,o lst lss,$(file).$(ext)))

# additionally remove the dependency makefile
distclean: clean
	$(RMDIR) .dep

# avrdude-related targets
install program: program-$(TARGET)

avrdude-terminal:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -c $(PROG) -P $(DEV) -t

program-%: %.hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -c $(PROG) -P $(DEV) -U flash:w:$<

program-eeprom-%: %.eep.hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -c $(PROG) -P $(DEV) -U eeprom:w:$<

# special programming targets
%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	@echo "========================================"
	@echo "$@ compiled for: $(MCU)"
	@echo "for hardware $(HARDWARE)"
	@echo -n "size for $< is "
	@$(SIZE) -A $@ | grep '\.sec1' | tr -s ' ' | cut -d" " -f2
	@echo "========================================"

%.eep.hex: %.elf
	$(OBJCOPY) --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex -j .eeprom $< $@

%.lss: %.elf
	$(OBJDUMP) -h -S $< > $@

-include $(shell $(MKDIR) .dep 2>/dev/null) $(wildcard .dep/*)
