# ATmega8 clock + outdoor thermometer

Firmware for the hand-built board: ATmega8, DS1302 RTC with 32.768 kHz crystal
and 3 V backup cell, DS18B20 outdoor sensor, two 4-digit common-cathode LED
displays driven by BC238 NPN transistors, HRS/MIN push buttons, powered from a
5 V phone charger.

Most Used commands(copy and paste them in terminal):
cd ~/atmega8-clock-thermo
export PATH="/opt/homebrew/opt/avr-gcc@14/bin:$PATH"

# 1. check the connection — should print "Device signature = 0x1e9307"
avrdude -c avrisp2 -p m8 -B 10

# 2. fuses once (internal RC 8 MHz), then the firmware
make fuses PROGRAMMER=avrisp2
make flash PROGRAMMER=avrisp2

---

## 1. Where everything is

```
~/atmega8-clock-thermo/
├── Makefile            build / flash / fuses / hardware test commands (macOS/Linux)
├── build.bat           same, for Windows without make (see §8)
├── flash.bat           flashing on Windows (see §8)
├── README.md           this file
├── .gitignore          ignores build/
├── build/              compiler output, safe to delete (make clean)
│   └── firmware.hex    the file that goes onto the chip
└── src/
    ├── config.h        ALL the knobs: pin map, brightness, timings, options
    ├── main.c          application: buttons, RTC polling, temperature loop,
    │                   hardware test mode
    ├── display.c/.h    8-digit multiplexing (Timer1 interrupt), 7-seg font,
    │                   time/temperature formatting
    ├── ds1302.c/.h     DS1302 real-time clock driver (3-wire)
    ├── onewire.c/.h    bit-banged 1-Wire bus
    └── ds18b20.c/.h    DS18B20 temperature sensor (uses onewire)
```

Rule of thumb: if you want to *change a setting*, edit `src/config.h`.
If you want to *change behaviour*, edit `src/main.c`. The driver files
should not need touching unless the hardware changes.

---

## 2. What the device does

| | |
|---|---|
| Left display (X1) | time `HH:MM`, 24 h, colon blinking at 1 Hz. `--:--` if the DS1302 does not answer |
| Right display (X2) | temperature with one decimal: ` 21.4`, `-14.8`, ` -9.8`. `Err` after 3 failed reads in a row (sensor unplugged, bad wire, CRC error). Updates once per second |
| HRS button | hours + 1. Hold for auto-repeat |
| MIN button | minutes + 1 and seconds reset to 0. Hold for auto-repeat |
| Power-up | all segments on for 0.6 s (lamp test), then normal display |
| First power-up / dead battery | clock starts at 12:00 |

The DS1302 keeps time on the backup cell while the board is unplugged. If it
ever comes up showing `12:00` after being unplugged, the cell is dead.

---

## 3. Pin map

| MCU pin | Function |
|---------|----------|
| PD0..PD7 | segments a, b, c, d, e, f, g, DP (through 8 × 100 Ω, shared by both displays) |
| PB0..PB3 | clock digits A0 (right) .. A3 (left), via Q4..Q1 |
| PB4..PB7 | thermometer digits A4 (right) .. A7 (left), via Q8..Q5 |
| PC0 / PC1 | HRS / MIN buttons (to GND, internal pull-ups) |
| PC2 | DS18B20 DQ (4K7 pull-up to +5 V) |
| PC3 / PC4 / PC5 | DS1302 CE / IO / SCLK |
| PC6 | /RESET (10 k pull-up) |

PB6/PB7 (the XTAL pins) drive display digits, so the MCU has **no crystal**
and runs from its internal RC oscillator. Never set the fuses to an external
clock; the chip would stop responding to the programmer.

---

## 4. Tools

(Windows users: skip to §8.)

One-time setup on macOS:

```sh
brew tap osx-cross/avr
brew install avr-gcc@14 avrdude
```

`avr-gcc@14` is not on the PATH by default. Either run this in every new
terminal:

```sh
export PATH="/opt/homebrew/opt/avr-gcc@14/bin:$PATH"
```

or add that line to `~/.zshrc` once so it is permanent.

Programmer: AVRISP mkII (`PROGRAMMER=avrisp2`). ISP pinout on the 6-pin cable,
red stripe = pin 1, looking into the connector:

```
  MISO (1)  o  o  (2) VCC      MISO  -> PB4, ATmega8 pin 18
  SCK  (3)  o  o  (4) MOSI     SCK   -> PB5, pin 19
  RESET(5)  o  o  (6) GND      MOSI  -> PB3, pin 17
                               RESET -> PC6, pin 1
                               VCC/GND -> pins 7 / 8
```

The mkII does not power the board; power it from the charger first, then
connect the programmer. LED green = ready.

---

## 5. Everyday commands

All from inside `~/atmega8-clock-thermo`:

