#include "ui.h"

#include <M5Cardputer.h>
#include <M5GFX.h>

namespace {

// Everything is drawn into this off-screen sprite and pushed to the panel
// in one shot at the end of each screen function. Drawing straight to the
// live display primitive-by-primitive is what caused the visible flicker
// on every redraw (e.g. the idle screen's 500ms channel-hop refresh).
M5Canvas canvas(&M5Cardputer.Display);

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
    const int len = 10;
    canvas.drawFastHLine(0, 0, len, color);
    canvas.drawFastVLine(0, 0, len, color);
    canvas.drawFastHLine(240 - len, 0, len, color);
    canvas.drawFastVLine(239, 0, len, color);
    canvas.drawFastHLine(0, 134, len, color);
    canvas.drawFastVLine(0, 134 - len, len, color);
    canvas.drawFastHLine(240 - len, 134, len, color);
    canvas.drawFastVLine(239, 134 - len, len, color);
}

void drawScanlines(uint16_t color) {
    for (int y = 0; y < 135; y += 4) {
        canvas.drawFastHLine(0, y, 240, color);
    }
}

// Faux-neon-glow text: a dim copy one pixel down-right, then the bright
// copy on top.
void drawGlowText(int x, int y, const char* text, uint8_t size, uint16_t glow,
                   uint16_t bright) {
    canvas.setTextSize(size);
    canvas.setTextColor(glow);
    canvas.setCursor(x + 1, y + 1);
    canvas.print(text);
    canvas.setTextColor(bright);
    canvas.setCursor(x, y);
    canvas.print(text);
}

// "CyberEwe" mascot: a fluffy blocky body, one ear, four stub legs, and a
// glowing sensor eye. Built from plain shape primitives rather than a
// bitmap so it's easy to verify without a device on hand.
void drawSheep(int x, int y, uint16_t bodyColor, uint16_t headColor,
               uint16_t eyeColor) {
    canvas.fillRoundRect(x + 9, y, 30, 18, 6, bodyColor);
    canvas.fillCircle(x + 9, y + 6, 6, bodyColor);
    canvas.fillCircle(x + 18, y - 2, 6, bodyColor);
    canvas.fillCircle(x + 30, y - 2, 6, bodyColor);
    canvas.fillCircle(x + 39, y + 6, 6, bodyColor);

    canvas.fillRect(x + 12, y + 17, 3, 8, headColor);
    canvas.fillRect(x + 21, y + 17, 3, 8, headColor);
    canvas.fillRect(x + 30, y + 17, 3, 8, headColor);
    canvas.fillRect(x + 38, y + 17, 3, 8, headColor);

    canvas.fillRoundRect(x, y + 3, 14, 12, 3, headColor);
    canvas.fillTriangle(x + 2, y + 3, x + 6, y - 3, x + 9, y + 3, headColor);

    canvas.fillCircle(x + 5, y + 9, 2, eyeColor);
}

// Small satellite glyph tucked next to the sheep only while the GPS has
// a fix — it's simply omitted otherwise, rather than shown in a dimmer
// color (grey vs. cyan was too hard to tell apart on this small screen).
void drawSatellite(int x, int y, uint16_t color) {
    canvas.fillRect(x, y + 5, 8, 4, color);
    canvas.fillRect(x + 17, y + 5, 8, 4, color);
    canvas.fillRect(x + 9, y + 4, 6, 6, color);
    canvas.fillRect(x + 11, y, 2, 4, color);
    canvas.fillCircle(x + 12, y - 1, 1, color);
}

// Battery icon + percentage in the top-right corner. Outline/fill/text
// all share one status color: cyan (fine), amber (getting low), magenta
// (critical), or hint-grey with "--" if the level can't be read.
void drawBattery(int x, int y, int8_t levelPercent) {
    bool known = levelPercent >= 0;
    uint16_t color;
    if (!known) {
        color = colHint;
    } else if (levelPercent > 50) {
        color = colCyan;
    } else if (levelPercent > 20) {
        color = colAmber;
    } else {
        color = colMagenta;
    }

    canvas.drawRect(x, y, 18, 9, color);
    canvas.fillRect(x + 18, y + 2, 2, 5, color);
    if (known) {
        int fillWidth = (levelPercent * 14) / 100;
        if (fillWidth > 0) canvas.fillRect(x + 2, y + 2, fillWidth, 5, color);
    }

    canvas.setTextSize(1);
    canvas.setTextColor(color);
    canvas.setCursor(x + 23, y);
    if (known) {
        char buf[6];
        snprintf(buf, sizeof(buf), "%d%%", levelPercent);
        canvas.print(buf);
    } else {
        canvas.print("--");
    }
}

// Redraws only the alert screen's bottom row (used both for the initial
// draw and for the once-a-second countdown tick), then pushes the frame.
void drawCountdownRow(int secondsRemaining) {
    canvas.fillRect(0, 96, 230, 20, colAlertBg);
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(8, 96);
    char buf[24];
    snprintf(buf, sizeof(buf), "RETURN %2ds", secondsRemaining);
    canvas.print(buf);
    canvas.pushSprite(0, 0);
}

}  // namespace

void uiInit() {
    auto& d = M5Cardputer.Display;
    d.setRotation(1);

    canvas.setColorDepth(16);
    canvas.createSprite(240, 135);
    canvas.setTextSize(1);

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

    canvas.fillSprite(colBg);
    canvas.pushSprite(0, 0);
}

