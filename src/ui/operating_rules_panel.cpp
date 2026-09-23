#include "ui/operating_rules_panel.h"

#include "ui/rows.h"

#include <imgui.h>

namespace ui {

void DrawOperatingRulesPanel(const std::vector<std::string>& rules) {
    // Docked into the dashboard's dockspace by default (see ui::DrawDockSpace); this position is
    // only used the first time the window ever appears, before it has a dock to fall into.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + 20.0F),
                            ImGuiCond_FirstUseEver);

    if (ImGui::Begin("OPERATING RULES", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (rules.empty()) {
            PlaceholderText("NO RULES LOADED");
        } else {
            // Extra breathing room between rules, on top of ImGui's own default item spacing, so
            // a dense numbered list doesn't read as a single run-on block.
            constexpr float kExtraLineSpacing = 6.0F;
            for (std::size_t i = 0; i < rules.size(); ++i) {
                ImGui::Text("%02zu  %s", i + 1, rules[i].c_str());
                if (i + 1 < rules.size()) {
                    ImGui::Dummy(ImVec2(0.0F, kExtraLineSpacing));
                }
            }
        }
    }
    ImGui::End();
}

} // namespace ui
