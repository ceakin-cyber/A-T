#include "app/ground_track.h"
#include "app/observer.h"
#include "app/observing.h"
#include "core/passes.h"
#include "core/sun.h"
#include "core/tle.h"

#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;
constexpr double kSecondsPerDay = 86400.0;

// ISS elements from Celestrak, 2026-09-20 (epoch JD 2461304.02959654).
const char* const kIss = "ISS (ZARYA)\n"
                         "1 25544U 98067A   26263.52959654  .00008422  00000+0  15975-3 0  9999\n"
                         "2 25544  51.6308 188.2246 0004825 162.2847 197.8311 15.49196792586535\n";
constexpr double kEpochJd = 2461304.02959654;

core::Sgp4Model Model() {
    const auto tle = core::ParseTle(kIss);
    EXPECT_TRUE(tle.has_value());
    const auto model = core::InitSgp4(tle.value_or(core::Tle{}));
    EXPECT_TRUE(model.has_value());
    return model.value_or(core::Sgp4Model{});
}

double Radians(double degrees) {
    return degrees * kPi / 180.0;
}

double Degrees(double radians) {
    return radians * 180.0 / kPi;
}

net::Clock::time_point FromJulianDate(double julianDate) {
    const std::chrono::duration<double> sinceEpoch((julianDate - 2440587.5) * 86400.0);
    return net::Clock::time_point(std::chrono::duration_cast<net::Clock::duration>(sinceEpoch));
}

TEST(ClassifyVisibility, VisibleWhenUpInADarkSkyAndSunlit) {
    EXPECT_EQ(app::ClassifyVisibility(Radians(40.0), Radians(-10.0), true),
              app::Visibility::Visible);
}

TEST(ClassifyVisibility, GivesTheFirstReasonThatFails) {
    // Below the horizon wins over everything else...
    EXPECT_EQ(app::ClassifyVisibility(Radians(-5.0), Radians(30.0), false),
              app::Visibility::BelowHorizon);
    // ...then a bright sky...
    EXPECT_EQ(app::ClassifyVisibility(Radians(40.0), Radians(30.0), false),
              app::Visibility::Daylight);
    // ...then the Earth's shadow.
    EXPECT_EQ(app::ClassifyVisibility(Radians(40.0), Radians(-30.0), false),
              app::Visibility::InShadow);
}

TEST(ClassifyVisibility, CivilTwilightIsTheCutoffForADarkEnoughSky) {
    EXPECT_EQ(app::ClassifyVisibility(Radians(40.0), Radians(-5.9), true),
              app::Visibility::Daylight);
    EXPECT_EQ(app::ClassifyVisibility(Radians(40.0), Radians(-6.1), true),
              app::Visibility::Visible);
}

TEST(VisibilityToString, NamesEachState) {
    EXPECT_STREQ(app::ToString(app::Visibility::Visible), "VISIBLE");
    EXPECT_STREQ(app::ToString(app::Visibility::BelowHorizon), "BELOW HORIZON");
    EXPECT_STREQ(app::ToString(app::Visibility::Daylight), "DAYLIGHT");
    EXPECT_STREQ(app::ToString(app::Visibility::InShadow), "IN SHADOW");
}

TEST(CompassPoint, RoundsToTheNearestOfEight) {
    EXPECT_STREQ(app::CompassPoint(Radians(0.0)), "N");
    EXPECT_STREQ(app::CompassPoint(Radians(20.0)), "N");
    EXPECT_STREQ(app::CompassPoint(Radians(44.0)), "NE");
    EXPECT_STREQ(app::CompassPoint(Radians(90.0)), "E");
    EXPECT_STREQ(app::CompassPoint(Radians(135.0)), "SE");
    EXPECT_STREQ(app::CompassPoint(Radians(200.0)), "S");
    EXPECT_STREQ(app::CompassPoint(Radians(240.0)), "SW");
    EXPECT_STREQ(app::CompassPoint(Radians(275.0)), "W");
    EXPECT_STREQ(app::CompassPoint(Radians(310.0)), "NW");
    EXPECT_STREQ(app::CompassPoint(Radians(355.0)), "N"); // wraps back round to north
}

