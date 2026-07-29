#include "ui.h"

#include <M5Cardputer.h>

void uiInit() {
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.fillScreen(TFT_BLACK);
}

void uiShowIdle(uint8_t channel, uint32_t totalDetections, bool sdReady) {
    auto& d = M5Cardputer.Display;
    d.fillScreen(TFT_BLACK);
    d.setTextColor(TFT_GREEN);
    d.setCursor(4, 4);
    d.println("Flock-Ewe: scanning");

    d.setTextColor(TFT_WHITE);
    d.setCursor(4, 24);
    d.printf("Channel: %d\n", channel);
    d.setCursor(4, 40);
    d.printf("Detections: %lu\n", (unsigned long)totalDetections);
    d.setCursor(4, 56);
    d.setTextColor(sdReady ? TFT_GREEN : TFT_RED);
    d.printf("SD log: %s\n", sdReady ? "ready" : "unavailable");
}

void uiShowAlert(const Detection& det) {
    auto& d = M5Cardputer.Display;
    d.fillScreen(TFT_RED);
    d.setTextColor(TFT_WHITE);
    d.setCursor(4, 4);
    d.println("FLOCK CAMERA DETECTED");

    d.setCursor(4, 24);
    d.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", det.mac[0], det.mac[1],
              det.mac[2], det.mac[3], det.mac[4], det.mac[5]);
    d.setCursor(4, 44);
    d.printf("RSSI: %d dBm  Ch: %d\n", det.rssi, det.channel);
    d.setCursor(4, 64);
    d.printf("Match: %s%s\n", det.ouiMatch ? "OUI " : "", det.ieMatch ? "IE" : "");
}
