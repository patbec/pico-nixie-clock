#include "pico/stdlib.h"
#include "BusDriver.h"

uint64_t BusDriver::baudrate = 100;
uint64_t BusDriver::baudsync = 1200;
uint64_t BusDriver::baudclear = 500;
uint64_t BusDriver::dataSend = 0;

uint64_t BusDriver::MAX_PACKAGE_LENGTH = 1024; // Chars
uint64_t BusDriver::MAX_HEADER_LENGTH = 32; // Bits

//bool BusDriver::bufferValid[8] = {0,1,0,0,1,1,0,1};

BusDriver::BusDriver(int pin)
{
    this->pin = pin;
    this->state = BusDriverState::SynchronizeSignal;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
}

bool BusDriver::sendData(char buffer[], int pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);

    int myCurrentPackageLength = 11;

    unsigned char package[myCurrentPackageLength + 1];
    bool debug[(myCurrentPackageLength + 1) * 8];

    package[0] = 'M';         // Magic Char
    package[1] = (uint8_t)8; // Package Length
    package[2] = 0;           // Content CRC
    package[3] = 0;           // Header CRC

    package[4] = 'A'; // Content
    package[5] = 'B'; // Content
    package[6] = 'C'; // Content
    package[7] = 'D'; // Content

    package[8] = 'Q'; // Content
    package[9] = 'W'; // Content
    package[10] = 'E'; // Content
    package[11] = 'R'; // Content

    // 01001101 01000000 00000000 00000000
    // 01001101 01000001 01001001 01001110

    for (size_t i = 0; i < myCurrentPackageLength; i++)
    {
        int pos = i * 8;

        unsigned char character = package[i];

        debug[pos + 0] = (character >> 7) & 1;
        debug[pos + 1] = (character >> 6) & 1;
        debug[pos + 2] = (character >> 5) & 1;
        debug[pos + 3] = (character >> 4) & 1;
        debug[pos + 4] = (character >> 3) & 1;
        debug[pos + 5] = (character >> 2) & 1;
        debug[pos + 6] = (character >> 1) & 1;
        debug[pos + 7] = (character >> 0) & 1;
    }

    gpio_put(pin, false);
    sleep_us(baudclear);
    // Sync signal
    gpio_put(pin, true);
    sleep_us(baudsync);

    int target = (myCurrentPackageLength + 1) * 8;

    for (size_t i = 0; i < target; i++)
    {
        gpio_put(pin, debug[i]);
        sleep_us(baudrate);
    }

    dataSend++;
    return true;
}

uint8_t* BusDriver::getContent() {
    return NULL; 
}

void BusDriver::changeState(BusDriverState newState) {

    state = newState;

    switch (state)
    {
        case BusDriverState::SynchronizeSignal:
        {
            errors = 0;
            lastData = false;
            lastSignalTime = time_us_64();
            break;
        }
        case BusDriverState::PackageInfo:
        {
            // Werte zurücksetzen.
            bufferCount = 0;
            //buffer = {0};
            //content = {0};

            // Nice for debugging.
            // for (size_t i = 0; i < MAX_PACKAGE_LENGTH * 8; i++)
            // {
            //     buffer[i] = 0;
            // }
            // for (size_t i = 0; i < MAX_PACKAGE_LENGTH; i++)
            // {
            //     content[i] = 0;
            // }

            packageCharacter = 0;
            packageLength = 0;
            packageChecksumHeader = 0;
            packageChecksumContent = 0;

            break;
        }
        case BusDriverState::ReadContent:
        {
            // Werte zurücksetzen.
            //bufferCount = 4;
            break;
        }
        case BusDriverState::PackageCheck: {
            break;
        }
    }
}

unsigned char  BusDriver::readFromHeader(uint octet) {
    int pos = octet * 8;
    return buffer[pos + 0] << 7 |
           buffer[pos + 1] << 6 |
           buffer[pos + 2] << 5 |
           buffer[pos + 3] << 4 |
           buffer[pos + 4] << 3 |
           buffer[pos + 5] << 2 |
           buffer[pos + 6] << 1 |
           buffer[pos + 7] << 0;
}

