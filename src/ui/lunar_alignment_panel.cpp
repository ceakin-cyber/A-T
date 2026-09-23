#include "ui/lunar_alignment_panel.h"

#include "app/format.h"
#include "ui/rows.h"

#include <imgui.h>

namespace ui {

namespace {

// "NEXT": whichever of the next new/full moon comes first, as "<NAME> IN <countdown>". Neither
// present (both searches failed) falls back to a plain placeholder rather than showing nothing.
void NextEventLabelValueRow(double julianDate, std::optional<double> nextNewMoonJd,
                            std::optional<double> nextFullMoonJd) {
    const char* label = nullptr;
    std::optional<double> soonestJd;
    if (nextNewMoonJd.has_value() &&
        (!nextFullMoonJd.has_value() || *nextNewMoonJd <= *nextFullMoonJd)) {
        label = "NEW MOON";
        soonestJd = nextNewMoonJd;
    } else if (nextFullMoonJd.has_value()) {
        label = "FULL MOON";
        soonestJd = nextFullMoonJd;
    }

    if (!soonestJd.has_value()) {
        LabelValueRow("NEXT", "UNKNOWN");
        return;
    }

    const double daysUntil = *soonestJd - julianDate;
    const std::string countdown =
        app::FormatCountdown(std::chrono::duration<double>(daysUntil * 86400.0));
    LabelValueRow("NEXT", std::string(label) + " IN " + countdown);
}

} // namespace

void DrawLunarAlignmentPanel(const core::LunarPhase& phase, double julianDate,
                            std::optional<double> nextNewMoonJd,
                            std::optional<double> nextFullMoonJd) {
    // Docked into the dashboard's dockspace by default (see ui::DrawDockSpace); this position is
    // only used the first time the window ever appears, before it has a dock to fall into.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + 20.0F),
                            ImGuiCond_FirstUseEver);

    if (ImGui::Begin("LUNAR ALIGNMENT", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("##fields", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 180.0F);

            LabelValueRow("PHASE", core::MoonPhaseName(phase.ageDays));
            LabelValueRow("ILLUM", app::FormatIlluminatedFraction(phase.illuminatedFraction));
            LabelValueRow("AGE",
                          app::FormatCountdown(std::chrono::duration<double>(phase.ageDays * 86400.0)));
            NextEventLabelValueRow(julianDate, nextNewMoonJd, nextFullMoonJd);

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ui
