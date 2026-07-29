#include "ui.h"

#include <M5Cardputer.h>

namespace {

uint16_t colBg;
uint16_t colDim;
uint16_t colCyan;
uint16_t colMagenta;
uint16_t colAmber;
uint16_t colAlertBg;
uint16_t colSheepBody;
uint16_t colSheepHead;
uint16_t colEyeCalm;
uint16_t colEyeAlert;
uint16_t colHint;

void drawHudCorners(uint16_t color) {
    auto& d = M5Cardputer.Display;
    const int len = 10;
    d.drawFastHLine(0, 0, len, color);
    d.drawFastVLine(0, 0, len, color);
    d.drawFastHLine(240 - len, 0, len, color);
    d.drawFastVLine(239, 0, len, color);
    d.drawFastHLine(0, 134, len, color);
    d.drawFastVLine(0, 134 - len, len, color);
    d.drawFastHLine(240 - len, 134, len, color);
    d.drawFastVLine(239, 134 - len, len, color);
}

void drawScanlines(uint16_t color) {
    auto& d = M5Cardputer.Display;
    for (int y = 0; y < 135; y += 4) {
        d.drawFastHLine(0, y, 240, color);
    }
}

// Faux-neon-glow text: a dim copy one pixel down-right, then the bright
// copy on top.
void drawGlowText(int x, int y, const char* text, uint8_t size, uint16_t glow,
                   uint16_t bright) {
    auto& d = M5Cardputer.Display;
    d.setTextSize(size);
    d.setTextColor(glow);
    d.setCursor(x + 1, y + 1);
    d.print(text);
    d.setTextColor(bright);
    d.setCursor(x, y);
    d.print(text);
}

// "CyberEwe" mascot: a fluffy blocky body, one ear, four stub legs, and a
// glowing sensor eye. Built from plain shape primitives rather than a
// bitmap so it's easy to verify without a device on hand.
void drawSheep(int x, int y, uint16_t bodyColor, uint16_t headColor,
               uint16_t eyeColor) {
    auto& d = M5Cardputer.Display;

    d.fillRoundRect(x + 9, y, 30, 18, 6, bodyColor);
    d.fillCircle(x + 9, y + 6, 6, bodyColor);
    d.fillCircle(x + 18, y - 2, 6, bodyColor);
    d.fillCircle(x + 30, y - 2, 6, bodyColor);
    d.fillCircle(x + 39, y + 6, 6, bodyColor);

    d.fillRect(x + 12, y + 17, 3, 8, headColor);
    d.fillRect(x + 21, y + 17, 3, 8, headColor);
    d.fillRect(x + 30, y + 17, 3, 8, headColor);
    d.fillRect(x + 38, y + 17, 3, 8, headColor);

    d.fillRoundRect(x, y + 3, 14, 12, 3, headColor);
    d.fillTriangle(x + 2, y + 3, x + 6, y - 3, x + 9, y + 3, headColor);

    d.fillCircle(x + 5, y + 9, 2, eyeColor);
}

// Redraws only the alert screen's bottom row (used both for the initial
// draw and for the once-a-second countdown tick).
void drawCountdownRow(int secondsRemaining) {
    auto& d = M5Cardputer.Display;
    d.fillRect(0, 96, 230, 20, colAlertBg);
    d.setTextSize(2);
    d.setTextColor(TFT_WHITE);
    d.setCursor(8, 96);
    char buf[24];
    snprintf(buf, sizeof(buf), "RETURN %2ds", secondsRemaining);
    d.print(buf);
}

}  // namespace

