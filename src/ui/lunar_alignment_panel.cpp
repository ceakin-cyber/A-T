#include "ui/lunar_alignment_panel.h"

#include "app/format.h"
#include "app/moon_disk.h"
#include "ui/rows.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace ui {

namespace {

// "NEXT": whichever of the next new/full moon comes first, as "<NAME> IN <countdown>". Neither
// present (both searches failed) falls back to a plain placeholder rather than showing nothing.
void NextEventLabelValueRow(double julianDateAsOf, std::optional<double> nextNewMoonJd,
                            std::optional<double> nextFullMoonJd) {
    const char* label = nullptr;
    std::optional<double> soonestJd;
    if (nextNewMoonJd.has_value() &&
        (!nextFullMoonJd.has_value() || *nextNewMoonJd <= *nextFullMoonJd)) {
        label = "NEW MOON";
        soonestJd = nextNewMoonJd;
    } else if (nextFullMoonJd.has_value()) {
        label = "FULL MOON";
        soonestJd = nextFullMoonJd;
    }

    if (!soonestJd.has_value()) {
        LabelValueRow("NEXT", "UNKNOWN");
        return;
    }

    const double daysUntil = *soonestJd - julianDateAsOf;
    const std::string countdown =
        app::FormatCountdown(std::chrono::duration<double>(daysUntil * 86400.0));
    LabelValueRow("NEXT", std::string(label) + " IN " + countdown);
}

// The Moon's large dark plains (maria), roughly placed and sized, as circles on a disk of radius 1
// seen from the northern hemisphere (x right, y up): enough to make the drawing read as the
// familiar face of the Moon rather than a plain disk, not a map of it.
struct Mare {
    double x;
    double y;
    double radius;
};
constexpr Mare kMaria[] = {
    {-0.55, 0.05, 0.30},  // Oceanus Procellarum
    {-0.30, 0.45, 0.22},  // Mare Imbrium
    {0.08, 0.42, 0.14},   // Mare Serenitatis
    {0.30, 0.15, 0.17},   // Mare Tranquillitatis
    {0.66, 0.30, 0.10},   // Mare Crisium
    {0.50, -0.15, 0.12},  // Mare Fecunditatis
    {0.30, -0.32, 0.08},  // Mare Nectaris
    {-0.15, -0.35, 0.15}, // Mare Nubium
};

// Draws the Moon as it looks tonight: a dim full disk (the unlit part, faintly visible as
// earthshine on a real Moon), with the lit part filled in over it one pixel row at a time (see
// app::MoonLitSpan) and its maria shaded in wherever they fall in the light, and a thin outline
// to smooth its edge. From the southern hemisphere the Moon is seen upside down, so the maria are
// turned round to match. Takes up a `diameter`-square space in the current layout.
void DrawMoonImage(const core::LunarPhase& phase, double observerLatitudeRad, float diameter) {
    const ImVec2 corner = ImGui::GetCursorScreenPos();
    ImGui::Dummy({diameter, diameter});

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float radius = diameter / 2.0F;
    const ImVec2 center = {corner.x + radius, corner.y + radius};
    const ImU32 darkColor = ImGui::GetColorU32(ImGuiCol_Text, 0.12F);
    const ImU32 litColor = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 edgeColor = ImGui::GetColorU32(ImGuiCol_Text, 0.5F);

    drawList->AddCircleFilled(center, radius, darkColor, 64);

    // A darker shade of the lit color, not a transparent one: drawn over the lit color, a
    // translucent copy of it would blend back to exactly the same color.
    const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    const ImU32 mareColor =
        ImGui::GetColorU32(ImVec4(text.x * 0.68F, text.y * 0.68F, text.z * 0.68F, text.w));
    const double flip = observerLatitudeRad < 0.0 ? -1.0 : 1.0;

    const bool litOnRight = app::MoonLitOnRight(phase.ageDays, observerLatitudeRad);
    const int rows = static_cast<int>(diameter);
    for (int row = 0; row < rows; ++row) {
        // Each row sampled through its middle, in the disk's own [-1, 1] units (y up).
        const double y = 1.0 - (row + 0.5) * 2.0 / rows;
        const app::LitSpan span = app::MoonLitSpan(y, phase.illuminatedFraction, litOnRight);
        if (span.right - span.left <= 0.0) {
            continue;
        }
        const float top = corner.y + static_cast<float>(row);
        drawList->AddRectFilled({center.x + static_cast<float>(span.left) * radius, top},
                                {center.x + static_cast<float>(span.right) * radius, top + 1.0F},
                                litColor);

        // Each mare's own slice at this height, shaded only where it overlaps the lit part.
        for (const Mare& mare : kMaria) {
            const double dy = y - flip * mare.y;
            if (std::abs(dy) >= mare.radius) {
                continue;
            }
            const double half = std::sqrt(mare.radius * mare.radius - dy * dy);
            const double left = std::max(flip * mare.x - half, span.left);
            const double right = std::min(flip * mare.x + half, span.right);
            if (right > left) {
                drawList->AddRectFilled(
                    {center.x + static_cast<float>(left) * radius, top},
                    {center.x + static_cast<float>(right) * radius, top + 1.0F}, mareColor);
            }
        }
    }
    drawList->AddCircle(center, radius, edgeColor, 64, 1.0F);
}

// How big the Moon is drawn: about as tall as the rows of text beside it.
constexpr float kMoonDiameter = 120.0F;

} // namespace

bool DrawLunarAlignmentPanel(const core::LunarPhase& phase, double julianDateAsOf,
                            std::optional<double> nextNewMoonJd,
                            std::optional<double> nextFullMoonJd,
                            net::Clock::time_point refreshedAt, double observerLatitudeRad) {
    // Docked into the dashboard's dockspace by default (see ui::DrawDockSpace); this position is
    // only used the first time the window ever appears, before it has a dock to fall into.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + 20.0F),
                            ImGuiCond_FirstUseEver);

    bool refreshClicked = false;
    if (ImGui::Begin("LUNAR ALIGNMENT", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        refreshClicked = ImGui::Button("REFRESH");

        DrawMoonImage(phase, observerLatitudeRad, kMoonDiameter);
        ImGui::SameLine(0.0F, 16.0F);

        if (ImGui::BeginTable("##fields", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
            ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 180.0F);

            LabelValueRow("PHASE", core::MoonPhaseName(phase.ageDays));
            LabelValueRow("ILLUM", app::FormatIlluminatedFraction(phase.illuminatedFraction));
            LabelValueRow("AGE",
                          app::FormatCountdown(std::chrono::duration<double>(phase.ageDays * 86400.0)));
            NextEventLabelValueRow(julianDateAsOf, nextNewMoonJd, nextFullMoonJd);
            LabelValueRow("REFRESHED", app::FormatTime(refreshedAt));

            ImGui::EndTable();
        }
    }
    ImGui::End();
    return refreshClicked;
}

} // namespace ui
