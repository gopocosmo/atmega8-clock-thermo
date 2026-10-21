/*
 * Hardware configuration for the ATmega8 clock + thermometer board.
 *
 * Pin map (traced from the schematic):
 *
 *   PD0..PD7  segments a,b,c,d,e,f,g,DP   (8 x 100R, shared by both displays)
 *   PB0..PB3  clock digits  A0(right) .. A3(left)   via Q4..Q1 (NPN, active high)
 *   PB4..PB7  thermo digits A4(right) .. A7(left)   via Q8..Q5 (NPN, active high)
 *   PC0       HRS button  (to GND, internal pull-up)
 *   PC1       MIN button  (to GND, internal pull-up)
 *   PC2       DS18B20 DQ  (4K7 pull-up to +5V)
 *   PC3       DS1302 CE
 *   PC4       DS1302 IO
 *   PC5       DS1302 SCLK
 *   PC6       /RESET (10k pull-up)
 *
 * PB6/PB7 are used as GPIO, so the MCU must run from the internal RC
 * oscillator. See the Makefile "fuses" target.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <avr/io.h>

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

/* ---- 7-segment bus ------------------------------------------------------ */
#define SEG_PORT PORTD
#define SEG_DDR  DDRD

/* ---- Digit drivers ------------------------------------------------------ */
#define DIG_PORT PORTB
#define DIG_DDR  DDRB

/* ---- Buttons (active low) ---------------------------------------------- */
#define BTN_PORT PORTC
#define BTN_PIN  PINC
#define BTN_DDR  DDRC
#define BTN_HRS  PC0
#define BTN_MIN  PC1

/* ---- DS18B20 1-Wire ----------------------------------------------------- */
#define OW_PORT PORTC
#define OW_PIN  PINC
#define OW_DDR  DDRC
#define OW_BIT  PC2

/* ---- DS1302 RTC --------------------------------------------------------- */
#define RTC_PORT PORTC
#define RTC_PIN  PINC
#define RTC_DDR  DDRC
#define RTC_CE   PC3
#define RTC_IO   PC4
#define RTC_SCLK PC5

/* ---- Behaviour ---------------------------------------------------------- */

/* Display duty cycle in percent (10..100). Lower it if the board runs hot:
 * with 100R segment resistors a full "8." pulls ~200 mA through the MCU. */
#define DISPLAY_BRIGHTNESS      100

/* 1 = show " 9:05" instead of "09:05" */
#define CLOCK_BLANK_LEADING_ZERO 1

/* Colon (DP of digit A2) blinks at 1 Hz when 1, steady when 0 */
#define CLOCK_BLINK_COLON        1

/* How often the RTC is polled and how often a temperature conversion starts */
#define RTC_POLL_MS              100
#define TEMP_PERIOD_MS           1000
#define TEMP_CONVERT_MS          800   /* 12-bit conversion takes <= 750 ms */

/* Button timing */
#define BTN_POLL_MS              10
#define BTN_REPEAT_DELAY_MS      600
#define BTN_REPEAT_RATE_MS       150

#endif /* CONFIG_H */
