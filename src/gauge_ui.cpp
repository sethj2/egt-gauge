// LVGL 8.3 UI. Two colour-zoned ring segments with needles:
//   top    = EGT, 400..1300 F, blue (left) -> red (right) over the top
//   bottom = CLT, -10..240 F,  blue (left) -> red (right) over the bottom
// Live values are printed in the centre.
#include <math.h>
#include <lvgl.h>
#include "gauge_ui.h"

// ---- geometry (screen is 360x360) ----
static const int   CX = 180, CY = 180;
static const int   RING_D = 340;            // outer diameter of the ring
static const int   RING_W = 24;             // ring thickness
static const float R_OUT = RING_D / 2.0f;
static const float R_IN  = R_OUT - RING_W;
static const float NEEDLE_OVERHANG = 8;     // needle sticks out past both ring edges
static const float SWEEP = 160;             // degrees covered by each segment (gaps at 3 and 9 o'clock)

struct Zone { float from, to; uint32_t color; };  // in gauge units

struct Gauge {
  float lo, hi;
  bool  top;             // top segment (left->right over the top) or bottom (over the bottom)
  const Zone *zones;
  int   nZones;
  lv_point_t pts[2];     // needle endpoints (must outlive the lv_line)
  lv_obj_t *needle;
  lv_obj_t *value;
  int lastShown;
};

static const Zone EGT_ZONES[] = {
  { 400, 1200, 0x3498DB},   // blue
  {1200, 1300, 0xE74C3C},   // red above 1200 F
};
static const Zone CLT_ZONES[] = {
  { -10, 220, 0x3498DB},    // blue
  { 220, 240, 0xE74C3C},    // red above 220 F
};

static Gauge egt = {400, 1300, true,  EGT_ZONES, 2, {}, nullptr, nullptr, INT32_MIN};
static Gauge clt = {-10,  240, false, CLT_ZONES, 2, {}, nullptr, nullptr, INT32_MIN};

// Fraction 0..1 along the gauge -> LVGL angle (0 = 3 o'clock, clockwise, y down).
static float frac_to_angle(const Gauge &g, float f) {
  return g.top ? 190.0f + f * SWEEP    // 190 (left) -> 350 (right) through 270 (top)
               : 170.0f - f * SWEEP;   // 170 (left) -> 10 (right) through 90 (bottom)
}

static void add_zone_arc(lv_obj_t *parent, const Gauge &g, const Zone &z) {
  float f1 = (z.from - g.lo) / (g.hi - g.lo);
  float f2 = (z.to   - g.lo) / (g.hi - g.lo);
  float a1 = frac_to_angle(g, f1), a2 = frac_to_angle(g, f2);
  if (a1 > a2) { float t = a1; a1 = a2; a2 = t; }   // arcs are drawn clockwise start->end

  lv_obj_t *arc = lv_arc_create(parent);
  lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(arc, RING_D, RING_D);
  lv_obj_center(arc);
  lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(arc, 0, 0);
  lv_obj_set_style_pad_all(arc, 0, 0);
  lv_arc_set_bg_angles(arc, (uint16_t)roundf(a1), (uint16_t)roundf(a2));
  lv_obj_set_style_arc_width(arc, RING_W, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, lv_color_hex(z.color), LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_INDICATOR);  // zones only, no fill
}

static void make_gauge(Gauge &g, lv_obj_t *scr, int valueY, const char *caption) {
  for (int i = 0; i < g.nZones; i++) add_zone_arc(scr, g, g.zones[i]);

  g.needle = lv_line_create(scr);
  lv_obj_set_style_line_width(g.needle, 6, 0);
  lv_obj_set_style_line_color(g.needle, lv_color_white(), 0);
  lv_obj_set_style_line_rounded(g.needle, false, 0);

  g.value = lv_label_create(scr);
  lv_obj_set_style_text_font(g.value, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(g.value, lv_color_white(), 0);
  lv_label_set_text(g.value, "---");
  lv_obj_align(g.value, LV_ALIGN_CENTER, 0, valueY);

  lv_obj_t *cap = lv_label_create(scr);
  lv_obj_set_style_text_font(cap, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(cap, lv_color_hex(0x9A9A9A), 0);
  lv_label_set_text(cap, caption);
  lv_obj_align_to(cap, g.value, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
}

static void set_gauge(Gauge &g, float v) {
  float c = v < g.lo ? g.lo : v > g.hi ? g.hi : v;
  float a = frac_to_angle(g, (c - g.lo) / (g.hi - g.lo)) * (float)M_PI / 180.0f;
  float r0 = R_IN - NEEDLE_OVERHANG, r1 = R_OUT + NEEDLE_OVERHANG;
  g.pts[0] = {(lv_coord_t)roundf(CX + r0 * cosf(a)), (lv_coord_t)roundf(CY + r0 * sinf(a))};
  g.pts[1] = {(lv_coord_t)roundf(CX + r1 * cosf(a)), (lv_coord_t)roundf(CY + r1 * sinf(a))};
  lv_line_set_points(g.needle, g.pts, 2);

  // Only touch the label when the shown number changes, so it isn't redrawn every frame.
  int shown = (int)roundf(v);
  if (shown != g.lastShown) {
    g.lastShown = shown;
    lv_label_set_text_fmt(g.value, "%d", shown);
    const Zone &top = g.zones[g.nZones - 1];
    lv_obj_set_style_text_color(g.value, v >= top.from ? lv_color_hex(top.color) : lv_color_white(), 0);
  }
}

void gauge_ui_init() {
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  make_gauge(egt, scr, -55, "EGT \xC2\xB0""F");
  make_gauge(clt, scr,  45, "CLT \xC2\xB0""F");
}

void gauge_ui_update(const Readings &r) {
  if (r.egtValid) set_gauge(egt, r.egtF);
  else            lv_label_set_text(egt.value, "---");
  set_gauge(clt, r.coolantF);
}
