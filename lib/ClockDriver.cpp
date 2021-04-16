#include "pico/stdlib.h"
#include "ClockDriver.h"
#include "TubeDriver.h"
#include "DCFDriver.h"

ClockDriver::ClockDriver(ClockPinLayout pinLayout)
{
    tube1Driver = new TubeDriver(pinLayout.tube1Pins);
    tube2Driver = new TubeDriver(pinLayout.tube2Pins);
    tube3Driver = new TubeDriver(pinLayout.tube3Pins);
    tube4Driver = new TubeDriver(pinLayout.tube4Pins);
    pointsDriver = new TubeDriver(pinLayout.pointsPins);
    dcfDriver = new DCFDriver(pinLayout.DCFPinData, 3000, pinLayout.DCFSignalStatus, pinLayout.DCFSignalHeartbeat);
}

ClockDriver::~ClockDriver()
{
    delete tube1Driver;
    delete tube2Driver;
    delete tube3Driver;
    delete tube4Driver;
    delete pointsDriver;
}

void ClockDriver::clock()
{
    dcfDriver->update(); // takes at least 5 milli

    static uint32_t lastTime;
    static uint32_t deltaTime = 1000000;
    uint32_t currentTime = time_us_64();
    bool update = false;

    if (currentTime - lastTime > deltaTime)
    {
        update = true;
        lastTime = currentTime;
    }

    switch (state)
    {
    case ClockDriverState::SETUP:
        setState(ClockDriverState::GETTING_TIME);
        break;

    case ClockDriverState::GOT_TIME:
        setState(ClockDriverState::RUNNING);
        update = false;
        // No break for fallthrough

    case ClockDriverState::RUNNING:
        if (update)
        {
            if (dcfDriver->getTimeState() == DCFDriverState::OK)
            {
                int minutes = dcfDriver->getMinutes();
                int hours = dcfDriver->getHours();
                tube1Driver->showDigit(hours / 10);
                tube2Driver->showDigit(hours % 10);
                tube3Driver->showDigit(minutes / 10);
                tube4Driver->showDigit(minutes % 10);
            }
            else
            {
                if (dcfDriver->getTimeState() == DCFDriverState::SEARCH)
                    setState(ClockDriverState::GETTING_TIME);
                else
                    setState(ClockDriverState::NO_SIGNAL);
            }
        }
        break;

    case ClockDriverState::GETTING_TIME:
        if (update)
        {
            switch (dcfDriver->getTimeState())
            {
            case DCFDriverState::OK:
                setState(ClockDriverState::GOT_TIME);
                break;

            case DCFDriverState::ERROR:
                setState(ClockDriverState::NO_SIGNAL);
                break;

            case DCFDriverState::SEARCH:
                uint8_t pointPosition = pointsDriver->getLastDigit() + 1;
                pointsDriver->showDigit(pointPosition % 8);
                break;
            }
        }
        break;

    case ClockDriverState::NO_SIGNAL:
        if (update)
        {
            if (dcfDriver->getTimeState() != DCFDriverState::ERROR)
            {
                setState(ClockDriverState::GETTING_TIME);
            }
        }
        break;
    default:
        break;
    }
}

ClockDriverState ClockDriver::getDriverState()
{
    return state;
}

int ClockDriver::getHour()
{
    return dcfDriver->getHours();
}

int ClockDriver::getMinute()
{
    return dcfDriver->getMinutes();
}

bool ClockDriver::getSignalOkay()
{
    return dcfDriver->getSignalOkay();
}

void ClockDriver::setState(ClockDriverState new_state)
{
    if (new_state != state)
    {
        switch (new_state)
        {
        case ClockDriverState::GETTING_TIME:
            tube1Driver->powerOff();
            tube2Driver->powerOff();
            tube3Driver->powerOff();
            tube4Driver->powerOff();

            pointsDriver->powerOn();
            break;

        case ClockDriverState::GOT_TIME:
            tube1Driver->powerOn();
            tube2Driver->powerOn();
            tube3Driver->powerOn();
            tube4Driver->powerOn();

            pointsDriver->powerOff();
            break;

        case ClockDriverState::RUNNING:
            break;

        case ClockDriverState::NO_SIGNAL:
            tube1Driver->powerOn();
            tube2Driver->powerOn();
            tube3Driver->powerOff();
            tube4Driver->powerOff();

            // Fehlercode: 9 5 - - = Kein oder ein invalides Signal vom DCF Treiber.
            this->tube1Driver->showDigit(9);
            this->tube2Driver->showDigit(5);

            pointsDriver->powerOff();
            break;

        default:
            break;
        }

        state = new_state;
    }
}