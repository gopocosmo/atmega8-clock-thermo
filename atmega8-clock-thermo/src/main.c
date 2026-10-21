/*
 * ATmega8 clock + outdoor thermometer
 *
 *   X1 (4 digits)  : HH:MM from the DS1302, colon blinking at 1 Hz
 *   X2 (4 digits)  : temperature from the DS18B20 with one decimal
 *   HRS button     : hours + 1          (hold for auto-repeat)
 *   MIN button     : minutes + 1, seconds reset to 0
 *
 * Everything in the main loop is non-blocking; the display is refreshed
 * from the Timer1 interrupt in display.c.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "config.h"
#include "display.h"
#include "ds1302.h"
#include "onewire.h"
#include "ds18b20.h"

/* ---- Buttons ------------------------------------------------------------ */

typedef struct {
    uint8_t  mask;
    uint8_t  stable;      /* debounced state: 1 = pressed */
    uint8_t  history;     /* last 3 raw samples */
    uint16_t held_ms;
    uint16_t repeat_ms;
} button_t;

static button_t btn_hrs = { .mask = _BV(BTN_HRS) };
static button_t btn_min = { .mask = _BV(BTN_MIN) };

/* Call every BTN_POLL_MS. Returns 1 when an action should fire. */
static uint8_t button_poll(button_t *b)
{
    uint8_t raw = !(BTN_PIN & b->mask);    /* active low */
    uint8_t fire = 0;

    b->history = (uint8_t)(((b->history << 1) | raw) & 0x07);

    if (b->history == 0x07 && !b->stable) {
        b->stable = 1;
        b->held_ms = 0;
        b->repeat_ms = 0;
        fire = 1;                           /* initial press */
    } else if (b->history == 0x00 && b->stable) {
        b->stable = 0;
    }

    if (b->stable) {
        if (b->held_ms < 0xFFFF - BTN_POLL_MS)
            b->held_ms += BTN_POLL_MS;
        if (b->held_ms >= BTN_REPEAT_DELAY_MS) {
            b->repeat_ms += BTN_POLL_MS;
            if (b->repeat_ms >= BTN_REPEAT_RATE_MS) {
                b->repeat_ms = 0;
                fire = 1;                   /* auto-repeat */
            }
        }
    }
    return fire;
}

/* ---- Application state -------------------------------------------------- */

static rtc_time_t now;
static uint8_t    rtc_ok;

static int16_t    temp_tenths;
static uint8_t    temp_ok;
static uint8_t    temp_fail_count;

static void refresh_clock(void)
{
    uint8_t colon;

#if CLOCK_BLINK_COLON
    colon = (millis() & 0x3FF) < 512;      /* ~1 Hz, 50% duty */
#else
    colon = 1;
#endif

    if (rtc_ok) {
        display_show_time(now.hour, now.min, colon);
    } else {
        /* RTC not answering: "--:--" */
        display_set_glyph(DIG_CLK_HR_TENS,   GLYPH_MINUS, 0);
        display_set_glyph(DIG_CLK_HR_UNITS,  GLYPH_MINUS, colon);
        display_set_glyph(DIG_CLK_MIN_TENS,  GLYPH_MINUS, 0);
        display_set_glyph(DIG_CLK_MIN_UNITS, GLYPH_MINUS, 0);
    }
}

static void refresh_temp(void)
{
    if (temp_ok)
        display_show_temp(temp_tenths);
    else
        display_show_temp_error();
}

/* A single bad read keeps the last good value; 3 in a row shows "Err". */
static void temp_failed(void)
{
    if (temp_fail_count < 3)
        temp_fail_count++;
    if (temp_fail_count >= 3)
        temp_ok = 0;
}

static void adjust_hours(void)
{
    if (!rtc_ok) { now.hour = 0; now.min = 0; }
    now.hour = (uint8_t)((now.hour + 1) % 24);
    rtc_write(&now);
    rtc_ok = 1;
    refresh_clock();
}

static void adjust_minutes(void)
{
    if (!rtc_ok) { now.hour = 0; now.min = 0; }
    now.min = (uint8_t)((now.min + 1) % 60);
    now.sec = 0;
    rtc_write(&now);
    rtc_ok = 1;
    refresh_clock();
}

