#include "ui/tracker_panel.h"

#include "app/format.h"

#include <imgui.h>

namespace ui {

namespace {

void Row(const char* label, const std::string& value) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", label);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value.c_str());
}

} // namespace

void DrawTrackerPanel(const app::TrackedSatellite* satellite,
                      const std::optional<app::SatellitePosition>& position, float headerHeight) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + headerHeight + 20.0F),
                            ImGuiCond_FirstUseEver);

    if (ImGui::Begin("ISS TRACKER", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (satellite == nullptr) {
            ImGui::TextUnformatted("NO DATA");
        } else if (ImGui::BeginTable("##fields", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 110.0F);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 180.0F);

            Row("SATELLITE", satellite->tle.name);
            if (position) {
                Row("LAT", app::FormatLatitude(position->geodetic.latitude));
                Row("LON", app::FormatLongitude(position->geodetic.longitude));
                Row("ALT", app::FormatAltitudeKm(position->geodetic.altitudeKm));
                Row("SPEED", app::FormatSpeedKmPerSec(position->speedKmPerSec));
            } else {
                Row("STATUS", "POSITION UNAVAILABLE");
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ui
