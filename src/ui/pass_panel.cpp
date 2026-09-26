#include "ui/pass_panel.h"

#include "app/format.h"
#include "app/time_zone.h"
#include "core/time.h"
#include "ui/rows.h"

#include <imgui.h>

namespace ui {

namespace {

using Seconds = std::chrono::duration<double>;

} // namespace

void DrawPassPanel(const app::TrackedSatellite* satellite, const std::optional<core::Pass>& pass,
                   const core::Geodetic& observer, net::Clock::time_point now, float headerHeight) {
    // Docked into the dashboard's dockspace by default (see ui::DrawDockSpace); this position is
    // only used the first time the window ever appears, before it has a dock to fall into.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->Pos.x + 380.0F, viewport->Pos.y + headerHeight + 20.0F),
        ImGuiCond_FirstUseEver);

    if (ImGui::Begin("NEXT PASS", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (satellite == nullptr) {
            PlaceholderText("NO DATA");
        } else if (ImGui::BeginTable("##pass", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 230.0F);

            LabelValueRow("SITE", app::FormatLatitude(observer.latitude) + "  " +
                                      app::FormatLongitude(observer.longitude));
            if (!pass) {
                LabelValueRow("STATUS", "NO PASS IN NEXT 48 HOURS");
            } else {
                const auto rise = core::TimePointFromJulianDate(pass->riseJd);
                const auto set = core::TimePointFromJulianDate(pass->setJd);
                const auto peak = core::TimePointFromJulianDate(pass->maxElevationJd);

                if (now >= rise) {
                    LabelValueRow("STATUS", "ABOVE HORIZON, SETS IN " +
                                                app::FormatCountdown(Seconds(set - now)));
                } else {
                    LabelValueRow("STATUS",
                                  "RISES IN " + app::FormatCountdown(Seconds(rise - now)));
                }
                // Labeled with the display zone's own short name, e.g. "RISE (EDT)".
                const std::string zone = " (" + app::DisplayTimeZoneAbbreviation(rise) + ")";
                LabelValueRow(("RISE" + zone).c_str(), app::FormatTime(rise));
                LabelValueRow(("MAX" + zone).c_str(), app::FormatTime(peak));
                LabelValueRow("MAX ELEV", app::FormatElevation(pass->maxElevation));
                LabelValueRow(("SET" + zone).c_str(), app::FormatTime(set));
                LabelValueRow("DURATION", app::FormatCountdown(Seconds(set - rise)));
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ui
