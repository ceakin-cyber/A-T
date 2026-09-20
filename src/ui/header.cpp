#include "ui/header.h"

#include <imgui.h>

namespace ui {

float DrawHeaderBar() {
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
        ImGui::TextUnformatted("ACTIVE");
    }
    ImGui::End();

    return height;
}

} // namespace ui
