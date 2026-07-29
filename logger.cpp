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

void loggerLogDetection(const Detection& d) {
    if (!ready) return;

    logFile.printf(
        "{\"t\":%lu,\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"rssi\":%d,"
        "\"ch\":%d,\"oui\":%s,\"ie\":%s}\n",
        (unsigned long)d.timestampMs, d.mac[0], d.mac[1], d.mac[2], d.mac[3],
        d.mac[4], d.mac[5], d.rssi, d.channel, d.ouiMatch ? "true" : "false",
        d.ieMatch ? "true" : "false");
    logFile.flush();
}
