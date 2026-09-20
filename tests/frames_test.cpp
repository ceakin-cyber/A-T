#include "core/frames.h"
#include "core/sgp4.h"
#include "core/time.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;
constexpr double kTwoPi = 2.0 * std::numbers::pi;

double Norm(const core::Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void ExpectVec(const core::Vec3& actual, const core::Vec3& expected, double tolerance) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
    EXPECT_NEAR(actual.z, expected.z, tolerance);
}

TEST(TemeToEcef, ZeroAngleIsTheIdentity) {
    ExpectVec(core::TemeToEcefFromGmst({1234.5, -6789.0, 4321.0}, 0.0), {1234.5, -6789.0, 4321.0},
              1e-12);
}

TEST(TemeToEcef, QuarterTurnMapsTheXAxisToMinusY) {
    // The Earth has turned 90 degrees east, so the inertial +X direction now points to the
    // ECEF -Y side.
    ExpectVec(core::TemeToEcefFromGmst({1.0, 0.0, 0.0}, kPi / 2.0), {0.0, -1.0, 0.0}, 1e-15);
    ExpectVec(core::TemeToEcefFromGmst({0.0, 1.0, 0.0}, kPi / 2.0), {1.0, 0.0, 0.0}, 1e-15);
}

TEST(TemeToEcef, HalfTurnFlipsXAndY) {
    ExpectVec(core::TemeToEcefFromGmst({3.0, -4.0, 5.0}, kPi), {-3.0, 4.0, 5.0}, 1e-12);
}

TEST(TemeToEcef, FullTurnIsTheIdentity) {
    ExpectVec(core::TemeToEcefFromGmst({3.0, -4.0, 5.0}, kTwoPi), {3.0, -4.0, 5.0}, 1e-12);
}

TEST(TemeToEcef, KnownAngle) {
    // A 30 degree rotation of (1, 0, 0): (cos 30, -sin 30, 0).
    ExpectVec(core::TemeToEcefFromGmst({1.0, 0.0, 0.0}, kPi / 6.0),
              {std::sqrt(3.0) / 2.0, -0.5, 0.0}, 1e-15);
}

TEST(TemeToEcef, PreservesLengthAndZ) {
    const core::Vec3 teme{7022.465, -1400.083, 3000.0};
    for (double angle = -7.0; angle < 7.0; angle += 0.4) {
        const core::Vec3 ecef = core::TemeToEcefFromGmst(teme, angle);
        EXPECT_NEAR(Norm(ecef), Norm(teme), 1e-9) << angle;
        EXPECT_DOUBLE_EQ(ecef.z, teme.z);
    }
}

TEST(TemeToEcef, RotationsCompose) {
    const core::Vec3 teme{5000.0, 4000.0, -3000.0};
    const core::Vec3 twoSteps = core::TemeToEcefFromGmst(core::TemeToEcefFromGmst(teme, 0.7), 1.1);
    ExpectVec(twoSteps, core::TemeToEcefFromGmst(teme, 1.8), 1e-9);
}

TEST(TemeToEcef, NegativeAngleIsTheInverse) {
    const core::Vec3 teme{-2500.0, 6100.0, 1500.0};
    const core::Vec3 ecef = core::TemeToEcefFromGmst(teme, 2.3);
    ExpectVec(core::TemeToEcefFromGmst(ecef, -2.3), teme, 1e-9);
}

TEST(TemeToEcef, UsesTheGreenwichSiderealTimeOfTheJulianDate) {
    const core::Vec3 teme{4000.0, 3000.0, 2000.0};
    const double jd = core::JulianDate({1987, 4, 10, 19, 21, 0.0});
    ExpectVec(core::TemeToEcef(teme, jd),
              core::TemeToEcefFromGmst(teme, core::GreenwichMeanSiderealTime(jd)), 1e-12);
}

TEST(TemeToEcef, TheEarthTurnsOnceEverySiderealDay) {
    // After one sidereal day (23h 56m 4.0905s) the frames line up again.
    const core::Vec3 teme{6000.0, 1000.0, 500.0};
    const double jd = 2458000.5;
    const double siderealDay = (23.0 * 3600.0 + 56.0 * 60.0 + 4.0905) / 86400.0;
    ExpectVec(core::TemeToEcef(teme, jd), core::TemeToEcef(teme, jd + siderealDay), 0.01);
}

TEST(TemeToEcef, IssFromPropagatorToEarthFixed) {
    // ISS elements from Celestrak, 2026-09-20.
    const auto tle =
        core::ParseTle("ISS (ZARYA)\n"
                       "1 25544U 98067A   26263.14255447  .00007470  00000+0  14267-3 0  9991\n"
                       "2 25544  51.6307 190.1401 0004820 160.6694 199.4478 15.49188396586472\n");
    ASSERT_TRUE(tle.has_value());
    const auto model = core::InitSgp4(*tle);
    ASSERT_TRUE(model.has_value());

    for (double minutes = 0.0; minutes <= 180.0; minutes += 20.0) {
        const auto state = core::Propagate(*model, minutes);
        ASSERT_TRUE(state.has_value());
        const double jd = model->epochJd + minutes / 1440.0;
        const core::Vec3 ecef = core::TemeToEcef(state->position, jd);

        // Length and height above the equatorial plane are unchanged...
        EXPECT_NEAR(Norm(ecef), Norm(state->position), 1e-9) << minutes;
        EXPECT_DOUBLE_EQ(ecef.z, state->position.z);

        // ...and the longitude has moved back by the sidereal angle.
        const double expected =
            std::atan2(state->position.y, state->position.x) - core::GreenwichMeanSiderealTime(jd);
        double difference = std::fmod(std::atan2(ecef.y, ecef.x) - expected, kTwoPi);
        if (difference > kPi) {
            difference -= kTwoPi;
        } else if (difference < -kPi) {
            difference += kTwoPi;
        }
        EXPECT_NEAR(difference, 0.0, 1e-12) << minutes;
    }
}

} // namespace
