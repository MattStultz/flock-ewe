// Flock-Ewe: passive WiFi-based Flock Safety camera detector for the
// M5Stack CardputerADV. Receive-only — listens for probe-request beacons,
// never transmits or associates. Built for privacy-auditing/research use.
//
// Board:     M5Stack CardputerADV (ESP32-S3) + Cap LoRa-1262 module (GNSS)
// Framework: Arduino IDE (esp32 board package by Espressif Systems)
// Libraries: M5Cardputer (pulls in M5Unified, M5GFX, IRremote), TinyGPSPlus
// Launcher:  compatible as a standalone sketch — see README for details.

#include <M5Cardputer.h>

#include "flock_detect.h"
#include "gps.h"
#include "logger.h"
#include "ui.h"
#include "wifi_scan.h"

namespace {

uint32_t totalDetections = 0;
uint32_t lastUiRefresh = 0;
uint32_t alertUntilMs = 0;
bool sdReady = false;

}  // namespace

void setup() {
    // CardputerADV: hold GPIO5 high during init to avoid SD/I2C
    // (keyboard + IMU) bus contention on this board.
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    uiInit();
    wifiScanInit();
    gpsInit();
    sdReady = loggerInit();

    uiShowIdle(wifiScanCurrentChannel(), totalDetections, sdReady);
}

void loop() {
    M5Cardputer.update();
    wifiScanLoop();
    gpsLoop();

    Detection det;
    while (wifiScanPopDetection(det)) {
        totalDetections++;
        loggerLogDetection(det, gpsGetFix());
        M5Cardputer.Speaker.tone(2000, 150);
        uiShowAlert(det);
        alertUntilMs = millis() + 3000;
    }

    uint32_t now = millis();
    if (now > alertUntilMs && now - lastUiRefresh > 500) {
        uiShowIdle(wifiScanCurrentChannel(), totalDetections, sdReady);
        lastUiRefresh = now;
    }
}
