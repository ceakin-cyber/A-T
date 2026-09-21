#include "ui/rows.h"

namespace ui {

void LabelValueRow(const char* label, const std::string& value, const ImVec4* color) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", label);
    ImGui::TableNextColumn();
    if (color != nullptr) {
        ImGui::TextColored(*color, "%s", value.c_str());
    } else {
        ImGui::TextUnformatted(value.c_str());
    }
}

} // namespace ui
