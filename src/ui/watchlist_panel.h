#pragma once

#include "app/satellite_roster.h"

namespace ui {

// Draws the SATELLITES panel: every configured watchlist entry, clickable to switch which one
// the other panels show. Green if it loaded, red if it did not.
void DrawWatchlistPanel(app::SatelliteRoster& roster);

} // namespace ui
