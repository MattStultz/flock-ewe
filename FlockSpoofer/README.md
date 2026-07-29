# FlockSpoofer

A test transmitter for Flock-Ewe. It injects fabricated 802.11 probe
requests that spoof known Flock Safety hardware OUIs and carry the same
wildcard-probe IE fingerprint Flock-Ewe looks for, so you can verify the
whole detection pipeline — OUI match, IE match, alert screen, beep,
SD logging — without needing a real camera nearby.

Runs on any generic ESP32 dev board. No display, keyboard, GPS, or SD
card required — just Serial output.

**This transmits fabricated frames over the air.** Use it only for
short-range, short-duration testing of your own Flock-Ewe device — not
as a general-purpose WiFi tool, and not run continuously or over a wide
area.

## How it works

- Cycles through the same 30 confirmed Flock Safety OUIs as
  [flock_detect.cpp](../flock_detect.cpp), picking one at random (with a
  random suffix) for each frame.
- Builds a wildcard probe request (empty SSID) with the exact IE tag
  sequence Flock-Ewe's `matchesFlockProbeSignature()` checks for, so
  both the OUI and IE detection paths get exercised.
- Sends a short burst of frames on channels 1, 6, and 11 in turn — the
  same channels Flock-Ewe's scanner hops across — so the two devices are
  likely to land on the same channel within a few seconds even though
  they're not synchronized.

## Setup (Arduino IDE)

1. Board: **Tools > Board > ESP32 Dev Module** (or any ESP32/S2/S3/C3
   board — no special build flags needed).
2. No extra libraries beyond the `esp32` board package itself
   (`WiFi.h`/`esp_wifi.h` are bundled with it).
3. Open `FlockSpoofer.ino`, upload, and open the Serial Monitor at
   115200 baud to watch it transmit.

## Using it

Power on both this device and Flock-Ewe near each other. Watch
Flock-Ewe's screen — it should alert within a few seconds, with `MATCH
OUI IE` on the alert screen (both signals should trigger, since the
spoofed frames carry both).
