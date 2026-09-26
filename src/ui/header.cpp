#include "ui/header.h"

#include "app/format.h"
#include "app/time_zone.h"
#include "ui/style.h"

#include <algorithm>
#include <chrono>
#include <imgui.h>
#include <string>
#include <vector>

namespace ui {

namespace {

// The zones offered in the TIME ZONE list, besides UTC and LOCAL: one or two per inhabited
// continent, west to east. Any other IANA zone can be set with the config file's time_zone line,
// and then shows here too. A zone missing from this computer's time zone database is left out.
constexpr const char* kCommonTimeZones[] = {
    "America/Los_Angeles", "America/Denver",  "America/Chicago",     "America/New_York",
    "America/Sao_Paulo",   "Europe/London",   "Europe/Paris",        "Europe/Moscow",
    "Africa/Johannesburg", "Asia/Dubai",      "Asia/Kolkata",        "Asia/Shanghai",
    "Asia/Tokyo",          "Australia/Sydney", "Pacific/Auckland",
};

// "ZONE:" and a list to switch the time zone every time in the app is shown in (see
// app::SetDisplayTimeZone). The choice lasts until the app closes; the config file's time_zone
// line sets the one it starts in. Drawn with no vertical frame padding, so the list's button fits
// the header's single line of text.
void TimeZoneSelector() {
    ImGui::TextDisabled("ZONE:");
    ImGui::SameLine();

    const std::string current = app::DisplayTimeZone();
    const std::string preview =
        current + " (" + app::DisplayTimeZoneAbbreviation(std::chrono::system_clock::now()) + ")";
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {ImGui::GetStyle().FramePadding.x, 0.0F});
    ImGui::SetNextItemWidth(ImGui::CalcTextSize(preview.c_str()).x + ImGui::GetFrameHeight() +
                            ImGui::GetStyle().FramePadding.x * 2.0F);
    if (ImGui::BeginCombo("##timezone", preview.c_str())) {
        std::vector<std::string> choices = {app::kUtcTimeZone, app::kLocalTimeZone};
        for (const char* zone : kCommonTimeZones) {
            if (app::IsKnownTimeZone(zone)) {
                choices.emplace_back(zone);
            }
        }
        if (std::find(choices.begin(), choices.end(), current) == choices.end()) {
            choices.insert(choices.begin() + 2, current); // e.g. one set in the config file
        }
        for (const std::string& zone : choices) {
            const bool selected = zone == current;
            if (ImGui::Selectable(zone.c_str(), selected)) {
                app::SetDisplayTimeZone(zone);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleVar();
}

} // namespace

float DrawHeaderBar(const app::TrackedSatellite* satellite) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float height = ImGui::GetTextLineHeight() + style.WindowPadding.y * 2.0F;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, height));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("##header", nullptr, flags)) {
        ImGui::TextDisabled("NODE:");
        ImGui::SameLine();
        if (satellite != nullptr) {
            ImGui::TextUnformatted("ONLINE");

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::TextDisabled("MODE:");
            ImGui::SameLine();
            const std::string mode = app::FormatNodeMode(satellite->source);
            if (satellite->source == net::TleSource::StaleCache) {
                ImGui::TextColored(WarningColor(), "%s", mode.c_str());
            } else {
                ImGui::TextUnformatted(mode.c_str());
            }
        } else {
            ImGui::TextColored(CriticalColor(), "OFFLINE");
        }

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        TimeZoneSelector();
    }
    ImGui::End();

    return height;
}

} // namespace ui
