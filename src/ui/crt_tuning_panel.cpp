#include "ui/crt_tuning_panel.h"

#include <imgui.h>
#include <iostream>

namespace ui {

void DrawCrtTuningPanel(CrtSettings& settings, bool& open, float headerHeight) {
    if (!open) {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + headerHeight + 300.0F),
        ImGuiCond_FirstUseEver);

    if (ImGui::Begin("CRT TUNING (F2)", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::SliderFloat("SCANLINE STRENGTH", &settings.scanlineIntensity, 0.0F, 1.0F, "%.2f");

        // The period is whole rows, since a fraction of a row makes an uneven pattern.
        int period = static_cast<int>(settings.scanlinePeriod + 0.5F);
        if (ImGui::SliderInt("SCANLINE SPACING", &period, 1, 16, "%d PX")) {
            settings.scanlinePeriod = static_cast<float>(period);
        }

        if (ImGui::Button("RESET")) {
            settings = CrtSettings{};
        }
        ImGui::SameLine();
        if (ImGui::Button("PRINT")) {
            std::cout << "CrtSettings: scanlineIntensity = " << settings.scanlineIntensity
                      << ", scanlinePeriod = " << settings.scanlinePeriod << '\n';
        }
    }
    ImGui::End();
}

} // namespace ui
