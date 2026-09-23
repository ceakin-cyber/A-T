#pragma once

#include "app/star_map_time.h"
#include "core/constellation.h"
#include "core/star_catalog.h"

#include <chrono>
#include <vector>

namespace ui {

// Draws the star map panel: every currently visible star, plotted on a flat azimuth/altitude
// grid (azimuth 0-360 left to right, altitude 0-90 bottom to top), sized by magnitude, in the
// panel's own phosphor green rather than each star's true spectral color, to match the rest of
// this terminal-styled app, with constellation stick-figure lines drawn beneath them. This is a
// placeholder projection -- no dome/stereographic projection yet, a separate, later issue.
//
// `visibleStars` and `constellationLines` must already have been computed for whatever time the
// panel is currently showing (app::Effective(time, now)) -- this function only draws them and
// the controls below; it does not recompute the sky itself.
//
// A checkbox at the top of the panel toggles the constellation lines on and off;
// showConstellationLines holds that state across frames and is not persisted between runs.
//
// Below that, time controls let the sky be explored away from the present: jump buttons step
// `time` by a fixed amount, a PLAY/PAUSE button and speed slider animate it, and a LIVE button
// (shown once detached) snaps back to following the real clock. `now` is the real system time
// this frame, needed to seed a jump or play from the present the moment either first detaches.
// `time` is not persisted between runs.
void DrawStarMapPanel(const std::vector<core::VisibleStar>& visibleStars,
                      const std::vector<core::VisibleConstellationLine>& constellationLines,
                      bool& showConstellationLines, app::StarMapTime& time,
                      std::chrono::system_clock::time_point now);

} // namespace ui
