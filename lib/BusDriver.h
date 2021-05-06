#ifndef BUS_DRIVER_H
#define BUS_DRIVER_H

#include "pico/stdlib.h"

enum BusDriverState
{
    SynchronizeSignal = 0,
    PackageInfo = 1,
    PackageCheck = 2,
    ReadContent = 3,
};

class BusDriver
{
public:
    BusDriver(int pin);

    bool readData();
    uint8_t* getContent();
    char getBuffer();

    static bool sendData(char buffer[], int pin);
private:
    BusDriverState state;
    uint pin;
    bool lastData = false;
    
    int bufferCount = 0;
    bool buffer[2080] = {0};

    unsigned char packageCharacter = 0;
    unsigned char packageLength = 0;
    unsigned char packageChecksumHeader = 0;
    unsigned char packageChecksumContent = 0;

    void changeState(BusDriverState newState);

    uint64_t lastSignalTime = 0;
    uint64_t errors = 0;
    static uint64_t baudrate;
    static uint64_t baudsync;
    static uint64_t baudclear;
    static uint64_t dataSend;
    static uint64_t MAX_PACKAGE_LENGTH;
    static uint64_t MAX_HEADER_LENGTH;
    bool bufferValid[8] = {0,1,0,0,1,1,0,1};
    uint8_t readFromHeader(uint octet);
};

#endif
