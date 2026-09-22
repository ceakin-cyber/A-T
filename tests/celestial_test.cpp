#include "core/celestial.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>
#include <vector>

namespace {

constexpr double kPi = std::numbers::pi;

double Radians(double degrees) {
    return degrees * kPi / 180.0;
}

double Norm(const core::Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void ExpectVec(const core::Vec3& actual, const core::Vec3& expected, double tolerance) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
    EXPECT_NEAR(actual.z, expected.z, tolerance);
}

TEST(EquatorialToUnitVector, VernalEquinoxIsPlusX) {
    ExpectVec(core::EquatorialToUnitVector(0.0, 0.0), {1.0, 0.0, 0.0}, 1e-15);
}

TEST(EquatorialToUnitVector, NinetyDegreesRaOnTheEquatorIsPlusY) {
    ExpectVec(core::EquatorialToUnitVector(kPi / 2.0, 0.0), {0.0, 1.0, 0.0}, 1e-15);
}

TEST(EquatorialToUnitVector, OneEightyDegreesRaOnTheEquatorIsMinusX) {
    ExpectVec(core::EquatorialToUnitVector(kPi, 0.0), {-1.0, 0.0, 0.0}, 1e-15);
}

TEST(EquatorialToUnitVector, TwoSeventyDegreesRaOnTheEquatorIsMinusY) {
    ExpectVec(core::EquatorialToUnitVector(3.0 * kPi / 2.0, 0.0), {0.0, -1.0, 0.0}, 1e-15);
}

TEST(EquatorialToUnitVector, NorthCelestialPoleIsPlusZRegardlessOfRa) {
    for (const double raDeg : {0.0, 45.0, 123.0, 359.0}) {
        ExpectVec(core::EquatorialToUnitVector(Radians(raDeg), kPi / 2.0), {0.0, 0.0, 1.0}, 1e-12);
    }
}

TEST(EquatorialToUnitVector, SouthCelestialPoleIsMinusZRegardlessOfRa) {
    for (const double raDeg : {0.0, 90.0, 200.0}) {
        ExpectVec(core::EquatorialToUnitVector(Radians(raDeg), -kPi / 2.0), {0.0, 0.0, -1.0},
                  1e-12);
    }
}

TEST(EquatorialToUnitVector, IsAlwaysUnitLength) {
    for (double raDeg = 0.0; raDeg < 360.0; raDeg += 37.0) {
        for (double decDeg = -90.0; decDeg <= 90.0; decDeg += 23.0) {
            const core::Vec3 v = core::EquatorialToUnitVector(Radians(raDeg), Radians(decDeg));
            EXPECT_NEAR(Norm(v), 1.0, 1e-12) << "ra=" << raDeg << " dec=" << decDeg;
        }
    }
}

TEST(EquatorialToUnitVector, ACoordinateOnTheEquatorHasZeroZ) {
    for (const double raDeg : {10.0, 130.0, 250.0}) {
        EXPECT_NEAR(core::EquatorialToUnitVector(Radians(raDeg), 0.0).z, 0.0, 1e-15);
    }
}

// A few well-known bright stars, J2000 coordinates from published catalog values. Expected unit
// vectors were computed independently in Python from those coordinates, not from this code's
// own output.
struct KnownStar {
    const char* name;
    double raDeg;
    double decDeg;
    core::Vec3 expected;
};

const std::vector<KnownStar>& KnownStars() {
    static const std::vector<KnownStar> stars = {
        // Polaris: RA 2h31m49.09s, Dec +89 15'50.8" -- near the north celestial pole.
        {"Polaris",
         (2.0 + 31.0 / 60.0 + 49.09 / 3600.0) * 15.0,
         89.0 + 15.0 / 60.0 + 50.8 / 3600.0,
         {0.010126953205375844, 0.007899111853183503, 0.9999175210239628}},
        // Sirius, the brightest star in the sky: RA 6h45m08.9s, Dec -16 42'58".
        {"Sirius",
         (6.0 + 45.0 / 60.0 + 8.9 / 3600.0) * 15.0,
         -(16.0 + 42.0 / 60.0 + 58.0 / 3600.0),
         {-0.18745405323332234, 0.9392177877421448, -0.2876298404462758}},
        // Vega: RA 18h36m56.3s, Dec +38 47'01".
        {"Vega",
         (18.0 + 36.0 / 60.0 + 56.3 / 3600.0) * 15.0,
         38.0 + 47.0 / 60.0 + 1.0 / 3600.0,
         {0.12509456633092567, -0.7694142985947372, 0.626380863845988}},
    };
    return stars;
}

TEST(EquatorialToUnitVector, MatchesIndependentlyComputedRealStars) {
    for (const KnownStar& star : KnownStars()) {
        SCOPED_TRACE(star.name);
        const core::Vec3 v =
            core::EquatorialToUnitVector(Radians(star.raDeg), Radians(star.decDeg));
        ExpectVec(v, star.expected, 1e-12);
        EXPECT_NEAR(Norm(v), 1.0, 1e-12);
    }
}

} // namespace
