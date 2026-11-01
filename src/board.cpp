// Board bring-up, ported from Waveshare's LVGL_Arduino demo (init order matters:
// I2C -> TCA9554 IO expander -> backlight -> LCD + touch -> LVGL).
#include <Arduino.h>
#include <lvgl.h>
#include <esp_timer.h>
#include "board.h"
#include "drivers/I2C_Driver.h"
#include "drivers/TCA9554PWR.h"
#include "drivers/Display_ST77916.h"
#include "drivers/Touch_CST816.h"

#define LVGL_BUF_LEN (EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT / 10)
#define LVGL_TICK_MS 10

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[LVGL_BUF_LEN];
static lv_color_t buf2[LVGL_BUF_LEN];

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint16_t *)&color_p->full);
  lv_disp_flush_ready(drv);
}

static void touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  Touch_Read_Data();
  if (touch_data.points != 0) {
    data->point.x = touch_data.x;
    data->point.y = touch_data.y;
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
  touch_data.x = 0;
  touch_data.y = 0;
  touch_data.points = 0;
  touch_data.gesture = NONE;
}

static void tick_cb(void *) { lv_tick_inc(LVGL_TICK_MS); }

void board_init() {
  I2C_Init();
  TCA9554PWR_Init(0x00);
  Backlight_Init();
  LCD_Init();  // ST77916 + CST816 touch

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LVGL_BUF_LEN);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = EXAMPLE_LCD_WIDTH;
  disp_drv.ver_res = EXAMPLE_LCD_HEIGHT;
  disp_drv.flush_cb = flush_cb;
  disp_drv.full_refresh = 0;  // only redraw dirty areas (the demo's full refresh is much slower)
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = touch_cb;
  lv_indev_drv_register(&indev_drv);

  const esp_timer_create_args_t tick_args = {.callback = &tick_cb, .name = "lvgl_tick"};
  esp_timer_handle_t tick_timer = NULL;
  esp_timer_create(&tick_args, &tick_timer);
  esp_timer_start_periodic(tick_timer, LVGL_TICK_MS * 1000);
}
