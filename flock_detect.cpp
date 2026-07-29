#include "flock_detect.h"

#include <Arduino.h>

namespace {

struct OuiEntry {
    uint8_t b0, b1, b2;
};

// Seed list only. Cross-reference https://standards-oui.ieee.org/ (or your
// own captured data) and add entries here as you confirm them.
const OuiEntry KNOWN_OUIS[] = {
    {0x70, 0xC9, 0x4E},
};
const size_t KNOWN_OUIS_COUNT = sizeof(KNOWN_OUIS) / sizeof(KNOWN_OUIS[0]);

// Known wildcard-probe IE tag signature: comma-separated tag numbers, with
// vendor-specific tags (221) additionally appending their payload bytes as
// hex after a colon.
const char* FLOCK_PROBE_IE_SIGNATURE =
    "2,12,127,221:506f9a16030103,45,191,221:0050f208000000";

}  // namespace

bool matchesKnownOui(const uint8_t mac[6]) {
    // Locally administered (randomized) addresses aren't real vendor
    // hardware OUIs, so they can never match this table.
    if (mac[0] & 0x02) return false;

    for (size_t i = 0; i < KNOWN_OUIS_COUNT; i++) {
        if (mac[0] == KNOWN_OUIS[i].b0 && mac[1] == KNOWN_OUIS[i].b1 &&
            mac[2] == KNOWN_OUIS[i].b2) {
            return true;
        }
    }
    return false;
}

bool matchesFlockProbeSignature(const uint8_t* body, uint16_t len) {
    if (len < 2) return false;

    // First IE must be SSID (tag 0) with zero length (wildcard probe).
    if (body[0] != 0x00 || body[1] != 0x00) return false;

    String sig;
    sig.reserve(64);
    uint16_t i = 2;  // skip the zero-length SSID IE
    bool first = true;

    while (i + 2 <= len) {
        uint8_t tag = body[i];
        uint8_t tagLen = body[i + 1];
        uint16_t valStart = i + 2;
        if (valStart + tagLen > len) break;  // truncated/malformed IE

        if (!first) sig += ',';
        first = false;
        sig += tag;

        if (tag == 221) {
            sig += ':';
            uint8_t n = tagLen > 32 ? 32 : tagLen;  // safety cap
            char byteHex[3];
            for (uint8_t b = 0; b < n; b++) {
                snprintf(byteHex, sizeof(byteHex), "%02x", body[valStart + b]);
                sig += byteHex;
            }
        }

        i = valStart + tagLen;
    }

    return sig.equals(FLOCK_PROBE_IE_SIGNATURE);
}
