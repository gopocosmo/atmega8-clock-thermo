#include "ds18b20.h"
#include "onewire.h"

#define CMD_SKIP_ROM        0xCC
#define CMD_CONVERT_T       0x44
#define CMD_READ_SCRATCHPAD 0xBE

uint8_t ds18b20_start_conversion(void)
{
    if (!ow_reset())
        return 0;
    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_CONVERT_T);
    return 1;
}

uint8_t ds18b20_read_tenths(int16_t *tenths)
{
    uint8_t sp[9];

    if (!ow_reset())
        return 0;
    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_READ_SCRATCHPAD);
    for (uint8_t i = 0; i < 9; i++)
        sp[i] = ow_read_byte();

    if (ow_crc8(sp, 8) != sp[8])
        return 0;

    /* All 0xFF means the bus was floating (sensor unplugged) */
    if (sp[0] == 0xFF && sp[1] == 0xFF)
        return 0;

    /* Raw value is in 1/16 degree, 12-bit default resolution. 0x0550 is the
     * power-on reset value (85.0 C) that shows up if we read before the
     * first conversion finished; treat it as invalid. */
    int16_t raw = (int16_t)(((uint16_t)sp[1] << 8) | sp[0]);
    if (raw == 0x0550)
        return 0;

    /* Round to nearest tenth: tenths = raw * 10 / 16 */
    int16_t scaled = raw * 10;
    if (scaled >= 0)
        *tenths = (scaled + 8) / 16;
    else
        *tenths = -((-scaled + 8) / 16);
    return 1;
}