| Command | What it does |
|---|---|
| `make` | compile, produce `build/firmware.hex`, print size |
| `make flash PROGRAMMER=avrisp2` | compile if needed, then write to the chip |
| `make fuses PROGRAMMER=avrisp2` | set internal RC 8 MHz (lfuse 0xE4, hfuse 0xD9). **Only needed once per chip**, already done |
| `make hwtest PROGRAMMER=avrisp2` | flash the display hardware test (see §7), then clean |
| `make clean` | delete `build/` |
| `avrdude -c avrisp2 -p m8` | just check the chip answers (prints nothing but "done" on success) |

To avoid typing `PROGRAMMER=avrisp2` every time, change the default in the
Makefile:

```make
PROGRAMMER ?= avrisp2
```

Other programmers: `usbasp`, `usbtiny`, `stk500v1` (Arduino as ISP, add
`PORT=/dev/tty.usbmodemXXXX BAUD=19200`), `atmelice_isp`, `pickit4_isp`.

---

## 6. Modifying the firmware

Workflow is always: edit → `make` → `make flash PROGRAMMER=avrisp2`.
Fuses do not need to be redone.

### 6.1 Settings in `src/config.h`

| Setting | Default | Meaning |
|---|---|---|
| `DISPLAY_BRIGHTNESS` | `100` | Duty cycle in percent (10..100). With 100 Ω segment resistors a lit `8.` pulls ~200 mA through the MCU, which is at its absolute maximum. If the ATmega8 or the BC238s get hot, use 50–70 |
| `CLOCK_BLANK_LEADING_ZERO` | `1` | `1` shows ` 9:05`, `0` shows `09:05` |
| `CLOCK_BLINK_COLON` | `1` | `1` blinks the colon at 1 Hz, `0` keeps it steady |
| `RTC_POLL_MS` | `100` | how often the DS1302 is read |
| `TEMP_PERIOD_MS` | `1000` | how often a temperature measurement starts |
| `TEMP_CONVERT_MS` | `800` | wait before reading the result (DS18B20 needs ≤ 750 ms). Do not lower |
| `BTN_POLL_MS` | `10` | button sampling period |
| `BTN_REPEAT_DELAY_MS` | `600` | how long to hold before auto-repeat starts |
| `BTN_REPEAT_RATE_MS` | `150` | auto-repeat speed |

The pin assignments are also in this file (`SEG_PORT`, `DIG_PORT`, `BTN_*`,
`OW_*`, `RTC_*`). If you rewire something, change it there and nowhere else.

### 6.2 Changing what the displays show

`src/display.c`:

- `display_show_time()` — how the clock is formatted.
- `display_show_temp()` — how the temperature is formatted. For example to
  drop the decimal and show `-14` instead of `-14.8`, format the digits from
  `tenths / 10`.
- `glyph_font[]` — the 7-segment patterns. Add a letter by adding a row here
  and a matching `GLYPH_xxx` index in `display.h`.

Digit indexes (`DIG_CLK_HR_TENS`, `DIG_TMP_UNITS`, ...) are defined in
`display.h`; `display_set_glyph(digit, glyph, dp)` writes one digit.

### 6.3 Changing behaviour

`src/main.c`, the `for (;;)` loop in `main()`. Each block is independent and
timed with `millis()`:

- buttons → `button_poll()`, `adjust_hours()`, `adjust_minutes()`
- RTC poll → `rtc_read()` then `refresh_clock()`
- temperature → `ds18b20_start_conversion()` / `ds18b20_read_tenths()`

Ideas that fit easily: show seconds while both buttons are held
(`now.sec` is already available), show the date (`now.date`, `now.month`),
alternate the thermometer between temperature and time.

### 6.4 Running at 1 MHz instead of 8 MHz

Only relevant for a brand-new chip whose fuses you do not want to touch:

```sh
make clean && make F_CPU=1000000UL && make flash PROGRAMMER=avrisp2 F_CPU=1000000UL
```

The current chip is fused for 8 MHz, so the plain `make` is correct for it.

---

## 7. Troubleshooting

### Displays dim or dark

Segment current is set by the 8 resistors on PD0–PD7 and must be **100 Ω**
(brown‑black‑brown). 10 kΩ (brown‑black‑orange) gives 100× less current:
the small display glows faintly, a large display with 2 LEDs per segment
(forward voltage ~4 V) shows nothing at all. Measure one with the board off.

If the clock display is a very large type (2.3" / 57 mm, forward voltage
7–8 V per segment), 5 V cannot drive it regardless of resistor value and the
segment supply would need to be raised. Check the display's part number.

### Hardware test mode

```sh
make hwtest PROGRAMMER=avrisp2
```

Flashes a test program that loops forever:

1. all eight digits show `8.` for 3 s → every segment and digit should light
2. a single `8.` walks across the eight digits, 0.5 s each → checks each
   digit transistor and PB0..PB7 wire
