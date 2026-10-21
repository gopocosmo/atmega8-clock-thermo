/*
 * Bit-banged 1-Wire master on a single pin with an external pull-up.
 *
 * The pin's PORT bit is kept at 0 permanently; the line is driven low by
 * switching the pin to output and released by switching it to input.
 * Each time slot runs with interrupts disabled so the display ISR cannot
 * stretch the critical timings.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "config.h"
#include "onewire.h"

#define ow_low()     (OW_DDR |=  _BV(OW_BIT))
#define ow_release() (OW_DDR &= ~_BV(OW_BIT))
#define ow_level()   ((OW_PIN >> OW_BIT) & 1)

void ow_init(void)
{
    OW_PORT &= ~_BV(OW_BIT);   /* never enable the internal pull-up */
    ow_release();
}

uint8_t ow_reset(void)
{
    uint8_t presence;
    uint8_t sreg = SREG;

    cli();
    ow_low();
    _delay_us(480);
    ow_release();
    _delay_us(70);
    presence = !ow_level();
    SREG = sreg;
    _delay_us(410);
    return presence;
}

static void ow_write_bit(uint8_t bit)
{
    uint8_t sreg = SREG;

    cli();
    ow_low();
    if (bit) {
        _delay_us(5);          /* write-1: release within 15 us */
        ow_release();
        _delay_us(60);
    } else {
        _delay_us(60);         /* write-0: hold low for the whole slot */
        ow_release();
        _delay_us(5);
    }
    SREG = sreg;
    _delay_us(2);              /* recovery */
}

static uint8_t ow_read_bit(void)
{
    uint8_t bit;
    uint8_t sreg = SREG;

    cli();
    ow_low();
    _delay_us(2);
    ow_release();
    _delay_us(7);              /* sample well inside the 15 us window */
    bit = ow_level();
    SREG = sreg;
    _delay_us(55);
    return bit;
}

void ow_write_byte(uint8_t b)
{
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_bit(b & 1);
        b >>= 1;
    }
}

uint8_t ow_read_byte(void)
{
    uint8_t b = 0;

    for (uint8_t i = 0; i < 8; i++) {
        b >>= 1;
        if (ow_read_bit())
            b |= 0x80;
    }
    return b;
}

uint8_t ow_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;

    while (len--) {
        uint8_t in = *data++;
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t mix = (crc ^ in) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8C;
            in >>= 1;
        }
    }
    return crc;
}
