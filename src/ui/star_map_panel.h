#pragma once

#include "core/constellation.h"
#include "core/star_catalog.h"

#include <vector>

namespace ui {

// Draws the star map panel: every currently visible star, plotted on a flat azimuth/altitude
// grid (azimuth 0-360 left to right, altitude 0-90 bottom to top), sized and colored by
// magnitude and B-V color index, with constellation stick-figure lines drawn beneath them. This
// is a placeholder projection -- no dome/stereographic projection yet, a separate, later issue.
void DrawStarMapPanel(const std::vector<core::VisibleStar>& visibleStars,
                      const std::vector<core::VisibleConstellationLine>& constellationLines);

} // namespace ui
