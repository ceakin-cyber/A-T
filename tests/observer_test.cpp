#include "app/observer.h"
#include "core/topocentric.h"

#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;

TEST(GeodeticFromDegrees, ConvertsToRadians) {
    const core::Geodetic g = core::GeodeticFromDegrees(90.0, -180.0, 1.5);
    EXPECT_NEAR(g.latitude, kPi / 2.0, 1e-15);
    EXPECT_NEAR(g.longitude, -kPi, 1e-15);
    EXPECT_DOUBLE_EQ(g.altitudeKm, 1.5);
}

TEST(GeodeticFromDegrees, IsUsableInAConstantExpression) {
    constexpr core::Geodetic g = core::GeodeticFromDegrees(10.0, 20.0, 0.0);
    static_assert(g.latitude > 0.17 && g.latitude < 0.18);
    EXPECT_GT(g.longitude, 0.0);
}

TEST(Observer, IsAValidPlaceOnEarth) {
    EXPECT_GE(app::kObserver.latitude, -kPi / 2.0);
    EXPECT_LE(app::kObserver.latitude, kPi / 2.0);
    EXPECT_GT(app::kObserver.longitude, -kPi);
    EXPECT_LE(app::kObserver.longitude, kPi);
    // Within the range of surface heights on Earth, in km.
    EXPECT_GT(app::kObserver.altitudeKm, -0.5);
    EXPECT_LT(app::kObserver.altitudeKm, 9.0);
}

TEST(Observer, IsGreenwichUntilItIsChanged) {
    EXPECT_NEAR(app::kObserver.latitude * 180.0 / kPi, 51.4779, 1e-9);
    EXPECT_NEAR(app::kObserver.longitude * 180.0 / kPi, 0.0, 1e-12);
    EXPECT_NEAR(app::kObserver.altitudeKm, 0.062, 1e-12);
}

TEST(Observer, SeesASatelliteDirectlyOverheadAtZenith) {
    // The observer works as the site argument of the topocentric transform.
    core::Geodetic above = app::kObserver;
    above.altitudeKm += 420.0;
    const core::LookAngles look =
        core::EcefToLookAngles(app::kObserver, core::GeodeticToEcef(above));
    EXPECT_NEAR(look.elevation * 180.0 / kPi, 90.0, 1e-8);
    EXPECT_NEAR(look.rangeKm, 420.0, 1e-8);
}

} // namespace
