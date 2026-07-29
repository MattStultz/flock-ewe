#pragma once

#include "wifi_scan.h"

// Mounts the SD card and opens a new session log file. Returns true on
// success; logging calls are safely skipped if this returns false.
bool loggerInit();

// Appends one JSON-lines record for a detection. No-op if loggerInit() failed.
void loggerLogDetection(const Detection& detection);
