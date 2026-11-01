#pragma once

typedef void (*OtaStatusFn)(const char *msg);

// Joins the hotspot from secrets.h, checks the latest GitHub release and, if its
// version differs from FW_VERSION, downloads firmware.bin and reboots into it.
// Blocking; call it from loop(), not from an LVGL callback. Returns only if no update was applied.
void ota_run(OtaStatusFn status);
