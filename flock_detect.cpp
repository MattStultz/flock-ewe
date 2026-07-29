#include "flock_detect.h"

#include <Arduino.h>

namespace {

struct OuiEntry {
    uint8_t b0, b1, b2;
};

// Verified Flock Safety hardware OUI prefixes. Cross-reference
// https://standards-oui.ieee.org/ (or your own captured data) if you find
// more to add.
const OuiEntry KNOWN_OUIS[] = {
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
