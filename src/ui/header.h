#pragma once

#include "app/tracked_satellite.h"

namespace ui {

// Draws the status header bar across the top of the main viewport. `satellite` is null if no
// TLE could be loaded, which the bar reports as NODE: OFFLINE.
// Returns the bar's height in pixels so other panels can be placed below it.
float DrawHeaderBar(const app::TrackedSatellite* satellite);

} // namespace ui
