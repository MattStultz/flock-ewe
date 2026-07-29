// FlockSpoofer: injects fabricated 802.11 probe-request frames spoofing
// known Flock Safety hardware OUIs and the matching wildcard-probe IE
// fingerprint, so Flock-Ewe's detector can be tested without a real
// camera nearby. Runs on any generic ESP32 dev board — no display,
// keyboard, GPS, or SD card required.
//
// This transmits fabricated test frames on channels 1/6/11 in a tight
// loop. Use it only to test your own receiver at close range and short
// duration — it is not a general-purpose WiFi testing tool, and running
// it for extended periods or over a wide area serves no purpose beyond
// that testing.
//
// Board: any ESP32 ("ESP32 Dev Module" in Arduino IDE) — no special
// build flags needed.

#include <WiFi.h>
#include <esp_wifi.h>
#include <string.h>

namespace {

struct OuiEntry {
    uint8_t b0, b1, b2;
};

// Same confirmed Flock Safety OUI prefixes as Flock-Ewe's flock_detect.cpp.
const OuiEntry FLOCK_OUIS[] = {
    {0xB4, 0x1E, 0x52}, {0x70, 0xC9, 0x4E}, {0x3C, 0x91, 0x80},
    {0xD8, 0xF3, 0xBC}, {0x80, 0x30, 0x49}, {0xB8, 0x35, 0x32},
    {0x14, 0x5A, 0xFC}, {0x74, 0x4C, 0xA1}, {0x08, 0x3A, 0x88},
    {0x9C, 0x2F, 0x9D}, {0xC0, 0x35, 0x32}, {0x94, 0x08, 0x53},
    {0xE4, 0xAA, 0xEA}, {0xF4, 0x6A, 0xDD}, {0xF8, 0xA2, 0xD6},
    {0x24, 0xB2, 0xB9}, {0x00, 0xF4, 0x8D}, {0xD0, 0x39, 0x57},
    {0xE8, 0xD0, 0xFC}, {0xE0, 0x4F, 0x43}, {0xB8, 0x1E, 0xA4},
    {0x70, 0x08, 0x94}, {0x58, 0x8E, 0x81}, {0xEC, 0x1B, 0xBD},
    {0x3C, 0x71, 0xBF}, {0x58, 0x00, 0xE3}, {0x90, 0x35, 0xEA},
    {0x5C, 0x93, 0xA2}, {0x64, 0x6E, 0x69}, {0x48, 0x27, 0xEA},
    {0xA4, 0xCF, 0x12},
};
const size_t FLOCK_OUIS_COUNT = sizeof(FLOCK_OUIS) / sizeof(FLOCK_OUIS[0]);

// Same channels Flock-Ewe's scanner hops across (wifi_scan.cpp), so a
// short burst on each one lines up with the receiver's dwell there.
const uint8_t TEST_CHANNELS[] = {1, 6, 11};
const uint8_t TEST_CHANNEL_COUNT = sizeof(TEST_CHANNELS) / sizeof(TEST_CHANNELS[0]);
const uint8_t BURSTS_PER_CHANNEL = 3;
const uint32_t BURST_GAP_MS = 100;

uint8_t frame[64];

// Builds a wildcard probe request whose IE tag sequence matches the
// known Flock Safety fingerprint (see flock_detect.cpp's
// FLOCK_PROBE_IE_SIGNATURE: "2,12,127,221:506f9a16030103,45,191,
// 221:0050f208000000"). Tags 2/12/127/45/191 are zero-length since only
// their tag number is checked; the two vendor-specific (221) tags carry
// the exact payload bytes that make up the fingerprint.
size_t buildFlockProbeFrame(uint8_t* buf, const uint8_t srcMac[6]) {
    size_t i = 0;

    buf[i++] = 0x40;  // Frame Control: management, subtype = probe request
    buf[i++] = 0x00;
    buf[i++] = 0x00;  // Duration
    buf[i++] = 0x00;
    memset(buf + i, 0xFF, 6);  // Addr1: destination (broadcast)
    i += 6;
    memcpy(buf + i, srcMac, 6);  // Addr2: source (spoofed Flock OUI)
    i += 6;
    memset(buf + i, 0xFF, 6);  // Addr3: BSSID (broadcast, wildcard probe)
    i += 6;
    buf[i++] = 0x00;  // Seq-ctl (overwritten by the driver, see en_sys_seq below)
    buf[i++] = 0x00;

    buf[i++] = 0x00;  // SSID IE: wildcard (zero length)
    buf[i++] = 0x00;

    buf[i++] = 2;
    buf[i++] = 0;
    buf[i++] = 12;
    buf[i++] = 0;
    buf[i++] = 127;
    buf[i++] = 0;

    buf[i++] = 221;
    buf[i++] = 7;
    const uint8_t vendor1[] = {0x50, 0x6f, 0x9a, 0x16, 0x03, 0x01, 0x03};
    memcpy(buf + i, vendor1, 7);
    i += 7;

    buf[i++] = 45;
    buf[i++] = 0;
    buf[i++] = 191;
    buf[i++] = 0;

    buf[i++] = 221;
    buf[i++] = 7;
    const uint8_t vendor2[] = {0x00, 0x50, 0xf2, 0x08, 0x00, 0x00, 0x00};
    memcpy(buf + i, vendor2, 7);
    i += 7;

    return i;
}

void randomFlockMac(uint8_t out[6]) {
    const OuiEntry& oui = FLOCK_OUIS[random(FLOCK_OUIS_COUNT)];
    out[0] = oui.b0;
    out[1] = oui.b1;
    out[2] = oui.b2;
    out[3] = random(256);
    out[4] = random(256);
    out[5] = random(256);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("Flock-Ewe test spoofer starting...");

    WiFi.mode(WIFI_STA);
    randomSeed(esp_random());
}

void loop() {
    static uint8_t chIdx = 0;

    esp_wifi_set_channel(TEST_CHANNELS[chIdx], WIFI_SECOND_CHAN_NONE);

    for (uint8_t burst = 0; burst < BURSTS_PER_CHANNEL; burst++) {
        uint8_t mac[6];
        randomFlockMac(mac);
        size_t len = buildFlockProbeFrame(frame, mac);
        esp_err_t err = esp_wifi_80211_tx(WIFI_IF_STA, frame, len, true);

        Serial.printf("[CH %2d] probe from %02X:%02X:%02X:%02X:%02X:%02X (%s)\n",
                      TEST_CHANNELS[chIdx], mac[0], mac[1], mac[2], mac[3],
                      mac[4], mac[5], err == ESP_OK ? "sent" : "FAILED");
        delay(BURST_GAP_MS);
    }

    chIdx = (chIdx + 1) % TEST_CHANNEL_COUNT;
}
