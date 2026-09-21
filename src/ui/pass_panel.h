#pragma once

#include "app/tracked_satellite.h"
#include "core/geodetic.h"
#include "core/passes.h"

#include <optional>

namespace ui {

// Draws the next-pass panel. `satellite` is null if no TLE could be loaded, and `pass` is empty
// if there is no pass in the planning window. `now` is the current time, for the countdown.
void DrawPassPanel(const app::TrackedSatellite* satellite, const std::optional<core::Pass>& pass,
                   const core::Geodetic& observer, net::Clock::time_point now, float headerHeight);

} // namespace ui
