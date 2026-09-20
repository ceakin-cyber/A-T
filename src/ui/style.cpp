#include "ui/style.h"

#include <imgui.h>

namespace ui {

namespace {

// Phosphor green at a given brightness (0..1) and alpha.
ImVec4 Phosphor(float brightness, float alpha = 1.0F) {
    return ImVec4(0.20F * brightness, 1.00F * brightness, 0.40F * brightness, alpha);
}

} // namespace

void ApplyTerminalStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Flat, square, minimal.
    style.WindowRounding = 0.0F;
    style.ChildRounding = 0.0F;
    style.FrameRounding = 0.0F;
    style.PopupRounding = 0.0F;
    style.ScrollbarRounding = 0.0F;
    style.GrabRounding = 0.0F;
    style.TabRounding = 0.0F;
    style.WindowBorderSize = 1.0F;
    style.ChildBorderSize = 1.0F;
    style.PopupBorderSize = 1.0F;
    style.FrameBorderSize = 0.0F;
    style.WindowTitleAlign = ImVec2(0.0F, 0.5F);

    const ImVec4 background = ImVec4(0.02F, 0.05F, 0.03F, 1.0F);
    const ImVec4 panel = ImVec4(0.03F, 0.08F, 0.05F, 1.0F);

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = Phosphor(1.0F);
    c[ImGuiCol_TextDisabled] = Phosphor(0.40F);
    c[ImGuiCol_TextSelectedBg] = Phosphor(0.30F, 0.60F);

    c[ImGuiCol_WindowBg] = background;
    c[ImGuiCol_ChildBg] = background;
    c[ImGuiCol_PopupBg] = background;
    c[ImGuiCol_MenuBarBg] = panel;

    c[ImGuiCol_Border] = Phosphor(0.55F);
    c[ImGuiCol_BorderShadow] = ImVec4(0.0F, 0.0F, 0.0F, 0.0F);

    c[ImGuiCol_FrameBg] = panel;
    c[ImGuiCol_FrameBgHovered] = Phosphor(0.20F);
    c[ImGuiCol_FrameBgActive] = Phosphor(0.30F);

    c[ImGuiCol_TitleBg] = panel;
    c[ImGuiCol_TitleBgActive] = Phosphor(0.25F);
    c[ImGuiCol_TitleBgCollapsed] = panel;

    c[ImGuiCol_ScrollbarBg] = background;
    c[ImGuiCol_ScrollbarGrab] = Phosphor(0.45F);
    c[ImGuiCol_ScrollbarGrabHovered] = Phosphor(0.70F);
    c[ImGuiCol_ScrollbarGrabActive] = Phosphor(1.0F);

    c[ImGuiCol_CheckMark] = Phosphor(1.0F);
    c[ImGuiCol_SliderGrab] = Phosphor(0.70F);
    c[ImGuiCol_SliderGrabActive] = Phosphor(1.0F);

    c[ImGuiCol_Button] = Phosphor(0.20F);
    c[ImGuiCol_ButtonHovered] = Phosphor(0.35F);
    c[ImGuiCol_ButtonActive] = Phosphor(0.55F);

    c[ImGuiCol_Header] = Phosphor(0.20F);
    c[ImGuiCol_HeaderHovered] = Phosphor(0.35F);
    c[ImGuiCol_HeaderActive] = Phosphor(0.55F);

    c[ImGuiCol_Separator] = Phosphor(0.40F);
    c[ImGuiCol_SeparatorHovered] = Phosphor(0.70F);
    c[ImGuiCol_SeparatorActive] = Phosphor(1.0F);

    c[ImGuiCol_ResizeGrip] = Phosphor(0.30F);
    c[ImGuiCol_ResizeGripHovered] = Phosphor(0.60F);
    c[ImGuiCol_ResizeGripActive] = Phosphor(1.0F);

    c[ImGuiCol_Tab] = panel;
    c[ImGuiCol_TabHovered] = Phosphor(0.35F);
    c[ImGuiCol_TabSelected] = Phosphor(0.25F);
    c[ImGuiCol_TabDimmed] = panel;
    c[ImGuiCol_TabDimmedSelected] = Phosphor(0.15F);

    c[ImGuiCol_DockingPreview] = Phosphor(0.40F, 0.70F);
    c[ImGuiCol_DockingEmptyBg] = background;
}

} // namespace ui
