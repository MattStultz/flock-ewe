#include "gps.h"

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

namespace {

const int PIN_GPS_RX = 13;  // GNSS module TX -> board RX
const int PIN_GPS_TX = 15;  // GNSS module RX -> board TX
const uint32_t GPS_BAUD = 115200;
const uint32_t FIX_MAX_AGE_MS = 5000;

HardwareSerial gpsSerial(1);  // UART1; UART0 is reserved by USB CDC
TinyGPSPlus gps;

}  // namespace

void gpsInit() {
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
}

void gpsLoop() {
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
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
