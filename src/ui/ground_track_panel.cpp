#include "ui/ground_track_panel.h"

#include "app/format.h"
#include "app/ground_track.h"
#include "app/observing.h"
#include "core/sun.h"
#include "core/time.h"
#include "ui/rows.h"
#include "ui/style.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <imgui.h>
#include <numbers>
#include <optional>
#include <string>

namespace ui {

namespace {

using Seconds = std::chrono::duration<double>;

constexpr double kRadToDeg = 180.0 / std::numbers::pi;
constexpr double kDegToRad = std::numbers::pi / 180.0;

// How much of the track to draw: a short tail behind, to show which way the satellite is going,
// and one orbit or so ahead (about 93 minutes for the ISS), with a time mark every half hour for
// the first hour.
constexpr std::chrono::minutes kTrackBehind(15);
constexpr std::chrono::minutes kTrackAhead(100);
constexpr double kTimeMarkMinutes = 30.0;

// How far ahead to look for the next time the satellite can be seen by eye, and how often to
// look again (the search is too slow to redo every frame; see app::FindNextVisiblePass).
constexpr double kSightingSearchDays = 3.0;
constexpr double kSightingRefreshDays = 10.0 / 1440.0;

// A pass that peaks lower than this is described as just skimming the horizon: trees and
// buildings usually hide it, so quoting a direction and height would only mislead.
constexpr double kLowPassRad = 10.0 * std::numbers::pi / 180.0;

// Equirectangular projection: longitude in [-180, 180] to x in [0, width], latitude in
// [-90, 90] to y in [0, height] (flipped, since screen y grows downward).
ImVec2 Project(const core::Geodetic& point, ImVec2 origin, ImVec2 size) {
    const double lonDeg = point.longitude * kRadToDeg;
    const double latDeg = point.latitude * kRadToDeg;
    return {origin.x + static_cast<float>((lonDeg + 180.0) / 360.0) * size.x,
            origin.y + static_cast<float>((90.0 - latDeg) / 180.0) * size.y};
}

ImU32 WithAlpha(const ImVec4& color, float alpha) {
    return ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, color.w * alpha));
}

// Shades the half of the Earth where the Sun is down. Worked out column by column: each strip
// of longitude is tested in 1-degree steps of latitude, and each run of night within it drawn as
// one rectangle, which follows the day/night line's curve closely without needing its formula
// (which degenerates at the equinoxes, when the line runs straight from pole to pole).
void DrawNightSide(ImDrawList* drawList, double julianDate, ImVec2 origin, ImVec2 size,
                   ImU32 color) {
    const core::Vec3 sun = core::SunDirectionEcef(julianDate);
    constexpr int kLonStepDeg = 2;
    constexpr int kLatStepDeg = 1;
    for (int lonDeg = -180; lonDeg < 180; lonDeg += kLonStepDeg) {
        const double lon = (lonDeg + kLonStepDeg / 2.0) * kDegToRad;
        const float x0 = origin.x + static_cast<float>(lonDeg + 180) / 360.0F * size.x;
        const float x1 =
            origin.x + static_cast<float>(lonDeg + kLonStepDeg + 180) / 360.0F * size.x;
        int runStartDeg = 0;
        bool inRun = false;
        for (int latDeg = 90; latDeg >= -90; latDeg -= kLatStepDeg) {
            bool night = false;
            if (latDeg > -90) {
                const double lat = (latDeg - kLatStepDeg / 2.0) * kDegToRad;
                night = std::cos(lat) * std::cos(lon) * sun.x +
                            std::cos(lat) * std::sin(lon) * sun.y + std::sin(lat) * sun.z <
                        0.0;
            }
            if (night && !inRun) {
                runStartDeg = latDeg;
                inRun = true;
            } else if (!night && inRun) {
                const float y0 = origin.y + static_cast<float>(90 - runStartDeg) / 180.0F * size.y;
                const float y1 = origin.y + static_cast<float>(90 - latDeg) / 180.0F * size.y;
                drawList->AddRectFilled({x0, y0}, {x1, y1}, color);
                inRun = false;
            }
        }
    }
}

