#ifndef DS1302_H
#define DS1302_H

#include <stdint.h>

typedef struct {
    uint8_t sec;    /* 0..59 */
    uint8_t min;    /* 0..59 */
    uint8_t hour;   /* 0..23 */
    uint8_t date;   /* 1..31 */
    uint8_t month;  /* 1..12 */
    uint8_t day;    /* 1..7  */
    uint8_t year;   /* 0..99 */
} rtc_time_t;

/* Configure pins, disable the trickle charger and make sure the oscillator
 * is running. If the chip reports it was halted (first power-up / dead
 * battery) the time is set to 12:00:00. */
void rtc_init(void);

/* Burst-read the clock. Returns 1 if the values look sane. */
uint8_t rtc_read(rtc_time_t *t);

/* Burst-write the clock (24h mode, oscillator running). */
void rtc_write(const rtc_time_t *t);

#endif /* DS1302_H */
