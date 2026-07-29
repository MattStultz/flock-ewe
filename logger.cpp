#include "logger.h"

#include <SD.h>
#include <SPI.h>

namespace {

const int PIN_SD_CS = 12;
const int PIN_SD_MOSI = 14;
const int PIN_SD_CLK = 40;
const int PIN_SD_MISO = 39;

SPIClass sdSpi(HSPI);
File logFile;
bool ready = false;

}  // namespace

bool loggerInit() {
    sdSpi.begin(PIN_SD_CLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    if (!SD.begin(PIN_SD_CS, sdSpi)) {
        ready = false;
        return false;
    }

    char path[32];
    snprintf(path, sizeof(path), "/flockewe_%lu.jsonl", (unsigned long)millis());
    logFile = SD.open(path, FILE_WRITE);
    ready = static_cast<bool>(logFile);
    return ready;
}

void loggerLogDetection(const Detection& d, const GpsFix& fix) {
    if (!ready) return;

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

    logFile.println(line);
    logFile.flush();
}
