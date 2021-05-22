#include "pico/stdlib.h"
#include "BusDriver.h"
#include "CRC8.h"

uint64_t BusDriver::baudrate = 100;
uint64_t BusDriver::baudsync = 1200;
uint64_t BusDriver::baudclear = 500;
uint64_t BusDriver::dataSend = 0;

BusDriver::BusDriver(int pin)
{
    this->pin = pin;
    this->state = BusDriverState::SynchronizeSignal;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
}

bool BusDriver::sendData(Package package, int pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);

    uint8_t sbuffer[259] = {0};

    // Schreibe den Inhalt in den Buffer.
    for (size_t i = 0; i < package.contentLength; i++)
    {
        sbuffer[PAK_LENGTH + i] = package.content[i];
    }

    // Paketkopf schreiben.
    sbuffer[PAK_HEAD0_IDENT]         = package.identity;
    sbuffer[PAK_HEAD1_LENGTH]        = package.contentLength;
    sbuffer[PAK_HEAD2_CRC_CONTENT]   = CRC8().Compute_CRC8(sbuffer, PAK_LENGTH, PAK_LENGTH + package.contentLength);
    sbuffer[PAK_HEAD3_CRC_HEADER]    = CRC8().Compute_CRC8(sbuffer, 0, PAK_HEAD2_CRC_CONTENT);

    uint16_t packageLength = PAK_LENGTH + package.contentLength;
    
    // Sende SYNC-Signal.
    gpio_put(pin, false);
    sleep_us(baudclear);
    gpio_put(pin, true);
    sleep_us(baudsync);

    // Sende Paket.
    for (uint16_t posByte = 0; posByte < packageLength; posByte++)
    {
        unsigned char character = sbuffer[posByte];
        unsigned char test = character;
        // Bits senden.
        for (int8_t posBit = 7; posBit >= 0; posBit--)
        {
            bool bitToSend = (character >> posBit) & 1;
            gpio_put(pin, bitToSend);
            sleep_us(baudrate);
        }
    }

    return true;
}

void BusDriver::changeState(BusDriverState newState)
{
    state = newState;

    switch (state)
    {
        case BusDriverState::SynchronizeSignal:
        {
            // Warte auf das SYNC-Signal vom Sender.

            lastData = false;
            lastSignalTime = time_us_64();

            break;
        }
        case BusDriverState::PackageHeader:
        {
            // Beginnt den ankommenden Header aus dem Paket zu lesen.

            for (size_t i = 0; i < 259; i++)
            {
                buffer[i] = 0;
            }
            bufferCount = 0;

            bytesLeft = PAK_LENGTH;
            cache = 0;
            bitsRead = 0;

            break;
        }
        case BusDriverState::PackageContent:
        {
            // Beginnt den ankommenden Inhalt aus dem Paket zu lesen.

            bytesLeft = buffer[PAK_HEAD1_LENGTH];
            cache = 0;

            break;
        }
        case BusDriverState::PackageError:
        {
            // Tritt meistens bei CRC-Fehlern auf.

            break;
        }
        case BusDriverState::PackageCompleted:
        {
            // Ein Paket wurde fehlerfrei gelesen.

            cache = 0;

            break;
        }
    }
}

bool BusDriver::canRead() {
    uint64_t currentTime = time_us_64();

    if ((currentTime - lastSignalTime) > baudrate) {
        lastSignalTime += baudrate;

        return true;
    }

    return false;
}

uint8_t BusDriver::readByte2() {
    bool data = gpio_get(pin);
    
    if (canRead())
    {
        // Bit auslesen und in den Byte Cache schreiben.
        cache |= (data << (7 - bitsRead));
        bitsRead++;

        // Wenn 8 Bits gelesen, das Byte in den Puffer schreiben.
        if (bitsRead == 8) {
            bitsRead = 0;

            buffer[bufferCount] = cache;
            bufferCount++;

            cache = 0;

            // Wird in <see="changeState"/> festgelegt.
            bytesLeft--;
        }
    }

    return bytesLeft;
}

bool BusDriver::readByte() {
    bool data = gpio_get(pin);
    
    if (canRead())
    {
        // Bit auslesen und in den Byte Cache schreiben.
        cache |= (data << (7 - bitsRead));
        bitsRead++;

        // Wenn 8 Bits gelesen, das Byte in den Puffer schreiben.
        if (bitsRead == 8) {
            bitsRead = 0;

            buffer[bufferCount] = cache;
            bufferCount++;

            cache = 0;

            // Wird in <see="changeState"/> festgelegt.
            bytesLeft--;

            return (bytesLeft == 0);
        }
    }

    return false;
}


