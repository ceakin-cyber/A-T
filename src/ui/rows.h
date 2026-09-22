#pragma once

#include <imgui.h>
#include <string>

namespace ui {

// One row of a two-column table: a dim label and a value, optionally in a color. Call between
// ImGui::BeginTable and EndTable.
void LabelValueRow(const char* label, const std::string& value, const ImVec4* color = nullptr);

// A dimmed line for a panel that has nothing to show yet, such as "NO DATA" or "AWAITING
// EVENTS...". Every panel uses this for its empty state, so they all read the same way.
void PlaceholderText(const char* text);

} // namespace ui
