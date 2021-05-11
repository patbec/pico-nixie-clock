# Minimal zulässige Version von CMake: 
cmake_minimum_required(VERSION 3.15)
# Inkludieren des Pico-SDK: 
include(include/pico_sdk_import.cmake)
# Name und Version des Projekts: 
project(pico-nixie-clock VERSION 1.0.0) 
# Das Projekt mit einer Quelldatei verknüpfen, das das Hauptprogramm enthält:
add_executable(pico-nixie-clock
    "lib/TubeDriver.h"
    "lib/TubeDriver.cpp"
    "lib/ClockDriver.h"
    "lib/ClockDriver.cpp"
    "lib/DCFDriver.h"
    "lib/DCFDriver.cpp"
    "lib/BusDriver.h"
    "lib/BusDriver.cpp"
    "lib/CRC8.h"
    "lib/CRC8.cpp"
    "src/main.cpp")

# Angabe der benötigten Bibliothek(en): 
target_link_libraries(pico-nixie-clock hardware_i2c pico_stdlib pico_multicore hardware_rtc)
# Initialisieren des SDK: 
pico_sdk_init()
# Zugriff auf USB und UART ermöglichen (1 = enable, 0 = disable): 
pico_enable_stdio_usb(pico-nixie-clock 1) 
pico_enable_stdio_uart(pico-nixie-clock 1)
# Definition notwendiger Extra-Zieldateien. Hieraus generiert das Tooling
# die für die Übertragung auf den Pico benötigte UF2-Datei: 
pico_add_extra_outputs(pico-nixie-clock)

pico_set_program_name(pico-nixie-clock "Nixie Clock")
pico_set_program_description(pico-nixie-clock "Example code for a Nixie clock with an Raspberry Pi Pico (RP2040).")
pico_set_program_url(pico-nixie-clock "https://github.com/patbec/pico-nixie-clock")