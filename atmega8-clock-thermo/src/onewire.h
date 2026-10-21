#ifndef ONEWIRE_H
#define ONEWIRE_H

#include <stdint.h>

void    ow_init(void);
/* Returns 1 if a device answered with a presence pulse. */
uint8_t ow_reset(void);
void    ow_write_byte(uint8_t b);
uint8_t ow_read_byte(void);
/* Dallas/Maxim CRC-8 (poly 0x31 reflected = 0x8C). */
uint8_t ow_crc8(const uint8_t *data, uint8_t len);

#endif /* ONEWIRE_H */
