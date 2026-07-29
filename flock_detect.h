#pragma once

#include <cstdint>

// Independent reimplementation of the detection approach used by the
// Flock-You project (https://github.com/colonelpanichacks/flock-you):
// match a probe-request transmitter's MAC OUI against known hardware
// vendor prefixes, and/or match the probe's Information Element (IE) tag
// signature against a known Flock Safety device fingerprint.
//
// Returns true if the given transmitter MAC matches a known Flock Safety
// hardware OUI prefix. This is a small seed list — MAC OUI-to-vendor
// assignments are public IEEE registry data (https://standards-oui.ieee.org/);
// expand this table with your own verified research before relying on it.
bool matchesKnownOui(const uint8_t mac[6]);

// Parses the IE (tagged parameter) region of an 802.11 probe request frame
// body and checks it against the known Flock Safety wildcard-probe
// fingerprint (tag sequence + vendor-specific payload bytes).
bool matchesFlockProbeSignature(const uint8_t* frameBody, uint16_t bodyLen);
