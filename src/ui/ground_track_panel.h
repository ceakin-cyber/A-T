#pragma once

#include "app/satellite_roster.h"
#include "core/geodetic.h"

namespace ui {

// Draws the ground-track panel: a plain equirectangular lat/lon grid with the selected
// satellite's recent and upcoming ground track, its current position, and the observer's
// location. `watched` is null if there is no selected entry.
void DrawGroundTrackPanel(const app::WatchedSatellite* watched, const core::Geodetic& observer);

} // namespace ui