bool BusDriver::readData()
{
    uint64_t currentTime = time_us_64();

    switch (state)
    {
        case BusDriverState::SynchronizeSignal:
        {
            bool data = gpio_get(pin);

            if (lastData != data) {

                if(lastData) {
                    uint64_t diffTime = currentTime - lastSignalTime;
                    lastSignalTime = currentTime;

                    // Prüfen ob die Synchronisierung geklappt hat.
                    if(diffTime > baudsync) {
                        // Für die perfekte Synchronisierung.
                        lastSignalTime -= (baudrate / 2);
                    
                        changeState(BusDriverState::PackageInfo);
                    }
                }
                else {
                    lastSignalTime = currentTime;
                }

                lastData = data;
            }
            break;
        }
        case BusDriverState::PackageInfo:
        {
            bool data = gpio_get(pin);

            if(currentTime - lastSignalTime > baudrate)
            {
                lastSignalTime += baudrate;

                buffer[bufferCount] = data;

                // Auf validen Paketstart prüfen.
                if(bufferCount < 8) {
                    if(buffer[bufferCount] != bufferValid[bufferCount]) {
                        errors++;

                        // Nach 32 Lesefehlern aufgeben und auf das nächste Sync Signal warten.
                        if(errors > MAX_HEADER_LENGTH) {
                            // Ignore this message and wait on next sync signal.
                            changeState(BusDriverState::SynchronizeSignal);
                        } else {
                            // Invalid Package start.
                            changeState(BusDriverState::PackageInfo);
                        }
                        break;
                    }
                }

                bufferCount++;

                // Prüfen ob der Paket-Header vollständig gelesen wurde.
                if(bufferCount > MAX_HEADER_LENGTH)
                {
                    changeState(BusDriverState::PackageCheck);
                }
            }
            break;
        }
        case BusDriverState::PackageCheck:
        {
            packageCharacter        = readFromHeader(0);
            packageLength           = readFromHeader(1);
            packageChecksumContent  = readFromHeader(2);
            packageChecksumHeader   = readFromHeader(3);

            // ToDo: Check CRC8 in Header.

            // Validate Header
            if(packageCharacter != 'M') // && packageLength > MAX_PACKAGE_LENGTH - MAX_HEADER_LENGTH
            {
                // Bei CRC Fehler im Header:
                changeState(BusDriverState::SynchronizeSignal);
            }
            else
            {
                // Bei gültigen Header:
                changeState(BusDriverState::ReadContent);
            }
            break;
        }
        case BusDriverState::ReadContent:
        {
            if(currentTime - lastSignalTime > baudrate)
            {
                lastSignalTime += baudrate;

                buffer[bufferCount] = gpio_get(pin);
                bufferCount++;

                unsigned char content[260] = {0};

                if(bufferCount > (packageLength * 8) + MAX_HEADER_LENGTH)
                {
                    for (size_t i = 0; i < packageLength + (MAX_HEADER_LENGTH / 8); i++)
                    {
                        int pos = i * 8;

                        unsigned char character;

                        character = buffer[pos + 0] << 7 |
                                    buffer[pos + 1] << 6 |
                                    buffer[pos + 2] << 5 |
                                    buffer[pos + 3] << 4 |
                                    buffer[pos + 4] << 3 |
                                    buffer[pos + 5] << 2 |
                                    buffer[pos + 6] << 1 |
                                    buffer[pos + 7] << 0;

                        content[i] = character;
                    }

                    // ToDo: Check Content CRC.

                    // Vorbereitung um das nächste Paket zu lesen.
                    changeState(BusDriverState::PackageInfo);

                    // Erhaltene Daten zurückgeben.
                    return content[0] == true;
                }
            }
            break;
        }
    }

    return false;
}

// bool BusDriver::readData()
// {

//     uint64_t currentTime = time_us_64();

//     if ( ! isSync)
//     {
//         bool bit = gpio_get(pin);


//         // if Signal time benutzen.
        
//         if( ! bit) {
//             lastSignalTime = currentTime;
//         } else {
//             // Wichtig: 50 Mikrosekunden Toleranz für die synchronisation.
//             if(currentTime - lastSignalTime > (baudsync - 50)) {
//                 isSync = true;
//                 lastSignalTime = currentTime - (baudrate / 2);
//             }
//         }
//     }
//     else
//     {
//         if (currentTime - lastSignalTime > baudrate)
//         {
//             lastSignalTime += baudrate;

//             bool bit = gpio_get(pin);

//             if ( ! isValidPackage)
//             {
//                 if (bit == bufferValid[bufferCount])
//                 {
//                     buffer[bufferCount] = bit;
//                     bufferCount++;

//                     if (bufferCount == 8)
//                     {
//                         isValidPackage = true;
//                     }
//                 }
//                 else
//                 {
//                     errors++;
//                     isValidPackage = false;
//                     bufferCount = 0;
//                 }
//             }
//             else
//             {
//                 buffer[bufferCount] = bit;
//                 bufferCount++;

//                 if (bufferCount == 64)
//                 {
//                     unsigned char result[8];

//                     for (size_t i = 0; i < 8; i++)
//                     {
//                         int pos = i * 8;

//                         unsigned char character;

//                         character = buffer[pos + 0] << 7 |
//                                     buffer[pos + 1] << 6 |
//                                     buffer[pos + 2] << 5 |
//                                     buffer[pos + 3] << 4 |
//                                     buffer[pos + 4] << 3 |
//                                     buffer[pos + 5] << 2 |
//                                     buffer[pos + 6] << 1 |
//                                     buffer[pos + 7] << 0;

//                         result[i] = character;
//                     }

//                     for (size_t i = 0; i < 8; i++)
//                     {
//                         char test = result[i];
//                     }

//                     // Successfully: Reset class data.
//                     errors = 0;
//                     isValidPackage = false;
//                     isSync = false;
//                     bufferCount = 0;

//                     uint64_t totalSends = dataSend;
//                     return result[3];
//                 }
//             }
//         }
//         // if( ! isValidPackage) {
//         //     for (size_t i = 0; i < bufferCount || i == 8; i++)
//         //     {
//         //         if(buffer[bufferCount] != bufferValid[i]) {
//         //             bufferCount = 0;
//         //             isValidPackage = false;
//         //             return false;
//         //         }
//         //     }

//         //     isValidPackage = true;
//         // }

//         // if(bufferCount == 2048) {

//         //     // Header:  00 - 08 = MagicChar
//         //     //          08 - 16 = Package Identification (Int)
//         //     //          16 - 24 = Package Length (Int)
//         //     //          24 - ?? = Content CRC
//         //     //          24 - ?? = Header CRC
//         //     //          ?? - ?? = Content

//         //     // Header:  00 - 08 = MagicChar
//         //     //          24 - ?? = CRC
//         //     //          ?? - ?? = Content
//         //     return true;
//         // }
//     }

//     return false;
// }