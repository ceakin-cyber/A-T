#pragma once

#include <imgui.h>
#include <string>

namespace ui {

// One row of a two-column table: a dim label and a value, optionally in a color. Call between
// ImGui::BeginTable and EndTable.
void LabelValueRow(const char* label, const std::string& value, const ImVec4* color = nullptr);

} // namespace ui
