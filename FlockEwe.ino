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

enum AppMode { MODE_IDLE, MODE_ALERT, MODE_MENU };

const uint32_t ALERT_HOLD_MS = 10000;
const uint32_t BEEP_INTERVAL_MS = 1000;
const uint32_t IDLE_REFRESH_MS = 500;

AppMode mode = MODE_IDLE;
uint32_t totalDetections = 0;
uint32_t lastUiRefresh = 0;
uint32_t alertEnteredMs = 0;
uint32_t lastBeepMs = 0;
bool sdReady = false;
bool audioAlertsEnabled = true;
bool prevMKeyPressed = false;
bool prevEnterKeyPressed = false;

void beep() {
    if (audioAlertsEnabled) M5Cardputer.Speaker.tone(2000, 120);
}

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

    uiShowIdle(wifiScanCurrentChannel(), totalDetections, sdReady, audioAlertsEnabled);
}

void loop() {
    M5Cardputer.update();
    wifiScanLoop();
    gpsLoop();

    uint32_t now = millis();

    // A new detection always takes priority: (re)enter alert mode and
    // reset the hold timer, even if an alert is already showing.
    Detection det;
    bool gotDetection = false;
    while (wifiScanPopDetection(det)) {
        gotDetection = true;
        totalDetections++;
        loggerLogDetection(det, gpsGetFix());
    }
    if (gotDetection) {
        mode = MODE_ALERT;
        alertEnteredMs = now;
        lastBeepMs = now;
        uiShowAlert(det);
        beep();
    }

    // Edge-detect the 'm' key so holding it doesn't repeatedly toggle.
    bool mNow = M5Cardputer.Keyboard.isKeyPressed('m');
    if (mNow && !prevMKeyPressed) {
        if (mode == MODE_IDLE) {
            mode = MODE_MENU;
            uiShowMenu(audioAlertsEnabled);
        } else if (mode == MODE_MENU) {
            mode = MODE_IDLE;
            uiShowIdle(wifiScanCurrentChannel(), totalDetections, sdReady,
                       audioAlertsEnabled);
        }
    }
    prevMKeyPressed = mNow;

    if (mode == MODE_MENU) {
        bool enterNow = M5Cardputer.Keyboard.keysState().enter;
        if (enterNow && !prevEnterKeyPressed) {
            audioAlertsEnabled = !audioAlertsEnabled;
            uiShowMenu(audioAlertsEnabled);
        }
        prevEnterKeyPressed = enterNow;
    }

    if (mode == MODE_ALERT) {
        if (now - alertEnteredMs >= ALERT_HOLD_MS) {
            mode = MODE_IDLE;
            uiShowIdle(wifiScanCurrentChannel(), totalDetections, sdReady,
                       audioAlertsEnabled);
        } else if (now - lastBeepMs >= BEEP_INTERVAL_MS) {
            lastBeepMs = now;
            beep();
            uint32_t elapsed = now - alertEnteredMs;
            uint8_t secondsLeft = (ALERT_HOLD_MS - elapsed + 999) / 1000;
            uiUpdateAlertCountdown(secondsLeft);
        }
    }

    if (mode == MODE_IDLE && now - lastUiRefresh > IDLE_REFRESH_MS) {
        uiShowIdle(wifiScanCurrentChannel(), totalDetections, sdReady,
                   audioAlertsEnabled);
        lastUiRefresh = now;
    }
}
