#pragma once

#include "app/system_status.h"

namespace ui {

// Draws the system status readout panel: the station's own identity and health (callsign, node,
// mode, lifecycle state, last sync), as label/value rows in the same style as every other panel
// in the dashboard (see ui::LabelValueRow), docked into the dashboard's own dockspace like the
// rest of them. `headerHeight` is the height of the bar across the top, so the panel can start
// below it.
void DrawSystemStatusPanel(const app::SystemStatus& status, float headerHeight);

} // namespace ui
