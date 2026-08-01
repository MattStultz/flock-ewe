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
// over UART on CardputerADV: board RX=GPIO15, board TX=GPIO13, 115200
// baud — confirmed against a working reference (meshtastic-firmware's
// CardputerADV variant), not the M5Stack docs table, which has the pins
// swapped from the board's point of view.
void gpsInit();

// Call every loop() iteration; feeds incoming NMEA bytes to the parser.
void gpsLoop();

// Latest known fix. `valid` is false until a fresh fix has been parsed.
GpsFix gpsGetFix();

// Diagnostic snapshot for the GPS status screen.
GpsStatus gpsGetStatus();

// Latest known UTC date/time from the GNSS almanac (NMEA RMC/GGA).
// `year2` is the two-digit year (e.g. 26 for 2026).
// Returns false if no valid date/time has been parsed yet.
bool gpsGetDateTime(int& month, int& day, int& year2, int& hour, int& minute);

// Estimated *local* date/time, used to name session log files. There's no
// network connection here for a real timezone/DST lookup, so this is an
// approximation: UTC offset is estimated from longitude (15 degrees per
// hour), then the US DST rule (2nd Sunday of March - 1st Sunday of
// November) is applied. That means it will be wrong by about an hour, for
// roughly two-thirds of the year, in US regions that don't observe DST
// (Arizona, parts of Indiana, Hawaii, etc.) — there's no way to detect
// that from longitude alone. Outside the US it's just a longitude-based
// standard-time guess with no DST adjustment at all, since DST rules
// vary by country. Falls back to UTC (offset 0) if there's no location
// fix yet, even if the date/time itself is already valid.
// Returns false if no valid date/time has been parsed yet.
bool gpsGetLocalDateTime(int& month, int& day, int& year2, int& hour,
                         int& minute);
