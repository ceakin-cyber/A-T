#pragma once

#include "app/satellite_position.h"
#include "app/tracked_satellite.h"

#include <optional>

namespace ui {

// Draws the live tracker panel. `satellite` is null if no TLE could be loaded, and `position`
// is empty if the propagator failed for the current time. `now` is the time the position was
// computed for, which the TLE age is measured against. `headerHeight` is the height of the bar
// across the top, so the panel can start below it.
void DrawTrackerPanel(const app::TrackedSatellite* satellite,
                      const std::optional<app::SatellitePosition>& position,
                      net::Clock::time_point now, float headerHeight);

} // namespace ui