// The track: faint behind the satellite, solid ahead of it, and thick amber wherever it will be
// above the observer's horizon -- the one thing on the map that matters most to them.
void DrawTrack(ImDrawList* drawList, const std::vector<app::GroundTrackPoint>& track, double nowJd,
               ImVec2 origin, ImVec2 size) {
    const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    for (std::size_t i = 1; i < track.size(); ++i) {
        const app::GroundTrackPoint& a = track[i - 1];
        const app::GroundTrackPoint& b = track[i];
        if (app::CrossesAntimeridian(a.geodetic.longitude * kRadToDeg,
                                     b.geodetic.longitude * kRadToDeg)) {
            continue;
        }
        const ImVec2 pa = Project(a.geodetic, origin, size);
        const ImVec2 pb = Project(b.geodetic, origin, size);
        if (b.julianDate <= nowJd) {
            drawList->AddLine(pa, pb, WithAlpha(text, 0.3F), 1.5F);
        } else if (a.aboveHorizon && b.aboveHorizon) {
            drawList->AddLine(pa, pb, ImGui::GetColorU32(WarningColor()), 3.5F);
        } else {
            drawList->AddLine(pa, pb, WithAlpha(text, 0.8F), 1.5F);
        }
    }

    // A dot and a label every half hour ahead, so it is clear how soon it gets anywhere.
    const ImU32 markColor = WithAlpha(text, 0.9F);
    // Only up to an hour: by the next mark, an orbit on, the satellite is nearly back where it
    // started, and the mark would sit on top of it.
    for (double minutes = kTimeMarkMinutes; minutes <= 60.0; minutes += kTimeMarkMinutes) {
        const double jd = nowJd + minutes / 1440.0;
        const app::GroundTrackPoint* nearest = nullptr;
        for (const app::GroundTrackPoint& point : track) {
            if (nearest == nullptr ||
                std::abs(point.julianDate - jd) < std::abs(nearest->julianDate - jd)) {
                nearest = &point;
            }
        }
        if (nearest == nullptr) {
            break;
        }
        const ImVec2 p = Project(nearest->geodetic, origin, size);
        drawList->AddCircleFilled(p, 2.5F, markColor);
        const int wholeMinutes = static_cast<int>(minutes);
        char label[16];
        if (wholeMinutes % 60 == 0) {
            std::snprintf(label, sizeof label, "+%dH", wholeMinutes / 60);
        } else if (wholeMinutes > 60) {
            std::snprintf(label, sizeof label, "+%dH%02d", wholeMinutes / 60, wholeMinutes % 60);
        } else {
            std::snprintf(label, sizeof label, "+%dM", wholeMinutes);
        }
        drawList->AddText({p.x + 4.0F, p.y - ImGui::GetTextLineHeight() - 1.0F}, markColor, label);
    }
}

// A marker's text label, e.g. the observer ("YOU") or the satellite's name: to the right of the
// marker, or to its left if it would otherwise run off the map's right edge (`rightEdge`).
void LabelAt(ImDrawList* drawList, ImVec2 p, const char* text, ImU32 color, float rightEdge) {
    const float width = ImGui::CalcTextSize(text).x;
    const float x = p.x + 8.0F + width > rightEdge ? p.x - 8.0F - width : p.x + 8.0F;
    drawList->AddText({x, p.y - ImGui::GetTextLineHeight() / 2.0F}, color, text);
}

// One sentence row of the readout: a dim label and a value that wraps rather than running off
// the edge of the panel.
void SentenceRow(const char* label, const std::string& value, const ImVec4* color = nullptr) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextDisabled("%s", label);
    ImGui::TableSetColumnIndex(1);
    if (color != nullptr) {
        ImGui::PushStyleColor(ImGuiCol_Text, *color);
    }
    ImGui::TextWrapped("%s", value.c_str());
    if (color != nullptr) {
        ImGui::PopStyleColor();
    }
}

std::string Degrees(double radians) {
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "%.0f DEG", radians * kRadToDeg);
    return buffer;
}

// "SW, 34 DEG UP": which way to face, and how high to look.
std::string WhereToLook(const core::LookAngles& look) {
    return std::string(app::CompassPoint(look.azimuth)) + ", " + Degrees(look.elevation) + " UP";
}

std::string At(double julianDate, net::Clock::time_point now) {
    return app::FormatUtcClock(core::TimePointFromJulianDate(julianDate), now);
}

