#ifndef BUS_DRIVER_H
#define BUS_DRIVER_H

#include "pico/stdlib.h"

enum BusDriverState
{
    SynchronizeSignal = 0,

    PackageHeader = 1,
    PackageContent = 2,
    PackageCompleted = 3,
    PackageError = 4,

    // Add HardwareError State?
};

struct Package {
    uint8_t identity;
    uint8_t content[255] = {0};
    uint8_t contentLength;
};

class BusDriver
{
public:
    BusDriver(int pin);

    //        8 Bits              8 Bits             8 Bits             8 Bits      
    // ┌──────────────────┬──────────────────┬──────────────────┬──────────────────┐
    // │ Magic Character  │ Package Length   │ Content CRC      │ Header CRC       │
    // └──────────────────┴──────────────────┴──────────────────┴──────────────────┘

    #define PAK_HEAD0_IDENT       _u(0)
    #define PAK_HEAD1_LENGTH      _u(1)
    #define PAK_HEAD2_CRC_CONTENT _u(2)
    #define PAK_HEAD3_CRC_HEADER  _u(3)

    #define PAK_LENGTH            _u(4) // Beginnt bei Index 1 an zu zählen.

    Package* readData();

    static bool sendData(Package package, int pin);
private:

    bool canRead();
    uint8_t readByte2();
    bool readByte();
    bool readSync();

    uint64_t getErrorCount();

    BusDriverState state;
    uint pin;
    bool lastData = false;
    
    uint8_t bytesLeft = 0;
    uint8_t cache = 0;
    uint8_t bitsRead = 0;

    uint8_t buffer[259] = {0};
    int bufferCount = 0;

    void changeState(BusDriverState newState);

    uint64_t lastSignalTime = 0;
    uint64_t errors = 0;

    static uint64_t baudrate;
    static uint64_t baudsync;
    static uint64_t baudclear;
    static uint64_t dataSend;
};

#endif
