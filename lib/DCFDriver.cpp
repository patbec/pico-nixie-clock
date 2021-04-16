//DCF77_03.ino
//Code fuer Arduino
//Author Retian
//Version 1.1
//https://arduino-projekte.webnode.at/meine-projekte/dcf77-empfanger/uhrzeit/

#include "pico/stdlib.h"
#include "DCFDriver.h"

DCFDriver::DCFDriver(int signal_pin_data, int signal_timeout, int signal_status, int signal_heartbeat)
{
    signalPinData = signal_pin_data;
    signalTimeout = signal_timeout;
    signalStatus = signal_status;
    signalHeartbeat = signal_heartbeat;

    gpio_init(signal_pin_data);
    gpio_set_dir(signal_pin_data, GPIO_IN);

    // Leuchtet wenn die empfangen Daten fehlerhaft sind. (z.B. durch eine Störquelle)
    if (signalStatus != UINT8_MAX) {
        gpio_init(signal_status);
        gpio_set_dir(signal_status, GPIO_OUT);
    }
    // Leuchtet wenn ein Signal empfangen wurde.
    if (signal_heartbeat != UINT8_MAX) {
        gpio_init(signal_heartbeat);
        gpio_set_dir(signal_heartbeat, GPIO_OUT);
    }
}

int DCFDriver::getHours()
{
    return hour;
}

int DCFDriver::getMinutes()
{
    return minute;
}

bool DCFDriver::getSignalOkay()
{
    return signalOkay;
}

DCFDriverState DCFDriver::getTimeState()
{
    return timeState;
}

bool DCFDriver::debug_getBufferChanged()
{
    return bufferChanged;
}

char DCFDriver::debug_getDCFSignal()
{
    return dcf77Signal;
}

void DCFDriver::updateSignalTimeBuffer(long time)
{
    for (int i = 0; i < SIGNAL_TIME_BUFFER_SIZE - 1; i++)
    {
        SignalTimeBuffer[i] = SignalTimeBuffer[i + 1];
    }
    SignalTimeBuffer[SIGNAL_TIME_BUFFER_SIZE - 1] = time;
}

bool DCFDriver::checkSignalOkay()
{
    bool okay = true;
    bool foundStayHighTimeAlready = false;
    //check first case
    //Assume start with high
    for (int i = 0; i < SIGNAL_TIME_BUFFER_SIZE; i += 2)
    {

        int highTime = SignalTimeBuffer[i];    // first element of segement is high
        int lowTime = SignalTimeBuffer[i + 1]; // second element of segment is low

        bool segmentOkay =  lowTime < 240 && lowTime > 80 &&  // Signal low should be 100ms or 200ms
                            highTime > 760 && highTime < 940; // Signal high should be 800ms or 900ms
        bool stayHighTime = highTime > 1750 && highTime < 1950; // Allow the "keine Absenkung" every 59 seconds

        if (stayHighTime && foundStayHighTimeAlready)
        {
            okay = false;
            break;
        }

        else if (stayHighTime)
            foundStayHighTimeAlready = true;

        if (!segmentOkay && !stayHighTime)
        {
            okay = false;
            break;
        }
    }

    if (okay)
        return true; //If okay already done

    // ToDo: Clean up

    //check second case
    //Assume start with low
    okay = true;
    foundStayHighTimeAlready = false;

    for (int i = 0; i < SIGNAL_TIME_BUFFER_SIZE; i += 2)
    {
        int lowTime = SignalTimeBuffer[i];      // first element of segment is low
        int highTime = SignalTimeBuffer[i + 1]; // second element of segement is high

        bool segmentOkay =  lowTime < 240 && lowTime > 80 &&  // Signal low should be 100ms or 200ms
                            highTime > 760 && highTime < 940; // Signal high should be 800ms or 900ms
        bool stayHighTime = highTime > 1750 && highTime < 1950; // Allow the "keine Absenkung" every 59 seconds

        if (stayHighTime && foundStayHighTimeAlready)
        {
            okay = false;
            break;
        }

        else if (stayHighTime)
            foundStayHighTimeAlready = true;

        if (!segmentOkay && !stayHighTime)
        {
            okay = false;
            break;
        }
    }

    return okay;
}

