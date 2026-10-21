/*
 * DS1302 real-time clock, 3-wire interface (CE, IO, SCLK).
 *
 * Data is shifted LSB first. The master changes IO before the rising edge
 * of SCLK; the DS1302 outputs data on the falling edge.
 */
#include <avr/io.h>
#include <util/delay.h>
#include "config.h"
#include "ds1302.h"

#define REG_SEC      0x80
#define REG_MIN      0x82
#define REG_HOUR     0x84
#define REG_DATE     0x86
#define REG_MONTH    0x88
#define REG_DAY      0x8A
#define REG_YEAR     0x8C
#define REG_WP       0x8E
#define REG_TRICKLE  0x90
#define REG_BURST    0xBE
#define RD           0x01      /* OR into a register address to read */

#define SEC_CH       0x80      /* clock halt flag in seconds register  */
#define HOUR_12H     0x80      /* 12/24 mode flag in hours register    */
#define WP_SET       0x80

#define ce_high()   (RTC_PORT |=  _BV(RTC_CE))
#define ce_low()    (RTC_PORT &= ~_BV(RTC_CE))
#define sclk_high() (RTC_PORT |=  _BV(RTC_SCLK))
#define sclk_low()  (RTC_PORT &= ~_BV(RTC_SCLK))
#define io_out()    (RTC_DDR  |=  _BV(RTC_IO))
#define io_in()     do { RTC_DDR &= ~_BV(RTC_IO); RTC_PORT &= ~_BV(RTC_IO); } while (0)
#define io_high()   (RTC_PORT |=  _BV(RTC_IO))
#define io_low()    (RTC_PORT &= ~_BV(RTC_IO))
#define io_read()   ((RTC_PIN >> RTC_IO) & 1)

static uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static void write_byte(uint8_t b)
{
    io_out();
    for (uint8_t i = 0; i < 8; i++) {
        if (b & 1) io_high(); else io_low();
        _delay_us(1);
        sclk_high();
        _delay_us(1);
        sclk_low();
        _delay_us(1);
        b >>= 1;
    }
}

static uint8_t read_byte(void)
{
    uint8_t b = 0;

    io_in();
    _delay_us(1);
    for (uint8_t i = 0; i < 8; i++) {
        b >>= 1;
        if (io_read())
            b |= 0x80;
        sclk_high();
        _delay_us(1);
        sclk_low();
        _delay_us(1);
    }
    return b;
}

static void begin(void)
{
    sclk_low();
    ce_high();
    _delay_us(4);              /* tCC */
}

static void end(void)
{
    ce_low();
    io_in();
    _delay_us(4);              /* tCWH */
}

static void reg_write(uint8_t reg, uint8_t val)
{
    begin();
    write_byte(reg);
    write_byte(val);
    end();
}

static void unprotect(void) { reg_write(REG_WP, 0x00); }
static void protect(void)   { reg_write(REG_WP, WP_SET); }

void rtc_write(const rtc_time_t *t)
{
    unprotect();
    begin();
    write_byte(REG_BURST);
    write_byte(bin2bcd(t->sec) & 0x7F);        /* CH = 0: oscillator on */
    write_byte(bin2bcd(t->min));
    write_byte(bin2bcd(t->hour) & 0x3F);       /* 24h mode              */
    write_byte(bin2bcd(t->date));
    write_byte(bin2bcd(t->month));
    write_byte(bin2bcd(t->day));
    write_byte(bin2bcd(t->year));
    write_byte(WP_SET);                        /* 8th burst byte = WP   */
    end();
}

uint8_t rtc_read(rtc_time_t *t)
{
    uint8_t raw[8];

    begin();
    write_byte(REG_BURST | RD);
    for (uint8_t i = 0; i < 8; i++)
        raw[i] = read_byte();
    end();

    if (raw[0] & SEC_CH)            /* halted */
        return 0;
    if (raw[2] & HOUR_12H)          /* we only run in 24h mode */
        return 0;

    t->sec   = bcd2bin(raw[0] & 0x7F);
    t->min   = bcd2bin(raw[1] & 0x7F);
    t->hour  = bcd2bin(raw[2] & 0x3F);
    t->date  = bcd2bin(raw[3] & 0x3F);
    t->month = bcd2bin(raw[4] & 0x1F);
    t->day   = bcd2bin(raw[5] & 0x07);
    t->year  = bcd2bin(raw[6]);

    if (t->sec > 59 || t->min > 59 || t->hour > 23)
        return 0;
    return 1;
}

void rtc_init(void)
{
    rtc_time_t t;

    RTC_PORT &= ~(_BV(RTC_CE) | _BV(RTC_SCLK) | _BV(RTC_IO));
    RTC_DDR  |=  _BV(RTC_CE) | _BV(RTC_SCLK);
    io_in();
    _delay_ms(2);

    /* BAT1 is a non-rechargeable 3 V cell: make sure trickle charge is off */
    unprotect();
    reg_write(REG_TRICKLE, 0x00);
    protect();

    if (!rtc_read(&t)) {
        /* First power-up, dead battery or garbage: start at 12:00:00 */
        t.sec = 0;  t.min = 0;   t.hour = 12;
        t.date = 1; t.month = 1; t.day = 1; t.year = 24;
        rtc_write(&t);
    }
}
