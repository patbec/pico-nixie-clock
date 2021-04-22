#ifndef CLOCK_DRIVER_H
#define CLOCK_DRIVER_H

#include "TubeDriver.h"
#include "DCFDriver.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

enum ClockDriverState
{
    Setup = 0,
    SearchTime = 1,
    Running = 2,
    Synchronize = 3,
    Error = 4,
};

struct ClockPinLayout
{
    TubePinLayout tube1Pins;
    TubePinLayout tube2Pins;
    TubePinLayout tube3Pins;
    TubePinLayout tube4Pins;
    TubePinLayout pointsPins;
    uint8_t DCFPin;
};

class ClockDriver
{
private:
    uint64_t lastSavedTime;
    datetime_t rtc_buff;

    TubeDriver *tube1Driver;
    TubeDriver *tube2Driver;
    TubeDriver *tube3Driver;
    TubeDriver *tube4Driver;
    TubeDriver *pointsDriver;
    DCFDriver *dcfDriver;
    ClockDriverState state = ClockDriverState::Setup;
    void setState(ClockDriverState state);
    void showCurrentTime();
    bool isSecondPassed();
    bool is5HoursPassed();
public:
    ClockDriver(ClockPinLayout pinLayout);
    ~ClockDriver();
    void clock();
};

#endif