#pragma once

#include "gps.h"
#include "wifi_scan.h"

void uiInit();
void uiShowIdle(uint8_t channel, uint32_t totalDetections, bool sdReady,
                bool audioAlertsEnabled, bool gpsLocked);
void uiShowAlert(const Detection& detection, bool gpsLocked);

// Redraws just the alert screen's countdown row (call once per second while
// the alert is held, without redrawing the rest of the screen).
void uiUpdateAlertCountdown(uint8_t secondsRemaining);

void uiShowMenu(bool audioAlertsEnabled, bool gpsLocked);

void uiShowGpsStatus(const GpsStatus& status);
