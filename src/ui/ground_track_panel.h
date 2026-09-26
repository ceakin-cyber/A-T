#pragma once

#include "app/satellite_roster.h"
#include "core/geodetic.h"
#include "net/tle_cache.h"
#include "ui/land_mesh.h"

namespace ui {

// Draws the ground-track panel, laid out for someone on the ground trying to see the selected
// satellite. At the top, in plain sentences: whether it is above them right now (and if so,
// where to look and whether it can be seen), when it next comes over, and when it can next
// actually be seen by eye (see app::Visibility). Below that, a world map (`land`, filled in)
// with the night side shaded, the observer marked "YOU", the satellite marked with its name, and
// its path over the next orbit or so -- highlighted amber wherever it passes over the observer,
// with a time mark every half hour.
//
// `watched` is null if there is no selected entry; `now` is the real system time this frame.
void DrawGroundTrackPanel(const app::WatchedSatellite* watched, const core::Geodetic& observer,
                          const LandMesh& land, net::Clock::time_point now);

} // namespace ui
