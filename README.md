# ATmega8 Clock + Outdoor Thermometer

Firmware for a hand-built ATmega8 board featuring:

- ATmega8 microcontroller
- DS1302 RTC with 32.768 kHz crystal and 3 V backup cell
- DS18B20 outdoor temperature sensor
- Two 4-digit common-cathode LED displays driven by BC238 NPN transistors
- HRS/MIN adjustment buttons
- 5 V phone-charger power supply

This project is designed to run as a compact wall clock with an outdoor thermometer display, using the internal RC oscillator of the ATmega8 and a custom 7-segment multiplexing driver.

## Features

- 24-hour clock display on the left panel (`HH:MM`)
- Outdoor temperature display on the right panel with one decimal place
- Clock keeps running from the RTC backup cell while unplugged
- Button-based time adjustment with repeat behavior
- Built-in display hardware test mode
- Simple make-based build and flash workflow for macOS/Linux/Windows

## Quick Start

From the project directory:

```bash
cd ~/atmega8-clock-thermo
export PATH="/opt/homebrew/opt/avr-gcc@14/bin:$PATH"
```

Check the programmer connection:

```bash
avrdude -c avrisp2 -p m8 -B 10
```

The output should include:

```text
Device signature = 0x1e9307
```

Then set the fuses once and flash the firmware:

```bash
make fuses PROGRAMMER=avrisp2
make flash PROGRAMMER=avrisp2
```

## Project Layout

```text
~/atmega8-clock-thermo/
├── Makefile            build / flash / fuses / hardware test commands
├── build.bat           Windows build helper
├── flash.bat           Windows flash helper
├── README.md           project documentation
├── .gitignore          ignores build/
├── build/              compiler output, safe to delete with make clean
│   └── firmware.hex    firmware image for the chip
└── src/
    ├── config.h        all hardware and runtime settings
    ├── main.c          main loop, buttons, RTC polling, sensor logic
    ├── display.c/.h    multiplexing and 7-seg glyph handling
    ├── ds1302.c/.h     DS1302 RTC driver
    ├── onewire.c/.h    1-Wire bitbanged bus driver
    └── ds18b20.c/.h    DS18B20 sensor driver
```

Rule of thumb:

- Edit `src/config.h` to change settings
- Edit `src/main.c` to change behavior
- Leave driver files alone unless the hardware changes

## Device Behavior

| Element | Behavior |
|---|---|
| Left display (X1) | Clock time in `HH:MM` format, 24-hour mode, colon blinks at 1 Hz |
| Right display (X2) | Temperature with one decimal place, e.g. `21.4`, `-14.8` |
| HRS button | Increments hours; repeats when held |
| MIN button | Increments minutes and resets seconds; repeats when held |
| Power-up | Shows all segments for ~0.6 s as a lamp test |
| Dead battery / first boot | Clock resets to `12:00` |

If the DS1302 is unplugged for too long, the clock may start at `12:00` again. That is a sign the backup cell is exhausted.

## Pin Map

| MCU pin | Function |
|---|---|
| PD0..PD7 | Segment lines a, b, c, d, e, f, g, DP |
| PB0..PB3 | Digit select lines for clock digits |
| PB4..PB7 | Digit select lines for temperature digits |
| PC0 / PC1 | HRS / MIN buttons |
| PC2 | DS18B20 data line |
| PC3 / PC4 / PC5 | DS1302 CE / IO / SCLK |
| PC6 | RESET |

The ATmega8 uses its internal RC oscillator. The fuse configuration must not be changed to an external clock, or the chip may stop responding to the programmer.

## Toolchain Setup

### macOS / Linux

```bash
brew tap osx-cross/avr
brew install avr-gcc@14 avrdude
```

If `avr-gcc@14` is not on your PATH, add:

```bash
export PATH="/opt/homebrew/opt/avr-gcc@14/bin:$PATH"
```

Or add it permanently to your shell config (`~/.zshrc`, for example).

### Programmer

This project uses an AVRISP mkII (`PROGRAMMER=avrisp2`).

ISP cable pinout (red stripe = pin 1, looking into the connector):

```text
MISO (1)  o  o  (2) VCC      MISO -> PB4
SCK  (3)  o  o  (4) MOSI     SCK  -> PB5
RESET(5)  o  o  (6) GND      MOSI -> PB3
                             RESET -> PC6
                             VCC/GND -> pins 7 / 8
```

Power the board from the charger first, then connect the programmer. A green LED indicates readiness.

## Common Commands

All commands are run from the project root:

