# EGT / CLT gauge (ESP32-S3 round display)

Automotive multifunction gauge on a Waveshare ESP32-S3-Touch-LCD-1.85 (360x360 round, ST77916 QSPI,
CST816 touch, 16 MB flash, PSRAM). Bench prototype; **all sensor data is currently simulated**.
Arduino framework + LVGL 8.3.10. Windows 11, PowerShell, `arduino-cli` and `gh` installed via scoop.

## Current status
- [x] Display + touch running (drivers ported from Waveshare's demo)
- [x] UI: top ring = EGT (400-1300 F), bottom ring = CLT (-10-240 F), needles, live numbers in centre.
      Ring is blue, turns red above 1200 F (EGT) / 220 F (CLT). Centre number also turns red there.
- [x] Faster refresh: dirty-area redraw (`full_refresh = 0`), 16 ms LVGL refresh, 30 Hz updates
- [x] Wireless updates pulled from GitHub Releases (long-press screen -> "Check for update")
- [x] GitHub repo: https://github.com/sethj2/egt-gauge (public, `gh` logged in as sethj2)
- [x] OTA-capable firmware flashed over USB (Wi-Fi creds from `src/secrets.h` are stored in the board's NVS)
- [x] Releases published (v0.1.0, v0.1.1) and the Wi-Fi update path tested end to end on the board: works.
      Publish new ones with `.\release.ps1 -Version X.Y.Z` (version is stamped into `src/version.h`).
- [ ] Real EGT: Adafruit MAX31850K (**1-Wire**, not I2C) + K-type thermocouple on GPIO43/44, 4.7k pull-up to 3.3V,
      OneWire + DallasTemperature libs. Implement as a `Sensors` subclass in `src/sensors.h`.
- [ ] Real coolant (NTC thermistor, needs part number for the curve) and boost (0.5-4.5 V sensor,
      needs a divider or an ADS1115). Boost is simulated but not drawn on screen right now.
- [ ] Possible later: blue looks cyan on the panel (try 0x1F5FFF), thicker needles, FPS counter
      (`LV_USE_PERF_MONITOR` in lv_conf.h), alerts / peak hold, automotive power (12 V -> 5 V buck, TVS, reverse protection)

## Layout
- `egt_gauge.ino`: setup/loop. Loop samples at ~30 Hz, runs LVGL, polls the update UI.
- `src/sensors.h`: `Sensors` interface + `SimulatedSensors`. Units: F and psi. Swap in real sensors here.
- `src/gauge_ui.cpp`: LVGL gauge drawing. Zones/limits/ranges are the `*_ZONES` tables and `Gauge` initialisers at the top.
- `src/update_ui.cpp`, `src/ota.cpp`: update panel and GitHub OTA. `src/version.h` holds `FW_VERSION` and `GITHUB_REPO`.
- `src/board.cpp`: init order (I2C -> TCA9554 -> backlight -> LCD+touch -> LVGL). Copy of Waveshare's drivers in `src/drivers/`.
- `src/secrets.h`: phone hotspot SSID/password. **Gitignored**; template is `src/secrets.example.h`. Never edit it on
  github.com and never print its values (an earlier mistake leaked a hotspot password; history was reset and the password changed).
  Dev/USB builds copy it into NVS; `release.ps1` builds without it (via a temporary `src/release_build.h`) and refuses to
  publish if the SSID/password text is found in `firmware.bin`.
- `config/lv_conf.h`: LVGL config (24/40 pt fonts, 16 ms refresh). Must be copied into the demo's lvgl library, see README.
- `ESP32-S3-Touch-LCD-1.85-Demo/`: Waveshare demo package (gitignored, third-party). The build uses its `lvgl` copy.
- `release.ps1 -Version X.Y.Z`: builds, commits the version bump, pushes, creates the GitHub release.

## Build and flash
FQBN: `esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc` (core 3.0.7).
This partition scheme has two 3 MB OTA app slots, so it supports OTA as is.

```powershell
$f = "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=cdc"
arduino-cli compile --fqbn $f --library ESP32-S3-Touch-LCD-1.85-Demo\Arduino\libraries\lvgl --build-path build\dev .
arduino-cli upload  -p COM4 --fqbn $f --input-dir build\dev .     # USB; the board was on COM4, check `arduino-cli board list`
```

## Hardware notes
- LCD QSPI: D0-D3 = GPIO46/45/42/41, SCK 40, CS 21, TE 18, backlight 5. LCD reset is on the TCA9554 expander (EXIO2).
- Touch I2C: SDA 1, SCL 3, INT 4, reset EXIO1. Onboard I2C (IMU/RTC/expander): SDA 11, SCL 10 (not free for sensors).
- Free header pins: GPIO43 (TX) and GPIO44 (RX), only while the UART Type-C port is unused.
- The ST77916 needs Waveshare's vendor init sequence (in `src/drivers/esp_lcd_st77916.c`). Do not rewrite it from memory.

## Decisions and gotchas
- Wi-Fi source for updates: phone hotspot. Repo is public; no secrets in git.
- OTA downloads use `setInsecure()` (no TLS certificate check). Acceptable for a bench prototype; revisit before putting it in a car.
- `ota_run()` blocks, so it runs from `loop()` via a flag, never inside an LVGL callback.
- If the demo ever fails to compile after working once, Waveshare says to re-extract the demo folder, then re-copy `config/lv_conf.h`.
