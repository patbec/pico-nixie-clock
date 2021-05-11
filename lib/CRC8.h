#ifndef CRC8_H
#define CRC8_H

#include "pico/stdlib.h"

class CRC8
{
public:
    static uint8_t Compute_CRC8(uint8_t buffer[], int offset, int count);

private:
    // static const uint8_t crc_table[];
};
#endif
