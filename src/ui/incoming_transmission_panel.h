#pragma once

#include "app/event_log.h"

namespace ui {

// Draws the scrolling incoming-transmission log: real events from the app's own pipeline (TLE
// fetches, propagator initialization, pass computations, and the like), across every watched
// satellite -- separate from the per-satellite EVENT LOG panel, which only logs signal
// acquired/lost for the selected one. Docked into the dashboard's dockspace by default.
void DrawIncomingTransmissionPanel(const app::EventLog& log);

} // namespace ui