TEST(SunElevationAt, IsOverheadAtTheSubsolarPointAndUnderfootOppositeIt) {
    const double jd = 2461309.0; // 2026-09-25 12:00 UTC
    const core::Geodetic subsolar = core::SubsolarPoint(jd);
    EXPECT_NEAR(Degrees(app::SunElevationAt(subsolar, jd)), 90.0, 0.1);
    const core::Geodetic antipode = {-subsolar.latitude, subsolar.longitude + kPi, 0.0};
    EXPECT_NEAR(Degrees(app::SunElevationAt(antipode, jd)), -90.0, 0.1);
}

TEST(LookAnglesAt, AgreesWithThePassFinder) {
    const core::Sgp4Model model = Model();
    for (int minute = 0; minute < 180; minute += 7) {
        const double jd = kEpochJd + minute / 1440.0;
        const std::optional<core::LookAngles> look = app::LookAnglesAt(model, app::kObserver, jd);
        const std::optional<double> elevation = core::ElevationAt(model, app::kObserver, jd);
        ASSERT_TRUE(look.has_value());
        ASSERT_TRUE(elevation.has_value());
        EXPECT_NEAR(look->elevation, *elevation, 1e-12);
    }
}

// Over a day of real ISS passes from a handful of places, any visible window found must sit
// inside its pass, be Visible at both ends, and not be Visible just outside them (unless that
// is past the edge of the pass); and a pass with no window must never be Visible.
TEST(FindVisibleWindow, MatchesSamplingVisibilityDirectly) {
    const core::Sgp4Model model = Model();
    const core::Geodetic observers[] = {app::kObserver, core::GeodeticFromDegrees(40.7, -74.0, 0),
                                        core::GeodeticFromDegrees(-33.9, 151.2, 0),
                                        core::GeodeticFromDegrees(35.7, 139.7, 0)};
    int visiblePasses = 0;
    int invisiblePasses = 0;
    constexpr double kStepSeconds = 5.0;
    for (const core::Geodetic& observer : observers) {
        for (const core::Pass& pass : core::FindPasses(model, observer, kEpochJd, kEpochJd + 1.0)) {
            const std::optional<app::VisibleWindow> window =
                app::FindVisibleWindow(model, observer, pass, kStepSeconds);
            if (!window) {
                ++invisiblePasses;
                for (double jd = pass.riseJd + 1.0 / kSecondsPerDay; jd < pass.setJd;
                     jd += 3.0 / kSecondsPerDay) {
                    EXPECT_NE(app::VisibilityAt(model, observer, jd), app::Visibility::Visible);
                }
                continue;
            }
            ++visiblePasses;
            EXPECT_GE(window->startJd, pass.riseJd);
            EXPECT_LE(window->endJd, pass.setJd);
            EXPECT_LE(window->startJd, window->endJd);
            EXPECT_GT(window->maxElevationRad, 0.0);
            EXPECT_LE(window->maxElevationRad, pass.maxElevation + 1e-9);
            EXPECT_EQ(app::VisibilityAt(model, observer, window->startJd),
                      app::Visibility::Visible);
            EXPECT_EQ(app::VisibilityAt(model, observer, window->endJd), app::Visibility::Visible);
            const double step = kStepSeconds / kSecondsPerDay;
            if (window->startJd - step > pass.riseJd) {
                EXPECT_NE(app::VisibilityAt(model, observer, window->startJd - step),
                          app::Visibility::Visible);
            }
            if (window->endJd + step < pass.setJd) {
                EXPECT_NE(app::VisibilityAt(model, observer, window->endJd + step),
                          app::Visibility::Visible);
            }
        }
    }
    // The sample must actually exercise both outcomes to mean anything.
    EXPECT_GT(visiblePasses, 0);
    EXPECT_GT(invisiblePasses, 0);
}