/* ---- Hardware test mode (build with: make hwtest) ----------------------- */

#ifdef DISPLAY_TEST
/*
 * Repeats forever:
 *   1. every digit shows "8." for 3 s            -> all segments/digits work?
 *   2. "8." walks across the 8 digits, 0.5 s each -> digit drivers (PB0..PB7)
 *   3. one segment at a time on all digits a..DP -> segment lines (PD0..PD7)
 *   4. the eight digits show 0 1 2 3 4 5 6 7       -> digit order right?
 */
static void hardware_test(void)
{
    for (;;) {
        for (uint8_t i = 0; i < 8; i++) display_set_raw(i, 0xFF);
        _delay_ms(3000);

        for (uint8_t d = 0; d < 8; d++) {
            for (uint8_t i = 0; i < 8; i++) display_set_raw(i, 0);
            display_set_raw(d, 0xFF);
            _delay_ms(500);
        }

        for (uint8_t s = 0; s < 8; s++) {
            for (uint8_t i = 0; i < 8; i++) display_set_raw(i, (uint8_t)(1 << s));
            _delay_ms(500);
        }

        for (uint8_t i = 0; i < 8; i++) display_set_glyph(i, i, 0);
        _delay_ms(3000);
    }
}
#endif

/* ---- Init --------------------------------------------------------------- */

static void io_init(void)
{
    /* PORTC: PC0/PC1 inputs with pull-up, PC2 1-Wire (input, no pull-up),
     * PC3..PC5 RTC outputs. display_init() takes care of PORTB/PORTD. */
    DDRC  = _BV(RTC_CE) | _BV(RTC_IO) | _BV(RTC_SCLK);
    PORTC = _BV(BTN_HRS) | _BV(BTN_MIN);
}

int main(void)
{
    uint16_t t_btn = 0, t_rtc = 0, t_temp = 0, t_colon = 0;
    uint8_t  temp_busy = 0;

    io_init();
    display_init();
    ow_init();
    sei();

#ifdef DISPLAY_TEST
    hardware_test();            /* never returns */
#endif

    /* Lamp test for ~600 ms so a bad segment/digit is obvious at power-up */
    for (uint8_t i = 0; i < 8; i++)
        display_set_raw(i, 0xFF);
    _delay_ms(600);

    rtc_init();
    rtc_ok = rtc_read(&now);
    refresh_clock();
    refresh_temp();

    /* Kick off the first conversion right away */
    if (ds18b20_start_conversion()) {
        temp_busy = 1;
    }
    t_temp = millis();

    for (;;) {
        uint16_t ms = millis();

        /* -- buttons, every 10 ms -------------------------------------- */
        if ((uint16_t)(ms - t_btn) >= BTN_POLL_MS) {
            t_btn = ms;
            if (button_poll(&btn_hrs)) adjust_hours();
            if (button_poll(&btn_min)) adjust_minutes();
        }

        /* -- RTC poll -------------------------------------------------- */
        if ((uint16_t)(ms - t_rtc) >= RTC_POLL_MS) {
            t_rtc = ms;
            rtc_time_t tmp;
            if (rtc_read(&tmp)) {
                now = tmp;
                rtc_ok = 1;
            } else {
                rtc_ok = 0;
            }
            refresh_clock();
        }

        /* -- colon blink without waiting for the RTC poll -------------- */
        if ((uint16_t)(ms - t_colon) >= 50) {
            t_colon = ms;
            refresh_clock();
        }

        /* -- temperature state machine --------------------------------- */
        if (temp_busy) {
            if ((uint16_t)(ms - t_temp) >= TEMP_CONVERT_MS) {
                int16_t v;
                if (ds18b20_read_tenths(&v)) {
                    temp_tenths = v;
                    temp_ok = 1;
                    temp_fail_count = 0;
                } else {
                    temp_failed();
                }
                refresh_temp();
                temp_busy = 0;
            }
        } else if ((uint16_t)(ms - t_temp) >= TEMP_PERIOD_MS) {
            t_temp = ms;
            if (ds18b20_start_conversion()) {
                temp_busy = 1;
            } else {
                temp_failed();
                refresh_temp();
            }
        }
    }
}
