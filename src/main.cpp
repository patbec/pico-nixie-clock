#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "stdio.h"
#include "../lib/TubeDriver.h"
#include "../lib/ClockDriver.h"
#include "../lib/BusDriver.h"

void core1_entry() {
    while(true) {
       sleep_us(200000); // Wait 200ms

        Package p = Package();
        p.identity = 'M';
        p.content[0] = 'A';
        p.content[1] = 'B';
        p.content[2] = 'C';
        p.content[3] = 'D';
        p.content[4] = 'Q';
        p.content[5] = 'W';
        p.content[6] = 'Z';

        p.contentLength = 7; // Nicht ab Index Null zählen.
        
        BusDriver::sendData(p, 27);
    }
}

int main()
{
    // bool debug[8] = {0,1,0,0,1,1,0,1};
    // uint8_t result = 0;

    // for (size_t i = 0; i <= 7; i++)
    // {
    //     result |= debug[i] << (7 - i);
    //     /* code */
    // }
    // return result;


    multicore_launch_core1(core1_entry);

    struct TubePinLayout pinLayoutPoints;
    pinLayoutPoints.pinA = 17;
    pinLayoutPoints.pinB = 16;
    pinLayoutPoints.pinC = 22;
    pinLayoutPoints.pinD = UINT8_MAX;

    struct TubePinLayout pinLayoutTube1;
    pinLayoutTube1.pinA = 2;
    pinLayoutTube1.pinB = 3;
    pinLayoutTube1.pinC = 4;
    pinLayoutTube1.pinD = 5;

    struct TubePinLayout pinLayoutTube2;
    pinLayoutTube2.pinA = 21;
    pinLayoutTube2.pinB = 20;
    pinLayoutTube2.pinC = 19;
    pinLayoutTube2.pinD = 18;

    struct TubePinLayout pinLayoutTube3;
    pinLayoutTube3.pinA = 6;
    pinLayoutTube3.pinB = 7;
    pinLayoutTube3.pinC = 8;
    pinLayoutTube3.pinD = 9;

    struct TubePinLayout pinLayoutTube4;
    pinLayoutTube4.pinA = 10;
    pinLayoutTube4.pinB = 11;
    pinLayoutTube4.pinC = 12;
    pinLayoutTube4.pinD = 13;

    struct ClockPinLayout pinLayoutClock;
    pinLayoutClock.tube1Pins = pinLayoutTube1;
    pinLayoutClock.tube2Pins = pinLayoutTube2;
    pinLayoutClock.tube3Pins = pinLayoutTube3;
    pinLayoutClock.tube4Pins = pinLayoutTube4;

    pinLayoutClock.pointsPins = pinLayoutPoints;
    pinLayoutClock.DCFPin = 28;

    // Funktionstest
    // uint8_t functionTestButtonPin = 15;
    // if(functionTestButtonPin != UINT8_MAX) {
    //     gpio_init(functionTestButtonPin);
    //     gpio_set_dir(functionTestButtonPin, GPIO_IN);

    //     if(gpio_get(functionTestButtonPin))
    // }

    ClockDriver *clock = new ClockDriver(pinLayoutClock);

    // TubeDriver *tubes[4] = {tube1, tube2, tube3, tube4};

    // tube1->powerOn();
    // tube2->powerOn();
    // tube3->powerOn();
    // tube4->powerOn();

    // pointsDriver->powerOn();

    // int counter = 0;
    // int frame = 0;
    // bool pointBit = true;
    //uint8_t functionTestButtonPin = 15;

    //if(functionTestButtonPin != UINT8_MAX) {
    //    gpio_init(functionTestButtonPin);
    //    gpio_set_dir(functionTestButtonPin, GPIO_IN);
    //}

    BusDriver *bus = new BusDriver(26);
    Package *package;

    while (true)
    {
        package = bus->readData();

        if(package != nullptr) {
            printf("Paket empfangen: /s", package->identity);

            delete package;
        }

        clock->clock();

        // tube1->showDigit(counter);
        // tube2->showDigit(counter);
        // tube3->showDigit(counter);
        // tube4->showDigit(counter);

        // pointsDriver->showDigit(counter);

        // uint8_t pointPosition = pointsDriver->getLastDigit() + 1;
        // pointsDriver->showDigit(pointPosition % 8);

        // if (frame % 5 == 0)
        // {
        //     for (int i = 0; i < 4; i++)
        //     {
        //         tubes[i]->showPoint(pointBit, pointBit);
        //     }
        //     pointBit = !pointBit;
        // }
        // counter++;
        // if (counter > 9)
        // {
        //     counter = 0;
        // }
        // frame++;
    }
}