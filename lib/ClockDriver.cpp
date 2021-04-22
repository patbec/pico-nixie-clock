#include "pico/stdlib.h"
#include "ClockDriver.h"
#include "TubeDriver.h"
#include "DCFDriver.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

ClockDriver::ClockDriver(ClockPinLayout pinLayout)
{
    tube1Driver = new TubeDriver(pinLayout.tube1Pins);
    tube2Driver = new TubeDriver(pinLayout.tube2Pins);
    tube3Driver = new TubeDriver(pinLayout.tube3Pins);
    tube4Driver = new TubeDriver(pinLayout.tube4Pins);
    pointsDriver = new TubeDriver(pinLayout.pointsPins);
    dcfDriver = new DCFDriver(pinLayout.DCFPin, 3000);

    rtc_init();
}

ClockDriver::~ClockDriver()
{
    delete tube1Driver;
    delete tube2Driver;
    delete tube3Driver;
    delete tube4Driver;
    delete pointsDriver;
    delete dcfDriver;
}

bool ClockDriver::isSecondPassed()
{
    static uint64_t TIME_1_SECOND = 1000000UL; // 1 Sekunde in Mikrosekunden (µs)

    uint64_t currentTime = time_us_64();

    if (currentTime - lastSavedTime > TIME_1_SECOND)
    {
        lastSavedTime = currentTime;
        return true;
    }

    return false;
}

bool ClockDriver::is5HoursPassed() {
    static uint64_t TIME_5_HOURS = 18000000000UL; // 5 Stunden in Mikrosekunden (62290616)

    uint64_t currentTime = time_us_64();

    if (currentTime - lastSavedTime > TIME_5_HOURS)
    {
        lastSavedTime = currentTime;
        return true;
    }

    return false;
}

void ClockDriver::showCurrentTime() {
    // Aktuelle Zeit von Echtzeituhr abrufen.
    rtc_get_datetime(&rtc_buff);

    // Auf Nixies übernehmen.
    int minutes = rtc_buff.min;
    int hours = rtc_buff.hour;
    tube1Driver->showDigit(hours / 10);
    tube2Driver->showDigit(hours % 10);
    tube3Driver->showDigit(minutes / 10);
    tube4Driver->showDigit(minutes % 10);
}

void ClockDriver::clock()
{
    // Dieser Aufruf benötigt mindestens 5 Millisekunden.
    dcfDriver->update();

    switch (state)
    {
        case ClockDriverState::Setup: // Hier einmaligen Setup-Code ausführen.
            {
                // Auf Zeitsuche vorbereiten. (Punkte aktivieren etc.)
                setState(ClockDriverState::SearchTime);
                break;
            }
        case ClockDriverState::SearchTime: // In diesem Zustand bleiben, wenn keine valide Zeit gefunden wurde.      
            {  
                // Prüfen ob eine Sekunde vergangen ist.
                if(isSecondPassed())
                {
                    // Punkte bewegen.
                    uint8_t pointPosition = pointsDriver->getLastDigit() + 1;
                    pointsDriver->showDigit(pointPosition % 8);
                }

                // Prüfen ob eine Zeit gefunden wurde, dann:
                if(dcfDriver->try_get_valid_time())
                {
                    // DCF Zeit übernehmen und auf normalen Betrieb umschalten.
                    setState(ClockDriverState::Running);
                }
                else if(dcfDriver->has_signal_timout()) {
                    // Prüfen ob Signale vom DCF Treiber empfangen wurden, falls nicht:
                    setState(ClockDriverState::Error);
                }

                break;
            }
        case ClockDriverState::Running: // Normaler Betrieb.
            {
                // Zeit von Echtzeituhr auf Nixies anzeigen.
                showCurrentTime();

                // Prüfen ob die Zeit vom DCF Treiber neu mit der Echtzeituhr syncronisiert werden muss. (Alle 5 Stunden auslösen)
                if(is5HoursPassed())
                {
                    setState(ClockDriverState::Synchronize);
                }

                break;
            }
        case ClockDriverState::Synchronize: // DCF Treiber mit Echtzeituhr syncronisieren.
            {
                // Weiterhin Zeit von Echtzeituhr auf Nixies anzeigen.
                showCurrentTime();

                // In den nächsten 5 Stunden versuchen die Zeit zu syncronisieren.
                if(is5HoursPassed())
                {
                    // Falls in 5 Stunden nicht möglich, Fallback auf Zeitsuche.
                    setState(ClockDriverState::SearchTime);
                }
                else if(dcfDriver->try_get_valid_time())
                {
                    // Wenn gültige Zeit gefunden, DCF Zeit übernehmen und auf normalen Betrieb umschalten.
                    setState(ClockDriverState::Running);
                }

                break;
            }
        case ClockDriverState::Error:
            {
                // Alle Punkte leuten lassen. (Schnell durchschalten)
                uint8_t pointPosition = pointsDriver->getLastDigit() + 1;
                pointsDriver->showDigit(pointPosition % 8);

                // Prüfen ob ein Signal vom DCF Treiber empfangen wurde. Falls nicht weiter in diesem Zustand bleiben.
                if( ! dcfDriver->has_signal_timout())
                {
                    setState(ClockDriverState::SearchTime);
                }

                break;
            }
    }
}

void ClockDriver::setState(ClockDriverState new_state)
{
    if (new_state != state)
    {
        switch (new_state)
        {
            case ClockDriverState::SearchTime: // Auf Zeitsuche vorbereiten.
            {
                // Vollständig Zurücksetzen.
                lastSavedTime = 0;

                // Nixie 1 - 4 deaktiveren.
                tube1Driver->powerOff();
                tube2Driver->powerOff();
                tube3Driver->powerOff();
                tube4Driver->powerOff();

                // Punkte aktivieren.
                pointsDriver->powerOn();

                break;
            }
            case ClockDriverState::Running: // Auf normalen Betrieb vorbereiten.
            {
                // DCF Zeit auf Echtzeituhr übernehmen.
                rtc_buff = {
                        .year  = 2020,
                        .month = 06,
                        .day   = 05,
                        .dotw  = 5, // 0 is Sunday, so 5 is Friday
                        .hour  = (int8_t)dcfDriver->getHours(),
                        .min   = (int8_t)dcfDriver->getMinutes(),
                        .sec   = 00
                }; // Placeholder!
                rtc_set_datetime(&rtc_buff);

                // Aktuelle Syncronisierung festhalten. (Nächste ist in 5 Stunden.)
                lastSavedTime = time_us_64();
                
                // Nixie 1 - 4 aktivieren.
                tube1Driver->powerOn();
                tube2Driver->powerOn();
                tube3Driver->powerOn();
                tube4Driver->powerOn();

                // Punkte deaktiveren.
                pointsDriver->powerOff();

                break;
            }
            case ClockDriverState::Synchronize:
            {
                // Aktuellen Startzeitpunkt festhalten. (Abbruch in 5 Stunden.)
                lastSavedTime = time_us_64();

                break;
            }
            case ClockDriverState::Error:
            {
                // Auf Hardware-Fehler hinweisen.

                // Vollständig Zurücksetzen.
                lastSavedTime = 0;

                // Nixie 1 - 4 deaktiveren.
                tube1Driver->powerOff();
                tube2Driver->powerOff();
                tube3Driver->powerOff();
                tube4Driver->powerOff();

                // Punkte (alle) aktivieren. Wenn alle Punkte leuchten ist ein Hardware-Fehler aufgetreten.
                pointsDriver->powerOn();
                break;
            }
        }

        state = new_state;
    }
}

//(./\_/\
//.)>^,^<
//(__(__)
// Code end reached. *Meow*