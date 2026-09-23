#include "ui/system_status_panel.h"

#include "app/format.h"
#include "ui/rows.h"
#include "ui/style.h"

#include <imgui.h>

namespace ui {

namespace {

// "LAST SYNC" for a station that has never synced (SystemStatus::lastSync at its own epoch
// default): app::FormatUtcTime would otherwise print that as a real-looking (but meaningless)
// date, "01-01 00:00:00".
void LastSyncLabelValueRow(net::Clock::time_point lastSync) {
    if (lastSync == net::Clock::time_point{}) {
        LabelValueRow("LAST SYNC", "NEVER");
    } else {
        LabelValueRow("LAST SYNC", app::FormatUtcTime(lastSync));
    }
}

} // namespace

void DrawSystemStatusPanel(const app::SystemStatus& status, float headerHeight) {
    // Docked into the dashboard's dockspace by default (see ui::DrawDockSpace); this position is
    // only used the first time the window ever appears, before it has a dock to fall into.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + headerHeight + 20.0F),
                            ImGuiCond_FirstUseEver);

    if (ImGui::Begin("SYSTEM STATUS", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("##fields", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 180.0F);

            LabelValueRow("CALLSIGN", status.callsign);
            LabelValueRow("NODE", status.nodeId);

            // Mirrors the header bar's own MODE/NODE coloring (see ui/header.cpp): the worst
            // case in each pair stands out from the rest of the readout.
            if (status.mode == "LOW-VISIBILITY") {
                const ImVec4 color = WarningColor();
                LabelValueRow("MODE", status.mode, &color);
            } else {
                LabelValueRow("MODE", status.mode);
            }

            if (status.state == "OFFLINE") {
                const ImVec4 color = CriticalColor();
                LabelValueRow("STATE", status.state, &color);
            } else {
                LabelValueRow("STATE", status.state);
            }

            LastSyncLabelValueRow(status.lastSync);

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ui
