#include "ui/tracker_panel.h"

#include "app/format.h"
#include "app/tle_age.h"
#include "ui/rows.h"
#include "ui/style.h"

#include <imgui.h>

namespace ui {

namespace {

// The age of the TLE, tagged and colored by how much it can be trusted.
void AgeLabelValueRow(const app::TrackedSatellite& satellite, net::Clock::time_point now) {
    const app::Seconds age = app::TleAge(satellite, now);
    std::string text = app::FormatAge(age);
    switch (app::ClassifyAge(age)) {
    case app::TleFreshness::Fresh:
        LabelValueRow("TLE AGE", text);
        break;
    case app::TleFreshness::Aging: {
        const ImVec4 color = WarningColor();
        LabelValueRow("TLE AGE", text + "  AGING", &color);
        break;
    }
    case app::TleFreshness::Stale: {
        const ImVec4 color = CriticalColor();
        LabelValueRow("TLE AGE", text + "  STALE", &color);
        break;
    }
    }
}

} // namespace

void DrawTrackerPanel(const app::TrackedSatellite* satellite,
                      const std::optional<app::SatellitePosition>& position,
                      net::Clock::time_point now, float headerHeight) {
    // Docked into the dashboard's dockspace by default (see ui::DrawDockSpace); this position is
    // only used the first time the window ever appears, before it has a dock to fall into.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + headerHeight + 20.0F),
                            ImGuiCond_FirstUseEver);

    if (ImGui::Begin("ISS TRACKER", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (satellite == nullptr) {
            PlaceholderText("NO DATA");
        } else if (ImGui::BeginTable("##fields", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 110.0F);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 180.0F);

            LabelValueRow("SATELLITE", satellite->tle.name);
            if (position) {
                LabelValueRow("LAT", app::FormatLatitude(position->geodetic.latitude));
                LabelValueRow("LON", app::FormatLongitude(position->geodetic.longitude));
                LabelValueRow("ALT", app::FormatAltitudeKm(position->geodetic.altitudeKm));
                LabelValueRow("SPEED", app::FormatSpeedKmPerSec(position->speedKmPerSec));
            } else {
                LabelValueRow("STATUS", "POSITION UNAVAILABLE");
            }
            AgeLabelValueRow(*satellite, now);
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ui
