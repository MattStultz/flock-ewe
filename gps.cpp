#include "gps.h"

#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

namespace {

const int PIN_GPS_RX = 13;  // GNSS module TX -> board RX
const int PIN_GPS_TX = 15;  // GNSS module RX -> board TX

// The Cap LoRa-1262's ATGM336H doesn't have a reliably documented NMEA
// baud rate, so probe common ones in order and lock onto whichever one
// actually produces valid, checksummed sentences. 9600 is the near-
// universal factory default for this chip family; 115200 is included
// since some vendor docs claim it.
const uint32_t BAUD_CANDIDATES[] = {9600, 115200};
const uint8_t BAUD_CANDIDATE_COUNT =
    sizeof(BAUD_CANDIDATES) / sizeof(BAUD_CANDIDATES[0]);
const uint32_t BAUD_PROBE_MS = 4000;  // time to wait before trying the next rate

const uint32_t FIX_MAX_AGE_MS = 5000;
const uint32_t COMMS_MAX_AGE_MS = 3000;

HardwareSerial gpsSerial(1);  // UART1; UART0 is reserved by USB CDC
TinyGPSPlus gps;

uint8_t baudIndex = 0;
uint32_t baudStartedMs = 0;
uint32_t lastValidSentenceMs = 0;
uint32_t lastPassedChecksum = 0;
bool baudLocked = false;  // stop probing once a rate proves valid

void beginAtCurrentBaud() {
    gpsSerial.end();
    gpsSerial.begin(BAUD_CANDIDATES[baudIndex], SERIAL_8N1, PIN_GPS_RX,
                     PIN_GPS_TX);
    baudStartedMs = millis();
}

}  // namespace

void gpsInit() {
    beginAtCurrentBaud();
}

void gpsLoop() {
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }

    uint32_t passed = gps.passedChecksum();
    if (passed != lastPassedChecksum) {
        lastPassedChecksum = passed;
        lastValidSentenceMs = millis();
        baudLocked = true;
    }

    if (!baudLocked && millis() - baudStartedMs >= BAUD_PROBE_MS) {
        baudIndex = (baudIndex + 1) % BAUD_CANDIDATE_COUNT;
        beginAtCurrentBaud();
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
