#include "wifi_scan.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

#include "flock_detect.h"

namespace {

struct IEEE80211MacHdr {
    uint16_t frameControl;
    uint16_t durationId;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t seqCtl;
} __attribute__((packed));

const int8_t RSSI_MIN = -95;
const uint32_t CHANNEL_DWELL_MS = 350;
const uint32_t COOLDOWN_MS = 5000;
const uint8_t COOLDOWN_SLOTS = 8;
const uint8_t CHANNELS[] = {11, 6, 1};
const uint8_t CHANNEL_COUNT = sizeof(CHANNELS) / sizeof(CHANNELS[0]);

QueueHandle_t detectionQueue = nullptr;
uint8_t channelIdx = 0;

struct CooldownEntry {
    uint8_t mac[6];
    uint32_t lastSeenMs;
    bool used;
};
CooldownEntry cooldownTable[COOLDOWN_SLOTS];

bool onCooldown(const uint8_t mac[6], uint32_t now) {
    for (uint8_t i = 0; i < COOLDOWN_SLOTS; i++) {
        if (cooldownTable[i].used && memcmp(cooldownTable[i].mac, mac, 6) == 0) {
            if (now - cooldownTable[i].lastSeenMs < COOLDOWN_MS) return true;
            cooldownTable[i].lastSeenMs = now;
            return false;
        }
    }
    return false;
}

void markCooldown(const uint8_t mac[6], uint32_t now) {
    // Reuse an existing slot for this MAC, otherwise the oldest slot.
    uint8_t oldestIdx = 0;
    uint32_t oldestTime = UINT32_MAX;
    for (uint8_t i = 0; i < COOLDOWN_SLOTS; i++) {
        if (cooldownTable[i].used && memcmp(cooldownTable[i].mac, mac, 6) == 0) {
            cooldownTable[i].lastSeenMs = now;
            return;
        }
        if (!cooldownTable[i].used) {
            oldestIdx = i;
            oldestTime = 0;
        } else if (cooldownTable[i].lastSeenMs < oldestTime) {
            oldestTime = cooldownTable[i].lastSeenMs;
            oldestIdx = i;
        }
    }
    memcpy(cooldownTable[oldestIdx].mac, mac, 6);
    cooldownTable[oldestIdx].lastSeenMs = now;
    cooldownTable[oldestIdx].used = true;
}

void IRAM_ATTR wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;

    auto* pkt = reinterpret_cast<wifi_promiscuous_pkt_t*>(buf);
    const uint8_t* payload = pkt->payload;
    uint16_t len = pkt->rx_ctrl.sig_len;
    if (len < sizeof(IEEE80211MacHdr)) return;

    const auto* hdr = reinterpret_cast<const IEEE80211MacHdr*>(payload);
    uint8_t frameType = (hdr->frameControl >> 2) & 0x3;
    uint8_t frameSubtype = (hdr->frameControl >> 4) & 0xF;
    if (frameType != 0 || frameSubtype != 4) return;  // probe requests only

    if (pkt->rx_ctrl.rssi < RSSI_MIN) return;

    const uint8_t* mac = hdr->addr2;
    const uint8_t* body = payload + sizeof(IEEE80211MacHdr);
    uint16_t bodyLen = len - sizeof(IEEE80211MacHdr);

    bool ouiMatch = matchesKnownOui(mac);
    bool ieMatch = matchesFlockProbeSignature(body, bodyLen);
    if (!ouiMatch && !ieMatch) return;

    uint32_t now = millis();
    if (onCooldown(mac, now)) return;
    markCooldown(mac, now);

    Detection d{};
    memcpy(d.mac, mac, 6);
    d.rssi = pkt->rx_ctrl.rssi;
    d.channel = pkt->rx_ctrl.channel;
    d.ouiMatch = ouiMatch;
    d.ieMatch = ieMatch;
    d.timestampMs = now;

    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(detectionQueue, &d, &woken);
    if (woken) portYIELD_FROM_ISR();
}

}  // namespace

void wifiScanInit() {
    detectionQueue = xQueueCreate(16, sizeof(Detection));

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifiSnifferCallback);
    esp_wifi_set_channel(CHANNELS[channelIdx], WIFI_SECOND_CHAN_NONE);
}

void wifiScanLoop() {
    static uint32_t lastHop = 0;
    uint32_t now = millis();
    if (now - lastHop >= CHANNEL_DWELL_MS) {
        channelIdx = (channelIdx + 1) % CHANNEL_COUNT;
        esp_wifi_set_channel(CHANNELS[channelIdx], WIFI_SECOND_CHAN_NONE);
        lastHop = now;
    }
}

bool wifiScanPopDetection(Detection& out) {
    return xQueueReceive(detectionQueue, &out, 0) == pdTRUE;
}

uint8_t wifiScanCurrentChannel() {
    return CHANNELS[channelIdx];
}
