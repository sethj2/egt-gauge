#pragma once

// Pin map for Waveshare ESP32-S3-Touch-LCD-1.85 (from the Waveshare wiki).
// LCD (ST77916, QSPI): D0..D3 = 46,45,42,41  SCK=40  CS=21  TE=18  BL=5
//   LCD_RST is on the TCA9554 IO expander, EXIO2
// Touch (CST816, I2C): SDA=1 SCL=3 INT=4, RST on EXIO1
// Onboard I2C (IMU/RTC/expander): SDA=11 SCL=10
// Free header pins for sensors: GPIO43 (TX), GPIO44 (RX) -- planned 1-Wire for MAX31850K.
#define PIN_LCD_D0 46
#define PIN_LCD_D1 45
#define PIN_LCD_D2 42
#define PIN_LCD_D3 41
#define PIN_LCD_SCK 40
#define PIN_LCD_CS 21
#define PIN_LCD_BL 5
#define PIN_TP_SDA 1
#define PIN_TP_SCL 3
#define PIN_TP_INT 4
#define PIN_I2C_SDA 11
#define PIN_I2C_SCL 10

// Brings up TCA9554, resets LCD + touch, initialises the ST77916, turns on the
// backlight, and registers the LVGL display + touch drivers.
void board_init();
