#include "battery.h"

#include <M5Unified.h>

namespace {

const uint32_t SAMPLE_INTERVAL_MS = 200;
const uint8_t SAMPLE_COUNT = 15;  // ~3 seconds of samples at the interval above

// Same 3300-4150mV linear map M5Unified/Launcher both use for this board.
const float MIN_MV = 3300.0f;
const float MAX_MV = 4150.0f;

uint32_t samples[SAMPLE_COUNT] = {0};
uint8_t sampleIdx = 0;
uint8_t samplesFilled = 0;
uint32_t lastSampleMs = 0;

}  // namespace

void batteryLoop() {
    uint32_t now = millis();
    if (samplesFilled != 0 && now - lastSampleMs < SAMPLE_INTERVAL_MS) return;
    lastSampleMs = now;

    samples[sampleIdx] = M5.Power.getBatteryVoltage();
    sampleIdx = (sampleIdx + 1) % SAMPLE_COUNT;
    if (samplesFilled < SAMPLE_COUNT) samplesFilled++;
}

int8_t batteryGetLevel() {
    if (samplesFilled == 0) return M5.Power.getBatteryLevel();

    uint32_t sum = 0;
    for (uint8_t i = 0; i < samplesFilled; i++) sum += samples[i];
    float avgMv = static_cast<float>(sum) / samplesFilled;

    float level = (avgMv - MIN_MV) * 100.0f / (MAX_MV - (MIN_MV + 50.0f));
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    return static_cast<int8_t>(level);
}