std::string In(double julianDate, net::Clock::time_point now) {
    return app::FormatCountdown(Seconds(core::TimePointFromJulianDate(julianDate) - now));
}

// The next time the satellite can be seen by eye, looked up again only every few minutes, when
// a different satellite is selected, or once the cached sighting is over.
const std::optional<app::VisiblePass>& NextSighting(const app::WatchedSatellite& watched,
                                                    const core::Geodetic& observer, double nowJd) {
    static int cachedId = -1;
    static double cachedAtJd = 0.0;
    static std::optional<app::VisiblePass> cached;
    const bool stale = watched.entry.noradId != cachedId ||
                       nowJd - cachedAtJd > kSightingRefreshDays ||
                       (cached && cached->window.endJd < nowJd);
    if (stale) {
        cached = app::FindNextVisiblePass(watched.satellite->model, observer, nowJd,
                                          kSightingSearchDays);
        cachedId = watched.entry.noradId;
        cachedAtJd = nowJd;
    }
    return cached;
}

// The readout above the map, in plain sentences: is it overhead now, when does it next come
// over, and when can it next actually be seen.
void DrawReadout(const app::WatchedSatellite& watched, const core::Geodetic& observer,
                 net::Clock::time_point now) {
    const core::Sgp4Model& model = watched.satellite->model;
    const double nowJd = core::JulianDateFromTimePoint(now);
    const ImVec4 highlight = WarningColor();

    ImGui::TextUnformatted(watched.satellite->tle.name.c_str());
    if (!ImGui::BeginTable("##readout", 2, ImGuiTableFlags_SizingFixedFit)) {
        return;
    }
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    const std::optional<core::LookAngles> look = app::LookAnglesAt(model, observer, nowJd);
    const std::optional<app::Visibility> visibility = app::VisibilityAt(model, observer, nowJd);
    if (!look || !visibility) {
        SentenceRow("RIGHT NOW", "POSITION UNAVAILABLE");
    } else {
        switch (*visibility) {
        case app::Visibility::Visible:
            SentenceRow("RIGHT NOW", "VISIBLE! LOOK " + WhereToLook(*look), &highlight);
            break;
        case app::Visibility::Daylight:
            SentenceRow("RIGHT NOW", "ABOVE YOU (" + WhereToLook(*look) +
                                         "), BUT THE SKY IS TOO BRIGHT TO SEE IT");
            break;
        case app::Visibility::InShadow:
            SentenceRow("RIGHT NOW", "ABOVE YOU (" + WhereToLook(*look) +
                                         "), BUT IN EARTH'S SHADOW, SO NOT LIT UP");
            break;
        case app::Visibility::BelowHorizon:
            SentenceRow("RIGHT NOW", "NOT ABOVE YOU");
            break;
        }
    }

    if (!watched.nextPass) {
        SentenceRow("NEXT PASS", "NONE IN THE NEXT 2 DAYS");
    } else {
        const core::Pass& pass = *watched.nextPass;
        if (pass.riseJd <= nowJd) {
            SentenceRow("NEXT PASS", "HAPPENING NOW, UNTIL " + At(pass.setJd, now));
        } else {
            std::string sentence = "IN " + In(pass.riseJd, now) + ", AT " + At(pass.riseJd, now);
            const std::optional<core::LookAngles> rise =
                app::LookAnglesAt(model, observer, pass.riseJd);
            const std::optional<core::LookAngles> set =
                app::LookAnglesAt(model, observer, pass.setJd);
            const std::string from = rise ? app::CompassPoint(rise->azimuth) : "";
            const std::string to = set ? app::CompassPoint(set->azimuth) : "";
            if (pass.maxElevation < kLowPassRad) {
                // Too low to be worth looking for: say so plainly rather than quote a direction
                // and a height of a degree or two.
                sentence += ". ONLY SKIMS THE HORIZON";
                if (!from.empty()) {
                    sentence += " IN THE " + from;
                }
            } else {
                if (!from.empty() && !to.empty()) {
                    sentence += from == to ? ". LOW IN THE " + from
                                           : ". CROSSES FROM " + from + " TO " + to;
                }
                sentence += ", UP TO " + Degrees(pass.maxElevation);
            }
            SentenceRow("NEXT PASS", sentence);
        }
    }

    const std::optional<app::VisiblePass>& sighting = NextSighting(watched, observer, nowJd);
    if (!sighting) {
        SentenceRow("SEE IT BY EYE", "NOT IN THE NEXT 3 DAYS");
    } else if (sighting->window.startJd <= nowJd) {
        SentenceRow("SEE IT BY EYE", "NOW, UNTIL " + At(sighting->window.endJd, now), &highlight);
    } else {
        std::string sentence =
            "IN " + In(sighting->window.startJd, now) + ", AT " + At(sighting->window.startJd, now);
        if (const std::optional<core::LookAngles> start =
                app::LookAnglesAt(model, observer, sighting->window.startJd)) {
            sentence += std::string(". LOOK ") + app::CompassPoint(start->azimuth);
        }
        sentence += ", UP TO " + Degrees(sighting->window.maxElevationRad);
        SentenceRow("SEE IT BY EYE", sentence, &highlight);
    }
    ImGui::EndTable();
}

} // namespace