TEST(FindVisibleWindow, NothingForAnEmptyOrInvertedPass) {
    const core::Sgp4Model model = Model();
    core::Pass pass;
    pass.riseJd = kEpochJd + 0.1;
    pass.setJd = kEpochJd + 0.1;
    EXPECT_FALSE(app::FindVisibleWindow(model, app::kObserver, pass).has_value());
    pass.setJd = kEpochJd;
    EXPECT_FALSE(app::FindVisibleWindow(model, app::kObserver, pass).has_value());
}

TEST(FindNextVisiblePass, IsTheFirstPassWithAVisibleWindow) {
    const core::Sgp4Model model = Model();
    const core::Geodetic observer = core::GeodeticFromDegrees(40.7, -74.0, 0.0);
    const std::optional<app::VisiblePass> next =
        app::FindNextVisiblePass(model, observer, kEpochJd, 3.0);
    ASSERT_TRUE(next.has_value());
    EXPECT_GE(next->window.startJd, next->pass.riseJd);
    EXPECT_LE(next->window.endJd, next->pass.setJd);
    // Every pass before it has no visible part.
    for (const core::Pass& pass : core::FindPasses(model, observer, kEpochJd, kEpochJd + 3.0)) {
        if (pass.riseJd >= next->pass.riseJd) {
            break;
        }
        EXPECT_FALSE(app::FindVisibleWindow(model, observer, pass).has_value());
    }
}

TEST(FindNextVisiblePass, CountsOneStillGoingOnButNotOneAlreadyOver) {
    const core::Sgp4Model model = Model();
    const core::Geodetic observer = core::GeodeticFromDegrees(40.7, -74.0, 0.0);
    const std::optional<app::VisiblePass> first =
        app::FindNextVisiblePass(model, observer, kEpochJd, 3.0);
    ASSERT_TRUE(first.has_value());
    const double middle = (first->window.startJd + first->window.endJd) / 2.0;
    const std::optional<app::VisiblePass> during =
        app::FindNextVisiblePass(model, observer, middle, 3.0);
    ASSERT_TRUE(during.has_value());
    EXPECT_NEAR(during->window.endJd, first->window.endJd, 10.0 / kSecondsPerDay);
    const std::optional<app::VisiblePass> after =
        app::FindNextVisiblePass(model, observer, first->pass.setJd, 3.0);
    if (after) {
        EXPECT_GT(after->window.startJd, first->pass.setJd);
    }
}

TEST(ComputeObservedGroundTrack, CoversTheSpanAtTheGivenStep) {
    using namespace std::chrono_literals;
    const std::vector<app::GroundTrackPoint> track = app::ComputeObservedGroundTrack(
        Model(), app::kObserver, FromJulianDate(kEpochJd), 10min, 20min, 60s);
    ASSERT_EQ(track.size(), 31U);
    EXPECT_NEAR(track.front().julianDate, kEpochJd - 10.0 / 1440.0, 1e-9);
    EXPECT_NEAR(track.back().julianDate, kEpochJd + 20.0 / 1440.0, 1e-9);
}

TEST(ComputeObservedGroundTrack, MarksEachPointAsTheObserverWouldSeeIt) {
    const core::Sgp4Model model = Model();
    const std::vector<app::GroundTrackPoint> track =
        app::ComputeObservedGroundTrack(model, app::kObserver, FromJulianDate(kEpochJd));
    ASSERT_FALSE(track.empty());
    int above = 0;
    int shadowed = 0;
    for (const app::GroundTrackPoint& point : track) {
        const std::optional<double> elevation =
            core::ElevationAt(model, app::kObserver, point.julianDate);
        ASSERT_TRUE(elevation.has_value());
        EXPECT_EQ(point.aboveHorizon, *elevation > 0.0);
        above += point.aboveHorizon ? 1 : 0;
        shadowed += point.sunlit ? 0 : 1;
    }
    // Nearly four hours of low Earth orbit: some of it over Greenwich, some in shadow, not all.
    EXPECT_GT(above, 0);
    EXPECT_LT(above, static_cast<int>(track.size()));
    EXPECT_GT(shadowed, 0);
    EXPECT_LT(shadowed, static_cast<int>(track.size()));
}

} // namespace
