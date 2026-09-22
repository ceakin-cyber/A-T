#pragma once

#include "core/star_catalog.h"

#include <vector>

namespace ui {

// Draws the star map panel: every currently visible star, plotted on a flat azimuth/altitude
// grid (azimuth 0-360 left to right, altitude 0-90 bottom to top). This is a placeholder
// projection and a placeholder point style: uniform dots, no magnitude-based size or
// spectral-based color yet, and no dome/stereographic projection yet -- those are separate,
// later issues.
void DrawStarMapPanel(const std::vector<core::VisibleStar>& visibleStars);

} // namespace ui
