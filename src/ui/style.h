#pragma once

#include <imgui.h>

namespace ui {

// Applies the dark, flat, green-phosphor terminal style to the current ImGui context.
void ApplyTerminalStyle();

// Severity colors for text that should stand out from the green: amber for warnings, red for
// serious problems.
ImVec4 WarningColor();
ImVec4 CriticalColor();

} // namespace ui
