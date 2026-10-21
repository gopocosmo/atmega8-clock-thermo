@echo off
rem ---------------------------------------------------------------------------
rem  Windows flashing script for the AVRISP mkII
rem
rem    flash.bat check    -> only test that the programmer sees the chip
rem    flash.bat          -> write build\firmware.hex
rem    flash.bat test     -> write build\hwtest.hex (display hardware test)
rem    flash.bat fuses    -> set fuses for internal RC 8 MHz (once per chip)
rem
rem  Needs avrdude. Either put it on the PATH or unpack it into tools\avrdude\
rem  next to this file so that tools\avrdude\avrdude.exe exists.
rem  The AVRISP mkII also needs the WinUSB driver (Zadig), see README.
rem
rem  Other programmer:  set PROGRAMMER=usbasp   before running this script.
rem ---------------------------------------------------------------------------
setlocal EnableExtensions
cd /d "%~dp0"

if exist "%~dp0tools\avrdude\avrdude.exe" set "PATH=%~dp0tools\avrdude;%PATH%"

where avrdude >nul 2>&1
if errorlevel 1 (
    echo avrdude.exe not found.
    echo Unpack avrdude into tools\avrdude\ or add it to PATH.
    echo See README.md, section "Windows".
    exit /b 1
)

set "PROG=avrisp2"
if defined PROGRAMMER set "PROG=%PROGRAMMER%"

if /i "%~1"=="check" (
    avrdude -c %PROG% -p m8 -B 10
    exit /b %errorlevel%
)

if /i "%~1"=="fuses" (
    avrdude -c %PROG% -p m8 -U lfuse:w:0xE4:m -U hfuse:w:0xD9:m
    exit /b %errorlevel%
)

set "HEX=build\firmware.hex"
if /i "%~1"=="test" set "HEX=build\hwtest.hex"

if not exist "%HEX%" (
    echo %HEX% not found. Run build.bat first.
    exit /b 1
)

avrdude -c %PROG% -p m8 -U flash:w:%HEX%:i
exit /b %errorlevel%
