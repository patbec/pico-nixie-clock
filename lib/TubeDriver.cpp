#include "pico/stdlib.h"
#include "TubeDriver.h"

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

TubeDriver::TubeDriver(TubePinLayout pinLayout)
{
    pinA = pinLayout.pinA;
    pinB = pinLayout.pinB;
    pinC = pinLayout.pinC;
    pinD = pinLayout.pinD;
    // pinPointL = pinLayout.pinPointL;
    // pinPointR = pinLayout.pinPointR;
    lastDigit = 0;

    if(pinA != UINT8_MAX) gpio_init(pinA);
    if(pinB != UINT8_MAX) gpio_init(pinB);
    if(pinC != UINT8_MAX) gpio_init(pinC);
    if(pinD != UINT8_MAX) gpio_init(pinD);

    // gpio_init(pinPointL);
    // gpio_init(pinPointR);
}

void TubeDriver::showDigit(uint8_t digit)
{
    if (lastDigit != digit)
    {
        lastDigit = digit;

        if(pinA != UINT8_MAX) gpio_put(pinA, bitRead(digit, 0));
        if(pinB != UINT8_MAX) gpio_put(pinB, bitRead(digit, 1));
        if(pinC != UINT8_MAX) gpio_put(pinC, bitRead(digit, 2));
        if(pinD != UINT8_MAX) gpio_put(pinD, bitRead(digit, 3));
    }
}

// void TubeDriver::showPoint(bool left, bool right) {
//     gpio_put(pinPointL, left);
//     gpio_put(pinPointR, right);
// }

void TubeDriver::powerOn()
{
    if(pinA != UINT8_MAX) gpio_set_dir(pinA, GPIO_OUT);
    if(pinB != UINT8_MAX) gpio_set_dir(pinB, GPIO_OUT);
    if(pinC != UINT8_MAX) gpio_set_dir(pinC, GPIO_OUT);
    if(pinD != UINT8_MAX) gpio_set_dir(pinD, GPIO_OUT);

    // gpio_set_dir(pinPointL, GPIO_OUT);
    // gpio_set_dir(pinPointR, GPIO_OUT);

    lastDigit = 0;
}

void TubeDriver::powerOff()
{
    if(pinA != UINT8_MAX) gpio_set_dir(pinA, GPIO_IN);
    if(pinB != UINT8_MAX) gpio_set_dir(pinB, GPIO_IN);
    if(pinC != UINT8_MAX) gpio_set_dir(pinC, GPIO_IN);
    if(pinD != UINT8_MAX) gpio_set_dir(pinD, GPIO_IN);

    // gpio_set_dir(pinPointL, GPIO_IN);
    // gpio_set_dir(pinPointR, GPIO_IN);
}

int TubeDriver::getLastDigit()
{
    return lastDigit;
}