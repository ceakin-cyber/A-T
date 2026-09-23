#include "ui/star_map_panel.h"

#include "app/format.h"

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

// The play speed slider's range: up to a simulated day per real second, in either direction.
constexpr double kMinSpeed = -86400.0;
constexpr double kMaxSpeed = 86400.0;

// One jump button: steps `time` by `delta` (positive or negative) from its current effective
// time when clicked.
void JumpButton(const char* label, app::StarMapTime& time,
                std::chrono::system_clock::time_point now, std::chrono::duration<double> delta) {
    if (ImGui::Button(label)) {
        app::Jump(time, now, delta);
    }
    ImGui::SameLine();
}

void DrawTimeControls(app::StarMapTime& time, std::chrono::system_clock::time_point now) {
    using namespace std::chrono_literals;

    JumpButton("-1D", time, now, -24h);
    JumpButton("-1H", time, now, -1h);
    JumpButton("-10M", time, now, -10min);

    if (time.following) {
        ImGui::TextDisabled("LIVE");
    } else if (ImGui::Button("LIVE")) {
        app::Resume(time);
    }
    ImGui::SameLine();

    JumpButton("+10M", time, now, 10min);
    JumpButton("+1H", time, now, 1h);
    if (ImGui::Button("+1D")) {
        app::Jump(time, now, 24h);
    }

    bool playing = time.playing;
    if (ImGui::Checkbox(playing ? "PAUSE" : "PLAY", &playing)) {
        app::SetPlaying(time, now, playing);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0F);
    // A multiple of real time per second; 3600 plays an hour of sky per second.
    ImGui::SliderScalar("SPEED", ImGuiDataType_Double, &time.speed, &kMinSpeed, &kMaxSpeed,
                        "%.0fx");

    ImGui::Text("SKY TIME: %s", app::FormatUtcTime(app::Effective(time, now)).c_str());
}

} // namespace

void DrawStarMapPanel(const std::vector<core::VisibleStar>& visibleStars,
                      const std::vector<core::VisibleConstellationLine>& constellationLines,
                      bool& showConstellationLines, bool& showStarLabels, app::StarMapTime& time,
                      std::chrono::system_clock::time_point now) {
    if (!ImGui::Begin("STAR MAP")) {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("CONSTELLATION LINES", &showConstellationLines);
    ImGui::SameLine();
    ImGui::Checkbox("STAR LABELS", &showStarLabels);
    DrawTimeControls(time, now);

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
    const ImU32 labelColor = ImGui::GetColorU32(ImGuiCol_Text, 0.85F);

    // Drawn well below the panel's own text size (there is no label collision handling here, so
    // smaller text packs tighter and overlaps less where stars cluster).
    ImFont* labelFont = ImGui::GetFont();
    constexpr float kLabelFontSize = 11.0F;
    const float labelLineHeight =
        kLabelFontSize / labelFont->LegacySize * ImGui::GetTextLineHeight();

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
        const ImVec2 p = Project(visible.position, origin, size);
        // The panel's own phosphor green (the same color source as the border and constellation
        // lines above), at magnitude-based brightness, not the star's true spectral color: this
        // is a green terminal display, not a color photo of the sky.
        const ImU32 starColor = ImGui::GetColorU32(ImGuiCol_Text, pointStyle.brightness);
        drawList->AddCircleFilled(p, pointStyle.radiusPx, starColor);

        if (showStarLabels) {
            const std::string label = core::StarLabel(visible.star);
            if (!label.empty()) {
                const ImVec2 labelPos = {p.x + pointStyle.radiusPx + 3.0F,
                                         p.y - 0.5F * labelLineHeight};
                drawList->AddText(labelFont, kLabelFontSize, labelPos, labelColor, label.c_str());
            }
        }
    }

    ImGui::Dummy(size);
    ImGui::End();
}

} // namespace ui
