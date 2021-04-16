#ifndef DCFDriver_H
#define DCFDriver_H

#include "pico/stdlib.h"

// Must always be a multiple of 2 !!!!!! 
int const SIGNAL_TIME_BUFFER_SIZE = 8;

enum DCFDriverState { SEARCH, OK, ERROR };

class DCFDriver {
    private:
        int signalPinData, signalStatus, signalHeartbeat;
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
        DCFDriver(int signal_pin, int signal_timeout, int signal_status, int signal_heartbeat);
        int getHours();
        int getMinutes();
        DCFDriverState getTimeState();
        bool getSignalOkay();
        bool debug_getBufferChanged();
        char debug_getDCFSignal();
        void update();
};

#endif