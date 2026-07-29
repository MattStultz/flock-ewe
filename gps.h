#pragma once

#include <cstdint>

struct GpsFix {
    bool valid;
    double lat;
    double lng;
    double altitudeM;
    uint8_t satellites;
};

// Starts the UART toward the Cap LoRa-1262's GNSS chip (ATGM336H, NMEA
// over UART on CardputerADV pins RX=13/TX=15).
void gpsInit();

// Call every loop() iteration; feeds incoming NMEA bytes to the parser.
void gpsLoop();

// Latest known fix. `valid` is false until a fresh fix has been parsed.
GpsFix gpsGetFix();
