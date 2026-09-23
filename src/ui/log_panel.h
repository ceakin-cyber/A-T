#pragma once

#include "app/event_log.h"

namespace ui {

// Draws a scrolling table of timestamped log entries: `title` names the window, and `placeholder`
// is shown instead while `log` is empty. Docked into the dashboard's dockspace by default. Shared
// by every panel that shows an app::EventLog -- they differ only in title, which log, and their
// placeholder text (see event_log_panel.h and incoming_transmission_panel.h).
void DrawLogPanel(const char* title, const app::EventLog& log, const char* placeholder);

} // namespace ui
