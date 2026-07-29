#pragma once

#include <cstdint>

struct Detection {
    uint8_t mac[6];
    int8_t rssi;
    uint8_t channel;
    bool ouiMatch;
    bool ieMatch;
    uint32_t timestampMs;
};

// Puts the radio into promiscuous mode and starts channel hopping.
void wifiScanInit();

// Call every loop() iteration; advances the channel-hop timer.
void wifiScanLoop();

// Non-blocking. Returns true and fills `out` if a detection was queued.
bool wifiScanPopDetection(Detection& out);

// Channel currently being listened on.
uint8_t wifiScanCurrentChannel();
