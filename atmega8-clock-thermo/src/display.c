/*
 * 8-digit multiplexed common-cathode display driver.
 *
 * Timer1 runs in CTC mode with a 1 kHz tick. Every tick one digit is lit,
 * so each digit refreshes at 125 Hz. OCR1B optionally blanks the digit
 * early to implement DISPLAY_BRIGHTNESS.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include "config.h"
#include "display.h"

#define TICK_HZ   1000UL
#define T1_DIV    8UL
#define T1_TOP    (F_CPU / T1_DIV / TICK_HZ - 1)

#if T1_TOP > 65535 || T1_TOP < 50
#error "F_CPU not supported by the display timer setup"
#endif

static volatile uint8_t  disp_buf[8];
static volatile uint16_t ms_ticks;

static const uint8_t glyph_font[] PROGMEM = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,          /* 0 */
    SEG_B | SEG_C,                                          /* 1 */
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                  /* 2 */
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                  /* 3 */
    SEG_B | SEG_C | SEG_F | SEG_G,                          /* 4 */
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                  /* 5 */
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,          /* 6 */
    SEG_A | SEG_B | SEG_C,                                  /* 7 */
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  /* 8 */
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,          /* 9 */
    0,                                                      /* blank */
    SEG_G,                                                  /* - */
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,                  /* E */
    SEG_E | SEG_G,                                          /* r */
    SEG_A | SEG_B | SEG_F | SEG_G,                          /* ° */
    SEG_A | SEG_D | SEG_E | SEG_F,                          /* C */
};

void display_init(void)
{
    SEG_DDR  = 0xFF;
    SEG_PORT = 0x00;
    DIG_DDR  = 0xFF;
    DIG_PORT = 0x00;

    for (uint8_t i = 0; i < 8; i++)
        disp_buf[i] = 0;

    TCCR1A = 0;
    TCCR1B = _BV(WGM12) | _BV(CS11);     /* CTC, TOP = OCR1A, clk/8 */
    OCR1A  = T1_TOP;
#if DISPLAY_BRIGHTNESS < 100
    OCR1B  = (uint16_t)(((uint32_t)(T1_TOP + 1) * DISPLAY_BRIGHTNESS) / 100);
    TIMSK |= _BV(OCIE1A) | _BV(OCIE1B);
#else
    TIMSK |= _BV(OCIE1A);
#endif
}

ISR(TIMER1_COMPA_vect)
{
    static uint8_t cur;

    DIG_PORT = 0;                        /* all digits off: no ghosting   */
    cur = (cur + 1) & 7;
    SEG_PORT = disp_buf[cur];
    DIG_PORT = (uint8_t)(1 << cur);
    ms_ticks++;
}

#if DISPLAY_BRIGHTNESS < 100
ISR(TIMER1_COMPB_vect)
{
    DIG_PORT = 0;
}
#endif

uint16_t millis(void)
{
    uint16_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { t = ms_ticks; }
    return t;
}

void display_set_raw(uint8_t digit, uint8_t segments)
{
    disp_buf[digit & 7] = segments;
}

void display_set_glyph(uint8_t digit, uint8_t glyph, uint8_t dp)
{
    uint8_t s = pgm_read_byte(&glyph_font[glyph]);
    if (dp) s |= SEG_DP;
    disp_buf[digit & 7] = s;
}

void display_show_time(uint8_t hours, uint8_t minutes, uint8_t colon)
{
    uint8_t ht = hours / 10;

#if CLOCK_BLANK_LEADING_ZERO
    display_set_glyph(DIG_CLK_HR_TENS, ht ? ht : GLYPH_BLANK, 0);
#else
    display_set_glyph(DIG_CLK_HR_TENS, ht, 0);
#endif
    display_set_glyph(DIG_CLK_HR_UNITS, hours % 10, colon);
    display_set_glyph(DIG_CLK_MIN_TENS,  minutes / 10, 0);
    display_set_glyph(DIG_CLK_MIN_UNITS, minutes % 10, 0);
}

void display_show_temp(int16_t tenths)
{
    uint8_t  neg = tenths < 0;
    uint16_t a   = neg ? (uint16_t)(-tenths) : (uint16_t)tenths;

    if (a >= 1000) {
        /* >= 100.0 degrees: drop the decimal, show "125" / "-125" */
        uint16_t whole = a / 10;
        display_set_glyph(DIG_TMP_SIGN,   neg ? GLYPH_MINUS : GLYPH_BLANK, 0);
        display_set_glyph(DIG_TMP_TENS,   whole / 100, 0);
        display_set_glyph(DIG_TMP_UNITS,  (whole / 10) % 10, 0);
        display_set_glyph(DIG_TMP_TENTHS, whole % 10, 0);
        return;
    }

    uint8_t tens  = a / 100;
    uint8_t units = (a / 10) % 10;
    uint8_t tenth = a % 10;

    /* "-14.8", " -9.8", " 21.4", "  0.0" : sign floats next to the digits */
    if (tens) {
        display_set_glyph(DIG_TMP_SIGN, neg ? GLYPH_MINUS : GLYPH_BLANK, 0);
        display_set_glyph(DIG_TMP_TENS, tens, 0);
    } else {
        display_set_glyph(DIG_TMP_SIGN, GLYPH_BLANK, 0);
        display_set_glyph(DIG_TMP_TENS, neg ? GLYPH_MINUS : GLYPH_BLANK, 0);
    }
    display_set_glyph(DIG_TMP_UNITS,  units, 1);
    display_set_glyph(DIG_TMP_TENTHS, tenth, 0);
}

void display_show_temp_error(void)
{
    display_set_glyph(DIG_TMP_SIGN,   GLYPH_BLANK, 0);
    display_set_glyph(DIG_TMP_TENS,   GLYPH_E, 0);
    display_set_glyph(DIG_TMP_UNITS,  GLYPH_r, 0);
    display_set_glyph(DIG_TMP_TENTHS, GLYPH_r, 0);
}
