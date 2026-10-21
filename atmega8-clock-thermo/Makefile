# ATmega8 clock + thermometer firmware
#
#   make                 build build/firmware.hex (F_CPU = 8 MHz internal RC)
#   make F_CPU=1000000UL build for factory-default fuses (1 MHz internal RC)
#   make fuses           program fuses for 8 MHz internal RC (needed once)
#   make flash           upload the firmware
#   make hwtest          build + flash the display hardware test, then clean
#   make clean
#
# Programmer settings (override on the command line, e.g. make flash PROGRAMMER=stk500v1 PORT=/dev/tty.usbmodem14101 BAUD=19200)

MCU        = atmega8
F_CPU     ?= 8000000UL
PROGRAMMER ?= usbasp
PORT       ?=
BAUD       ?=

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size
AVRDUDE = avrdude

SRC_DIR   = src
BUILD_DIR = build
TARGET    = $(BUILD_DIR)/firmware

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu99
CFLAGS += -Wall -Wextra -Wundef -Wstrict-prototypes
CFLAGS += -ffunction-sections -fdata-sections -fshort-enums
CFLAGS += -I$(SRC_DIR) $(EXTRA_CFLAGS)
LDFLAGS = -mmcu=$(MCU) -Wl,--gc-sections,--relax

AVRDUDE_FLAGS = -c $(PROGRAMMER) -p m8
ifneq ($(PORT),)
AVRDUDE_FLAGS += -P $(PORT)
endif
ifneq ($(BAUD),)
AVRDUDE_FLAGS += -b $(BAUD)
endif

# Fuses for internal RC 8 MHz, no BOD, SPI enabled, reset enabled
LFUSE = 0xE4
HFUSE = 0xD9

.PHONY: all clean flash fuses size hwtest

all: $(TARGET).hex size

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/config.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(TARGET).elf
	$(SIZE) -C --mcu=$(MCU) $< 2>/dev/null || $(SIZE) $<

flash: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:w:$(TARGET).hex:i

fuses:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m

# Display hardware test: all on, walk digits, walk segments, show 01234567.
# Flash the normal firmware again afterwards with "make flash".
hwtest:
	$(MAKE) clean
	$(MAKE) EXTRA_CFLAGS=-DDISPLAY_TEST flash
	$(MAKE) clean

clean:
	rm -rf $(BUILD_DIR)
