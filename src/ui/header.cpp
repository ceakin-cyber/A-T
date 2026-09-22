#include "ui/header.h"

#include "app/format.h"
#include "ui/style.h"

#include <imgui.h>

namespace ui {

float DrawHeaderBar(const app::TrackedSatellite* satellite) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float height = ImGui::GetTextLineHeight() + style.WindowPadding.y * 2.0F;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("##header", nullptr, flags)) {
        ImGui::TextDisabled("NODE:");
        ImGui::SameLine();
        if (satellite != nullptr) {
            ImGui::TextUnformatted("ONLINE");

            ImGui::SameLine();
            ImGui::TextDisabled("   MODE:");
            ImGui::SameLine();
            const std::string mode = app::FormatNodeMode(satellite->source);
            if (satellite->source == net::TleSource::StaleCache) {
                ImGui::TextColored(WarningColor(), "%s", mode.c_str());
            } else {
                ImGui::TextUnformatted(mode.c_str());
            }
        } else {
            ImGui::TextColored(CriticalColor(), "OFFLINE");
        }
    }
    ImGui::End();

    return height;
}

} // namespace ui