void DCFDriver::decodeTime()
{
    timeState = DCFDriverState::OK;
    char paritaetStunde = 0;
    char paritaetMinute = 0;
    hour = 0;
    minute = 0;
    //Ueberpruefen der Stundenparitaet
    for (char i = 29; i < 35; i++)
        paritaetStunde ^= Impuls[i];
    if (Impuls[35] != paritaetStunde)
        timeState = DCFDriverState::SEARCH;
    //Ueberpruefen der Minutenparitaet
    for (char i = 21; i < 28; i++)
        paritaetMinute ^= Impuls[i];
    if (Impuls[28] != paritaetMinute)
        timeState = DCFDriverState::SEARCH;
    //Zuweisen der Impulswertigkeit
    for (char i = 29; i < 35; i++)
        (Impuls[i] == 1 ? hour += impulsWert[i - 29] : 0);
    for (char i = 21; i < 28; i++)
        (Impuls[i] == 1 ? minute += impulsWert[i - 21] : 0);
    //Ueberpruefen des Wertebereiches
    if (hour > 23 || minute > 59)
        timeState = DCFDriverState::SEARCH;
}

void DCFDriver::update()
{
    dcf77Signal = gpio_get(signalPinData);
    nowTime = time_us_64() / 1000; // ToDo: Check
    if (dcf77Signal != buffer)
    {
        sleep_us(5000); // ToDo: Check and Test //Wartezeit bis eingeschwungener Zustand
        diffTime = nowTime - lastTime;
        lastTime = nowTime;

        updateSignalTimeBuffer(diffTime); // Must happen here, dont move !!!!

        // ToDo: Bei einem schlechten Signal wird KEIN Fehler ausgelöst.
        signalOkay = checkSignalOkay();   // Must happen here, dont move !!!!

        // ToDo: Fix this, Status if always ERROR
        // if( ! signalOkay) {
        //     timeState = DCFDriverState::ERROR;
        // }

        if (diffTime < 150)
            impulsLevel = 0;
        else if (diffTime < 250)
            impulsLevel = 1;
        else if (diffTime > 1000)
        {
            if (impulsZaehler == 59 || impulsZaehler == 60)
                decodeTime();
            impulsZaehler = 0;
        }
        if (dcf77Signal == 1) // Abfrage auf "1", wenn das invertierte Signal verwendet wird
        {
            Impuls[impulsZaehler] = impulsLevel;
            impulsZaehler++;
        }
        if (timeState == DCFDriverState::ERROR)
        {
            timeState = DCFDriverState::SEARCH;
        }
        // if (timeState == DCFDriverState::ERROR)
        // {   
        //     // ToDo: Fix this, Status if always ERROR
        //     if(signalOkay) {
        //         timeState = DCFDriverState::SEARCH;
        //     } else {
        //         timeState = DCFDriverState::ERROR;       
        //     }
        // }
        buffer = dcf77Signal;
        bufferChanged = true;
    }
    else
    {
        bufferChanged = false;

        unsigned long noSignalTime = nowTime - lastTime;
        if (noSignalTime > signalTimeout)
            timeState = DCFDriverState::ERROR;
    }

    // Pulls signalHeartbeat to HIGH on good Signal, else LOW
    if (signalHeartbeat != UINT8_MAX)
    {
        gpio_put(signalHeartbeat, signalOkay);
    }
    // Lights up shortly when a signal has been received via the radio-controlled clock.
    if (signalStatus != UINT8_MAX)
    {
        gpio_put(signalStatus, bufferChanged);
    }
}