void uiInit() {
    auto& d = M5Cardputer.Display;
    d.setRotation(1);
    d.setTextSize(1);

    colBg = d.color565(8, 8, 20);
    colDim = d.color565(20, 40, 50);
    colCyan = d.color565(0, 229, 255);
    colMagenta = d.color565(255, 46, 154);
    colAmber = d.color565(255, 196, 0);
    colAlertBg = d.color565(180, 0, 60);
    colSheepBody = d.color565(210, 230, 235);
    colSheepHead = d.color565(30, 34, 46);
    colEyeCalm = colCyan;
    colEyeAlert = d.color565(255, 40, 40);
    colHint = d.color565(110, 150, 160);

    d.fillScreen(colBg);
}

void uiShowIdle(uint8_t channel, uint32_t totalDetections, bool sdReady,
                bool audioAlertsEnabled) {
    auto& d = M5Cardputer.Display;
    d.fillScreen(colBg);
    drawScanlines(colDim);
    drawHudCorners(colCyan);

    drawGlowText(6, 4, "FLOCK-EWE", 2, colDim, colMagenta);

    char buf[24];
    d.setTextSize(2);

    d.setTextColor(colCyan);
    d.setCursor(8, 30);
    snprintf(buf, sizeof(buf), "CH  %02d", channel);
    d.print(buf);

    d.setCursor(8, 52);
    snprintf(buf, sizeof(buf), "DET %03lu", (unsigned long)totalDetections);
    d.print(buf);

    d.setTextColor(sdReady ? colCyan : colMagenta);
    d.setCursor(8, 74);
    d.print(sdReady ? "SD  OK" : "SD  FAIL");

    d.setTextColor(colAmber);
    d.setCursor(8, 96);
    d.print("SCANNING...");

    drawSheep(184, 58, colSheepBody, colSheepHead, colEyeCalm);

    d.setTextSize(1);
    d.setTextColor(colHint);
    d.setCursor(6, 120);
    snprintf(buf, sizeof(buf), "[M] MENU   AUDIO:%s",
             audioAlertsEnabled ? "ON" : "OFF");
    d.print(buf);
}

void uiShowAlert(const Detection& det) {
    auto& d = M5Cardputer.Display;
    d.fillScreen(colAlertBg);
    drawHudCorners(colMagenta);

    drawGlowText(6, 4, "!! FLOCK CAM !!", 2, TFT_BLACK, TFT_WHITE);

    char buf[24];
    d.setTextSize(2);
    d.setTextColor(TFT_WHITE);

    d.setCursor(8, 30);
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", det.mac[0],
             det.mac[1], det.mac[2], det.mac[3], det.mac[4], det.mac[5]);
    d.print(buf);

    d.setCursor(8, 52);
    snprintf(buf, sizeof(buf), "%d dBm  CH%d", det.rssi, det.channel);
    d.print(buf);

    d.setCursor(8, 74);
    snprintf(buf, sizeof(buf), "%s%s", det.ouiMatch ? "OUI " : "",
             det.ieMatch ? "IE" : "");
    d.print(buf);

    drawSheep(184, 58, TFT_WHITE, TFT_BLACK, colEyeAlert);

    drawCountdownRow(10);
}

void uiUpdateAlertCountdown(uint8_t secondsRemaining) {
    drawCountdownRow(secondsRemaining);
}

void uiShowMenu(bool audioAlertsEnabled) {
    auto& d = M5Cardputer.Display;
    d.fillScreen(colBg);
    drawHudCorners(colAmber);

    drawGlowText(6, 4, "SETTINGS", 2, colDim, colAmber);

    d.setTextSize(2);
    d.setTextColor(TFT_WHITE);
    d.setCursor(8, 34);
    d.print("AUDIO ALERTS");

    d.setTextColor(audioAlertsEnabled ? colCyan : colMagenta);
    d.setCursor(8, 58);
    d.print(audioAlertsEnabled ? "> ON <" : "> OFF <");

    drawSheep(184, 58, colSheepBody, colSheepHead, colEyeCalm);

    d.setTextSize(1);
    d.setTextColor(colHint);
    d.setCursor(6, 100);
    d.print("[ENTER] TOGGLE   [M] BACK");
}
