#include "ui/dockspace.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace ui {

namespace {

constexpr const char* kDockSpaceName = "MainDockSpace";

} // namespace

void DrawDockSpace(float headerHeight) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + headerHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - headerHeight));
    ImGui::SetNextWindowViewport(viewport->ID);

    // An invisible host window that exists only to carry the dockspace; its own border, padding
    // and title bar would otherwise show through.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));
    constexpr ImGuiWindowFlags kHostFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;
    ImGui::Begin("##DockSpaceHost", nullptr, kHostFlags);
    ImGui::PopStyleVar(3);

    const ImGuiID dockSpaceId = ImGui::GetID(kDockSpaceName);
    if (ImGui::DockBuilderGetNode(dockSpaceId) == nullptr) {
        ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->Size);

        ImGuiID top = 0;
        ImGuiID bottom = 0;
        ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Down, 0.3F, &bottom, &top);
        ImGuiID left = 0;
        ImGuiID right = 0;
        ImGui::DockBuilderSplitNode(top, ImGuiDir_Left, 0.5F, &left, &right);
        ImGui::DockBuilderDockWindow("ISS TRACKER", left);
        ImGui::DockBuilderDockWindow("NEXT PASS", right);
        ImGui::DockBuilderDockWindow("EVENT LOG", bottom);
        ImGui::DockBuilderFinish(dockSpaceId);
    }

    // Passthru: where no panel covers the dockspace, the background (and the CRT effects over
    // it) still show through, instead of the dockspace painting its own flat fill.
    ImGui::DockSpace(dockSpaceId, ImVec2(0.0F, 0.0F), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

} // namespace ui