| Command | Purpose |
|---|---|
| `make` | Build firmware and generate `build/firmware.hex` |
| `make flash PROGRAMMER=avrisp2` | Compile if needed and write the firmware |
| `make fuses PROGRAMMER=avrisp2` | Set the ATmega8 fuses once |
| `make hwtest PROGRAMMER=avrisp2` | Flash the hardware test program |
| `make clean` | Remove the build directory |
| `avrdude -c avrisp2 -p m8` | Check that the programmer and chip are communicating |

To avoid repeating `PROGRAMMER=avrisp2`, set the default in the Makefile:

```make
PROGRAMMER ?= avrisp2
```

Other supported programmers include `usbasp`, `usbtiny`, `stk500v1`, `atmelice_isp`, and `pickit4_isp`.

## Modifying the Firmware

Recommended workflow:

```bash
make
make flash PROGRAMMER=avrisp2
```

### Settings in `src/config.h`

| Setting | Default | Meaning |
|---|---:|---|
| `DISPLAY_BRIGHTNESS` | `100` | Duty cycle percentage |
| `CLOCK_BLANK_LEADING_ZERO` | `1` | Hide leading zero in the time |
| `CLOCK_BLINK_COLON` | `1` | Blink the time colon |
| `RTC_POLL_MS` | `100` | RTC polling interval |
| `TEMP_PERIOD_MS` | `1000` | Temperature read cycle |
| `TEMP_CONVERT_MS` | `800` | Temperature conversion delay |
| `BTN_POLL_MS` | `10` | Button sampling period |
| `BTN_REPEAT_DELAY_MS` | `600` | Delay before auto-repeat |
| `BTN_REPEAT_RATE_MS` | `150` | Auto-repeat rate |

### Display behavior

The formatting logic lives in `src/display.c`:

- `display_show_time()` formats the time
- `display_show_temp()` formats the temperature
- `glyph_font[]` defines the 7-segment character patterns

### Runtime behavior

`src/main.c` contains the main loop and timing logic for:

- button polling
- RTC refresh
- temperature conversion and reads

This makes it easy to extend the project with features such as:

- showing seconds while both buttons are held
- displaying the date
- alternating between clock and temperature

## Troubleshooting

### Displays dim or dark

The segment current is set by the eight resistors on PD0–PD7 and should be 100 Ω. Using a much larger resistor value greatly reduces brightness, and a large display may not light at all from a 5 V supply.

### Hardware test mode

```bash
make hwtest PROGRAMMER=avrisp2
```

The hardware test program performs these checks:

1. All digits light up `8.` for 3 seconds
2. A single `8.` walks across the digits to verify transistors and digit wiring
3. Each segment lights individually across all digits
4. The digits show `0 1 2 3 4 5 6 7` to verify digit order

When testing is complete, restore normal firmware:

```bash
make flash PROGRAMMER=avrisp2
```

### Thermometer shows `Err`

This usually means the DS18B20 is not detected or returned invalid data. Check:

- GND connection
- DQ signal on PC2 with the 4.7 kΩ pull-up to VCC
- DS18B20 orientation and power
- correct sensor type (must be DS18B20)

### Clock shows `--:--`

The DS1302 is not answering correctly. Check:

- CE / IO / SCLK wiring to PC3 / PC4 / PC5
- 32.768 kHz crystal
- RTC power rail and backup cell

### Programmer errors

| Message | Cause |
|---|---|
| `Target not detected` | board not powered or ISP VCC not connected |
| `Target reverse inserted` | cable rotated 180° |
| `target doesn't answer` | bad wiring or slow clock; try `-B 10` |

Never use `-F` to force programming; it hides the real problem.

## Windows Build and Flash

Everything works on Windows too, although the setup differs slightly.

### Required tools

- `avrdude` for flashing
- AVRISP mkII driver via Zadig (WinUSB)
- optional AVR GCC toolchain if rebuilding source files

### Flashing

Open a Command Prompt in the project folder and run:

```bat
flash.bat check
flash.bat
```

`flash.bat fuses` is only for a brand-new chip. The board in this project already has the correct fuses.

### Building from source on Windows

1. Download a Windows AVR GCC toolchain
2. Unpack it into `tools\avr-gcc\` inside the project
3. Run:

```bat
build.bat
flash.bat
```

### Alternative: WSL / MSYS2

If you already use WSL or MSYS2, you can install:

```bash
sudo apt install gcc-avr avr-libc avrdude make
```

Then use the same `make` commands from the Linux/macOS instructions.

## Summary

This project is a compact embedded firmware project for a custom ATmega8-based clock and outdoor thermometer. It is intended to be easy to modify, easy to flash, and quick to troubleshoot with a small hardware test suite.

If you want to change the display behavior or the timing logic, start with `src/config.h` and `src/main.c`.