void uiShowIdle(uint8_t channel, uint32_t totalDetections, bool sdReady,
                bool audioAlertsEnabled, bool gpsLocked) {
    canvas.fillSprite(colBg);
    drawScanlines(colDim);
    drawHudCorners(colCyan);
    drawBattery(186, 8, M5.Power.getBatteryLevel());

    drawGlowText(6, 4, "FLOCK-EWE", 2, colDim, colMagenta);

    char buf[24];
    canvas.setTextSize(2);

    canvas.setTextColor(colCyan);
    canvas.setCursor(8, 30);
    snprintf(buf, sizeof(buf), "CH  %02d", channel);
    canvas.print(buf);

    canvas.setCursor(8, 52);
    snprintf(buf, sizeof(buf), "DET %03lu", (unsigned long)totalDetections);
    canvas.print(buf);

    canvas.setTextColor(sdReady ? colCyan : colMagenta);
    canvas.setCursor(8, 74);
    canvas.print(sdReady ? "SD  OK" : "SD  FAIL");

    canvas.setTextColor(colAmber);
    canvas.setCursor(8, 96);
    canvas.print("SCANNING...");

    if (gpsLocked) drawSatellite(155, 50, colCyan);
    drawSheep(184, 70, colSheepBody, colSheepHead, colEyeCalm);

    canvas.setTextSize(1);
    canvas.setTextColor(colHint);
    canvas.setCursor(6, 120);
    snprintf(buf, sizeof(buf), "[M] MENU   AUDIO:%s",
             audioAlertsEnabled ? "ON" : "OFF");
    canvas.print(buf);

    canvas.pushSprite(0, 0);
}

void uiShowAlert(const Detection& det, bool gpsLocked) {
    canvas.fillSprite(colAlertBg);
    drawHudCorners(colMagenta);
    drawBattery(186, 8, M5.Power.getBatteryLevel());

    drawGlowText(6, 4, "!! FLOCK CAM !!", 2, TFT_BLACK, TFT_WHITE);

    char buf[24];
    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);

    canvas.setCursor(8, 30);
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", det.mac[0],
             det.mac[1], det.mac[2], det.mac[3], det.mac[4], det.mac[5]);
    canvas.print(buf);

    canvas.setCursor(8, 52);
    snprintf(buf, sizeof(buf), "%d dBm  CH%d", det.rssi, det.channel);
    canvas.print(buf);

    canvas.setCursor(8, 74);
    snprintf(buf, sizeof(buf), "%s%s", det.ouiMatch ? "OUI " : "",
             det.ieMatch ? "IE" : "");
    canvas.print(buf);

    if (gpsLocked) drawSatellite(155, 50, colCyan);
    drawSheep(184, 70, TFT_WHITE, TFT_BLACK, colEyeAlert);

    drawCountdownRow(10);  // also pushes the finished frame
}

void uiUpdateAlertCountdown(uint8_t secondsRemaining) {
    drawCountdownRow(secondsRemaining);
}

void uiShowGpsStatus(const GpsStatus& status) {
    canvas.fillSprite(colBg);
    drawHudCorners(colCyan);
    drawBattery(186, 8, M5.Power.getBatteryLevel());

    drawGlowText(6, 4, "GPS STATUS", 2, colDim, colCyan);
    if (status.fix.valid) drawSatellite(155, 50, colCyan);

    char buf[24];
    canvas.setTextSize(2);

    canvas.setTextColor(status.communicating ? colCyan : colMagenta);
    canvas.setCursor(8, 24);
    canvas.print(status.communicating ? "LINK OK" : "LINK NONE");

    canvas.setTextColor(colCyan);
    canvas.setCursor(8, 42);
    snprintf(buf, sizeof(buf), "SATS %02d", status.satellites);
    canvas.print(buf);

    canvas.setTextColor(status.fix.valid ? TFT_WHITE : colHint);
    canvas.setCursor(8, 60);
    if (status.fix.valid) {
        snprintf(buf, sizeof(buf), "LAT %.6f", status.fix.lat);
    } else {
        snprintf(buf, sizeof(buf), "LAT  --");
    }
    canvas.print(buf);

    canvas.setCursor(8, 78);
    if (status.fix.valid) {
        snprintf(buf, sizeof(buf), "LNG %.6f", status.fix.lng);
    } else {
        snprintf(buf, sizeof(buf), "LNG  --");
    }
    canvas.print(buf);

    canvas.setCursor(8, 96);
    if (status.fix.valid) {
        snprintf(buf, sizeof(buf), "ALT %.1fM", status.fix.altitudeM);
    } else {
        snprintf(buf, sizeof(buf), "ALT  --");
    }
    canvas.print(buf);

    canvas.setTextSize(1);
    canvas.setTextColor(colHint);
    canvas.setCursor(6, 120);
    canvas.print("[G] BACK");

    canvas.pushSprite(0, 0);
}

void uiShowMenu(bool audioAlertsEnabled, bool gpsLocked) {
    canvas.fillSprite(colBg);
    drawHudCorners(colAmber);
    drawBattery(186, 8, M5.Power.getBatteryLevel());

    drawGlowText(6, 4, "SETTINGS", 2, colDim, colAmber);

    canvas.setTextSize(2);
    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(8, 34);
    canvas.print("AUDIO ALERTS");

    canvas.setTextColor(audioAlertsEnabled ? colCyan : colMagenta);
    canvas.setCursor(8, 58);
    canvas.print(audioAlertsEnabled ? "> ON <" : "> OFF <");

    if (gpsLocked) drawSatellite(155, 50, colCyan);
    drawSheep(184, 70, colSheepBody, colSheepHead, colEyeCalm);

    canvas.setTextSize(1);
    canvas.setTextColor(colHint);
    canvas.setCursor(6, 100);
    canvas.print("[ENTER] TOGGLE   [M] BACK");

    canvas.pushSprite(0, 0);
}
