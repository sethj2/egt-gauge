# EGT / CLT gauge

Multifunction gauge for the Waveshare ESP32-S3-Touch-LCD-1.85 (360x360 round display).
Currently runs on simulated sensor data (see `src/sensors.h`).

## Build setup
1. Download Waveshare's demo package from the [wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.85)
   and extract it into this folder as `ESP32-S3-Touch-LCD-1.85-Demo/` (it is gitignored).
2. Copy `config/lv_conf.h` over `ESP32-S3-Touch-LCD-1.85-Demo/Arduino/libraries/lvgl/src/lv_conf.h`
   (enables the 24/40 pt fonts and a 16 ms refresh).
3. Copy `src/secrets.example.h` to `src/secrets.h` and enter your phone hotspot name and password.
4. Install the `esp32` Arduino core 3.0.7 and `arduino-cli`.

## Wireless updates
Long-press the screen, then tap "Check for update". The gauge joins the hotspot from `secrets.h`,
reads `version.txt` from the latest GitHub release, and flashes `firmware.bin` if the version differs.
Publish a new version with `.\release.ps1 -Version 0.2.0`.
