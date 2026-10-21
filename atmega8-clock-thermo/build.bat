@echo off
rem ---------------------------------------------------------------------------
rem  Windows build script (no "make" needed)
rem
rem    build.bat          -> build\firmware.hex   (normal firmware)
rem    build.bat test     -> build\hwtest.hex     (display hardware test)
rem
rem  Needs avr-gcc. Either put it on the PATH or unpack it into
rem  tools\avr-gcc\ next to this file so that tools\avr-gcc\bin\avr-gcc.exe
rem  exists. See README, section "Windows".
rem ---------------------------------------------------------------------------
setlocal EnableExtensions
cd /d "%~dp0"

if exist "%~dp0tools\avr-gcc\bin\avr-gcc.exe" set "PATH=%~dp0tools\avr-gcc\bin;%PATH%"

where avr-gcc >nul 2>&1
if errorlevel 1 (
    echo avr-gcc.exe not found.
    echo Unpack the avr-gcc toolchain into tools\avr-gcc\ or add it to PATH.
    echo See README.md, section "Windows".
    exit /b 1
)

set "MCU=atmega8"
set "F_CPU=8000000UL"
set "OUT=firmware"
set "EXTRA="
if /i "%~1"=="test" (
    set "EXTRA=-DDISPLAY_TEST"
    set "OUT=hwtest"
)

if not exist build mkdir build

set "CFLAGS=-mmcu=%MCU% -DF_CPU=%F_CPU% -Os -std=gnu99 -Wall -Wextra -Wundef -Wstrict-prototypes -ffunction-sections -fdata-sections -fshort-enums -Isrc %EXTRA%"

echo Compiling...
avr-gcc %CFLAGS% -Wl,--gc-sections,--relax src\main.c src\display.c src\ds1302.c src\onewire.c src\ds18b20.c -o build\%OUT%.elf
if errorlevel 1 exit /b 1

avr-objcopy -O ihex -R .eeprom build\%OUT%.elf build\%OUT%.hex
if errorlevel 1 exit /b 1

avr-size build\%OUT%.elf
echo.
echo OK: build\%OUT%.hex
exit /b 0
