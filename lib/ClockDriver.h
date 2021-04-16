#ifndef CLOCK_DRIVER_H
#define CLOCK_DRIVER_H

#include "TubeDriver.h"
#include "DCFDriver.h"

enum ClockDriverState
{
    // Einmalig auszuführende Aktionen.
    SETUP,
    // Die Uhrzeit wird gesucht.
    GETTING_TIME,
    // Die Uhrzeit wurde gefunden und wird fortlaufend aktualisiert.
    RUNNING,
    // 
    GOT_TIME,
    // Bei der Zeitsuche wurde ein Problem erkannt, z.B. keine Rückmeldung vom DCF Treiber.
    NO_SIGNAL
};

struct ClockPinLayout
{
    TubePinLayout tube1Pins;
    TubePinLayout tube2Pins;
    TubePinLayout tube3Pins;
    TubePinLayout tube4Pins;
    TubePinLayout pointsPins;
    uint8_t DCFPinData;
    uint8_t DCFSignalHeartbeat; //Old: DCFSignalLight;
    uint8_t DCFSignalStatus; //DCFSignalOkayPin;
};

class ClockDriver
{
private:
    TubeDriver *tube1Driver;
    TubeDriver *tube2Driver;
    TubeDriver *tube3Driver;
    TubeDriver *tube4Driver;
    TubeDriver *pointsDriver;
    DCFDriver *dcfDriver;
    ClockDriverState state = ClockDriverState::SETUP;
    void setState(ClockDriverState state);

public:
    ClockDriver(ClockPinLayout pinLayout);
    ~ClockDriver();
    void clock();
    int getHour();
    int getMinute();
    bool getSignalOkay();
    ClockDriverState getDriverState();
};

#endif