#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

/* Start a temperature conversion on the (single) sensor.
 * Returns 1 if a sensor responded, 0 if the bus is empty. */
uint8_t ds18b20_start_conversion(void);

/* Read the result of the last conversion. Call >= 750 ms after start.
 * On success writes the temperature in tenths of a degree C and returns 1.
 * Returns 0 on missing sensor or CRC error. */
uint8_t ds18b20_read_tenths(int16_t *tenths);

#endif /* DS18B20_H */