3. one segment at a time (a, b, c, d, e, f, g, DP) on all digits → checks
   each PD0..PD7 resistor and wire
4. the digits show `0 1 2 3 4 5 6 7` for 3 s → checks the digit order
   (clock should read `3 2 1 0` left to right, thermometer `7 6 5 4`)

A digit that never lights in step 2 → its transistor, base resistor or PB
wire. A segment that never lights in step 3 → its PD resistor or wire. Both
displays missing the same segment → the shared bus line. When done:

```sh
make flash PROGRAMMER=avrisp2
```

### Thermometer shows `Err`

No sensor detected or bad data three times in a row. Check the DS18B20
wiring: GND, DQ (to PC2 with the 4K7 pull-up), VCC; and that the sensor
really is a DS18B20 (the firmware uses 12-bit scratchpad reads with CRC).
Exactly 85.0 °C is also rejected because it is the sensor's power-on value.

### Clock shows `--:--`

The DS1302 is not answering or reports garbage. Check CE/IO/SCLK to
PC3/PC4/PC5, the 32.768 kHz crystal, and VCC2 (pin 1) = 5 V. Pressing HRS
or MIN writes a time and will make it show briefly even if the chip is not
really working.

### Programmer errors

| Message | Cause |
|---|---|
| `Target not detected` | board not powered, or VCC (pin 2) of the ISP cable not connected |
| `Target reverse inserted` | cable rotated 180°, or RESET wire tied to 5 V / swapped |
| `target doesn't answer` / signature `0x000000` | MOSI/MISO/SCK/RESET wiring; try `-B 10` for a slower clock |
| (Windows) `did not find any USB device "usb"` / `cannot open USB device` | AVRISP mkII driver not switched to WinUSB, see §8.2 |

Never use `-F`; it only hides the problem.

---

## 8. Windows

Everything works on Windows too; the differences are the tools and the
USB driver for the AVRISP mkII. `build.bat` and `flash.bat` replace `make`,
so nothing else has to be installed.

The folder already contains a compiled `build\firmware.hex`. **If you only
want to flash, you need avrdude and the driver (8.1 + 8.2), not the compiler.**

### 8.1 avrdude

1. Download the Windows zip from <https://github.com/avrdudes/avrdude/releases>
   (file name like `avrdude-v8.x-windows-x64.zip`).
2. Unpack it so that `avrdude.exe` ends up in `tools\avrdude\` **inside this
   folder** (next to `flash.bat`). The scripts look there first, so nothing
   needs to be added to the PATH.

### 8.2 AVRISP mkII driver (the important part)

Windows does not have a driver for the mkII out of the box, and if
Microchip/Atmel Studio is installed it grabs the programmer with its own
"Jungo" driver, which avrdude cannot use. Either way, install the WinUSB
driver once with Zadig:

1. Download Zadig from <https://zadig.akeo.ie> (no install, just run it).
2. Plug in the AVRISP mkII.
3. In Zadig: *Options → List All Devices*, then pick **AVRISP mkII** in the
   dropdown.
4. Make sure the target driver (right-hand box) says **WinUSB**, then click
   *Install Driver* / *Replace Driver*.

This only needs to be done once per PC. (If you later want to use the mkII
with Microchip Studio again, Zadig can switch it back.)

### 8.3 Flashing

Power the board from its charger, connect the ISP cable (pinout in §4,
LED on the programmer must be green), open a Command Prompt in this folder
(type `cmd` in the Explorer address bar) and run:

```bat
flash.bat check      REM must end with "Avrdude done. Thank you." and no error
flash.bat            REM writes build\firmware.hex
```

`flash.bat fuses` is only for a brand-new ATmega8; the chip in this board
already has the right fuses. `flash.bat test` writes the hardware test (§7)
after `build.bat test`.

Different programmer, e.g. USBasp: `set PROGRAMMER=usbasp` first, then run
the script in the same window.

### 8.4 Building (only if you change the source)

1. Download a Windows avr-gcc toolchain. The easiest are the ready-made
   builds at <https://blog.zakkemble.net/avr-gcc-builds/> (zip named like
   `avr-gcc-14.x.x-x64-windows.zip`). Microchip's "AVR 8-bit Toolchain"
   also works but requires a login.
2. Unpack it into `tools\avr-gcc\` inside this folder so that
   `tools\avr-gcc\bin\avr-gcc.exe` exists.
3. Run:

```bat
build.bat            REM -> build\firmware.hex
flash.bat
```

`build.bat test` produces `build\hwtest.hex` for the hardware test.

### 8.5 Alternative: WSL / MSYS2

If you already use WSL (Ubuntu) or MSYS2, `sudo apt install gcc-avr avr-libc
avrdude make` and then the normal `make` commands from §5 work unchanged.
USB access to the programmer from WSL needs `usbipd-win`; it is usually
simpler to build in WSL and flash from Windows with `flash.bat`.
