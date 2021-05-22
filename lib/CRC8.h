#ifndef CRC8_H
#define CRC8_H

#include "pico/stdlib.h"

class CRC8
{
public:
    static uint8_t Compute_CRC8(uint8_t buffer[], int start, int end);

    static const int TABLE[256];
private:

};
#endif
