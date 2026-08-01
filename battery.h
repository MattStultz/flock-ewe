#pragma once

#include <cstdint>

// Smooths out M5Unified's battery reading for this board.
//
// Confirmed by reading both Launcher's (github.com/bmorcelli/Launcher)
// and M5Unified's source: they use the *identical* method for the
// CardputerADV — GPIO10, a 2.0x divider ratio, and the same linear
// 3300-4150mV map. So a single instantaneous reading disagreeing with
// Launcher isn't a formula bug; it's almost certainly real voltage sag
// under load. Flock-Ewe is nearly always mid-scan (WiFi promiscuous mode,
// channel-hopping every 350ms), unlike Launcher's idle menu, and that
// current draw can sag the battery rail enough to read meaningfully lower
// at the exact instant it's sampled. Averaging over a few seconds smooths
// out that kind of transient dip — it won't fully erase a genuine
// sustained sag while actively scanning, but it stops a single unlucky
// sample from being reported as-is.
void batteryLoop();

// Rolling-average battery percentage, using the same linear voltage
// formula as above. Falls back to a raw instantaneous reading until the
// first few samples have accumulated after boot.
int8_t batteryGetLevel();
