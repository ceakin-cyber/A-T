#include "core/celestial.h"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

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

// Polaris, J2000: RA 2h31m49.09s, Dec +89 15'50.8". Computed independently in Python from these
// published coordinates, not from this code's own output.
TEST(EquatorialToUnitVector, MatchesAnIndependentlyComputedRealStar) {
    const double raDeg = (2.0 + 31.0 / 60.0 + 49.09 / 3600.0) * 15.0;
    const double decDeg = 89.0 + 15.0 / 60.0 + 50.8 / 3600.0;
    const core::Vec3 v = core::EquatorialToUnitVector(Radians(raDeg), Radians(decDeg));
    ExpectVec(v, {0.010126953205375844, 0.007899111853183503, 0.9999175210239628}, 1e-12);
    EXPECT_NEAR(Norm(v), 1.0, 1e-12);
}

} // namespace
