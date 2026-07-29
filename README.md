# Flock-Ewe

A passive, receive-only WiFi scanner for the M5Stack CardputerADV that
detects nearby Flock Safety ALPR (automated license plate reader) camera
units and alerts you on-device. It never transmits, associates, or
interferes with any network — it only listens.

Loosely inspired by [Flock-You](https://github.com/colonelpanichacks/flock-you)
(ESP32-S3 / XIAO), which pioneered the detection approach used here. That
repo carries no license, so this project reimplements the detection
algorithm independently rather than copying its code — see
[Detection approach](#detection-approach) below.

**Disclaimer:** this is a research/privacy-auditing tool. It only receives
public over-the-air broadcast frames; you are responsible for complying
with your local laws regarding RF reception and monitoring.

## Hardware

- M5Stack CardputerADV (ESP32-S3, ST7789V2 display, TCA8418 keyboard,
  ES8311 speaker codec, microSD slot)
- [M5Stack Cap LoRa-1262](https://docs.m5stack.com/en/cap/Cap_LoRa-1262)
  expansion module — used here only for its onboard GNSS chip (ATGM336H),
  read over UART (NMEA) on pins RX=13/TX=15. The module's SX1262 LoRa
  radio isn't used by this project yet.

## Detection approach

Two independent signals, either of which triggers an alert:

1. **OUI matching** — the transmitting MAC's vendor prefix is checked
   against a table of 30 confirmed Flock Safety hardware prefixes in
   [flock_detect.cpp](flock_detect.cpp). MAC OUI-to-vendor assignments are
   public IEEE registry data (https://standards-oui.ieee.org/) — add more
   entries there as you confirm additional hardware.
2. **Probe IE fingerprint** — Flock Safety units emit a distinctive
   wildcard probe request (empty SSID) with a specific sequence of
   Information Element tags. `matchesFlockProbeSignature()` parses the
   IEs from a captured probe request and compares the tag signature
   against the known fingerprint.

Matches are deduplicated per-MAC with a 5-second cooldown so a single
camera doesn't spam repeat alerts.

## On-device UI

The screen has three states: an idle/scanning screen, a full-screen alert,
and a settings menu.

- Press **M** on the idle screen to open **Settings**, where **Enter**
  toggles audio alerts on/off; press **M** again to go back. The idle
  screen always shows a `[M] MENU` / `AUDIO:ON` hint at the bottom.
- On detection, the screen flashes to the alert view and holds for
  **10 seconds**, beeping once a second (if audio alerts are on) with a
  live countdown to the idle screen. A new detection while the alert is
  already showing resets the 10-second hold and refreshes the displayed
  MAC/RSSI/match info rather than queuing behind it.
- The menu isn't reachable while an alert is showing — a real detection
  always takes priority.

## Firmware setup (Arduino IDE)

1. In **Boards Manager**, install `esp32` by Espressif Systems.
2. Select **Tools > Board > ESP32S3 Dev Module** (there's no dedicated
   Cardputer board entry; the M5Cardputer library handles the rest).
3. Under **Tools**, set:
   - USB CDC On Boot: **Enabled**
   - Partition Scheme: default (8MB) or larger, since PSRAM/flash usage
     is modest but SD/SPIFFS headroom helps
4. In **Library Manager**, install `M5Cardputer` (this pulls in
   `M5Unified`, `M5GFX`, and `IRremote` as dependencies) and `TinyGPSPlus`.
5. Open `FlockEwe.ino` and upload.

## Files

- `FlockEwe.ino` — setup()/loop(), ties everything together
- `wifi_scan.{h,cpp}` — promiscuous-mode capture, channel hopping,
  per-MAC cooldown, hands detections to the main loop via a FreeRTOS queue
- `flock_detect.{h,cpp}` — OUI table + probe IE signature matching
- `ui.{h,cpp}` — on-device display (idle screen, alert screen)
- `gps.{h,cpp}` — reads NMEA position fixes from the Cap LoRa-1262's GNSS chip
- `logger.{h,cpp}` — SD card JSON-lines logging of detections + GPS fix

## Logging

Detections are appended as JSON-lines to a session file on the SD card,
named `flockeweMMDDYYHHmm.txt` from the GPS's UTC date/time (e.g.
`flockewe072926143022.txt`). The file is created lazily — on the *first*
detection of the session, not at boot — so a session with no detections
never creates an empty file, and each run gets its own file. If no GPS
fix/time has been acquired yet at that first detection, it falls back to
`flockewe_nofix_<millis>.txt`.

The file is opened, appended to, and closed again on every single write
(rather than held open for the whole session) so a sudden power loss
can't leave it locked or corrupted. `lat`/`lng`/`alt_m`/`sats` are only
present when a GPS fix was available at detection time:

```json
{"t":12345,"mac":"70:C9:4E:AA:BB:CC","rssi":-62,"ch":6,"oui":true,"ie":false,"gps_fix":true,"lat":40.712800,"lng":-74.006000,"alt_m":10.5,"sats":8}
```

Note: the filename's date/time is UTC (as broadcast by GPS satellites),
not local time.

## Using with Launcher

Flock-Ewe is a plain standalone Arduino sketch, which is exactly what
[Launcher](https://github.com/bmorcelli/Launcher) expects — no manifest
or special entry point required. To install it:

1. `Sketch > Export Compiled Binary` in Arduino IDE.
2. Install the resulting `FlockEwe.ino.bin` through Launcher (SD, WebUI,
   or OTA, per Launcher's own docs).

There's currently no in-app "return to Launcher" shortcut — use the
physical reset button (or enable Launcher's "Boot to Launcher" option so
it always stops at the menu instead of auto-booting the last app).

## Roadmap / ideas

- Keep expanding the OUI table as more Flock Safety hardware is confirmed
- On-screen detection history / log viewer
- Return-to-Launcher shortcut
- Persist the audio-alerts setting across reboots (currently resets to on)
