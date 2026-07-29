#pragma once

#include "wifi_scan.h"

void uiInit();
void uiShowIdle(uint8_t channel, uint32_t totalDetections, bool sdReady);
void uiShowAlert(const Detection& detection);
