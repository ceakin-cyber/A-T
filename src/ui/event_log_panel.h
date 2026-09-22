#pragma once

#include "app/event_log.h"

namespace ui {

// Draws the scrolling event log panel. Docked into the dashboard's dockspace by default.
void DrawEventLogPanel(const app::EventLog& log);

} // namespace ui
