#include "ui/star_map_panel.h"

#include <imgui.h>
#include <numbers>

namespace ui {

namespace {

// Maps azimuth (0-360 degrees) and altitude (0-90 degrees) onto the panel rectangle: azimuth
// left to right, altitude bottom to top (the zenith at the top, the horizon at the bottom).
ImVec2 Project(const core::HorizontalPosition& position, ImVec2 origin, ImVec2 size) {
    const double azDeg = position.azimuthRad * 180.0 / std::numbers::pi;
    const double altDeg = position.altitudeRad * 180.0 / std::numbers::pi;
    return {origin.x + static_cast<float>(azDeg / 360.0) * size.x,
            origin.y + static_cast<float>(1.0 - altDeg / 90.0) * size.y};
}

} // namespace

void DrawStarMapPanel(const std::vector<core::VisibleStar>& visibleStars,
                      const std::vector<core::VisibleConstellationLine>& constellationLines,
                      bool& showConstellationLines) {
    if (!ImGui::Begin("STAR MAP")) {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("CONSTELLATION LINES", &showConstellationLines);

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 2.0F || avail.y < 2.0F) {
        ImGui::End();
        return;
    }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size = avail;
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImU32 borderColor = ImGui::GetColorU32(ImGuiCol_Text, 0.6F);
    const ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_Text, 0.25F);

    drawList->AddRect(origin, {origin.x + size.x, origin.y + size.y}, borderColor);

    // Drawn before the stars, so the star points sit on top of the lines rather than under them.
    if (showConstellationLines) {
        for (const core::VisibleConstellationLine& line : constellationLines) {
            const ImVec2 a = Project(line.a, origin, size);
            const ImVec2 b = Project(line.b, origin, size);
            drawList->AddLine(a, b, lineColor);
        }
    }

    for (const core::VisibleStar& visible : visibleStars) {
        const core::StarPointStyle pointStyle = core::MagnitudeToPointStyle(visible.star.magnitude);
        const core::Rgb tint = core::ColorIndexToRgb(visible.star.colorIndex);
        const ImVec2 p = Project(visible.position, origin, size);
        const ImU32 starColor =
            ImGui::ColorConvertFloat4ToU32({tint.r, tint.g, tint.b, pointStyle.brightness});
        drawList->AddCircleFilled(p, pointStyle.radiusPx, starColor);
    }

    ImGui::Dummy(size);
    ImGui::End();
}

} // namespace ui
