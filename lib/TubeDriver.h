#ifndef TUBE_DRIVER_H
#define TUBE_DRIVER_H

#include "pico/stdlib.h"

struct TubePinLayout
{
    uint8_t pinA;
    uint8_t pinB;
    uint8_t pinC;
    uint8_t pinD;

    // uint8_t pinPointL;
    // uint8_t pinPointR;
};

class TubeDriver
{
public:
    TubeDriver(TubePinLayout pinLayout);
    void showDigit(uint8_t digit);
    // void showPoint(bool left, bool right);
    void powerOn();
    void powerOff();
    int getLastDigit();

private:
    uint8_t lastDigit;
    uint8_t pinA;
    uint8_t pinB;
    uint8_t pinC;
    uint8_t pinD;

    // uint8_t pinPointL;
    // uint8_t pinPointR;
};

#endif
