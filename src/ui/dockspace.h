#pragma once

namespace ui {

// Fills the area below the header bar with a dockspace, so panels can be dragged, resized and
// rearranged like a real dashboard. The first time the app runs (or whenever there is no saved
// layout), the tracker and pass panels are docked side by side, matching where they used to sit
// as fixed floating windows. After that, whatever arrangement the user drags them into is kept
// in imgui.ini (gitignored, so it stays per-user) and this default is not applied again.
void DrawDockSpace(float headerHeight);

} // namespace ui
