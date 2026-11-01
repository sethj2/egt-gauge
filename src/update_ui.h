#pragma once

// Long-press anywhere on the gauge to open the firmware update panel.
void update_ui_init();
// Call from loop(): runs the (blocking) update when the user tapped "Check for update".
void update_ui_poll();
