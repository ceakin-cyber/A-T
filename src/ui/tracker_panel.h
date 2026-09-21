#pragma once

#include "app/satellite_position.h"
#include "app/tracked_satellite.h"

#include <optional>

namespace ui {

// Draws the live tracker panel. `satellite` is null if no TLE could be loaded, and `position`
// is empty if the propagator failed for the current time. `headerHeight` is the height of the
// bar across the top, so the panel can start below it.
void DrawTrackerPanel(const app::TrackedSatellite* satellite,
                      const std::optional<app::SatellitePosition>& position, float headerHeight);

} // namespace ui
