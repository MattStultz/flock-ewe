#include "gps.h"

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>
#include <cmath>

namespace {

// Confirmed against psifertex/meshtastic-firmware's working CardputerADV +
// Cap LoRa-1262 variant config: board RX (receives the GNSS module's TX
// output) is GPIO15, board TX is GPIO13, fixed 115200 baud. An earlier
// version of this file had RX/TX swapped, which produced zero NMEA data
// regardless of baud rate — that was the actual bug, not the baud rate.
const int PIN_GPS_RX = 15;
const int PIN_GPS_TX = 13;
const uint32_t GPS_BAUD = 115200;

const uint32_t FIX_MAX_AGE_MS = 5000;
const uint32_t COMMS_MAX_AGE_MS = 3000;

HardwareSerial gpsSerial(1);  // UART1; UART0 is reserved by USB CDC
TinyGPSPlus gps;

uint32_t lastValidSentenceMs = 0;
uint32_t lastPassedChecksum = 0;

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return dim[month - 1];
}

// Sakamoto's algorithm. Returns 0=Sunday..6=Saturday.
int dayOfWeek(int year, int month, int day) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) year -= 1;
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

int nthSundayOfMonth(int year, int month, int n) {
    int dow1 = dayOfWeek(year, month, 1);
    int firstSunday = (dow1 == 0) ? 1 : (8 - dow1);
    return firstSunday + (n - 1) * 7;
}

// Approximates whether US DST is in effect (2nd Sunday of March through
// the 1st Sunday of November), ignoring the exact 2am local transition
// time — good enough for a log filename, not precise to the hour on the
// two transition days themselves. Says nothing about whether the
// underlying location actually observes DST at all (see gpsGetLocalDateTime
// in gps.h for that caveat).
bool isUsDstActive(int year, int month, int day) {
    if (month < 3 || month > 11) return false;
    if (month > 3 && month < 11) return true;
    if (month == 3) return day >= nthSundayOfMonth(year, 3, 2);
    return day < nthSundayOfMonth(year, 11, 1);
}

// Adds a signed hour offset to a UTC calendar date/time, rolling the
// day/month/year over as needed.
void addHoursToDate(int& year, int& month, int& day, int& hour, int offsetHours) {
    hour += offsetHours;
    while (hour < 0) {
        hour += 24;
        day -= 1;
        if (day < 1) {
            month -= 1;
            if (month < 1) {
                month = 12;
                year -= 1;
            }
            day = daysInMonth(year, month);
        }
    }
    while (hour >= 24) {
        hour -= 24;
        day += 1;
        if (day > daysInMonth(year, month)) {
            day = 1;
            month += 1;
            if (month > 12) {
                month = 1;
                year += 1;
            }
        }
    }
}

}  // namespace

void gpsInit() {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
}

void gpsLoop() {
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }

    uint32_t passed = gps.passedChecksum();
    if (passed != lastPassedChecksum) {
        lastPassedChecksum = passed;
        lastValidSentenceMs = millis();
    }
}

GpsFix gpsGetFix() {
    GpsFix fix{};
    fix.valid = gps.location.isValid() && gps.location.age() < FIX_MAX_AGE_MS;
    if (fix.valid) {
        fix.lat = gps.location.lat();
        fix.lng = gps.location.lng();
        fix.altitudeM = gps.altitude.meters();
        fix.satellites = gps.satellites.value();
    }
    return fix;
}

GpsStatus gpsGetStatus() {
    GpsStatus status{};
    status.communicating = lastValidSentenceMs != 0 &&
                            millis() - lastValidSentenceMs < COMMS_MAX_AGE_MS;
    status.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;
    status.fix = gpsGetFix();
    return status;
}

bool gpsGetDateTime(int& month, int& day, int& year2, int& hour, int& minute) {
    if (!gps.date.isValid() || !gps.time.isValid()) return false;
    month = gps.date.month();
    day = gps.date.day();
    year2 = gps.date.year() % 100;
    hour = gps.time.hour();
    minute = gps.time.minute();
    return true;
}

bool gpsGetLocalDateTime(int& month, int& day, int& year2, int& hour,
                         int& minute) {
    if (!gps.date.isValid() || !gps.time.isValid()) return false;

    int year = gps.date.year();
    month = gps.date.month();
    day = gps.date.day();
    hour = gps.time.hour();
    minute = gps.time.minute();

    if (gps.location.isValid()) {
        int offset = static_cast<int>(std::lround(gps.location.lng() / 15.0));
        if (offset > 14) offset = 14;
        if (offset < -12) offset = -12;

        if (isUsDstActive(year, month, day)) offset += 1;

        addHoursToDate(year, month, day, hour, offset);
    }

    year2 = year % 100;
    return true;
}
