#include <lvgl.h>
#include "update_ui.h"
#include "ota.h"
#include "version.h"

static lv_obj_t *panel;
static lv_obj_t *statusLabel;
static lv_obj_t *checkBtn;
static bool pending = false;

static void show_panel(bool show) {
  if (show) lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
  else      lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
}

static void screen_cb(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED && lv_obj_has_flag(panel, LV_OBJ_FLAG_HIDDEN)) {
    lv_label_set_text_fmt(statusLabel, "Firmware v%s", FW_VERSION);
    lv_obj_clear_state(checkBtn, LV_STATE_DISABLED);
    show_panel(true);
  }
}

static void check_cb(lv_event_t *) {
  pending = true;
  lv_obj_add_state(checkBtn, LV_STATE_DISABLED);
  lv_label_set_text(statusLabel, "Starting...");
}

static void close_cb(lv_event_t *) { show_panel(false); }

static lv_obj_t *make_button(const char *text, int y, lv_event_cb_t cb) {
  lv_obj_t *b = lv_btn_create(panel);
  lv_obj_set_size(b, 200, 48);
  lv_obj_align(b, LV_ALIGN_CENTER, 0, y);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, text);
  lv_obj_center(l);
  return b;
}

void update_ui_init() {
  lv_obj_add_event_cb(lv_scr_act(), screen_cb, LV_EVENT_LONG_PRESSED, NULL);

  panel = lv_obj_create(lv_layer_top());  // full-screen overlay above the gauge
  lv_obj_set_size(panel, 360, 360);
  lv_obj_center(panel);
  lv_obj_set_style_radius(panel, 0, 0);
  lv_obj_set_style_border_width(panel, 0, 0);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x101010), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(panel);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
  lv_label_set_text(title, "Firmware update");
  lv_obj_align(title, LV_ALIGN_CENTER, 0, -100);

  statusLabel = lv_label_create(panel);
  lv_obj_set_width(statusLabel, 240);
  lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, -45);

  checkBtn = make_button("Check for update", 30, check_cb);
  make_button("Close", 90, close_cb);

  show_panel(false);
}

static void status_cb(const char *msg) {
  lv_label_set_text(statusLabel, msg);
  lv_timer_handler();  // keep the screen alive while the update blocks loop()
}

void update_ui_poll() {
  if (!pending) return;
  pending = false;
  ota_run(status_cb);  // returns only if no update was applied
  lv_obj_clear_state(checkBtn, LV_STATE_DISABLED);
}
