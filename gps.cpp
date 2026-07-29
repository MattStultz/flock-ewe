#include "gps.h"

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

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
