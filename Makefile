####################################################
# kahuna -- simple USBasp compatible isp programmer
# 'make' configuration file for avr-gcc
# by fd0@koeln.ccc.de, for U23 2008
####################################################

# hardware platform
HARDWARE = kahuna

####################################################
# Note: 
#  To flash the Kahuna firmware to an empty ATmega8 (no bootloader, no self-update):
#  - Connect an ISP (can be Arduino as ISP) to the 6- or 10-pin header on the Kahuna, 
#      including VCC and ground. Make sure the programmer pulls the T_RST pin low.
#  - Do not connect USB to the Kahuna!
#  - Set the jumpers (i.e. close) T_VCC and RSTIN.
#  - Edit the Makefile and set PROG and DEV according to your setup
#  - run 'make all'
#  - run 'make program'
#  - run 'make fuses'
#  - The red LED on the Kahuna indicates that it is creating a new USB ID. 
#      This should only happen once every time the EEPROM on the chip is erased, 
#      e.g. when reflashing.
#  - Disconnect the programmer and remove the jumpers from the Kahuna. Plug USB into 
#      the Kahuna. The green LED should start blinking to show it is ready. 
####################################################


# controller
MCU = atmega8

# frequency
F_CPU = 16000000UL

# main application name (without .hex)
# eg 'test' when the main function is defined in 'test.c'
TARGET = kahuna

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
# use avrisp for Arduino as ISP
#PROG = avrisp 

# avrdude programmer device
DEV = usb

# further flags for avrdude
AVRDUDE_FLAGS =

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

# set hardware platform
CFLAGS += "-DHARDWARE_$(HARDWARE)"
ASFLAGS += "-DHARDWARE_$(HARDWARE)"

####################################################
# avrdude configuration
####################################################
ifeq ($(MCU),atmega8)
	AVRDUDE_MCU=m8
	HFUSE=0xc9  
	LFUSE=0x3f
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

fuses:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -c $(PROG) -P $(DEV) -u -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m	
	
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
