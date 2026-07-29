#pragma once

#include <cstdint>

struct GpsFix {
    bool valid;
    double lat;
    double lng;
    double altitudeM;
    uint8_t satellites;
};

struct GpsStatus {
    bool communicating;  // valid NMEA checksums seen recently (UART link is good)
    uint8_t satellites;  // satellites currently reported, even before a fix
    GpsFix fix;
};

// Starts the UART toward the Cap LoRa-1262's GNSS chip (ATGM336H, NMEA
// over UART on CardputerADV pins RX=13/TX=15). The chip's actual NMEA
// baud rate isn't reliably documented, so this probes common rates and
// locks onto whichever one produces valid sentences.
void gpsInit();

// Call every loop() iteration; feeds incoming NMEA bytes to the parser
// and drives the baud-rate probe.
void gpsLoop();

// Latest known fix. `valid` is false until a fresh fix has been parsed.
GpsFix gpsGetFix();

// Diagnostic snapshot for the GPS status screen.
GpsStatus gpsGetStatus();

// Latest known UTC date/time from the GNSS almanac (NMEA RMC/GGA), used to
// name session log files. `year2` is the two-digit year (e.g. 26 for 2026).
// Returns false if no valid date/time has been parsed yet.
bool gpsGetDateTime(int& month, int& day, int& year2, int& hour, int& minute);
