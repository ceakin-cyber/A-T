#include "ui/tracker_panel.h"

#include "app/format.h"
#include "app/tle_age.h"
#include "ui/style.h"

#include <imgui.h>

namespace ui {

namespace {

void Row(const char* label, const std::string& value, const ImVec4* color = nullptr) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", label);
    ImGui::TableNextColumn();
    if (color != nullptr) {
        ImGui::TextColored(*color, "%s", value.c_str());
    } else {
        ImGui::TextUnformatted(value.c_str());
    }
}

// The age of the TLE, tagged and colored by how much it can be trusted.
void AgeRow(const app::TrackedSatellite& satellite, net::Clock::time_point now) {
    const app::Seconds age = app::TleAge(satellite, now);
    std::string text = app::FormatAge(age);
    switch (app::ClassifyAge(age)) {
    case app::TleFreshness::Fresh:
        Row("TLE AGE", text);
        break;
    case app::TleFreshness::Aging: {
        const ImVec4 color = WarningColor();
        Row("TLE AGE", text + "  AGING", &color);
        break;
    }
    case app::TleFreshness::Stale: {
        const ImVec4 color = CriticalColor();
        Row("TLE AGE", text + "  STALE", &color);
        break;
    }
    }
}

} // namespace

void DrawTrackerPanel(const app::TrackedSatellite* satellite,
                      const std::optional<app::SatellitePosition>& position,
                      net::Clock::time_point now, float headerHeight) {
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
            AgeRow(*satellite, now);
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ui
