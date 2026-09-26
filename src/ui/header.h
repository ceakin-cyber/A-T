#pragma once

#include "app/tracked_satellite.h"

namespace ui {

// Draws the status header bar across the top of the main viewport: NODE and MODE, then the ZONE
// every time in the app is shown in, with a list to switch it. `satellite` is null if no TLE
// could be loaded, which the bar reports as NODE: OFFLINE.
// Returns the bar's height in pixels so other panels can be placed below it.
float DrawHeaderBar(const app::TrackedSatellite* satellite);

} // namespace ui
