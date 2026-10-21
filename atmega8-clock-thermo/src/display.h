#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

/* Segment bit positions on SEG_PORT */
#define SEG_A  0x01
#define SEG_B  0x02
#define SEG_C  0x04
#define SEG_D  0x08
#define SEG_E  0x10
#define SEG_F  0x20
#define SEG_G  0x40
#define SEG_DP 0x80

/* Special glyph indexes accepted by display_glyph() */
#define GLYPH_BLANK 10
#define GLYPH_MINUS 11
#define GLYPH_E     12
#define GLYPH_r     13
#define GLYPH_DEG   14   /* ° */
#define GLYPH_C     15

/* Digit indexes into the display buffer (== PORTB bit) */
#define DIG_CLK_MIN_UNITS 0  /* A0 */
#define DIG_CLK_MIN_TENS  1  /* A1 */
#define DIG_CLK_HR_UNITS  2  /* A2 (DP = colon) */
#define DIG_CLK_HR_TENS   3  /* A3 */
#define DIG_TMP_TENTHS    4  /* A4 */
#define DIG_TMP_UNITS     5  /* A5 (DP = decimal point) */
#define DIG_TMP_TENS      6  /* A6 */
#define DIG_TMP_SIGN      7  /* A7 */

/* Start Timer1 and the multiplexing interrupt. Call once, then sei(). */
void display_init(void);

/* Write a raw segment pattern to one digit (0..7). */
void display_set_raw(uint8_t digit, uint8_t segments);

/* Write a glyph (0..9 or GLYPH_*) to one digit, optionally with DP. */
void display_set_glyph(uint8_t digit, uint8_t glyph, uint8_t dp);

/* Show HH:MM on the clock display. */
void display_show_time(uint8_t hours, uint8_t minutes, uint8_t colon);

/* Show a temperature given in tenths of a degree (e.g. -148 -> "-14.8"). */
void display_show_temp(int16_t tenths);

/* Show "Err" on the thermometer display. */
void display_show_temp_error(void);

/* Millisecond tick counter maintained by the display ISR (wraps). */
uint16_t millis(void);

#endif /* DISPLAY_H */
