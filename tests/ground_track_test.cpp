#include "app/ground_track.h"
#include "core/frames.h"
#include "core/geodetic.h"
#include "core/sgp4.h"
#include "core/tle.h"

#include <chrono>
#include <gtest/gtest.h>
#include <numbers>

namespace {

using namespace std::chrono_literals;

constexpr double kPi = std::numbers::pi;

// ISS elements from Celestrak, 2026-09-20 (epoch JD 2461304.52959654).
const char* const kIss = "ISS (ZARYA)\n"
                         "1 25544U 98067A   26263.52959654  .00008422  00000+0  15975-3 0  9999\n"
                         "2 25544  51.6308 188.2246 0004825 162.2847 197.8311 15.49196792586535\n";

// A synthetic orbit with a perigee near 90 km, which drag destroys within hours.
const char* const kDecaying =
    "1 90002U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9992\n"
    "2 90002  51.6307 190.1401 0005000 160.6694 199.4478 16.67663952123456\n";

core::Sgp4Model Model(const char* text = kIss) {
    const auto tle = core::ParseTle(text);
    EXPECT_TRUE(tle.has_value());
    const auto model = core::InitSgp4(tle.value_or(core::Tle{}));
    EXPECT_TRUE(model.has_value());
    return model.value_or(core::Sgp4Model{});
}

net::Clock::time_point FromJulianDate(double julianDate) {
    const std::chrono::duration<double> sinceEpoch((julianDate - 2440587.5) * 86400.0);
    return net::Clock::time_point(std::chrono::duration_cast<net::Clock::duration>(sinceEpoch));
}

double Degrees(double radians) {
    return radians * 180.0 / kPi;
}

TEST(ComputeGroundTrack, ReturnsOnePointPerStepAcrossTheFullSpan) {
    const core::Sgp4Model model = Model();
    const auto track = app::ComputeGroundTrack(model, FromJulianDate(model.epochJd), 10min, 2min);
    // -10, -8, ..., 0, ..., 8, 10 minutes: 11 points.
    EXPECT_EQ(track.size(), 11U);
}

TEST(ComputeGroundTrack, PointsAreInTimeOrder) {
    // Longitude increases roughly monotonically (mod wraparound) over a short span for a
    // prograde low Earth orbit; check the simpler invariant that consecutive points are close
    // together rather than jumping around, which would indicate the wrong order.
    const core::Sgp4Model model = Model();
    const auto track = app::ComputeGroundTrack(model, FromJulianDate(model.epochJd), 10min, 2min);
    ASSERT_GE(track.size(), 2U);
    for (std::size_t i = 1; i < track.size(); ++i) {
        const double dLat = std::fabs(Degrees(track[i].latitude) - Degrees(track[i - 1].latitude));
        EXPECT_LT(dLat, 15.0) << i;
    }
}

TEST(ComputeGroundTrack, StaysWithinTheIssInclination) {
    const core::Sgp4Model model = Model();
    const auto track = app::ComputeGroundTrack(model, FromJulianDate(model.epochJd), 50min, 2min);
    for (const core::Geodetic& point : track) {
        EXPECT_LE(Degrees(point.latitude), 52.0);
        EXPECT_GE(Degrees(point.latitude), -52.0);
    }
}

TEST(ComputeGroundTrack, TheMiddlePointIsAtTheRequestedTime) {
    const core::Sgp4Model model = Model();
    const auto track = app::ComputeGroundTrack(model, FromJulianDate(model.epochJd), 10min, 2min);
    ASSERT_EQ(track.size(), 11U);
    const core::Geodetic expected = core::EcefToGeodetic(
        core::TemeToEcef(core::Propagate(model, 0.0)->position, model.epochJd));
    EXPECT_NEAR(track[5].latitude, expected.latitude, 1e-9);
    EXPECT_NEAR(track[5].longitude, expected.longitude, 1e-9);
}

TEST(ComputeGroundTrack, DoesNotCrashOnAZeroOrNegativeStep) {
    const core::Sgp4Model model = Model();
    EXPECT_TRUE(app::ComputeGroundTrack(model, FromJulianDate(model.epochJd), 10min, 0min).empty());
    EXPECT_TRUE(
        app::ComputeGroundTrack(model, FromJulianDate(model.epochJd), 10min, -2min).empty());
}

TEST(ComputeGroundTrack, SkipsPointsPastDecayInsteadOfCrashing) {
    const core::Sgp4Model model = Model(kDecaying);
    // Far enough out that the orbit has already decayed for at least some of the span.
    const auto track =
        app::ComputeGroundTrack(model, FromJulianDate(model.epochJd + 1.0), 30min, 2min);
    EXPECT_LT(track.size(), 31U);
}

TEST(CrossesAntimeridian, DetectsALargeJump) {
    EXPECT_TRUE(app::CrossesAntimeridian(179.0, -179.0));
    EXPECT_TRUE(app::CrossesAntimeridian(-170.0, 170.0));
}

TEST(CrossesAntimeridian, DoesNotFlagAnOrdinaryStep) {
    EXPECT_FALSE(app::CrossesAntimeridian(10.0, 12.0));
    EXPECT_FALSE(app::CrossesAntimeridian(-5.0, -8.0));
    EXPECT_FALSE(app::CrossesAntimeridian(0.0, 0.0));
}

TEST(CrossesAntimeridian, TheBoundaryItselfIsNotFlagged) {
    EXPECT_FALSE(app::CrossesAntimeridian(90.0, -90.0));
    EXPECT_FALSE(app::CrossesAntimeridian(-180.0, 0.0));
}

} // namespace
