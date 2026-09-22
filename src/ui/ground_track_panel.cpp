#include "ui/ground_track_panel.h"

#include "app/ground_track.h"
#include "ui/rows.h"
#include "ui/style.h"

#include <imgui.h>
#include <numbers>

namespace ui {

namespace {

// Equirectangular projection: longitude in [-180, 180] to x in [0, width], latitude in
// [-90, 90] to y in [0, height] (flipped, since screen y grows downward).
ImVec2 Project(const core::Geodetic& point, ImVec2 origin, ImVec2 size) {
    const double lonDeg = point.longitude * 180.0 / std::numbers::pi;
    const double latDeg = point.latitude * 180.0 / std::numbers::pi;
    return {origin.x + static_cast<float>((lonDeg + 180.0) / 360.0) * size.x,
            origin.y + static_cast<float>((90.0 - latDeg) / 180.0) * size.y};
}

void DrawGraticule(ImDrawList* drawList, ImVec2 origin, ImVec2 size, ImU32 lineColor) {
    for (int lonDeg = -180; lonDeg <= 180; lonDeg += 30) {
        const float x = origin.x + static_cast<float>(lonDeg + 180) / 360.0F * size.x;
        drawList->AddLine({x, origin.y}, {x, origin.y + size.y}, lineColor);
    }
    for (int latDeg = -90; latDeg <= 90; latDeg += 30) {
        const float y = origin.y + static_cast<float>(90 - latDeg) / 180.0F * size.y;
        drawList->AddLine({origin.x, y}, {origin.x + size.x, y}, lineColor);
    }
}

} // namespace

void DrawGroundTrackPanel(const app::WatchedSatellite* watched, const core::Geodetic& observer) {
    if (!ImGui::Begin("GROUND TRACK")) {
        ImGui::End();
        return;
    }

    if (watched == nullptr || !watched->satellite) {
        PlaceholderText("NO DATA");
        ImGui::End();
        return;
    }

    // Fit the largest 2:1 (lon:lat) rectangle inside the available space.
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    float width = avail.x;
    float height = width / 2.0F;
    if (height > avail.y) {
        height = avail.y;
        width = height * 2.0F;
    }
    if (width < 2.0F || height < 2.0F) {
        ImGui::End();
        return;
    }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size(width, height);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImU32 gridColor = ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.6F);
    const ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Text, 0.6F);
    const ImU32 trackColor = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 observerColor = ImGui::GetColorU32(WarningColor());

    drawList->AddRect(origin, {origin.x + size.x, origin.y + size.y}, borderColor);
    DrawGraticule(drawList, origin, size, gridColor);

    const std::vector<core::Geodetic> track =
        app::ComputeGroundTrack(watched->satellite->model, net::Clock::now());
    for (std::size_t i = 1; i < track.size(); ++i) {
        const double lon1 = track[i - 1].longitude * 180.0 / std::numbers::pi;
        const double lon2 = track[i].longitude * 180.0 / std::numbers::pi;
        if (app::CrossesAntimeridian(lon1, lon2)) {
            continue;
        }
        drawList->AddLine(Project(track[i - 1], origin, size), Project(track[i], origin, size),
                          trackColor, 1.5F);
    }

    if (watched->position) {
        const ImVec2 p = Project(watched->position->geodetic, origin, size);
        drawList->AddCircleFilled(p, 4.0F, trackColor);
    }

    const ImVec2 obs = Project(observer, origin, size);
    drawList->AddCircle(obs, 5.0F, observerColor, 12, 1.5F);
    drawList->AddLine({obs.x - 7.0F, obs.y}, {obs.x + 7.0F, obs.y}, observerColor);
    drawList->AddLine({obs.x, obs.y - 7.0F}, {obs.x, obs.y + 7.0F}, observerColor);

    ImGui::Dummy(size);
    ImGui::End();
}

} // namespace ui
