#pragma once

#include <imgui.h>

namespace ui {

// Applies the dark, flat, green-phosphor terminal style to the current ImGui context.
void ApplyTerminalStyle();

// The color of the empty screen behind the panels. It is a dark green and not black, like a lit
// CRT, because a scanline effect cannot show on pure black.
ImVec4 ScreenBackground();

// Severity colors for text that should stand out from the green: amber for warnings, red for
// serious problems.
ImVec4 WarningColor();
ImVec4 CriticalColor();

} // namespace ui
