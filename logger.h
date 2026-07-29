#pragma once

#include "gps.h"
#include "wifi_scan.h"

// Mounts the SD card. Returns true on success; logging calls are safely
// skipped if this returns false. The session log file itself isn't created
// yet — that happens lazily on the first detection, see loggerLogDetection().
bool loggerInit();

// Appends one JSON-lines record for a detection, tagged with the GPS fix
// active at detection time (if any). No-op if loggerInit() failed.
void loggerLogDetection(const Detection& detection, const GpsFix& fix);
