#include "ui/signal_quality_panel.h"

#include "app/format.h"
#include "app/signal_quality.h"
#include "ui/rows.h"
#include "ui/style.h"

#include <imgui.h>

namespace ui {

void DrawSignalQualityPanel(const std::optional<net::LoadedKp>& kp) {
    // Docked into the dashboard's dockspace by default (see ui::DrawDockSpace); this position is
    // only used the first time the window ever appears, before it has a dock to fall into.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + 20.0F),
                            ImGuiCond_FirstUseEver);

    if (ImGui::Begin("SIGNAL QUALITY", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!kp.has_value()) {
            PlaceholderText("NO DATA");
        } else if (ImGui::BeginTable("##fields", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 180.0F);

            const app::SignalQuality quality = app::ClassifyKp(kp->kp);
            const char* signal = app::ToString(quality);
            if (quality == app::SignalQuality::Disrupted) {
                const ImVec4 color = CriticalColor();
                LabelValueRow("SIGNAL", signal, &color);
            } else if (quality == app::SignalQuality::Degraded) {
                const ImVec4 color = WarningColor();
                LabelValueRow("SIGNAL", signal, &color);
            } else {
                LabelValueRow("SIGNAL", signal);
            }

            LabelValueRow("KP INDEX", app::FormatKp(kp->kp));

            // A stale reading (the fetch failed, so an older cached one stands in) is flagged the
            // same way the header flags a stale TLE: the tier above may no longer be current.
            if (kp->source == net::KpSource::StaleCache) {
                const ImVec4 color = WarningColor();
                LabelValueRow("UPDATED", app::FormatTime(kp->fetchedAt), &color);
            } else {
                LabelValueRow("UPDATED", app::FormatTime(kp->fetchedAt));
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ui
