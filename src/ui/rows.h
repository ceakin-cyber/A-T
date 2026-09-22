#pragma once

#include <imgui.h>
#include <string>

namespace ui {

// The width of a panel's label column, shared by every panel that uses LabelValueRow, so their
// value text lines up at the same x position across the whole dashboard.
inline constexpr float kLabelColumnWidth = 110.0F;

// One row of a two-column table: a dim label and a value, optionally in a color. Call between
// ImGui::BeginTable and EndTable.
void LabelValueRow(const char* label, const std::string& value, const ImVec4* color = nullptr);

// A dimmed line for a panel that has nothing to show yet, such as "NO DATA" or "AWAITING
// EVENTS...". Every panel uses this for its empty state, so they all read the same way.
void PlaceholderText(const char* text);

} // namespace ui
