#pragma once

#include <string>
#include <vector>

namespace ui {

// Draws the operating rules panel: each rule from app::LoadOperatingRules, one per line, in file
// order. Static, not a scrolling log: `rules` is meant to be loaded once at startup and handed
// to this every frame unchanged, there is no in-panel way to add, remove or reload one. Docked
// into the dashboard's dockspace by default.
void DrawOperatingRulesPanel(const std::vector<std::string>& rules);

} // namespace ui
