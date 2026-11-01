// Multifunction gauge: EGT, coolant temp, boost. Proof of concept with simulated sensors.
// Target: Waveshare ESP32-S3-Touch-LCD-1.85 (360x360 round, ST77916 QSPI, CST816 touch).
#include <lvgl.h>
#include "src/board.h"
#include "src/sensors.h"
#include "src/gauge_ui.h"
#include "src/update_ui.h"

static SimulatedSensors sensors;  // swap for RealSensors later (same Sensors interface)

void setup() {
  Serial.begin(115200);
  board_init();   // display, touch, backlight, LVGL driver registration
  gauge_ui_init();
  update_ui_init();  // long-press the screen to open the update panel
}

void loop() {
  static uint32_t lastSample = 0;
  uint32_t now = millis();
  if (now - lastSample >= 33) {  // ~30 Hz sensor/UI update
    lastSample = now;
    gauge_ui_update(sensors.read());
  }
  lv_timer_handler();
  update_ui_poll();
  delay(1);
}
