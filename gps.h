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

// Latest known UTC date/time from the GNSS almanac (NMEA RMC/GGA), used to
// name session log files. `year2` is the two-digit year (e.g. 26 for 2026).
// Returns false if no valid date/time has been parsed yet.
bool gpsGetDateTime(int& month, int& day, int& year2, int& hour, int& minute);