bool BusDriver::readSync() {
    uint64_t currentTime = time_us_64();

    bool data = gpio_get(pin);

    // Nur auf Änderungen reagieren.
    if (lastData != data) {
        bool dummy = lastData;
        lastData = data;

        // Nur auf das High-Signal reagieren.
        if(dummy)
        {
            // Prüfen ob die Synchronisierung geklappt hat.
            if ((currentTime - lastSignalTime) > baudsync)
            {
                // Für die perfekte Synchronisierung.
                lastSignalTime = currentTime - (baudrate / 2);
                // Erfolgreich synchronisiert.
                return true;
            }
        }

        // Letzte Signaländerung festhalten.
        lastSignalTime = currentTime;
    }

    return false;
}


Package* BusDriver::readData()
{
    if(bufferCount > 259) {
        changeState(BusDriverState::PackageError);
        return NULL;
    }

    switch (state)
    {
        case BusDriverState::SynchronizeSignal:
        {
            // Auf das SYNC-Signal vom Sender warten.
            if(readSync()) {
                changeState(BusDriverState::PackageHeader);    
            }
            break;
        }
        case BusDriverState::PackageHeader:
        {
            // Alternative Methode:
            // uint8_t bytesLeft = readByte2();

            bool isCompleted = readByte();

            // Prüfen ob der Header fertig gelesen wurde.
            if (isCompleted)
            {
                // Informationen aus dem Header auslesen.
                uint8_t packageIdentity = buffer[PAK_HEAD0_IDENT];
                uint8_t packageHeaderChecksum = buffer[PAK_HEAD3_CRC_HEADER];

                // CRC8 des empfangenen Headers berechnen.
                uint8_t currentHeaderChecksum = CRC8().Compute_CRC8(buffer, 0, PAK_HEAD2_CRC_CONTENT);

                // Prüfen ob ein CRC-Fehler aufgetreten oder nicht ein unterstütztes Paket empfangen wurde.
                if (packageIdentity == 'M' && packageHeaderChecksum == currentHeaderChecksum) {
                    changeState(BusDriverState::PackageContent);
                } else {
                    changeState(BusDriverState::PackageError);
                }
            }
            break;
        }
        case BusDriverState::PackageContent:
        {
            // Alternative Methode:
            // uint8_t bytesLeft = readByte2();

            bool isCompleted = readByte();

            // Prüfen ob der Inhalt fertig gelesen wurde.
            if (isCompleted)
            {
                // Informationen aus dem Header auslesen.
                uint8_t packageContentChecksum = buffer[PAK_HEAD2_CRC_CONTENT];

                // CRC8 des empfangenen Inhaltes berechnen.
                uint8_t currentHeaderChecksum = CRC8().Compute_CRC8(buffer, PAK_LENGTH, bufferCount);

                // Prüfen ob ein CRC-Fehler im Inhalt aufgetreten ist.
                if (packageContentChecksum == currentHeaderChecksum) {
                    changeState(BusDriverState::PackageCompleted);
                } else {
                    changeState(BusDriverState::PackageError);
                }
            }
            break;
        }
        case BusDriverState::PackageCompleted:
        {
            uint8_t packageIdentity         = buffer[PAK_HEAD0_IDENT];
            uint8_t packageContentLength    = buffer[PAK_HEAD1_LENGTH];
            uint8_t packageContentChecksum  = buffer[PAK_HEAD2_CRC_CONTENT];
            uint8_t packageHeaderChecksum   = buffer[PAK_HEAD3_CRC_HEADER];

            Package* package = new Package();
            package->identity = packageIdentity;
            package->contentLength = packageContentLength;

            for (size_t i = 0; i < packageContentLength; i++)
            {
                package->content[i] = buffer[PAK_LENGTH + i];
            }

            // Vorbereitung um das nächste Paket zu lesen. (Cache leeren etc.)
            changeState(BusDriverState::PackageHeader);

            // Später Speicherfreigabe nicht vergessen.
            return package;
        }
        case BusDriverState::PackageError: {
            errors++;
            changeState(BusDriverState::SynchronizeSignal);
            
            break;
        }
    }

    // Kein vollständiges Paket empfangen.
    return NULL;
}