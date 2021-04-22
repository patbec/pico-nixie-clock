#ifndef DCFDriver_H
#define DCFDriver_H

#include "pico/stdlib.h"

// Must always be a multiple of 2 !!!!!! 
int const SIGNAL_TIME_BUFFER_SIZE = 8;


enum DCFDriverState2
{
    // Wartet bis das Start-Segment erkannt wurde.
    WAIT_ON_START,
    // Das Start-Segment wurde erkannt, jetzt dauert es 60 Sekunden bis der Buffer gefüllt wurde.
    FILL_BUFFER,
    // Im Buffer ist ein Bit gekippt. Kann an einer Störquelle liegen.
    INVALID_DATA,
    // Es kommen keine Signale an, DCF77 falsch angechlossen oder falschen Pin angegeben.
    NO_SIGNAL,
};
enum DCFDriverState { SEARCH, OK, ERROR };

class DCFDriver {
    private:
        int signalPinData;
        unsigned long signalTimeout;
        char dcf77Signal;
        char buffer;
        char debug_buffer[60] = {0};
        unsigned long nowTime = 0, lastTime = 0;
        int diffTime;
        DCFDriverState timeState = DCFDriverState::SEARCH;
        bool bufferChanged = false;
        char impulsLevel; // Impulsdauer 100ms => logisch 0, 200 ms => logisch 1
        char Impuls[59] = {0}; // eingehende Impulse
        long SignalTimeBuffer[SIGNAL_TIME_BUFFER_SIZE] = {0};
        char impulsWert[8] = {1, 2, 4, 8, 10, 20, 40, 80}; //Impulswertigkeit für Level logisch 1
        char impulsZaehler = 0; //Anzahl eingehende Impulse
        char hour = 0;
        char minute = 0;
        char dcfSignal;
        void decodeTime();
        bool checkSignalOkay();
        bool signalOkay = false;
        void updateSignalTimeBuffer(long time);
    public:
        DCFDriver(uint8_t dataPin, int timeout);
        bool has_signal_timout();
        bool try_get_valid_time();
        int getHours();
        int getMinutes();
        DCFDriverState getTimeState();
        bool getSignalOkay();
        bool debug_getBufferChanged();
        char debug_getDCFSignal();
        void update();
};

#endif