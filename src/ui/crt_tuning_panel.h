#pragma once

#include "ui/screen_pass.h"

namespace ui {

// A panel with sliders for the CRT effect settings, for finding a look that works by eye. The
// settings change live as the sliders move. "PRINT" writes the current values to the console,
// so they can be copied into the code as the new defaults. Pass `open` as the window's close
// button state; the panel draws nothing while it is false.
void DrawCrtTuningPanel(CrtSettings& settings, bool& open, float headerHeight);

} // namespace ui
