#include "logger.h"

#include <SD.h>
#include <SPI.h>

#include "gps.h"

namespace {

const int PIN_SD_CS = 12;
const int PIN_SD_MOSI = 14;
const int PIN_SD_CLK = 40;
const int PIN_SD_MISO = 39;

SPIClass sdSpi(HSPI);
bool sdReady = false;
bool sessionNamed = false;
char sessionPath[40] = "";

// Names the session file from the GPS's UTC date/time at the moment of the
// first detection, as "flockeweMMDDYYHHmm.txt". Falls back to a millis()-
// based name if no GPS fix/time has been acquired yet.
void nameSessionFile() {
    int month, day, year2, hour, minute;
    if (gpsGetDateTime(month, day, year2, hour, minute)) {
        snprintf(sessionPath, sizeof(sessionPath), "/flockewe%02d%02d%02d%02d%02d.txt",
                 month, day, year2, hour, minute);
    } else {
        snprintf(sessionPath, sizeof(sessionPath), "/flockewe_nofix_%lu.txt",
                 (unsigned long)millis());
    }
    sessionNamed = true;
}

}  // namespace

bool loggerInit() {
    sdSpi.begin(PIN_SD_CLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    sdReady = SD.begin(PIN_SD_CS, sdSpi);
    return sdReady;
}

void loggerLogDetection(const Detection& d, const GpsFix& fix) {
    if (!sdReady) return;
    if (!sessionNamed) nameSessionFile();

    // Opened, appended, and closed on every call (rather than held open for
    // the session) so a sudden power loss can't leave the file locked or
    // corrupted.
    File f = SD.open(sessionPath, FILE_WRITE);
    if (!f) return;

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", d.mac[0],
             d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);

    String line = "{";
    line += "\"t\":" + String(d.timestampMs);
    line += ",\"mac\":\"" + String(macStr) + "\"";
    line += ",\"rssi\":" + String(d.rssi);
    line += ",\"ch\":" + String(d.channel);
    line += ",\"oui\":" + String(d.ouiMatch ? "true" : "false");
    line += ",\"ie\":" + String(d.ieMatch ? "true" : "false");
    line += ",\"gps_fix\":" + String(fix.valid ? "true" : "false");
    if (fix.valid) {
        line += ",\"lat\":" + String(fix.lat, 6);
        line += ",\"lng\":" + String(fix.lng, 6);
        line += ",\"alt_m\":" + String(fix.altitudeM, 1);
        line += ",\"sats\":" + String(fix.satellites);
    }
    line += "}";

    f.println(line);
    f.close();
}