void DrawGroundTrackPanel(const app::WatchedSatellite* watched, const core::Geodetic& observer,
                          const LandMesh& land, net::Clock::time_point now) {
    if (!ImGui::Begin("GROUND TRACK")) {
        ImGui::End();
        return;
    }

    if (watched == nullptr || !watched->satellite) {
        PlaceholderText("NO DATA");
        ImGui::End();
        return;
    }

    DrawReadout(*watched, observer, now);
    ImGui::Spacing();

    // Fit the largest 2:1 (lon:lat) map into what is left, leaving a line for the note under it.
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    float width = avail.x;
    float height = width / 2.0F;
    const float maxHeight = avail.y - ImGui::GetTextLineHeightWithSpacing();
    if (height > maxHeight) {
        height = maxHeight;
        width = height * 2.0F;
    }
    if (width < 2.0F || height < 2.0F) {
        ImGui::End();
        return;
    }

    const double nowJd = core::JulianDateFromTimePoint(now);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size(width, height);
    const ImVec2 corner(origin.x + size.x, origin.y + size.y);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    drawList->PushClipRect(origin, corner, true);
    DrawLandMesh(drawList, land, origin, size, WithAlpha(text, 0.16F),
                 ImGui::GetColorU32(ImGuiCol_WindowBg));
    DrawNightSide(drawList, nowJd, origin, size, ImGui::GetColorU32(ImVec4(0, 0, 0, 0.4F)));

    DrawTrack(drawList,
              app::ComputeObservedGroundTrack(watched->satellite->model, observer, now,
                                              kTrackBehind, kTrackAhead),
              nowJd, origin, size);

    const ImU32 observerColor = ImGui::GetColorU32(WarningColor());
    const ImVec2 you = Project(observer, origin, size);
    drawList->AddCircle(you, 5.0F, observerColor, 12, 1.5F);
    drawList->AddLine({you.x - 7.0F, you.y}, {you.x + 7.0F, you.y}, observerColor);
    drawList->AddLine({you.x, you.y - 7.0F}, {you.x, you.y + 7.0F}, observerColor);
    LabelAt(drawList, you, "YOU", observerColor, corner.x);

    if (watched->position) {
        const ImU32 satelliteColor = ImGui::GetColorU32(text);
        const ImVec2 p = Project(watched->position->geodetic, origin, size);
        drawList->AddCircleFilled(p, 5.0F, satelliteColor);
        // Above the dot rather than beside it, where the track itself runs.
        const char* name = watched->satellite->tle.name.c_str();
        const ImVec2 nameSize = ImGui::CalcTextSize(name);
        const float nameX =
            std::clamp(p.x - nameSize.x / 2.0F, origin.x + 2.0F, corner.x - nameSize.x - 2.0F);
        drawList->AddText({nameX, p.y - 8.0F - nameSize.y}, satelliteColor, name);
    }
    drawList->PopClipRect();
    drawList->AddRect(origin, corner, WithAlpha(text, 0.6F));
    ImGui::Dummy(size);

    ImGui::TextDisabled("AMBER: WHERE IT PASSES OVER YOU.  DARK: NIGHT.  +30M, +1H: WHERE IT "
                        "WILL BE THEN.");
    ImGui::End();
}

} // namespace ui
